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


int dapfs_getattr_dap(const char *path, struct stat *stbuf)
{
	dap_connection conn(0);
	char dirname[256] = {'\0'};
	char name[80],cdt[25],owner[20],prot[22];
	int size;

	if (!conn.connect(prefix, dap_connection::FAL_OBJECT, dirname))
	{
		return -ENOTCONN;
	}

	// Exchange config messages
	if (!conn.exchange_config())
	{
		fprintf(stderr, "Error in config: %s\n", conn.get_error());
		return -1;
	}

	dap_access_message acc;
	acc.set_accfunc(dap_access_message::DIRECTORY);
	acc.set_accopt(1);
	acc.set_filespec(path); // PJC TODO Translate to VMS format...
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
		switch (m->get_type())
		{
		case dap_message::NAME:
		break;

		case dap_message::PROTECT:
		{
			dap_protect_message *pm = (dap_protect_message *)m;
			stbuf->st_mode = pm->get_mode();
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

	        case dap_message::ACK:
		break;

	        case dap_message::STATUS:
		case dap_message::ACCOMP:
		delete m;
		goto finished;
		}
	}
// TODO if name ends in .DIR;1 then add DIRECTORY to st_mode
// TODO Errors;
finished:
	conn.close();
	return 0;
}

int dapfs_readdir_dap(const char *path, void *buf, fuse_fill_dir_t filler,
		      off_t offset, struct fuse_file_info *fi)
{
	dap_connection conn(0);
	char dirname[256] = {'\0'};
	char name[80],cdt[25],owner[20],prot[22];
	int size;

	if (!conn.connect(prefix, dap_connection::FAL_OBJECT, dirname))
	{
		return -ENOTCONN;
	}

	// Exchange config messages
	if (!conn.exchange_config())
	{
		fprintf(stderr, "Error in config: %s\n", conn.get_error());
		return -1;
	}

	dap_access_message acc;
	acc.set_accfunc(dap_access_message::DIRECTORY);
	acc.set_accopt(1);
	acc.set_filespec(path); // PJC TODO Translate to VMS format...
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
		switch (m->get_type())
		{
		case dap_message::NAME:
		{
			if (name_pending)
			{
				name_pending = false;
				// TODO fill stat
				filler(buf, name, NULL, 0);
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

		case dap_message::PROTECT:
		{
			dap_protect_message *pm = (dap_protect_message *)m;
			strcpy(owner, pm->get_owner());
			strcpy(prot, pm->get_protection());
		}
		break;

	        case dap_message::ATTRIB:
		{
			dap_attrib_message *am = (dap_attrib_message *)m;
			size = am->get_size();
		}
		break;

	        case dap_message::DATE:
		{
			dap_date_message *dm = (dap_date_message *)m;
			strcpy(cdt, dm->make_y2k(dm->get_cdt()));
		}
		break;

	        case dap_message::ACK:
		{
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
				printf("Error opening %s: %s\n", dirname,
				       sm->get_message());
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
		// TODO fill stat
		filler(buf, name, NULL, 0);
	}
	conn.close();
	return 0;
}
