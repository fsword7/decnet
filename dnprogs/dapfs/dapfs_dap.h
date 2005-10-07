#ifdef __cplusplus
extern "C" {
#endif
	int dapfs_readdir_dap(const char *path, void *buf, fuse_fill_dir_t filler,
			      off_t offset, struct fuse_file_info *fi);

	int dapfs_getattr_dap(const char *path, struct stat *stbuf);
#ifdef __cplusplus
}
#endif


extern char prefix[];
