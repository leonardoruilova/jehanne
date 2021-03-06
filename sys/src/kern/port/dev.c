#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

extern uint32_t	kerndate;

void
mkqid(Qid *q, int64_t path, uint32_t vers, int type)
{
	q->type = type;
	q->vers = vers;
	q->path = path;
}

void
devdir(Chan *c, Qid qid, char *n, int64_t length, char *user, long perm, Dir *db)
{
	db->name = n;
	if(c->flag&CMSG)
		qid.type |= QTMOUNT;
	db->qid = qid;
	/*
	 * When called via devwalk c->dev is nil
	 * until the walk succeeds.
	 */
	if(c->dev != nil)
		db->type = c->dev->dc;
	else
		db->type = -1;
	db->dev = c->devno;
	db->mode = perm;
	db->mode |= qid.type << 24;
	db->atime = seconds();
	db->mtime = kerndate;
	db->length = length;
	db->uid = user;
	db->gid = eve;
	db->muid = user;
}

/*
 * (here, Devgen is the prototype; devgen is the function in dev.c.)
 *
 * a Devgen is expected to return the directory entry for ".."
 * if you pass it s==DEVDOTDOT (-1).  otherwise...
 *
 * there are two contradictory rules.
 *
 * (i) if c is a directory, a Devgen is expected to list its children
 * as you iterate s.
 *
 * (ii) whether or not c is a directory, a Devgen is expected to list
 * its siblings as you iterate s.
 *
 * devgen always returns the list of children in the root
 * directory.  thus it follows (i) when c is the root and (ii) otherwise.
 * many other Devgens follow (i) when c is a directory and (ii) otherwise.
 *
 * devwalk assumes (i).  it knows that devgen breaks (i)
 * for children that are themselves directories, and explicitly catches them.
 *
 * devstat assumes (ii).  if the Devgen in question follows (i)
 * for this particular c, devstat will not find the necessary info.
 * with our particular Devgen functions, this happens only for
 * directories, so devstat makes something up, assuming
 * c->name, c->qid, eve, DMDIR|0555.
 *
 * devdirread assumes (i).  the callers have to make sure
 * that the Devgen satisfies (i) for the chan being read.
 */
/*
 * the zeroth element of the table MUST be the directory itself for ..
*/
int
devgen(Chan *c, char *name, Dirtab *tab, int ntab, int i, Dir *dp)
{
	if(tab == 0)
		return -1;
	if(i == DEVDOTDOT){
		/* nothing */
	}else if(name){
		for(i=1; i<ntab; i++)
			if(strcmp(tab[i].name, name) == 0)
				break;
		if(i==ntab)
			return -1;
		tab += i;
	}else{
		/* skip over the first element, that for . itself */
		i++;
		if(i >= ntab)
			return -1;
		tab += i;
	}
	devdir(c, tab->qid, tab->name, tab->length, eve, tab->perm, dp);
	return 1;
}

void
devreset(void)
{
}

void
devinit(void)
{
}

void
devshutdown(void)
{
}

Chan*
devattach(int dc, char *spec)
{
	Chan *c;
	char *buf;
	int n;

	/*
	 * There are no error checks here because
	 * this can only be called from the driver of dc
	 * which pretty much guarantees devtabget will
	 * succeed.
	 */
	c = newchan();
	mkqid(&c->qid, 0, 0, QTDIR);
	c->dev = devtabget(dc, 0);
	if(spec == nil)
		spec = "";
	n = 1+UTFmax+strlen(spec)+1;
	buf = smalloc(n);
	snprint(buf, n, "#%C%s", dc, spec);
	c->path = newpath(buf);
	free(buf);
	return c;
}


Chan*
devclone(Chan *c)
{
	Chan *nc;

	if(c->flag & COPEN){
		panic("devclone: file of type %C already open\n",
			c->dev != nil? c->dev->dc: -1);
	}

	nc = newchan();

	/*
	 * The caller fills dev in if and when necessary.
	nc->dev = nil;					//XDYNXX
	 */
	nc->devno = c->devno;
	nc->mode = c->mode;
	nc->qid = c->qid;
	nc->offset = c->offset;
	nc->umh = nil;
	nc->aux = c->aux;
	nc->mqid = c->mqid;
	nc->mc = c->mc;
	return nc;
}

Walkqid*
devwalk(Chan *c, Chan *nc, char **name, int nname, Dirtab *tab, int ntab, Devgen *gen)
{
	int i, j, alloc;
	Walkqid *wq;
	char *n;
	Dir dir;

	if(nname > 0)
		isdir(c);

	alloc = 0;
	wq = smalloc(sizeof(Walkqid)+(nname-1)*sizeof(Qid));
	if(waserror()){
		if(alloc && wq->clone!=nil)
			cclose(wq->clone);
		free(wq);
		return nil;
	}
	if(nc == nil){
		nc = devclone(c);
		/*
		 * nc->dev remains nil for now.		//XDYNX
		 */
		alloc = 1;
	}
	wq->clone = nc;

	for(j=0; j<nname; j++){
		if(!(nc->qid.type & QTDIR)){
			if(j==0)
				error(Enotdir);
			goto Done;
		}
		n = name[j];
		if(strcmp(n, ".") == 0){
    Accept:
			wq->qid[wq->nqid++] = nc->qid;
			continue;
		}
		if(strcmp(n, "..") == 0){
			/*
			 * Use c->dev->name in the error because
			 * nc->dev should be nil here.
			 */
			if((*gen)(nc, nil, tab, ntab, DEVDOTDOT, &dir) != 1){
				print("devgen walk .. in dev%s %#llux broken\n",
					c->dev->name, nc->qid.path);
				error("broken devgen");
			}
			nc->qid = dir.qid;
			goto Accept;
		}
		/*
		 * Ugly problem: If we're using devgen, make sure we're
		 * walking the directory itself, represented by the first
		 * entry in the table, and not trying to step into a sub-
		 * directory of the table, e.g. /net/net. Devgen itself
		 * should take care of the problem, but it doesn't have
		 * the necessary information (that we're doing a walk).
		 */
		if(gen==devgen && nc->qid.path!=tab[0].qid.path)
			goto Notfound;
		for(i=0;; i++) {
			switch((*gen)(nc, n, tab, ntab, i, &dir)){
			case -1:
			Notfound:
				if(j == 0)
					error(Enonexist);
				kstrcpy(up->errstr, Enonexist, ERRMAX);
				goto Done;
			case 0:
				continue;
			case 1:
				if(strcmp(n, dir.name) == 0){
					nc->qid = dir.qid;
					goto Accept;
				}
				continue;
			}
		}
	}
	/*
	 * We processed at least one name, so will return some data.
	 * If we didn't process all nname entries succesfully, we drop
	 * the cloned channel and return just the Qids of the walks.
	 */
Done:
	poperror();
	if(wq->nqid < nname){
		if(alloc)
			cclose(wq->clone);
		wq->clone = nil;
	}else if(wq->clone){
		/* attach cloned channel to same device */
//what goes here:					//XDYNX
// ->dev must be nil because can't walk an open chan, right?
// what about ref count on dev?
		wq->clone->dev = c->dev;
		//if(wq->clone->dev)			//XDYNX
		//	devtabincr(wq->clone->dev);
	}
	return wq;
}

long
devstat(Chan *c, uint8_t *db, long n, Dirtab *tab, int ntab, Devgen *gen)
{
	int i;
	Dir dir;
	char *p, *elem;

	for(i=0;; i++){
		switch((*gen)(c, nil, tab, ntab, i, &dir)){
		case -1:
			if(c->qid.type & QTDIR){
				if(c->path == nil)
					elem = "???";
				else if(strcmp(c->path->s, "/") == 0)
					elem = "/";
				else
					for(elem=p=c->path->s; *p; p++)
						if(*p == '/')
							elem = p+1;
				devdir(c, c->qid, elem, 0, eve, DMDIR|0555, &dir);
				n = convD2M(&dir, db, n);
				if(n == 0)
					error(Ebadarg);
				return n;
			}

			error(Enonexist);
		case 0:
			break;
		case 1:
			if(c->qid.path == dir.qid.path) {
				if(c->flag&CMSG)
					dir.mode |= DMMOUNT;
				n = convD2M(&dir, db, n);
				if(n == 0)
					error(Ebadarg);
				return n;
			}
			break;
		}
	}
}

long
devdirread(Chan *c, char *d, long n, Dirtab *tab, int ntab, Devgen *gen)
{
	long m, dsz;
	Dir dir;

	for(m=0; m<n; c->dri++) {
		switch((*gen)(c, nil, tab, ntab, c->dri, &dir)){
		case -1:
			return m;

		case 0:
			break;

		case 1:
			dsz = convD2M(&dir, (uint8_t*)d, n-m);
			if(dsz <= BIT16SZ){	/* <= not < because this isn't stat; read is stuck */
				if(m == 0)
					error(Eshort);
				return m;
			}
			m += dsz;
			d += dsz;
			break;
		}
	}

	return m;
}

/*
 * error(Eperm) if open permission not granted for up->user.
 */
void
devpermcheck(char *fileuid, int perm, int omode)
{
	int required;
	static int access[] = { 0000, 0400, 0200, 0600, 0100, 0xff, 0xff, 0xff };

	if(strcmp(up->user, fileuid) == 0)
		perm <<= 0;
	else if(ingroup(up->user, eve))
		perm <<= 3;
	else
		perm <<= 6;

	required = access[omode&7];
	if((required&perm) != required)
		error(Eperm);
}

Chan*
devopen(Chan *c, unsigned long omode, Dirtab *tab, int ntab, Devgen *gen)
{
	int i;
	Dir dir;

	for(i=0;; i++) {
		switch((*gen)(c, nil, tab, ntab, i, &dir)){
		case -1:
			goto Return;
		case 0:
			break;
		case 1:
			if(c->qid.path == dir.qid.path) {
				devpermcheck(dir.uid, dir.mode, omode);
				goto Return;
			}
			break;
		}
	}
Return:
	c->offset = 0;
	if((c->qid.type & QTDIR) && omode!=OREAD)
		error(Eperm);
	c->mode = openmode(omode);
	c->flag |= COPEN;
	return c;
}

Chan*
devcreate(Chan* _1, char* _2, unsigned long _3, unsigned long _4)
{
	error(Eperm);
	return nil;
}

Block*
devbread(Chan *c, long n, int64_t offset)
{
	Block *bp;

	bp = allocb(n);
	if(bp == 0)
		error(Enomem);
	if(waserror()) {
		freeb(bp);
		nexterror();
	}
	bp->wp += c->dev->read(c, bp->wp, n, offset);
	poperror();
	return bp;
}

long
devbwrite(Chan *c, Block *bp, int64_t offset)
{
	long n;

	if(waserror()) {
		freeb(bp);
		nexterror();
	}
	n = c->dev->write(c, bp->rp, BLEN(bp), offset);
	poperror();
	freeb(bp);

	return n;
}

void
devremove(Chan* _1)
{
	error(Eperm);
}

long
devwstat(Chan* _1, uint8_t* _2, long _3)
{
	error(Eperm);
	return 0;
}

void
devpower(int _1)
{
	error(Eperm);
}

int
devconfig(int _1, char * _2, DevConf * _3)
{
	error(Eperm);
	return 0;
}
