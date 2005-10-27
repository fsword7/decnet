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
//  mount -tfuse  dapfs#'alpha1"patrick password"::' /mnt/dap

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
#include "dapfs.h"
#include "dapfs_dap.h"
#include "filenames.h"

/* Copied from dncopy */

struct dapfs_handle
{
	RMSHANDLE rmsh;
	/* RMS File attributes */
	int org;
	int rat;
	int rfm;
	int mrs;
	int fsz;
	/* Last known offset in the file */
	off_t offset;
};

char prefix[BUFLEN];

static const int RAT_DEFAULT = -1; // Use RMS defaults
static const int RAT_FTN  = 1; // RMS RAT values from fab.h
static const int RAT_CR   = 2;
static const int RAT_PRN  = 4;
static const int RAT_NONE = 0;

static const int RFM_DEFAULT = -1; // Use RMS defaults
static const int RFM_UDF = 0; // RMS RFM values from fab.h
static const int RFM_FIX = 1;
static const int RFM_VAR = 2;
static const int RFM_VFC = 3;
static const int RFM_STM = 4;
static const int RFM_STMLF = 5;
static const int RFM_STMCR = 6;

/* Convert RMS record carriage control into something more unixy */
static int convert_rms_record(char *buf, int len, struct dapfs_handle *fh)
{
	int retlen = len;

	/* If the file has implied carriage control then add an LF to the end of the line */
	if ((fh->rfm != RFM_STMLF) &&
	    (fh->rat & RAT_CR || fh->rat & RAT_PRN))
		buf[retlen++] = '\n';

	/* Print files have a two-byte header indicating the line length. */
	if (fh->rat & RAT_PRN)
	{
		memmove(buf, buf + fh->fsz, retlen - fh->fsz);
		retlen -= fh->fsz;
	}

/* FORTRAN files have a leading character that indicates carriage control */
	if (fh->rat & RAT_FTN)
	{
		switch (buf[0])
		{

		case '+': // No new line
			buf[0] = '\r';
			break;

		case '1': // Form Feed
			buf[0] = '\f';
			break;

		case '0': // Two new lines
			memmove(buf+1, buf, retlen+1);
			buf[0] = '\n';
			buf[1] = '\n';
			retlen++;
			break;


		case ' ': // new line
		default:  // Default to a new line. This seems to be what VMS does.
			buf[0] = '\n';
			break;
		}
	}

	return retlen;
}

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

int dapfs_chown(const char *path, uid_t u, gid_t g)
{
	// Can we do this ??
	return 0;
}

static int dapfs_flush(const char *path, struct fuse_file_info *fi)
{
	// NO-OP
	return 0;
}

static int dapfs_truncate(const char *path, off_t size)
{
	RMSHANDLE rmsh;
	int offset;
	int res;
	struct RAB rab;
	char fullname[VMSNAME_LEN];
	char vmsname[VMSNAME_LEN];

	make_vms_filespec(path, vmsname, 0);
	sprintf(fullname, "%s%s", prefix, vmsname);

	rmsh = rms_open(fullname, O_WRONLY, NULL);
	if (!rmsh) {
		return -errno;
	}

	memset(&rab, 0, sizeof(rab));
	if (size) {
		rab.rab$l_kbf = &offset;
		rab.rab$b_rac = 2;//FB$RFA;
		rab.rab$b_ksz = sizeof(offset);
	}
	res = rms_find(rmsh, &rab);
	if (!res)
		goto finish;
	res = rms_truncate(rmsh, NULL);
finish:
	rms_close(rmsh);
	return res;
}

static int dapfs_mkdir(const char *path, mode_t mode)
{
	char fullname[VMSNAME_LEN];
	char vmsname[VMSNAME_LEN];
	char reply[BUFLEN];
	int len;
	char *lastbracket;

	make_vms_filespec(path, vmsname, 0);
	// Ths gives is a name like '[]pjc' which we
	// need to turn into [.pjc]
	lastbracket = strchr(vmsname, ']');
	if (!lastbracket)
		return -EINVAL;

	syslog(1, "dir = %s, vmsname: %s\n", path, vmsname);

	*lastbracket = '.';
	if (vmsname[strlen(vmsname)-1] == '.')
		vmsname[strlen(vmsname)-1] = '\0';
	strcat(vmsname, "]");


	sprintf(fullname, "CREATE %s", vmsname);


	len = get_object_info(fullname, reply);
	if (len != 2) // "OK"
		return -errno;
	else
		return 0;
}

static int dapfs_statfs(const char *path, struct statfs *stbuf)
{
	int len;
	char reply[BUFLEN];
	long size, free;

	len = get_object_info("STATFS", reply);
	if (len <= 0)
		return -errno;

	memset(stbuf, 0, sizeof(*stbuf));

	if (sscanf(reply, "%ld, %ld", &free, &size) != 2)
		return -EINVAL;

	stbuf->f_bsize = 512;
	stbuf->f_blocks = size;
	stbuf->f_bfree = free;
	stbuf->f_bavail = free;

	return 0;
}


/* This gets called for normal files too... */
// TODO mode is ignored.
static int dapfs_mknod(const char *path, mode_t mode, dev_t dev)
{
	RMSHANDLE rmsh;
	char fullname[VMSNAME_LEN];
	char vmsname[VMSNAME_LEN];

	if (!S_ISREG(mode))
		return -ENOSYS;

	make_vms_filespec(path, vmsname, 0);
	sprintf(fullname, "%s%s", prefix, vmsname);

	rmsh = rms_t_open(fullname, O_CREAT|O_WRONLY, "rfm=stmlf");
	if (!rmsh)
		return -errno;
	rms_close(rmsh);
	return 0;
}

static int dapfs_open(const char *path, struct fuse_file_info *fi)
{
	struct dapfs_handle *h;
	struct FAB fab;
	char fullname[VMSNAME_LEN];
	char vmsname[VMSNAME_LEN];

	syslog(LOG_DEBUG, "open %s, flags=%x\n", path, fi->flags);
	h = malloc(sizeof(struct dapfs_handle));
	if (!h)
		return -ENOMEM;

	memset(&fab, 0, sizeof(struct FAB));
	make_vms_filespec(path, vmsname, 0);
	sprintf(fullname, "%s%s", prefix, vmsname);
	if (fi->flags & O_CREAT)
		fab.fab$b_rfm = RFM_STMLF;

	h->rmsh = rms_open(fullname, fi->flags, &fab);
	if (!h->rmsh) {
		int saved_errno = errno;
		free(h);
		return -saved_errno;
	}
	/* Save RMS attributes */
	h->org = fab.fab$b_org;
	h->rat = fab.fab$b_rat;
	h->rfm = fab.fab$b_rfm;
	h->mrs = fab.fab$w_mrs;
	h->fsz = fab.fab$b_fsz;

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

	/* This can be quite slow, because it reads records.
	   However, it's safer this way as we can make sense of sequential files. */
	do {
		res = rms_read(h->rmsh, buf+got, size-got, &rab);
		if (res > 0) {
			res = convert_rms_record(buf+got, res, h);
			got += res;
		}
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
	struct RAB rab;
	struct dapfs_handle *h = (struct dapfs_handle *)fi->fh;

	if (!h) {
		res = dapfs_open(path, fi);
		if (!res)
			return res;
	}

	syslog(LOG_DEBUG, "dapfs_write. offset=%d, (%p) fh->offset=%d\n", (int)offset, h, (int)h->offset);
	memset(&rab, 0, sizeof(rab));
	if (offset && offset != h->offset) {
		rab.rab$l_kbf = &offset;
		rab.rab$b_rac = 2;//FB$RFA;
		rab.rab$b_ksz = sizeof(offset);
	}

	res = rms_write(h->rmsh, (char *)buf, size, &rab);
	if (res == -1) {
		syslog(LOG_DEBUG, "rms_write returned %d, errno=%d\n", res, errno);
		res = -errno;
	}
	else {
		h->offset += size;
		res = size;
		syslog(LOG_DEBUG, "rms_write returned 0, offset (%p) now=%d\n", h, (int)h->offset);
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
	fi->fh = 0L;

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

		/* If this failed and there's no file type, see if it is a directory */
		if (res == -ENOENT && strchr(path, '.')==NULL) {
			char dirname[BUFLEN];
			sprintf(dirname, "%s.dir", path);
			res = dapfs_getattr_dap(dirname, stbuf);
		}
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
	.chown    = dapfs_chown,
	.mkdir    = dapfs_mkdir,
	.flush    = dapfs_flush,
	.statfs   = dapfs_statfs,
	.release  = dapfs_release,
};

int main(int argc, char *argv[])
{
	if (argc < 2)
		return 1;

	// This is the host name ending :: (eg zarqon"patrick password"::)
	strcpy(prefix, argv[1]);

	// Make a scratch connection - also verifies the path name nice and early
	if (dap_init()) {
		syslog(LOG_ERR, "Cannot connect to '%s'\n", prefix);
		return -ENOTCONN;
	}

	return fuse_main(argc-1, argv+1, &dapfs_oper);
}

