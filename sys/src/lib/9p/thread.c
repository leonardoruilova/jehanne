#include <u.h>
#include <libc.h>
#include <9P2000.h>
#include <thread.h>
#include <9p.h>

static void
tforker(void (*fn)(void*), void *arg, int rflag)
{
	procrfork(fn, arg, 32*1024, rflag);
}

void
threadlistensrv(Srv *s, char *addr)
{
	_forker = tforker;
	_listensrv(s, addr);
}

void
threadpostmountsrv(Srv *s, char *name, char *mtpt, int flag)
{
	_forker = tforker;
	_postmountsrv(s, name, mtpt, flag);
}

void
threadpostsharesrv(Srv *s, char *name, char *mtpt, char *desc)
{
	_forker = tforker;
	_postsharesrv(s, name, mtpt, desc);
}
