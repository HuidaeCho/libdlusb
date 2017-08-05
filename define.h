/* $Id: define.h,v 1.2 2005/10/06 09:05:11 geni Exp $
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

#ifndef _DEFINE_H_
#define _DEFINE_H_

/* general quick sort definition */
#define DEFINE_QSORT(app) \
static void \
qsort_##app(app##_t *db, app##_rec_t *head, app##_rec_t *tail) \
{ \
	int l; \
	app##_rec_t pivot, *p, *h, *t; \
 \
	if(!head || !tail) \
		return; \
 \
	l = sizeof(app##_rec_t) - 2 * sizeof(app##_rec_t *); \
 \
	h = head; \
	t = tail; \
	memcpy(&pivot, head, l); \
 \
	while(head != tail){ \
		while(cmp_##app(tail, &pivot) >= 0 && head != tail) \
			tail = tail->prev; \
		if(head != tail){ \
			memcpy(head, tail, l); \
			head = head->next; \
		} \
		while(cmp_##app(head, &pivot) <= 0 && head != tail) \
			head = head->next; \
		if(head != tail){ \
			memcpy(tail, head, l); \
			tail = tail->prev; \
		} \
	} \
 \
	memcpy(head, &pivot, l); \
	p = head; \
	head = h; \
	tail = t; \
	if(head != p) \
		qsort_##app(db, head, p->prev); \
	if(tail != p) \
		qsort_##app(db, p->next, tail); \
 \
	return; \
}

#endif
