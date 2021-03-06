/*
 * This file is part of Jehanne.
 *
 * Copyright (C) 2016 Giacomo Tesio <giacomo@tesio.it>
 *
 * Jehanne is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * Jehanne is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Jehanne.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <u.h>
#include <libc.h>

int
chdir(const char *dirname)
{
	char buf[32];
	int tmp, fd;

	tmp = getpid();
	snprint(buf, sizeof(buf), "/proc/%d/wdir", tmp);
	fd = open(buf, OWRITE);
	if(fd < 0)
		fd = open("#0/wdir", OWRITE);
	if(fd < 0)
		return fd;
	tmp = write(fd, dirname, 1+strlen(dirname));
	close(fd);
	return tmp;
}
