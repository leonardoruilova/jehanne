/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <9P2000.h>
#include <plumb.h>
#include "dat.h"
#include "fns.h"

static Image *scrtmp;

static
Rectangle
scrpos(Rectangle r, uint32_t p0, uint32_t p1, uint32_t tot)
{
	Rectangle q;
	int h;

	q = r;
	h = q.max.y-q.min.y;
	if(tot == 0)
		return q;
	if(tot > 1024*1024){
		tot>>=10;
		p0>>=10;
		p1>>=10;
	}
	if(p0 > 0)
		q.min.y += h*p0/tot;
	if(p1 < tot)
		q.max.y -= h*(tot-p1)/tot;
	if(q.max.y < q.min.y+2){
		if(q.min.y+2 <= r.max.y)
			q.max.y = q.min.y+2;
		else
			q.min.y = q.max.y-2;
	}
	return q;
}

void
scrlresize(void)
{
	freeimage(scrtmp);
	scrtmp = allocimage(display, Rect(0, 0, 32, screen->r.max.y), screen->chan, 0, DNofill);
	if(scrtmp == nil)
		error("scroll alloc");
}

void
textscrdraw(Text *t)
{
	Rectangle r, r1, r2;
	Image *b;

	if(t->w==nil || t!=&t->w->body)
		return;
	if(scrtmp == nil)
		scrlresize();
	r = t->scrollr;
	b = scrtmp;
	r1 = r;
	r1.min.x = 0;
	r1.max.x = Dx(r);
	r2 = scrpos(r1, t->org, t->org+t->nchars, t->file->nc);
	if(!eqrect(r2, t->lastsr)){
		t->lastsr = r2;
		draw(b, r1, t->cols[BORD], nil, ZP);
		draw(b, r2, t->cols[BACK], nil, ZP);
		r2.min.x = r2.max.x-1;
		draw(b, r2, t->cols[BORD], nil, ZP);
		draw(t->b, r, b, nil, Pt(0, r1.min.y));
/*flushimage(display, 1);/ *BUG?*/
	}
}

void
scrsleep(uint32_t dt)
{
	Timer	*timer;
	static Alt alts[3];

	timer = timerstart(dt);
	alts[0].c = timer->c;
	alts[0].v = nil;
	alts[0].op = CHANRCV;
	alts[1].c = mousectl->c;
	alts[1].v = &mousectl->Mouse;
	alts[1].op = CHANRCV;
	alts[2].op = CHANEND;
	for(;;)
		switch(alt(alts)){
		case 0:
			timerstop(timer);
			return;
		case 1:
			timercancel(timer);
			return;
		}
}

void
textscroll(Text *t, int but)
{
	uint32_t p0, oldp0;
	Rectangle s;
	int y, my, h, first;

	s = insetrect(t->scrollr, 1);
	h = s.max.y-s.min.y;
	oldp0 = ~0;
	first = TRUE;
	do{
		flushimage(display, 1);
		my = mouse->xy.y;
		if(my < s.min.y)
			my = s.min.y;
		if(my >= s.max.y)
			my = s.max.y;
		if(but == 2){
			y = my;
			p0 = (int64_t)t->file->nc*(y-s.min.y)/h;
			if(p0 >= t->q1)
				p0 = textbacknl(t, p0, 2);
			if(oldp0 != p0)
				textsetorigin(t, p0, FALSE);
			oldp0 = p0;
			readmouse(mousectl);
			continue;
		}
		if(but == 1)
			p0 = textbacknl(t, t->org, (my-s.min.y)/t->font->height);
		else
			p0 = t->org+frcharofpt(t, Pt(s.max.x, my));
		if(oldp0 != p0)
			textsetorigin(t, p0, TRUE);
		oldp0 = p0;
		/* debounce */
		if(first){
			flushimage(display, 1);
			sleep(200);
			nbrecv(mousectl->c, &mousectl->Mouse);
			first = FALSE;
		}
		scrsleep(80);
	}while(mouse->buttons & (1<<(but-1)));
	while(mouse->buttons)
		readmouse(mousectl);
}
