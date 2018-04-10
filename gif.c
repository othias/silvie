/*
 * gif.c -- GIF helper functions
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

#include <gif_lib.h>
#include <stdbool.h>
#include <stddef.h>
#include "error.h"
#include "gif.h"
#include "utils.h"

bool slv_read_pal(const char *path, struct GifColorType *colors,
                  struct slv_err *err)
{
	FILE *pal = slv_fopen(path, "rb", err);
	if (!pal)
		return false;
	bool ret = false;
	for (size_t i = 0; i < SLV_NUM_PAL_COLORS; ++i) {
		struct GifColorType *color = &colors[i];
		if (slv_fread(&color->Red, 1, 1, pal, err) != 1
		    || slv_fread(&color->Green, 1, 1, pal, err) != 1
		    || slv_fread(&color->Blue, 1, 1, pal, err) != 1)
			goto close_pal;
	}
	ret = true;
close_pal:
	fclose(pal);
	return ret;
}

struct GifFileType *slv_open_gif(const struct slv_gif_opts *opts,
                                 struct slv_err *err)
{
	struct GifFileType *gif = EGifOpenFileName(opts->file_path, false,
	                                           &err->code);
	if (!gif) {
		err->lib = SLV_LIB_GIF;
		return NULL;
	}
	struct GifFileType *ret = NULL;
	struct ColorMapObject *map = GifMakeMapObject(opts->num_colors,
	                                              opts->colors);
	if (!map) {
		slv_set_errno(err);
		goto close_gif;
	}
	if (!EGifPutScreenDesc(gif, opts->width, opts->height, opts->num_colors,
	                       0, map)) {
		slv_set_err(err, SLV_LIB_GIF, gif->Error);
		goto free_map;
	}
	if (opts->animated) {
		GifByteType block_0[11] = "NETSCAPE2.0";
		GifByteType block_1[] = {0x01, 0x00, 0x00};
		if (!EGifPutExtensionLeader(gif, APPLICATION_EXT_FUNC_CODE)
		    || !EGifPutExtensionBlock(gif, sizeof block_0, block_0)
		    || !EGifPutExtensionBlock(gif, sizeof block_1, block_1)
		    || !EGifPutExtensionTrailer(gif)) {
			slv_set_err(err, SLV_LIB_GIF, gif->Error);
			goto free_map;
		}
	}
	ret = gif;
free_map:
	GifFreeMapObject(map);
close_gif:
	if (!ret)
		EGifCloseFile(gif, NULL);
	return ret;
}

bool slv_fill_gif(struct GifFileType *gif, const struct slv_gif_buf_info *info,
                  struct slv_err *err)
{
	if (info->alpha) {
		struct GraphicsControlBlock block = {
			.DisposalMode = info->disposal,
			.DelayTime = info->delay,
		};
		GifByteType extension[4];
		if (!EGifGCBToExtension(&block, extension)) {
			slv_set_errno(err);
			return false;
		}
		if (!EGifPutExtension(gif, GRAPHICS_EXT_FUNC_CODE,
		                      sizeof extension, extension))
			goto set_err;
	}
	if (!EGifPutImageDesc(gif, info->left, info->top, info->width,
	                      info->height, false, NULL))
		goto set_err;
	for (int i = 0; i < info->height; ++i)
		if (!EGifPutLine(gif, &info->buf[i * info->width], info->width))
			goto set_err;
	return true;
set_err:
	slv_set_err(err, SLV_LIB_GIF, gif->Error);
	return false;
}
