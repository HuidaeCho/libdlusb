/* $Id: misc.c,v 1.20 2009/04/13 07:21:42 geni Exp $
 *
 * Copyright (c) 2005 Huidae Cho
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define _LIBDLUSB_MISC_C_
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include "dlusb.h"

char *
squeeze_spaces(char *buf)
{
	int i, l, s;

	l = strlen(buf);
	for(i = 0; i < l && buf[i] == ' '; i++);
	s = i;
	for(i = l-1; i >= 0 && buf[i] == ' '; i--);
	buf[i+1] = 0;

	return buf + s;
}

char *
read_line(FILE *fp)
{
	char	buf[BUFSIZ], *line;
	int	i, l;

	line = NULL;
	l = 0;
	do{
		if(feof(fp) || fgets(buf, BUFSIZ, fp) == NULL){
			if(line)
				free(line);
			return NULL;
		}
		i = strlen(buf);
		l += i;
		line = (char *)realloc(line, l+1);
		strncpy(line+l-i, buf, i);
	}while(buf[i-1] != '\r' && buf[i-1] != '\n');

	l--;
	if(line[l-1] == '\r' || line[l-1] == '\n')
		l--;
	line[l] = 0;

	return line;
}

int
read_file(char *file, u8 **data, u16 *len)
{
	struct stat sb;
	int fd;

	if(stat(file, &sb)){
		ERROR("%s: stat failed", file);
		return -1;
	}

	*len = sb.st_size;

	if((fd = open(file, O_RDONLY)) < 0){
		ERROR("%s: open failed", file);
		return -1;
	}

	*data = (u8 *)malloc(*len);
	if(read(fd, *data, *len) != *len){
		ERROR("%s: read failed", file);
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

int
convert_ch(u8 *fontch, u8 ch)
{
	u8 uch;
	int space = -1;
	u8 i, n;

	uch = toupper(ch);

	n = strlen((char *)fontch);
	for(i = 0; i < n && uch != fontch[i]; i++){
		if(fontch[i] == ' ' && space == -1)
			space = i;
	}

	return (i == n ? space : i);
}

void
print_rdm(u8 *msg, u8 msg_len, FILE *fp)
{
	int i;

	for(i = 0; i < msg_len && msg[i] != DM_SENTINEL; i++)
		fprintf(fp, "%c", rdm(msg[i]));

	return;
}

int
str_chr(char *str, char ch)
{
	char *ptr;

	if((ptr = strchr(str, ch)))
		return (int)(ptr - str);

	return -1;
}

int
str_str(char *big, char *little)
{
	return strstr(big, little) == big;
}

int
day_of_week(int y, int m, int d)
{
/* http://www.mathematik.uni-bielefeld.de/~sillke/ALGORITHMS/calendar/weekday.c
 *
 * Keith & Craver;
 * The ultimate perpetual calendar?, JoRM 22:4 (1990) 280-282
 * day of the week as a 44 character expression in C. (illegal use of --)
 * The following 45 character C expression by Keith is correct.
 * dow(y,m,d) { return (d+=m<3?y--:y-2,23*m/9+d+4+y/4-y/100+y/400)%7; }
 *
 * sunday = 0, ...
 */
	return (d+=m<3?y--:y-2,23*m/9+d+4+y/4-y/100+y/400)%7;
}
