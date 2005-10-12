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

static int dap_connect(dap_connection &conn)
{
	char dirname[256] = {'\0'};
	if (!conn.connect(prefix, dap_connection::FAL_OBJECT, dirname))
	{
		return -ENOTCONN;
	}

	// Exchange config messages
	if (!conn.exchange_config())
	{
		fprintf(stderr, "Error in config: %s\n", conn.get_error());
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
				syslog(1, "%s is a directory\n", nm->get_namespec());
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
		syslog(1, "got date message\n");
		stbuf->st_atime = dm->get_rdt_time();
		stbuf->st_ctime = dm->get_cdt_time();
		stbuf->st_mtime = dm->get_cdt_time();
	}
	break;
	}
}

int dapfs_getattr_dap(const char *path, struct stat *stbuf)
{
	dap_connection conn(0);
	char vmsname[1024];
	char name[80];
	int ret;
	int size;

	ret = dap_connect(conn);
	if (ret)
		return ret;

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
		if (m->get_type() == dap_message::ACCOMP)
		{
			goto finished;
		}
		delete m;
	}
finished:
	conn.close();
	return 0;
}

int dapfs_readdir_dap(const char *path, void *buf, fuse_fill_dir_t filler,
		      off_t offset, struct fuse_file_info *fi)
{
	dap_connection conn(0);
	char vmsname[1024];
	char wildname[strlen(path)+2];
	char name[80];
	struct stat stbuf;
	int size;
	int ret;

	memset(&stbuf, 0, sizeof(stbuf));
	ret = dap_connect(conn);
	if (ret)
		return ret;

	// Add wildcard to path
	if (path[strlen(path)-1] == '/') {
		sprintf(wildname, "%s*", path);
		path = wildname;
	}

	make_vms_filespec(path, vmsname, 0);
	syslog(1, "readdir: dir = %s: %s\n", path, vmsname);

	dap_access_message acc;
	acc.set_accfunc(dap_access_message::DIRECTORY);
	acc.set_accopt(1);
	acc.set_filespec(vmsname);
	acc.set_display(dap_access_message::DISPLAY_MAIN_MASK |
			dap_access_message::DISPLAY_DATE_MASK |
			dap_access_message::DISPLAY_PROT_MASK);
	acc.write(conn);

	bool name_pending = false;
	dap_message *m;
	char volname[256];

	// Loop through the files we find
	while ( ((m=dap_message::read_message(conn, true) )) )
	{
		add_to_stat(m, &stbuf);

		switch (m->get_type())
		{
		case dap_message::NAME:
		{
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
				if (!cm.write(conn))
				{
					fprintf(stderr, "Error sending skip: %s\n", conn.get_error());
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
	fprintf(stderr, "Error: %s\n", conn.get_error());
 	conn.close();
	return 2;

flush:
	if (name_pending)
	{
		char unixname[1024];
		make_unix_filespec(unixname, name);
		filler(buf, name, &stbuf, 0);
	}
	conn.close();
	return 0;
}
