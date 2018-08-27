/*
 * gif.h -- GIF helper functions
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

#ifndef SLV_GIF_H
#define SLV_GIF_H

#include <stdbool.h>

struct GifColorType;
struct GifFileType;
struct slv_err;

struct slv_gif_opts {
	const char *file_path;
	int num_colors;
	const struct GifColorType *colors;
	int width;
	int height;
	bool animated;
};

struct slv_gif_buf_info {
	int left;
	int top;
	int width;
	int height;
	unsigned char *buf;
	bool alpha;
	int disposal;
	int delay;
};

#define SLV_NUM_PAL_COLORS 256

bool slv_read_pal(const char *path, struct GifColorType *colors,
                  struct slv_err *err);
struct GifFileType *slv_open_gif(const struct slv_gif_opts *opts,
                                 struct slv_err *err);
bool slv_fill_gif(struct GifFileType *gif, const struct slv_gif_buf_info *info,
                  struct slv_err *err);

#endif // SLV_GIF_H
