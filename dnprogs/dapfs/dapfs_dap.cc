/******************************************************************************
    (c) 2005 P.J. Caulfield               patrick@tykepenguin.cix.co.uk

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 ******************************************************************************
 */

#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 22

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <dirent.h>
#include <errno.h>
#include <sys/statfs.h>
#include <netdnet/dn.h>
#include "connection.h"
#include "protocol.h"
#include "dapfs_dap.h"
extern "C" {
#include "filenames.h"
}

// Use this for one-shot stuff like getattr & delete
static dap_connection conn(0);

static int dap_connect(dap_connection &c)
{
	char dirname[256] = {'\0'};
	if (!c.connect(prefix, dap_connection::FAL_OBJECT, dirname))
	{
		return -ENOTCONN;
	}

	// Exchange config messages
	if (!c.exchange_config())
	{
		fprintf(stderr, "Error in config: %s\n", c.get_error());
		return -ENOTCONN;
	}
	return 0;
}

static void add_to_stat(dap_message *m, struct stat *stbuf)
{
	switch (m->get_type())
	{
	case dap_message::NAME:
	{
		dap_name_message *nm = (dap_name_message *)m;

		// If name ends in .DIR;1 then add directory attribute
		if (nm->get_nametype() == dap_name_message::FILENAME) {
			if (strstr(nm->get_namespec(), ".DIR;1")) {
				stbuf->st_mode |= S_IFDIR;
			}
			else {
				stbuf->st_mode &= ~S_IFDIR;
			}
		}
	}
	break;

	case dap_message::PROTECT:
	{
		dap_protect_message *pm = (dap_protect_message *)m;
		stbuf->st_mode |= pm->get_mode();
		stbuf->st_uid = 0;
		stbuf->st_gid = 0;
	}
	break;

	case dap_message::ATTRIB:
	{
		dap_attrib_message *am = (dap_attrib_message *)m;
		stbuf->st_size = am->get_size();
		stbuf->st_blksize = am->get_bsz();
		stbuf->st_blocks = am->get_alq();
	}
	break;

	case dap_message::DATE:
	{
		dap_date_message *dm = (dap_date_message *)m;
		stbuf->st_atime = dm->get_rdt_time();
		stbuf->st_ctime = dm->get_cdt_time();
		stbuf->st_mtime = dm->get_cdt_time();
	}
	break;
	}
}

int dapfs_getattr_dap(const char *path, struct stat *stbuf)
{
	char vmsname[1024];
	char name[80];
	int ret = 0;
	int size;

	make_vms_filespec(path, vmsname, 0);

	dap_access_message acc;
	acc.set_accfunc(dap_access_message::DIRECTORY);
	acc.set_accopt(1);
	acc.set_filespec(vmsname);
	acc.set_display(dap_access_message::DISPLAY_MAIN_MASK |
			dap_access_message::DISPLAY_DATE_MASK |
			dap_access_message::DISPLAY_PROT_MASK);
	acc.write(conn);

	dap_message *m;

	// Loop through the files we find
	while ( ((m=dap_message::read_message(conn, true) )) )
	{
		add_to_stat(m, stbuf);
		if (m->get_type() == dap_message::ACCOMP) {
			delete m;
			goto finished;
		}

		if (m->get_type() == dap_message::STATUS)
		{
			dap_status_message *sm = (dap_status_message *)m;
			if (sm->get_code() == 0x4030)
			{
				dap_contran_message cm;
				cm.set_confunc(dap_contran_message::SKIP);
				cm.write(conn);
			}
			else
			{
				ret = -ENOENT; // TODO better error
				// Clean connection status.
				dap_contran_message cm;
				cm.set_confunc(dap_contran_message::SKIP);
				cm.write(conn);
			}
			// Wait for ACCOMP
			break;
		}
		delete m;
	}
finished:
	return ret;
}

int dapfs_readdir_dap(const char *path, void *buf, fuse_fill_dir_t filler,
		      off_t offset, struct fuse_file_info *fi)
{
	dap_connection c(0);
	char vmsname[1024];
	char wildname[strlen(path)+2];
	char name[80];
	struct stat stbuf;
	int size;
	int ret;

	// Use our own connection for this.
	ret = dap_connect(c);
	if (ret)
		return ret;

	memset(&stbuf, 0, sizeof(stbuf));

	// Add wildcard to path
	if (path[strlen(path)-1] == '/') {
		sprintf(wildname, "%s*.*", path);
		path = wildname;
	}
	make_vms_filespec(path, vmsname, 0);

	dap_access_message acc;
	acc.set_accfunc(dap_access_message::DIRECTORY);
	acc.set_accopt(1);
	acc.set_filespec(vmsname);
	acc.set_display(dap_access_message::DISPLAY_MAIN_MASK |
			dap_access_message::DISPLAY_DATE_MASK |
			dap_access_message::DISPLAY_PROT_MASK);
	acc.write(c);

	bool name_pending = false;
	dap_message *m;
	char volname[256];

	// Loop through the files we find
	while ( ((m=dap_message::read_message(c, true) )) )
	{
		add_to_stat(m, &stbuf);

		switch (m->get_type())
		{
		case dap_message::NAME:
		{
			// Got a new name, send the old stuff.
			if (name_pending)
			{
				char unixname[1024];
				name_pending = false;
				make_unix_filespec(unixname, name);
				filler(buf, unixname, &stbuf, 0);
				memset(&stbuf, 0, sizeof(stbuf));
			}

			dap_name_message *nm = (dap_name_message *)m;

			if (nm->get_nametype() == dap_name_message::VOLUME)
			{
				strcpy(volname, nm->get_namespec());
			}

			if (nm->get_nametype() == dap_name_message::FILENAME)
			{
				strcpy(name, nm->get_namespec());
				name_pending = true;
			}
		}
		break;

	        case dap_message::STATUS:
		{
			// Send a SKIP if the file is locked, else it's an error
			dap_status_message *sm = (dap_status_message *)m;
			if (sm->get_code() == 0x4030)
			{
				dap_contran_message cm;
				cm.set_confunc(dap_contran_message::SKIP);
				if (!cm.write(c))
				{
					fprintf(stderr, "Error sending skip: %s\n", c.get_error());
					delete m;
					goto finished;
				}
			}
			else
			{
				printf("Error opening %s: %s\n", vmsname, sm->get_message());
				name_pending = false;
				goto flush;
			}
			break;
		}
		case dap_message::ACCOMP:
			goto flush;
		}
		delete m;
	}

finished:
	// An error:
	fprintf(stderr, "Error: %s\n", c.get_error());
	c.close();
	return 2;

flush:
	delete m;
	if (name_pending)
	{
		char unixname[1024];
		make_unix_filespec(unixname, name);
		filler(buf, unixname, &stbuf, 0);
	}
	c.close();
	return 0;
}

/* Path already has version number appended to it -- this may be a mistake :) */
int dap_delete_file(const char *path)
{
	char vmsname[1024];
	char name[80];
	int ret;
	int size;

	make_vms_filespec(path, vmsname, 0);

	dap_access_message acc;
	acc.set_accfunc(dap_access_message::ERASE);
	acc.set_accopt(1);
	acc.set_filespec(vmsname);
	acc.set_display(0);
        acc.write(conn);

	// Wait for ACK or status
	ret = 0;
	while(1) {
		dap_message *m = dap_message::read_message(conn, true);

		switch (m->get_type())
		{
		case dap_message::ACCOMP:
			goto end;
		break;

		case dap_message::STATUS:
		{
			dap_status_message *sm = (dap_status_message *)m;
			if (sm->get_code() & 1 != 1)
				ret = -EPERM; // Default error!
			goto end;
		}
		break;
		}
		delete m;
	}
	end:
	return ret;
}

int dap_rename_file(const char *from, const char *to)
{
	char vmsfrom[1024];
	char vmsto[1024];
	int ret;
	int size;

	make_vms_filespec(from, vmsfrom, 0);
	make_vms_filespec(to, vmsto, 0);

	dap_access_message acc;
	acc.set_accfunc(dap_access_message::RENAME);
	acc.set_accopt(1);
	acc.set_filespec(vmsfrom);
	acc.set_display(0);
        acc.write(conn);

// TODO Test this, We may need to split the filespec up into DIR & FILE messages,
// at least for cross-directory renames.
	dap_name_message nam;
	nam.set_nametype(dap_name_message::FILESPEC);
	nam.set_namespec(vmsto);
	nam.write(conn);

	// Wait for ACK or status
	ret = 0;
	while (1) {
		dap_message *m = dap_message::read_message(conn, true);
		switch (m->get_type())
		{
		case dap_message::ACCOMP:
			goto end;

		case dap_message::STATUS:
		{
			dap_status_message *sm = (dap_status_message *)m;
			ret = -EPERM; // Default error!
		}
		}
	}
	end:
	return ret;
}


int dap_init()
{
	return dap_connect(conn);
}
