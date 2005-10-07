/* dapfs via FUSE */
/*
    FUSE: Filesystem in Userspace
    Copyright (C) 2001-2005  Miklos Szeredi <miklos@szeredi.hu>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.
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
#include "rms.h"
#include "dapfs_dap.h"


char prefix[1024];

static int dapfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	return dapfs_readdir_dap(path, buf, filler, offset, fi);
}


static int dapfs_unlink(const char *path)
{
	// TODO May need to use libdap directly.
	return -ENOSYS;
}

static int dapfs_rmdir(const char *path)
{
	// TODO May need to use libdap directly.
	return -ENOSYS;
}


static int dapfs_rename(const char *from, const char *to)
{
	// TODO May need to use libdap directly.
	return  -ENOSYS;
}


static int dapfs_truncate(const char *path, off_t size)
{
	// TODO
	return  -ENOSYS;
}

static int dapfs_utime(const char *path, struct utimbuf *buf)
{
	// TODO
	return  -ENOSYS;
}


static int dapfs_open(const char *path, struct fuse_file_info *fi)
{
    RMSHANDLE h;
    char fullname[2048];

    // TODO: This WILL break on VMS - we need to nick the Unix->VMS path stuff from FAL
    sprintf(fullname, "%s%s", prefix, path);
    syslog(1, "dapfs: filename: %s\n", fullname);

    h = rms_open(fullname, fi->flags, NULL);
    syslog(1, "dapfs: rms_open %p, %d\n", h, errno);
    if (!h)
	    return -errno;
    fi->fh = (unsigned long)h;
    return 0;
}

static int dapfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
    int res;
    int got = 0;
    struct RAB rab;
    RMSHANDLE h = (RMSHANDLE)fi->fh;

    // TODO Detect if offset hasn't changed...
    memset(&rab, 0, sizeof(rab));
    if (offset) {
        rab.rab$l_kbf = &offset;
        rab.rab$b_rac = 2;//FB$RFA;
        rab.rab$b_ksz = sizeof(offset);
    }
    do {
    	res = rms_read(h, buf+got, size-got, &rab);
	if (res)
		got += res;

    syslog(1, "rms_read returned %d (errno=%d) size = %d\n", res, errno, size);
    } while (res > 0);
    if (res >= 0)
	res = got;

    if (res == -1)
	    res = -errno;

    return res;
}

static int dapfs_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
	// TODO
	return  -ENOSYS;
}

static int dapfs_release(const char *path, struct fuse_file_info *fi)
{
	return rms_close((RMSHANDLE)fi->fh);
}

static int dapfs_getattr(const char *path, struct stat *stbuf)
{
	int res;
	syslog(1, "dapfs - getattr on %s\n", path);
	if (strcmp(path, "/") == 0)
	{
		res = stat("/", stbuf);
	}
	else
	{
		res = dapfs_getattr_dap(path, stbuf);
	}
	return  res;
}

static struct fuse_operations dapfs_oper = {
	.unlink   = dapfs_unlink,
	.getattr  = dapfs_getattr,
	.truncate = dapfs_truncate,
	.open	  = dapfs_open,
	.read	  = dapfs_read,
	.write	  = dapfs_write,
	.readdir  = dapfs_readdir,
	.rmdir    = dapfs_rmdir,
	.rename   = dapfs_rename,
	.utime    = dapfs_utime,
	.release  = dapfs_release,
};

int main(int argc, char *argv[])
{
	// This is the host name ending :: (eg zarqon"patrick password"::)
	strcpy(prefix, argv[1]);

	return fuse_main(argc-1, argv+1, &dapfs_oper);
}
