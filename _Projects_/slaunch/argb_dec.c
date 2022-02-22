/*
 *  Copyright (C) 2022 Darjan Krijan <krijan@dkmechatronics.eu>
 *
 *  Simple custom uncompressed ARGB image format for loading
 *  covers instantly on SSD.
 *
 *  See tools/png2argb.c for converting PNG images to ARGB
 */

#include <stdio.h>
#include "include/argb_dec.h"
#include "include/mem.h"
#include "include/vsh_exports.h"

const char argb_magic[] = "ARGB";

enum argb_flags {
	ARGB_FLAG_TRANSPARENCY = 0x1 // image has A values less than 0xFF
};

// 128 byte header, big-endian values
typedef struct {
	char magic[4];        // ASCII "ARGB"
	uint32_t nx;
	uint32_t ny;
	uint32_t flags;       // Used for indicating existing transparent pixels
	uint8_t padding[112]; // Padding to 128 byte cache line boundary on PS3
	// uncompressed pixel data following as ARGB (uint32_t)
} argb_h_t;

/***********************************************************************
* load argb file
* file_path = path to argb file e.g. "/dev_hdd0/test.argb"
***********************************************************************/
Buffer load_argb(const char *file_path, void* buf_addr)
{
	argb_h_t argb_h;
	FILE *fp;

	Buffer tmp;
	tmp.addr = (uint32_t*)buf_addr;

	tmp.b = 0; // transparency
	tmp.w = tmp.h = 0;
	tmp.x = tmp.y = 0;

	fp = fopen(file_path, "rb");
	fread(&argb_h, sizeof(uint8_t), sizeof(argb_h_t), fp); // 128 bytes

	if (memcmp(&argb_h.magic, argb_magic, sizeof(uint32_t)) || !argb_h.nx || !argb_h.ny) {
		fclose(fp);
		return tmp;
	}

	tmp.b = argb_h.flags&ARGB_FLAG_TRANSPARENCY;

	fread(buf_addr, sizeof(uint32_t), (size_t)argb_h.nx*(size_t)argb_h.ny, fp);
	fclose(fp);

	tmp.w = argb_h.nx;
	tmp.h = argb_h.ny;

	return tmp;
}
