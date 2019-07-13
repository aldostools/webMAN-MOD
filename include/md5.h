#ifdef CALC_MD5

#include <cell/hash/libmd5.h>

static void calc_md5(char *filename, char *md5)
{
	u64 _md5[2] = {0, 0};

	sys_addr_t sysmem = NULL; size_t buffer_size = _256KB_;

	if(webman_config->vsh_mc)
	{
		sys_memory_container_t vsh_mc = get_vsh_memory_container();
		if(vsh_mc)	sys_memory_allocate_from_container(buffer_size, vsh_mc, SYS_MEMORY_PAGE_SIZE_64K, &sysmem);
	}

	if(!sysmem) buffer_size = _128KB_;

	if(sysmem || (!sysmem && sys_memory_allocate(buffer_size, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK))
	{
		int fd;

		if(cellFsOpen(filename, CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
		{
			CellMd5WorkArea workarea;

			cellMd5BlockInit(&workarea);

			u8 *buf = (u8 *)sysmem;

			for(u64 nread = buffer_size; nread > 0; )
			{
				cellFsRead(fd, buf, buffer_size, &nread);

				cellMd5BlockUpdate(&workarea, buf, nread);
			}

			cellFsClose(fd);
			cellMd5BlockResult(&workarea, (u8*)_md5);
		}

		sys_memory_free(sysmem);
	}

	// return md5 hash as a string
	sprintf(md5, "%016llx%016llx", _md5[0], _md5[1]);
}

#endif
