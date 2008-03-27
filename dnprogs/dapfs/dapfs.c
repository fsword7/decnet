/******************************************************************************
    (c) 2005-2008 Christine Caulfield            christine.caulfield@gmail.com

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
//  # mount -tdapfs alpha1 /mnt/dap
//  # mount -tdapfs zarqon /mnt/dap -ousername=christine,password=password

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
	char *recordbuf; // For when we need to split a record
	int  rbuf_len;
};

char prefix[BUFLEN];
static char mountdir[BUFLEN];
int debuglevel = 0;

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

	/* If the file has implied carriage control then add a CR/LF to the end of the line. */
	if ((fh->rfm != RFM_STMLF) &&
	    (fh->rat & RAT_CR || fh->rat & RAT_PRN)) {
		buf[retlen++] = '\n';
	}

	/* Print files have a two-byte header indicating the line length. */
	if (fh->rat & RAT_PRN && len >= fh->fsz)
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
	if (debuglevel&1)
		fprintf(stderr, "dapfs_readdir: %s\n", path);

	return dapfs_readdir_dap(path, buf, filler, offset, fi);
}


static int dapfs_unlink(const char *path)
{
	char vername[strlen(path)+3];
	if (debuglevel&1)
		fprintf(stderr, "dapfs_unlink: %s\n", path);

	sprintf(vername, "%s;*", path);
	return dap_delete_file(vername);
}

/* We can't do chown/chmod/utime but don't error as the user gets annoyed */
static int dapfs_chown(const char *path, uid_t u, gid_t g)
{
	return 0;
}

static int dapfs_chmod(const char *path, mode_t m)
{
	return 0;
}
static int dapfs_utime(const char *path, struct utimbuf *u)
{
	return 0;
}

static int dapfs_rmdir(const char *path)
{
	char dirname[strlen(path)+7];
	char fullname[VMSNAME_LEN];
	char vmsname[VMSNAME_LEN];
	char reply[BUFLEN];
	int len;

	if (debuglevel&1)
		fprintf(stderr, "dapfs_rmdir: %s\n", path);

	/* Try the object first. if that fails then
	   use DAP. This is because the VMS protection on
	   directories can be problematic */
	make_vms_filespec(path, vmsname, 0);

	if (vmsname[strlen(vmsname)-1] == '.')
		vmsname[strlen(vmsname)-1] = '\0';

	sprintf(fullname, "REMOVE %s.DIR;1", vmsname);
	len = get_object_info(fullname, reply);
	if (len == 2) // "OK"
		return 0;

	sprintf(dirname, "%s.DIR;1", path);
	return dap_delete_file(dirname);
}

static int dapfs_rename(const char *from, const char *to)
{
	if (debuglevel&1)
		fprintf(stderr, "dapfs_rename: from: %s to: %s\n", from, to);
	return dap_rename_file(from, to);
}

static int dapfs_truncate(const char *path, off_t size)
{
	RMSHANDLE rmsh;
	int offset;
	int res;
	struct RAB rab;
	char fullname[VMSNAME_LEN];
	char vmsname[VMSNAME_LEN];
	if (debuglevel&1)

		fprintf(stderr, "dapfs_truncate: %s, %lld\n", path, size);

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
	char *lastbracket;
	int len;

	if (debuglevel&1)
		fprintf(stderr, "dapfs_mkdir: %s\n", path);

	make_vms_filespec(path, vmsname, 0);
	// for a top-level directory,
	// Ths gives is a name like 'newdir' which we
	// need to turn into [.newdir]
	if (vmsname[0] != '[') {
		memmove(vmsname+2, vmsname, strlen(vmsname)+1);
		vmsname[0]='[';
		vmsname[1]='.';
	}
	/* Replace closing ']' with '.'. eg
	   [mydir]newdir] becomes
	   [mydir.newdir]
	*/
	lastbracket = strchr(vmsname, ']');
	if (lastbracket)
		*lastbracket = '.';

	/* make_vms_filespec() often leaves a trailing dot */
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

	if (debuglevel&1)
		fprintf(stderr, "dapfs_stafs: %s\n", path);

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
/* Note, mode is ignored */
static int dapfs_mknod(const char *path, mode_t mode, dev_t dev)
{
	RMSHANDLE rmsh;
	char fullname[VMSNAME_LEN];
	char vmsname[VMSNAME_LEN];

	if (debuglevel&1)
		fprintf(stderr, "dapfs_mknod: %s\n", path);

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

	if (debuglevel&1)
		fprintf(stderr, "open %s, flags=%x\n", path, fi->flags);

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

		if (!saved_errno) // Catch all...TODO
			saved_errno = -ENOENT;
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
	unsigned int loffset = 0;
	struct RAB rab;
	struct dapfs_handle *h = (struct dapfs_handle *)fi->fh;

	if (debuglevel&1)
		fprintf(stderr, "dapfs_read: %s offset=%lld\n", path, offset);

	if (!h) {
		res = dapfs_open(path, fi);
		if (res)
			return res;
		h = (struct dapfs_handle *)fi->fh;
	}

	memset(&rab, 0, sizeof(rab));
	if (offset && offset != h->offset) {
		if (debuglevel&2)
			fprintf(stderr, "dapfs_read: new offset is %lld, old was %lld\n", offset, h->offset);
		loffset = (unsigned int)offset;
		rab.rab$l_kbf = &offset;
		rab.rab$b_rac = 6;// Stream 2;//FB$RFA;
		rab.rab$b_ksz = 6;// 3x words, like an RFA// sizeof(loffset);
		rab.rab$w_usz = size;

		h->offset = offset;

		/* Throw away saved partial record */
		if (h->recordbuf)
			free(h->recordbuf);
		h->recordbuf = NULL;
	}

	// Add in saved record
	if (h->recordbuf) {
		if (debuglevel&2)
			fprintf(stderr, "dapfs_read: adding saved partial record %d bytes\n", h->rbuf_len);

		res = convert_rms_record(h->recordbuf, h->rbuf_len, h);

		memcpy(buf, h->recordbuf, res);
		got += res;
		h->offset += res;
		free(h->recordbuf);
		h->recordbuf = NULL;
	}

	/* This can be quite slow, because it reads records.
	   However, it's safer this way as we can make sense of sequential files
	*/
	do {
		res = rms_read(h->rmsh, buf+got, size-got, &rab);

		if (rms_lasterror(h->rmsh) && debuglevel&2)
			fprintf(stderr, "dapfs_read: res=%d, rms error: %s\n", res, rms_lasterror(h->rmsh));

		if (res == -1)
			return -EOPNOTSUPP;

		// if res == 0 and there is no error then we read
		// an empty record.
		if (res >= 0 && !rms_lasterror(h->rmsh)) {
			res = convert_rms_record(buf+got, res, h);
			got += res;
			h->offset += res;
		}
		if (res < 0 && got) {
			int recordlen = -res;
			int remainderspace = size-got;

			// Not enough room for a full record. split it.
			// Read the partial record and return as much as the user
			// requested. Save the remainder for the next read.
			h->recordbuf = malloc(recordlen);
			if (!h->recordbuf)
				return -ENOMEM;

			recordlen = rms_read(h->rmsh, h->recordbuf, recordlen, &rab);
			memcpy(buf+got, h->recordbuf, remainderspace);

			memmove(h->recordbuf, h->recordbuf + remainderspace, recordlen-remainderspace);
			h->rbuf_len = recordlen-remainderspace;
			h->offset += remainderspace;

			if (debuglevel&2)
				fprintf(stderr, "dapfs_read: saving %d bytes of %d byte partial record\n", recordlen-remainderspace, recordlen);

			if (debuglevel&1)
				fprintf(stderr, "dapfs_read: returning %d, offset = %lld\n", got+remainderspace, h->offset);

			return got + remainderspace;
		}

	} while (!rms_lasterror(h->rmsh));
	if (res >= 0)
		res = got;

	if (res == -1)
		res = -errno;

	if (debuglevel&1)
		fprintf(stderr, "dapfs_read: returning %d, offset=%lld\n", res, h->offset);
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
		if (res)
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

	if (debuglevel&1)
		fprintf(stderr, "dapfs_release: %s\n", path);

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

	if (debuglevel&1)
		fprintf(stderr, "dapfs_getattr: %s\n", path);

	memset(stbuf,0x0, sizeof(*stbuf));

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
	if (debuglevel&1)
		fprintf(stderr, "dapfs_getattr: returning %d\n", res);
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
	.mkdir    = dapfs_mkdir,
	.chown    = dapfs_chown,
	.chmod    = dapfs_chmod,
	.utime    = dapfs_utime,
	.statfs   = dapfs_statfs,
	.release  = dapfs_release,
};

static void process_options(char *options)
{
	char *scratch = strdup(options);
	char *t;
	char *password = NULL;
	char *username = NULL;
	char *optptr;

	if (!scratch)
		return;

	t = strtok(scratch, ",");
	while (t)
	{
		char *option;

		option = strchr(t, '=');
		if (!option)
			goto next_tok;
		option++;

		optptr = t + strspn(t, " ");
		if (strncmp("username=", optptr, 9) == 0)
			username = strdup(option);
		if (strncmp("password=", optptr, 9) == 0)
			password = strdup(option);
		if (strncmp("debug=", optptr, 6) == 0)
			debuglevel = atoi(option);
	next_tok:
		t = strtok(NULL, ",");
	}
	if (!password)
		password = "";
	if (username)
		sprintf(prefix, "%s\"%s %s\"", prefix, username, password);

	free(scratch);
}

static void find_options(int *argc, char *argv[])
{
	int i;

	// Find -o, and process any we find
	for (i=0; i < *argc; i++)
	{
		if (strncmp(argv[i], "-o", 2) == 0)
		{
			// Allow -o options as well as
			//       -ooptions
			if (strlen(argv[i]) == 2) {
				argv[i] = NULL;
				process_options(argv[++i]);
			}
			else {
				process_options(argv[i] + 2);
			}
			argv[i] = NULL;
		}
	}

	// Remove any NULL args. These will be ones we have parsed but
	// mount_fuse will choke on
	for (i=1; i < *argc; i++)
	{
		if (argv[i] == NULL)
		{
			int j;
			for (j = i; j < *argc; j++)
				argv[j] = argv[j+1];
			(*argc)--;
			i--;
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "   mount.dapfs <node> <mountpoint> -ousername=<user>,password=<password>\n");
		return 1;
	}

	// This is just the host name at the moment
	strcpy(prefix, argv[1]);

	// Save the location we are mounted on.
	strcpy(mountdir, argv[2]);

	// Get username and password and other things from -o
	find_options(&argc, argv);

	// Add "::" to the hostname to get a prefix, now that the username
	// and password have been added in, if provided.
	strcat(prefix, "::");

	if (debuglevel&2)
		fprintf(stderr, "prefix is now: %s\n", prefix);

	// Make a scratch connection - also verifies the path name nice and early
	if (dap_init()) {
		syslog(LOG_ERR, "Cannot connect to '%s'\n", prefix);
		return -ENOTCONN;
	}

	return fuse_main(argc-1, argv+1, &dapfs_oper);
}

