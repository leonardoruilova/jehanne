#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "io.h"
#include "apic.h"

#undef DBG
#define	DBG	print
/*
 * MultiProcessor Specification Version 1.[14].
 */
typedef struct {				/* MP Floating Pointer */
	uint8_t	signature[4];			/* "_MP_" */
	uint8_t	addr[4];			/* PCMP */
	uint8_t	length;				/* 1 */
	uint8_t	revision;			/* [14] */
	uint8_t	checksum;
	uint8_t	feature[5];
} _MP_;

typedef struct {				/* MP Configuration Table */
	uint8_t	signature[4];			/* "PCMP" */
	uint8_t	length[2];
	uint8_t	revision;			/* [14] */
	uint8_t	checksum;
	uint8_t	string[20];			/* OEM + Product ID */
	uint8_t	oaddr[4];			/* OEM table pointer */
	uint8_t	olength[2];			/* OEM table length */
	uint8_t	entry[2];			/* entry count */
	uint8_t	apicpa[4];			/* local APIC address */
	uint8_t	xlength[2];			/* extended table length */
	uint8_t	xchecksum;			/* extended table checksum */
	uint8_t	reserved;

	uint8_t	entries[];
} PCMP;

typedef struct {
	char	type[6];
	int	polarity;			/* default for this bus */
	int	trigger;			/* default for this bus */
} Mpbus;

static Mpbus mpbusdef[] = {
	{ "PCI   ", IPlow, TMlevel, },
	{ "ISA   ", IPhigh, TMedge, },
};
static Mpbus* mpbus[Nbus];
int mpisabusno = -1;

static void
mpintrprint(char* s, uint8_t* p)
{
	char buf[128], *b, *e;
	char format[] = " type %d flags %#ux bus %d IRQ %d APIC %d INTIN %d\n";

	b = buf;
	e = b + sizeof(buf);
	b = seprint(b, e, "mpparse: intr:");
	if(s != nil)
		b = seprint(b, e, " %s:", s);
	seprint(b, e, format, p[1], l16get(p+2), p[4], p[5], p[6], p[7]);
	print(buf);
}

static uint32_t
mpmkintr(uint8_t* p)
{
	uint32_t v;
	Lapic *apic;
	IOapic *ioapic;
	int n, polarity, trigger;

	/*
	 * Check valid bus, interrupt input pin polarity
	 * and trigger mode. If the APIC ID is 0xff it means
	 * all APICs of this type so those checks for useable
	 * APIC and valid INTIN must also be done later in
	 * the appropriate init routine in that case. It's hard
	 * to imagine routing a signal to all IOAPICs, the
	 * usual case is routing NMI and ExtINT to all LAPICs.
	 */
	if(mpbus[p[4]] == nil){
		mpintrprint("no source bus", p);
		return 0;
	}
	if(p[6] != 0xff){
		if(Napic < 256 && p[6] >= Napic){
			mpintrprint("APIC ID out of range", p);
			return 0;
		}
		switch(p[0]){
		default:
			mpintrprint("INTIN botch", p);
			return 0;
		case 3:				/* IOINTR */
			ioapic = ioapiclookup(p[6]);
			if(ioapic == nil){
				mpintrprint("unuseable IO APIC", p);
				return 0;
			}
			if(p[7] >= ioapic->nrdt){
				mpintrprint("IO INTIN out of range", p);
				return 0;
			}
			break;
		case 4:				/* LINTR */
			apic = lapiclookup(p[6]);
			if(apic == nil){
				mpintrprint("unuseable local APIC", p);
				return 0;
			}
			if(p[7] >= nelem(apic->lvt)){
				mpintrprint("LOCAL INTIN out of range", p);
				return 0;
			}
			break;
		}
	}
	n = l16get(p+2);
	if((polarity = (n & 0x03)) == 2 || (trigger = ((n>>2) & 0x03)) == 2){
		mpintrprint("invalid polarity/trigger", p);
		return 0;
	}

	/*
	 * Create the low half of the vector table entry (LVT or RDT).
	 * For the NMI, SMI and ExtINT cases, the polarity and trigger
	 * are fixed (but are not always consistent over IA-32 generations).
	 * For the INT case, either the polarity/trigger are given or
	 * it defaults to that of the source bus;
	 * whether INT is Fixed or Lowest Priority is left until later.
	 */
	v = Im;
	switch(p[1]){
	default:
		mpintrprint("invalid type", p);
		return 0;
	case 0:					/* INT */
		switch(polarity){
		case 0:
			v |= mpbus[p[4]]->polarity;
			break;
		case 1:
			v |= IPhigh;
			break;
		case 3:
			v |= IPlow;
			break;
		}
		switch(trigger){
		case 0:
			v |= mpbus[p[4]]->trigger;
			break;
		case 1:
			v |= TMedge;
			break;
		case 3:
			v |= TMlevel;
			break;
		}
		break;
	case 1:					/* NMI */
		v |= TMedge|IPhigh|MTnmi;
		break;
	case 2:					/* SMI */
		v |= TMedge|IPhigh|MTsmi;
		break;
	case 3:					/* ExtINT */
		v |= TMedge|IPhigh|MTei;
		break;
	}

	return v;
}

static void
mpparse(PCMP* pcmp)
{
	uint32_t lo;
	uint8_t *e, *p;
	int i, n, bustype;
	Lapic *apic;

	p = pcmp->entries;
	e = ((uint8_t*)pcmp)+l16get(pcmp->length);
	while(p < e) switch(*p){
	default:
		print("mpparse: unknown PCMP type %d (e-p %#ld)\n", *p, e-p);
		for(i = 0; p < e; i++){
			if(i && ((i & 0x0f) == 0))
				print("\n");
			print(" %#2.2ux", *p);
			p++;
		}
		print("\n");
		break;
	case 0:					/* processor */
		/*
		 * Initialise the APIC if it is enabled (p[3] & 0x01).
		 * p[1] is the APIC ID, the memory mapped address comes
		 * from the PCMP structure as the addess is local to the
		 * CPU and identical for all. Indicate whether this is
		 * the bootstrap processor (p[3] & 0x02).
		 */
		DBG("mpparse: APIC %d pa %#ux useable %d\n",
			p[1], l32get(pcmp->apicpa), p[3] & 0x01);
		if(p[3] & 0x01)
			lapicinit(p[1], l32get(pcmp->apicpa), p[3] & 0x02);
		p += 20;
		break;
	case 1:					/* bus */
		DBG("mpparse: bus: %d type %6.6s\n", p[1], (char*)p+2);
		if(mpbus[p[1]] != nil){
			print("mpparse: bus %d already allocated\n", p[1]);
			p += 8;
			break;
		}
		for(i = 0; i < nelem(mpbusdef); i++){
			if(memcmp(p+2, mpbusdef[i].type, 6) != 0)
				continue;
			if(memcmp(p+2, "ISA   ", 6) == 0){
				if(mpisabusno != -1){
					print("mpparse: bus %d already have ISA bus %d\n",
						p[1], mpisabusno);
					continue;
				}
				mpisabusno = p[1];
			}
			mpbus[p[1]] = &mpbusdef[i];
			break;
		}
		if(mpbus[p[1]] == nil)
			print("mpparse: bus %d type %6.6s unknown\n",
				p[1], (char*)p+2);

		p += 8;
		break;
	case 2:					/* IOAPIC */
		/*
		 * Initialise the IOAPIC if it is enabled (p[3] & 0x01).
		 * p[1] is the APIC ID, p[4-7] is the memory mapped address.
		 */
		DBG("mpparse: IOAPIC %d pa %#ux useable %d\n",
			p[1], l32get(p+4), p[3] & 0x01);
		if(p[3] & 0x01)
			ioapicinit(p[1], -1, l32get(p+4));

		p += 8;
		break;
	case 3:					/* IOINTR */
		/*
		 * p[1] is the interrupt type;
		 * p[2-3] contains the polarity and trigger mode;
		 * p[4] is the source bus;
		 * p[5] is the IRQ on the source bus;
		 * p[6] is the destination APIC;
		 * p[7] is the INITIN pin on the destination APIC.
		 */
		if(p[6] == 0xff){
			mpintrprint("routed to all IOAPICs", p);
			p += 8;
			break;
		}
		if((lo = mpmkintr(p)) == 0){
			p += 8;
			break;
		}
		if(DBGFLG)
			mpintrprint(nil, p);

		/*
		 * Always present the device number in the style
		 * of a PCI Interrupt Assignment Entry. For the ISA
		 * bus the IRQ is the device number but unencoded.
		 * May need to handle other buses here in the future
		 * (but unlikely).
		 */
		bustype = -1;
		if(memcmp(mpbus[p[4]]->type, "PCI   ", 6) == 0)
			bustype = BusPCI;	/* had devno = p[5]<<2 */
		else if(memcmp(mpbus[p[4]]->type, "ISA   ", 6) == 0)
			bustype = BusISA;
		if(bustype != -1)
			ioapicintrinit(bustype, p[4], p[6], p[7], p[5], lo);

		p += 8;
		break;
	case 4:					/* LINTR */
		/*
		 * Format is the same as IOINTR above.
		 */
		if((lo = mpmkintr(p)) == 0){
			p += 8;
			break;
		}
		if(DBGFLG)
			mpintrprint(nil, p);

		/*
		 * Everything was checked in mpmkintr above.
		 */
		if(p[6] == 0xff){
			for(i = 0; i < Napic; i++){
				apic = lapiclookup(i);
				if(apic != nil)
					apic->lvt[p[7]] = lo;
			}
		}
		else{
			apic = lapiclookup(p[6]);
			if(apic != nil)
				apic->lvt[p[7]] = lo;
		}
		p += 8;
		break;
	}

	/*
	 * There's nothing of real interest in the extended table,
	 * should just move along, but check it for consistency.
	 */
	p = e;
	e = p + l16get(pcmp->xlength);
	while(p < e) switch(*p){
	default:
		n = p[1];
		print("mpparse: unknown extended entry %d length %d\n", *p, n);
		for(i = 0; i < n; i++){
			if(i && ((i & 0x0f) == 0))
				print("\n");
			print(" %#2.2ux", *p);
			p++;
		}
		print("\n");
		break;
	case 128:
		DBG("address space mapping\n");
		DBG(" bus %d type %d base %#llux length %#llux\n",
			p[2], p[3], l64get(p+4), l64get(p+12));
		p += p[1];
		break;
	case 129:
		DBG("bus hierarchy descriptor\n");
		DBG(" bus %d sd %d parent bus %d\n",
			p[2], p[3], p[4]);
		p += p[1];
		break;
	case 130:
		DBG("compatibility bus address space modifier\n");
		DBG(" bus %d pr %d range list %d\n",
			p[2], p[3], l32get(p+4));
		p += p[1];
		break;
	}
}

static int
sigchecksum(void* address, int length)
{
	uint8_t *p, sum;

	sum = 0;
	for(p = address; length-- > 0; p++)
		sum += *p;

	return sum;
}

static void*
sigscan(uint8_t* address, int length, char* signature)
{
	uint8_t *e, *p;
	int siglength;

	DBG("check for %s in system base memory @ %#p\n", signature, address);

	e = address+length;
	siglength = strlen(signature);
	for(p = address; p+siglength < e; p += 16){
		if(memcmp(p, signature, siglength))
			continue;
		return p;
	}

	return nil;
}

static uintmem mptab[] = {0, 1024, 0x9fc00, 1024, 0xf0000, 0x10000};

static void*
sigsearch(char* signature)
{
	uintmem p;
	int i;
	uint8_t *bda;
	void *r;

	/*
	 * Search for the data structure:
	 * 1) within the first KiB of the Extended BIOS Data Area (EBDA), or
	 * 2) within the last KiB of system base memory if the EBDA segment
	 *    is undefined, or
	 * 3) within the BIOS ROM address space between 0xf0000 and 0xfffff
	 */

	for(i = 0; i < nelem(mptab); i += 2)
		if(r = sigscan(KADDR(mptab[i]), mptab[i+1], signature))
			return r;
	bda = KADDR(0x400);
	if((p = (bda[0x0F]<<8|bda[0x0E])<<4) != 0){
		if((r = sigscan(KADDR(p), 1024, signature)) != nil)
			return r;
	}
	if((p = ((bda[0x14]<<8)|bda[0x13])*1024) != 0){
		if((r = sigscan(KADDR(p-1024), 1024, signature)) != nil)
			return r;
	}
	return nil;
}

void
mpsinit(void)
{
	uint8_t *p;
	int i, n;
	_MP_ *mp;
	PCMP *pcmp;

	if((mp = sigsearch("_MP_")) == nil)
		return;
	if(DBGFLG){
		DBG("_MP_ @ %#p, addr %#ux length %ud rev %d",
			mp, l32get(mp->addr), mp->length, mp->revision);
		for(i = 0; i < sizeof(mp->feature); i++)
			DBG(" %2.2#ux", mp->feature[i]);
		DBG("\n");
	}
	if(mp->revision != 1 && mp->revision != 4)
		return;
	if(sigchecksum(mp, mp->length*16) != 0)
		return;

	if((pcmp = vmap(l32get(mp->addr), sizeof(PCMP))) == nil)
		return;
	if(pcmp->revision != 1 && pcmp->revision != 4){
		vunmap(pcmp, sizeof(PCMP));
		return;
	}
	n = l16get(pcmp->length) + l16get(pcmp->xlength);
	vunmap(pcmp, sizeof(PCMP));
	if((pcmp = vmap(l32get(mp->addr), n)) == nil)
		return;
	if(sigchecksum(pcmp, l16get(pcmp->length)) != 0){
		vunmap(pcmp, n);
		return;
	}
	if(DBGFLG){
		DBG("PCMP @ %#p length %#ux revision %d\n",
			pcmp, l16get(pcmp->length), pcmp->revision);
		DBG(" %20.20s oaddr %#ux olength %#ux\n",
			(char*)pcmp->string, l32get(pcmp->oaddr),
			l16get(pcmp->olength));
		DBG(" entry %d apicpa %#ux\n",
			l16get(pcmp->entry), l32get(pcmp->apicpa));

		DBG(" xlength %#ux xchecksum %#ux\n",
			l16get(pcmp->xlength), pcmp->xchecksum);
	}
	if(pcmp->xchecksum != 0){
		p = ((uint8_t*)pcmp) + l16get(pcmp->length);
		i = sigchecksum(p, l16get(pcmp->xlength));
		if(((i+pcmp->xchecksum) & 0xff) != 0){
			print("extended table checksums to %#ux\n", i);
			vunmap(pcmp, n);
			return;
		}
	}

	/*
	 * Parse the PCMP table and set up the datastructures
	 * for later interrupt enabling and application processor
	 * startup.
	 */
	mpparse(pcmp);

	lapicdump();
	iordtdump();
}
