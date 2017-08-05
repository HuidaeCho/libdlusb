/* $Id: session.c,v 1.13 2009/05/12 05:44:00 geni Exp $
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

#include <time.h>
#include "dlusb.h"

int
start_session(dldev_t *dev)
{
	/* is the watch connected? */
	if(comm_dev_info_req(dev)){
		ERROR("comm_dev_info_req");
		return -1;
	}

	/* read the system map table */
	if(read_sysmap(dev)){
		ERROR("read_sysmap");
		return -1;
	}

	if(read_sysinfo(dev)){
		ERROR("read_sysinfo");
		return -1;
	}

	return 0;
}

int
end_session(dldev_t *dev)
{
	time_t tim;
	struct tm *now;

	/* send comm_idle in a certain condition */
	send_idle(dev);

	time(&tim);
	now = localtime(&tim);

	sprintf((char *)dev->icb.sync_id, "%04d%02d%02d%02d%02d%02d",
			now->tm_year+1900, now->tm_mon+1, now->tm_mday,
			now->tm_hour, now->tm_min, now->tm_sec);

	dev->icb.sync_id[14] = 0x00;
	dev->icb.sync_id[15] = 0xff;

	/* write sync id */
	if(write_abs_addr(dev, ICB_SIZE-16, ext_mem, dev->icb.sync_id, 16)){
		ERROR("write_abs_addr");
		return -1;
	}

	if(comm_proc_complete(dev)){
		ERROR("comm_proc_complete");
		return -1;
	}

	return 0;
}

int
exit_session(dldev_t *dev)
{
	/* send comm_idle in a certain condition */
	send_idle(dev);

	/* write sync id */
	if(write_abs_addr(dev, ICB_SIZE-16, ext_mem, dev->icb.sync_id, 16)){
		ERROR("write_abs_addr");
		return -1;
	}

	if(comm_proc_complete(dev)){
		ERROR("comm_proc_complete");
		return -1;
	}

	return 0;
}
