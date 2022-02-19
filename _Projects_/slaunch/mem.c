#include <sys/memory.h>

#include "include/mem.h"
#include "include/vsh_exports.h"

#ifndef CBE_CACHE_LINE
#define CBE_CACHE_LINE 128
#endif

static sys_memory_container_t mc_app = (sys_memory_container_t)-1;
static sys_addr_t heap_mem = 0;
static uint32_t heap_size = 0;
static uint32_t prx_heap = 0;

/***********************************************************************
*
***********************************************************************/
void destroy_heap(void)
{
	if(heap_mem) sys_memory_free((sys_addr_t)heap_mem);
	heap_mem = heap_size = prx_heap = 0;
	mc_app = (sys_memory_container_t)-1;
}

/***********************************************************************
* create prx heap from memory container 1("app")
***********************************************************************/
int32_t create_heap(int32_t size)
{
	mc_app = vsh_memory_container_by_id(1);
	if (!mc_app) {destroy_heap(); return(-1);}

	heap_size = MB(size);

	sys_memory_allocate_from_container(heap_size, mc_app, SYS_MEMORY_PAGE_SIZE_1M, &heap_mem);
	if (!heap_mem) {destroy_heap(); return(-1);}

	prx_heap = (uint32_t)heap_mem;
	return(0);
}

/***********************************************************************
*
***********************************************************************/
void *mem_alloc(uint32_t size)
{
	uint32_t add;
	uint32_t align_bytes, pad_bytes;

	align_bytes = CBE_CACHE_LINE - prx_heap%CBE_CACHE_LINE;
	pad_bytes   = CBE_CACHE_LINE -     size%CBE_CACHE_LINE;

	add  = prx_heap + align_bytes; // align to next cache line boundary (only first allocation)
	size = size + pad_bytes;       // pad to next cache line boundary

	if (((prx_heap + align_bytes - heap_mem) + size) < heap_size) {
		prx_heap += align_bytes + size;
		char *mem = (char*)prx_heap;
		*mem = 0;
	}

	return (void*)add;
}

/***********************************************************************
*
***********************************************************************/
int32_t mem_free(uint32_t size)
{
	if (prx_heap >= size + CBE_CACHE_LINE - size%CBE_CACHE_LINE) {
		prx_heap -= size + CBE_CACHE_LINE - size%CBE_CACHE_LINE;
	} else {
		return -1;
	}

	return 0;
}

void reset_heap(void)
{
	prx_heap = (uint32_t)heap_mem;
}
