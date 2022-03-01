/*
 *  Copyright (C) 2022 Darjan Krijan <krijan@dkmechatronics.eu>
 *
 *  Convert a PNG to a simple uncompressed ARGB image format for slaunch.
 *  Uses libspng [BSD 2-Clause] for simplicity and performance reasons.
 *  libspng uses zlib for decompression in this case
 *
 *  Usage:
 *  png2argb <png input file> [<argb output file>]
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "common.h"

#include "spng.h"

#define CHECK_OW 0

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} png_px_t;

typedef struct {
	uint8_t a;
	uint8_t r;
	uint8_t g;
	uint8_t b;
} argb_px_t;

enum argb_flags {
	ARGB_FLAG_TRANSPARENCY = 0x1 // image has A values less than 0xFF
};

// big-endian values
typedef struct argb {
	char magic[4];        // ASCII ARGB
	uint32_t nx;
	uint32_t ny;
	uint32_t flags;       // Used for indicating existing transparent pixels
	uint8_t padding[112]; // Padding to 128 byte cache line boundary on PS3
	argb_px_t *px;
} argb_t;

const char argb_magic[] = "ARGB";

int main(int argc, char *argv[]) {
	int ret = 0;

	FILE *fp_png;
	spng_ctx *lsp_ctx = NULL;
	png_px_t *png_px = NULL;

	uint64_t n;
	size_t png_px_size;

	argb_t argb = {0};
	char file_argb[4096] = {0};
	FILE *fp_argb;

	if (argc < 2 || argc > 3) {
 		printf("Copyright (C) 2022 Darjan Krijan <krijan@dkmechatronics.eu>\n\n");

		printf("png2argb converts a PNG to a simple uncompressed ARGB image format for slaunch.\n");
		printf("Provided under [GPLv3].\n");
		printf("Uses libspng [BSD 2-Clause License] for simplicity and performance reasons.\n");
		printf("libspng uses zlib [zlib License] for decompression in this case.\n\n");

		if (argc < 2) {
			printf("No input file provided!\n");
			printf("Usage: %s <png input file> [<argb output file>]\n", argv[0]);
			printf("Not providing an output file will produce an argb image next to the PNG image.\n", argv[0]);
		} else if (argc > 3) {
			printf("Too many input arguments!\n");
			printf("Usage: %s <png input file> [<argb output file>]\n", argv[0]);
			printf("Not providing an output file will produce an argb image next to the PNG image.\n", argv[0]);
		}

		return EXIT_FAILURE;
	}

	if (!(fp_png = fopen(argv[1], "rb"))) {
		printf("Error opening input PNG file '%s'\n", argv[1]);
		return EXIT_FAILURE;
	}

	if (!(lsp_ctx = spng_ctx_new(0))) {
		printf("spng_ctx_new() failed\n");
		return EXIT_FAILURE;
	}

	spng_set_png_file(lsp_ctx, fp_png);

	struct spng_ihdr ihdr;
	if (ret = spng_get_ihdr(lsp_ctx, &ihdr)) {
		printf("spng_get_ihdr() error: %s\n", spng_strerror(ret));
		return EXIT_FAILURE;
	}

	if (ihdr.width%4) {
		printf("Image '%s' doesn't have a width multiple of 4, aborting.\n", argv[1]);
		return EXIT_FAILURE;
	} else if (ihdr.height%4) {
		printf("Image '%s' doesn't have a height multiple of 4, aborting.\n", argv[1]);
		return EXIT_FAILURE;
	}

	argb.nx = ihdr.width;
	argb.ny = ihdr.height;
	n  = (uint64_t)argb.nx*(uint64_t)argb.ny;

	if (ret = spng_decoded_image_size(lsp_ctx, SPNG_FMT_RGBA8, &png_px_size)) {
		fprintf(stderr, "spng_decoded_image_size() error: %s\n", spng_strerror(ret));
		spng_ctx_free(lsp_ctx);

		return EXIT_FAILURE;
	}

	if (!(png_px = malloc(png_px_size))) {
		fprintf(stderr, "Error: malloc returned NULL with size %lu\n", png_px_size);
		spng_ctx_free(lsp_ctx);

		return EXIT_FAILURE;
	}

	if (ret = spng_decode_image(lsp_ctx, png_px, png_px_size, SPNG_FMT_RGBA8, 0)) {
		printf("spng_decode_image() error: %s\n", spng_strerror(ret));
		free(png_px);
		spng_ctx_free(lsp_ctx);

		return EXIT_FAILURE;
	}

	spng_ctx_free(lsp_ctx);
	fclose(fp_png);

	if (argc == 3) {
		strcpy(file_argb, argv[2]);
	} else { // in same directory as png
		strcpy(file_argb, argv[1]);
		strcpy(&file_argb[strlen(file_argb) - 4], ".argb");
	}

#if CHECK_OW == 1
	if (fp_argb = fopen(file_argb, "r")) {
		printf("Output file '%s' exists, aborting.\n", file_argb);
		fclose(fp_argb);
		return EXIT_FAILURE;
	}
#endif

	if (!(fp_argb = fopen(file_argb, "wb"))) {
		printf("Can't open output file '%s'\n", file_argb);
		return EXIT_FAILURE;
	}

	argb.px = malloc(n*sizeof(argb_px_t));

	for (uint64_t i = 0; i < n; i++) {
		argb.px[i].a = png_px[i].a;
		if (argb.px[i].a < 0xFF) {
			argb.flags |= ARGB_FLAG_TRANSPARENCY;
		}
		argb.px[i].r = png_px[i].r;
		argb.px[i].g = png_px[i].g;
		argb.px[i].b = png_px[i].b;
	}

	free(png_px);

	memcpy(argb.magic, argb_magic, strlen(argb_magic));

#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
	argb.nx    = BE32(argb.nx);
	argb.ny    = BE32(argb.ny);
	argb.flags = BE32(argb.flags);
#endif

	fwrite(&argb, sizeof(uint8_t), 128, fp_argb);
	fwrite(argb.px, sizeof(argb_px_t), n, fp_argb);

	fclose(fp_argb);
	free(argb.px);

	return EXIT_SUCCESS;
}
