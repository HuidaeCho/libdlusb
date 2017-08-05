/* $Id: misc.h,v 1.2 2005/10/02 07:38:51 geni Exp $
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

#ifndef _MISC_H_
#define _MISC_H_

#define BEGIN_OPT() \
	{ \
		int i, j; \
		for(i = 0; i < argc; i++){ \
			if((j = str_chr(argv[i], '=')) <= 0) \
				continue; \
			j++;
#define OPT(name, var) \
			if(str_str(argv[i], name) && \
					argv[i][strlen(name)] == '='){ \
				var = argv[i] + j; \
				continue; \
			}

typedef struct _appinfo {
	int app;
	char *file;
} appinfo;

#define APP_OPT(name, idx) \
	if(str_str(argv[i], name) && argv[i][strlen(name)] == '='){ \
		app = (appinfo *)realloc(app, (num_apps+1)*sizeof(appinfo)); \
		app[num_apps].app = idx; \
		app[num_apps].file = argv[i] + j; \
		num_apps++; \
		continue; \
	}
#define END_OPT() \
		} \
	}

#endif
