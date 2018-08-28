/*
 * raw.c -- RAW image format
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
#include <stdlib.h>
#include "asset.h"
#include "error.h"
#include "gif.h"
#include "raw.h"
#include "stream.h"
#include "utils.h"

static bool check_args(const void *me)
{
	return slv_check_args(me, 2, SLV_ERR_RAW_ARGS);
}

static bool load(void *me, struct slv_stream *stream)
{
	struct slv_raw *raw = me;
	struct slv_raw_hdr *hdr = &raw->hdr;
	return slv_read_le(stream, &hdr->cst_0)
	       && slv_read_le(stream, &hdr->width_0)
	       && slv_read_le(stream, &hdr->height)
	       && slv_read_le(stream, &hdr->cst_1)
	       && slv_read_le(stream, &hdr->buf_sz)
	       && (raw->buf = slv_malloc(hdr->buf_sz, stream->err))
	       && slv_read_le(stream, &hdr->width_1)
	       && slv_read_le(stream, &hdr->unk_0)
	       && slv_read_le(stream, &hdr->unk_1)
	       && slv_read_le(stream, &hdr->unk_2)
	       && slv_read_le(stream, &hdr->unk_3)
	       && slv_read_le(stream, &hdr->unk_4)
	       && slv_read_buf(stream, raw->colors, sizeof raw->colors)
	       && slv_read_buf(stream, raw->buf, hdr->buf_sz);
}

static bool save(const void *me)
{
	const struct slv_raw *raw = me;
	const struct slv_raw_hdr *hdr = &raw->hdr;
	struct GifColorType colors[SLV_NUM_RAW_COLORS];
	for (size_t i = 0, j = 0; i < SLV_NUM_RAW_COLORS; ++i, j += 3)
		colors[i] = (struct GifColorType) {
			.Red = raw->colors[j],
			.Green = raw->colors[j + 1],
			.Blue = raw->colors[j + 2],
		};
	int width;
	int height;
	if (!slv_ul_to_i(hdr->width_0, &width, raw->asset.err)
	    || !slv_ul_to_i(hdr->height, &height, raw->asset.err))
		return false;
	struct GifFileType *gif = slv_open_gif(&(struct slv_gif_opts) {
		.file_path = raw->asset.out,
		.num_colors = SLV_NUM_RAW_COLORS,
		.colors = colors,
		.width = width,
		.height = height,
	}, raw->asset.err);
	if (!gif)
		return false;
	bool ret = false;
	if (!slv_fill_gif(gif, &(struct slv_gif_buf_info) {
		.width = width,
		.height = height,
		.buf = raw->buf,
	}, raw->asset.err))
		goto close_gif;
	ret = true;
close_gif:
	EGifCloseFile(gif, NULL);
	return ret;
}

static void del(void *me)
{
	struct slv_raw *raw = me;
	free(raw->buf);
	free(raw);
}

static const struct slv_asset_ops ops = {
	.check_args = check_args,
	.load = load,
	.save = save,
	.del = del,
};

struct slv_asset *slv_new_raw(char **args, struct slv_err *err)
{
	return slv_alloc(1, sizeof (struct slv_raw), &(struct slv_raw) {
		.asset = {
			.ops = &ops,
			.args = args,
			.out = args[1],
			.err = err,
		},
	}, err);
}
