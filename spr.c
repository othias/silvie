/*
 * spr.c -- SPR spritesheet format
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
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "asset.h"
#include "error.h"
#include "gif.h"
#include "spr.h"
#include "stream.h"
#include "utils.h"

static bool check_args(const void *me)
{
	return slv_check_argc(me, 5, SLV_ERR_SPR_ARGS);
}

static bool load_animations(struct slv_spr *spr, struct slv_stream *stream)
{
	struct slv_spr_hdr *hdr = &spr->hdr;
	if (!(spr->unks = slv_malloc(hdr->num_animations * sizeof spr->unks[0],
	                             stream->err))
	    || !(spr->animations = slv_alloc(hdr->num_animations,
	                                     sizeof spr->animations[0],
	                                     &(struct slv_spr_anim) {0},
	                                     stream->err))
	    || !slv_read_le_arr(stream, hdr->num_animations, spr->unks))
		return false;
	for (size_t i = 0; i < hdr->num_animations; ++i) {
		struct slv_spr_anim *anim = &spr->animations[i];
		if (!slv_read_le(stream, &anim->num_frames)
		    || !slv_read_le(stream, &anim->delay)
		    || !slv_read_le(stream, &anim->num_indices)
		    || !(anim->indices = slv_malloc(anim->num_indices *
		                                    sizeof anim->indices[0],
		                                    stream->err))
		    || !slv_read_le(stream, &anim->fst_frame_idx)
		    || !slv_read_le(stream, &anim->unk)
		    || !slv_read_le_arr(stream, anim->num_indices,
		                        anim->indices))
			return false;
	}
	return true;
}

static bool load(void *me, struct slv_stream *stream)
{
	struct slv_spr *spr = me;
	struct slv_spr_hdr *hdr = &spr->hdr;
	if (!slv_read_le(stream, &hdr->file_sz)
	    || !slv_read_le(stream, &hdr->file_id)
	    || !slv_read_le(stream, &hdr->format)
	    || !slv_read_le(stream, &hdr->num_frames)
	    || !(spr->frame_infos = slv_malloc(hdr->num_frames *
	                                       sizeof spr->frame_infos[0],
	                                       stream->err))
	    || !(spr->frames = slv_alloc(hdr->num_frames,
	                                 sizeof spr->frames[0],
	                                 &(struct slv_spr_frame) {0},
	                                 stream->err))
	    || !slv_read_le(stream, &hdr->unk_0)
	    || !slv_read_le(stream, &hdr->num_animations)
	    || !slv_read_le(stream, &hdr->unk_1))
		return false;
	for (size_t i = 0; i < hdr->num_frames; ++i) {
		struct slv_spr_frame_info *frame_info = &spr->frame_infos[i];
		if (!slv_read_le(stream, &frame_info->sz)
		    || !slv_read_le(stream, &frame_info->width)
		    || !slv_read_le(stream, &frame_info->height)
		    || !slv_read_le(stream, &frame_info->left)
		    || !slv_read_le(stream, &frame_info->top))
			return false;
	}
	if (hdr->num_animations) {
		if (!load_animations(spr, stream))
			return false;
		/*
		 * The below hack is necessary because the animation data from
		 * SPINALL.SPR seems to be incorrect
		 */
		if (hdr->file_id == 43)
			spr->animations[0].num_frames = 30;
	}
	for (size_t i = 0; i < hdr->num_frames; ++i) {
		struct slv_spr_frame *frame = &spr->frames[i];
		if (!slv_read_le(stream, &frame->sz))
			return false;
		if (frame->sz != spr->frame_infos[i].sz) {
			slv_set_err(stream->err, SLV_LIB_SLV,
			            SLV_ERR_SPR_FRAME);
			return false;
		}
		if (!(frame->data = slv_malloc(frame->sz, stream->err))
		    || !slv_read_buf(stream, frame->data, frame->sz))
			return false;
	}
	if (hdr->file_sz + 4 != stream->pos) {
		slv_set_err(stream->err, SLV_LIB_SLV, SLV_ERR_FILE_SZ);
		return false;
	}
	return true;
}

static bool read_rle_strides(unsigned char *row, struct slv_stream *stream)
{
	unsigned row_sz;
	if (!slv_read_le(stream, &row_sz))
		return false;
	for (size_t len; row_sz; row += len) {
		char num_pixels;
		if (!slv_read_buf(stream, &num_pixels, 1))
			return false;
		--row_sz;
		if (num_pixels < 0) {
			len = (size_t)-num_pixels;
			memset(row, 0, len);
			continue;
		}
		len = (size_t)num_pixels;
		if (!slv_read_buf(stream, row, len))
			return false;
		row_sz -= len;
	}
	return true;
}

struct saved_frame {
	const struct slv_spr_frame_info *info;
	unsigned char *buf;
	unsigned char *mask;
	bool in_anim;
	bool free_buf;
};

static bool read_rle(const struct slv_spr_frame *frame,
                     struct saved_frame *saved, struct slv_err *err)
{
	saved->free_buf = true;
	struct slv_stream *stream = slv_new_ms(frame->data, frame->sz, err);
	if (!stream)
		return false;
	unsigned width;
	unsigned height;
	bool ret = false;
	if (!slv_read_le(stream, &width)
	    || !slv_read_le(stream, &height))
		goto del_stream;
	if (width != saved->info->width
	    || height != saved->info->height) {
		slv_set_err(err, SLV_LIB_SLV, SLV_ERR_SPR_RES);
		goto del_stream;
	}
	if (!(saved->buf = slv_malloc(width * height, err)))
		goto del_stream;
	for (size_t i = 0; i < height; ++i)
		if (!read_rle_strides(&saved->buf[i * width], stream))
			goto del_stream;
	if (frame->sz != stream->pos) {
		slv_set_err(err, SLV_LIB_SLV, SLV_ERR_SPR_FRAME);
		goto del_stream;
	}
	ret = true;
del_stream:
	SLV_DEL(stream);
	return ret;
}

static bool read_mask_strides(unsigned char *row, struct slv_stream *stream)
{
	unsigned row_sz;
	if (!slv_read_le(stream, &row_sz))
		return false;
	for (size_t len; row_sz--; row += len) {
		char num_pixels;
		if (!slv_read_buf(stream, &num_pixels, 1))
			return false;
		if (num_pixels < 0) {
			len = (size_t)-num_pixels;
			memset(row, 0, len);
		} else {
			len = (size_t)num_pixels;
			memset(row, 1, len);
		}
	}
	return true;
}

static bool read_with_masks(const struct slv_spr_frame *frame,
                            struct saved_frame *saved, struct slv_err *err)
{
	saved->free_buf = false;
	struct slv_stream *stream = slv_new_ms(frame->data, frame->sz, err);
	if (!stream)
		return false;
	unsigned long width = saved->info->width;
	unsigned long height = saved->info->height;
	unsigned mask_sz;
	bool ret = false;
	if (!(saved->mask = slv_malloc(width * height, err))
	    || !slv_read_le(stream, &mask_sz))
		goto del_stream;
	for (size_t i = 0; i < height; ++i)
		if (!read_mask_strides(&saved->mask[i * width], stream))
			goto del_stream;
	if (mask_sz != stream->pos) {
		slv_set_err(err, SLV_LIB_SLV, SLV_ERR_SPR_MASK);
		goto del_stream;
	}
	saved->buf = &frame->data[stream->pos];
	ret = true;
del_stream:
	SLV_DEL(stream);
	return ret;
}

static bool read_uncompressed(const struct slv_spr_frame *frame,
                              struct saved_frame *saved, struct slv_err *err)
{
	(void)err;
	saved->free_buf = false;
	saved->buf = frame->data;
	return true;
}

static bool save_anim(const struct slv_spr_anim *anim,
                      struct saved_frame *frames, const struct slv_spr *spr,
                      struct slv_gif_opts *opts)
{
	if (anim->num_frames == 1)
		return true; // Skip one-frame animations
	int left = INT_MAX;
	int top = INT_MAX;
	int right = INT_MIN;
	int bottom = INT_MIN;
	for (size_t i = 0; i < anim->num_frames; ++i) {
		long idx = anim->indices[i];
		const struct slv_spr_frame_info *info = &spr->frame_infos[idx];
		int width;
		int height;
		int info_left;
		int info_top;
		if (!slv_ul_to_i(info->width, &width, spr->asset.err)
		    || !slv_ul_to_i(info->height, &height, spr->asset.err)
		    || !slv_l_to_i(info->left, &info_left, spr->asset.err)
		    || !slv_l_to_i(info->top, &info_top, spr->asset.err))
			return false;
		left = slv_min(left, info_left);
		top = slv_min(top, info_top);
		right = slv_max(right, info_left + width);
		bottom = slv_max(bottom, info_top + height);
	}
	opts->width = right - left;
	opts->height = bottom - top;
	struct GifFileType *gif = slv_open_gif(opts, spr->asset.err);
	if (!gif)
		return false;
	bool ret = false;
	for (size_t i = 0; i < anim->num_frames; ++i) {
		long idx = anim->indices[i];
		const struct slv_spr_frame_info *info = &spr->frame_infos[idx];
		int delay;
		if (!slv_ul_to_i(anim->delay, &delay, spr->asset.err)
		    || !slv_fill_gif(gif, &(struct slv_gif_buf_info) {
			.left = (int)info->left - left,
			.top = (int)info->top - top,
			.width = (int)info->width,
			.height = (int)info->height,
			.buf = frames[idx].buf,
			.alpha = true,
			.disposal = DISPOSE_BACKGROUND,
			.delay = delay,
		}, spr->asset.err))
			goto close_gif;
		frames[idx].in_anim = true;
	}
	ret = true;
close_gif:
	EGifCloseFile(gif, NULL);
	return ret;
}

static bool save_frame(struct slv_gif_opts *opts, struct slv_gif_buf_info *info,
                       struct slv_err *err)
{
	struct GifFileType *gif = slv_open_gif(opts, err);
	if (!gif)
		return false;
	bool ret = false;
	if (!slv_fill_gif(gif, info, err))
		goto close_gif;
	ret = true;
close_gif:
	EGifCloseFile(gif, NULL);
	return ret;
}

typedef bool frame_reader(const struct slv_spr_frame *, struct saved_frame *,
                          struct slv_err *);

static bool save(const void *me)
{
	const struct slv_spr *spr = me;
	const struct slv_spr_hdr *hdr = &spr->hdr;
	char *path;
	char *suffix;
	if (!(path = slv_suf(&spr->asset, &suffix, sizeof "x000.gif")))
		return false;
	struct saved_frame *frames = slv_alloc(hdr->num_frames,
	                                       sizeof frames[0],
	                                       &(struct saved_frame) {0},
	                                       spr->asset.err);
	bool ret = false;
	if (!frames)
		goto free_path;
	if (hdr->format > SLV_SPR_UNCOMPRESSED) {
		slv_set_err(spr->asset.err, SLV_LIB_SLV, SLV_ERR_SPR_FORMAT);
		goto free_frames;
	}
	frame_reader *read_frame = (frame_reader *[]) {
		[SLV_SPR_RLE] = read_rle,
		[SLV_SPR_WITH_MASKS] = read_with_masks,
		[SLV_SPR_UNCOMPRESSED] = read_uncompressed,
	}[hdr->format];
	struct GifColorType pal_colors[SLV_NUM_PAL_COLORS];
	if (!slv_read_pal(spr->asset.argv[3], pal_colors, spr->asset.err))
		goto free_frames;
	struct slv_gif_opts opts = {
		.file_path = path,
		.num_colors = SLV_NUM_PAL_COLORS,
		.colors = pal_colors,
		.animated = true,
	};
	for (size_t i = 0; i < hdr->num_frames; ++i) {
		frames[i].info = &spr->frame_infos[i];
		if (!read_frame(&spr->frames[i], &frames[i], spr->asset.err))
			goto free_frames;
	}
	for (size_t i = 0; i < hdr->num_animations; ++i)
		if (slv_sprintf(suffix, spr->asset.err, "a%.3zu.gif", i) < 0
		    || !save_anim(&spr->animations[i], frames, spr, &opts))
			goto free_frames;
	opts.animated = false;
	for (size_t i = 0; i < hdr->num_frames; ++i) {
		if (frames[i].in_anim)
			continue;
		const struct slv_spr_frame_info *info = frames[i].info;
		if (slv_sprintf(suffix, spr->asset.err, "f%.3zu.gif", i) < 0
		    || !slv_ul_to_i(info->width, &opts.width, spr->asset.err)
		    || !slv_ul_to_i(info->height, &opts.height, spr->asset.err))
			goto free_frames;
		struct slv_gif_buf_info buf_info = {
			.width = opts.width,
			.height = opts.height,
			.buf = frames[i].buf,
			.alpha = true,
			.disposal = DISPOSAL_UNSPECIFIED,
		};
		if (!save_frame(&opts, &buf_info, spr->asset.err))
			goto free_frames;
		if (!frames[i].mask)
			continue;
		suffix[0] = 'm';
		struct GifColorType mask_colors[] = {
			{0, 0, 0},
			{0xff, 0xff, 0xff},
		};
		opts.num_colors = SLV_LEN(mask_colors);
		opts.colors = mask_colors;
		buf_info.buf = frames[i].mask;
		buf_info.alpha = false;
		if (!save_frame(&opts, &buf_info, spr->asset.err))
			goto free_frames;
	}
	ret = true;
free_frames:
	for (size_t i = 0; i < hdr->num_frames; ++i) {
		if (frames[i].free_buf)
			free(frames[i].buf);
		free(frames[i].mask);
	}
	free(frames);
free_path:
	free(path);
	return ret;
}

static void del_animations(const struct slv_spr *spr)
{
	free(spr->unks);
	if (!spr->animations)
		return;
	for (size_t i = 0; i < spr->hdr.num_animations; ++i)
		free(spr->animations[i].indices);
	free(spr->animations);
}

static void del(void *me)
{
	struct slv_spr *spr = me;
	const struct slv_spr_hdr *hdr = &spr->hdr;
	free(spr->frame_infos);
	if (hdr->num_animations)
		del_animations(spr);
	if (spr->frames) {
		for (size_t i = 0; i < hdr->num_frames; ++i)
			free(spr->frames[i].data);
		free(spr->frames);
	}
	free(spr);
}

static const struct slv_asset_ops ops = {
	.check_args = check_args,
	.load = load,
	.save = save,
	.del = del,
};

struct slv_asset *slv_new_spr(int argc, char **argv, struct slv_err *err)
{
	return slv_alloc(1, sizeof (struct slv_spr), &(struct slv_spr) {
		.asset = {
			.ops = &ops,
			.argc = argc,
			.argv = argv,
			.out_idx = 4,
			.err = err,
		},
	}, err);
}
