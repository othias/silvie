/*
 * silvie.c -- Entry point
 * Copyright (C) 2018 Lucas Petitiot <lucas.petitiot@gmail.com>
 *
 * Silvie is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Silvie is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Silvie.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asset.h"
#include "chr.h"
#include "eng.h"
#include "error.h"
#include "pak.h"
#include "raw.h"
#include "spr.h"
#include "stream.h"
#include "utils.h"

static bool process(struct slv_asset *asset)
{
	struct slv_stream *stream;
	bool ret = false;
	if (!SLV_CALL(check_args, asset)
	    || !(stream = slv_new_fs(asset->argv[2], asset->err)))
		goto del_asset;
	puts("Loading asset...");
	if (!SLV_CALL(load, asset, stream)
	    || ((void)puts("Saving asset..."), !SLV_CALL(save, asset)))
		goto del_stream;
	ret = true;
del_stream:
	SLV_DEL(stream);
del_asset:
	SLV_DEL(asset);
	return ret;
}

#define FORMATS(X) \
	X(chr, "3D model, saved as a 3DS file and a GIF file")          \
	X(eng, "Dialog text, saved as an XML file")                     \
	X(pak, "Level archive, saved as a GIF file (incomplete)")       \
	X(raw, "Raw image, saved as a GIF file")                        \
	X(spr, "Spritesheet, saved as GIF files")

int main(int argc, char *argv[])
{
	if (argc == 1) {
		puts("This is Silvie, an asset extractor for Silver.\n"
		     "The following formats are supported:\n\n"
#define X(fmt, desc) "\t" #fmt "\t" desc "\n"
		     FORMATS(X)
#undef X
		     "\nFor usage information on a given format, type:\n\n"
		     "\tsilvie format\n\n"
		     "A prefix argument denotes the common part of the paths "
		     "to the saved files.");
		return EXIT_FAILURE;
	}
	puts("Silvie v0.1\n"
	     "Copyright (C) 2018 Lucas Petitiot.\n"
	     "This is free software; "
	     "see the LICENSE file for copying conditions.\n");
	const char *formats[] = {
#define X(fmt, desc) #fmt,
		FORMATS(X)
#undef X
	};
	for (size_t i = 0; i < SLV_LEN(formats); ++i) {
		if (strcmp(argv[1], formats[i]))
			continue;
		slv_asset_ctor *new_asset = (slv_asset_ctor *[]) {
#define X(fmt, desc) slv_new_##fmt,
			FORMATS(X)
#undef X
		}[i];
		struct slv_err err = {0};
		struct slv_asset *asset = new_asset(argc, argv, &err);
		if (asset && process(asset))
			return EXIT_SUCCESS;
		fprintf(stderr, "%s\n", slv_err_msg(&err));
		return EXIT_FAILURE;
	}
	fputs("Unsupported format\n", stderr);
	return EXIT_FAILURE;
}
