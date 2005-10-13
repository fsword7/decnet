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
/* dapfs via FUSE */

/*
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
#include "filenames.h"

struct dapfs_handle
{
	RMSHANDLE rmsh;
	off_t offset;
};

char prefix[1024];

static int dapfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	return dapfs_readdir_dap(path, buf, filler, offset, fi);
}


static int dapfs_unlink(const char *path)
{
	char vername[strlen(path)+3];

	sprintf(vername, "%s;*", path);
	return dap_delete_file(vername);
}

static int dapfs_rmdir(const char *path)
{
	char dirname[strlen(path)+7];

	sprintf(dirname, "%s.DIR;1", path);
	return dap_delete_file(dirname);
}


static int dapfs_rename(const char *from, const char *to)
{
	return dap_rename_file(from, to);
}

int dapfs_chmod(const char *path, mode_t mode)
{
	// Can we do this ??
	return 0;
}

static int dapfs_truncate(const char *path, off_t size)
{
	// TODO - DAP::CONTROL::TRUNCATE message.
	// NOTE: This can also be called at create time to overwrite an
	// existing file
	return  0;
}

static int dapfs_mkdir(const char *path, mode_t mode)
{
	syslog(1, "PJC: dapfs_mkdir called for %s\n", path);
	// TODO Use remote object
	return -ENOSYS;
}

static int dapfs_statfs(const char *path, struct statfs *stbuf)
{
	syslog(1, "PJC: dapfs_statfs called for %s\n", path);
	// TODO Use remote object
	return -ENOSYS;
}


/* This gets called for normal files too... */
// TODO mode is ignored.
static int dapfs_mknod(const char *path, mode_t mode, dev_t dev)
{
	RMSHANDLE rmsh;
	char fullname[2048];
	char vmsname[2048];

	syslog(1, "PJC: mknod, path=%s, mode=%x\n", path, mode);

	if (S_ISDIR(mode))
		return -ENOSYS; // Does this happen ??

	if (!S_ISREG(mode))
		return -ENOSYS;

	make_vms_filespec(path, vmsname, 0);
	sprintf(fullname, "%s%s", prefix, vmsname);

	rmsh = rms_open(fullname, O_CREAT|O_WRONLY, NULL);
	if (!rmsh)
		return -errno;
	rms_close(rmsh);
	return 0;
}

static int dapfs_open(const char *path, struct fuse_file_info *fi)
{
    struct dapfs_handle *h;
    char fullname[2048];
    char vmsname[2048];

    h = malloc(sizeof(struct dapfs_handle));
    if (!h)
	    return -ENOMEM;

    make_vms_filespec(path, vmsname, 0);
    sprintf(fullname, "%s%s", prefix, vmsname);

    h->rmsh = rms_open(fullname, fi->flags, NULL);
    if (!h->rmsh) {
	    int saved_errno = errno;
	    free(h);
	    return -saved_errno;
    }
    fi->fh = (unsigned long)h;
    h->offset = 0;
    return 0;
}

static int dapfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
    int res;
    int got = 0;
    struct RAB rab;
    struct dapfs_handle *h = (struct dapfs_handle *)fi->fh;

    if (!h) {
	res = dapfs_open(path, fi);
	if (!res)
		return res;
    }

    memset(&rab, 0, sizeof(rab));
    if (offset && offset != h->offset) {
        rab.rab$l_kbf = &offset;
        rab.rab$b_rac = 2;//FB$RFA;
        rab.rab$b_ksz = sizeof(offset);
    }

    // TODO this can be quite slow, because it reads records. Maybe we
    // should abandon librms and pull blocks down as per dncopy
    do {
    	res = rms_read(h->rmsh, buf+got, size-got, &rab);
	if (res > 0)
		got += res;
	if (res < 0 && got) {
	        // Not enough room for a record but we read something
	        return got;
	}

    } while (res > 0);
    if (res >= 0)
	res = got;

    if (res == -1)
	    res = -errno;

    h->offset += res;
    return res;
}

static int dapfs_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
    int res;
    int got = 0;
    struct RAB rab;
    struct dapfs_handle *h = (struct dapfs_handle *)fi->fh;

    if (!h) {
        res = dapfs_open(path, fi);
        if (!res)
                return res;
    }

    memset(&rab, 0, sizeof(rab));
    if (offset && offset != h->offset) {
        rab.rab$l_kbf = &offset;
        rab.rab$b_rac = 2;//FB$RFA;
        rab.rab$b_ksz = sizeof(offset);
    }

    res = rms_write(h->rmsh, (char *)buf+got, size-got, &rab);
    if (res == -1) {
	    res = -errno;
    }
    else {
	    h->offset += res;
	    res = size;
    }
    return res;
}

static int dapfs_release(const char *path, struct fuse_file_info *fi)
{
	struct dapfs_handle *h = (struct dapfs_handle *)fi->fh;
	int ret;

	if (!h)
		return -EBADF;

	ret = rms_close(h->rmsh);
	free(h);

	return ret;
}

static int dapfs_getattr(const char *path, struct stat *stbuf)
{
	int res;
	if (strcmp(path, "/") == 0) {
		res = stat("/", stbuf);
	}
	else {
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
	.mknod    = dapfs_mknod,
	.chmod    = dapfs_chmod,
	.mkdir    = dapfs_mkdir,
	.statfs   = dapfs_statfs,
	.release  = dapfs_release,
};

int main(int argc, char *argv[])
{
	// This is the host name ending :: (eg zarqon"patrick password"::)
	strcpy(prefix, argv[1]);

	// Make a scratch connection - also verifies the path name nice and early
	if (dap_init()) {
		syslog(LOG_ERR, "Cannot connect to '%s'\n", prefix);
		return -ENOTCONN;
	}

	return fuse_main(argc-1, argv+1, &dapfs_oper);
}

// TODO statfs & mkkdir, use DECnet object used by old dapfs!
