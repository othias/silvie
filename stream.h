/*
 * stream.h -- File / memory input stream
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

#ifndef SLV_STREAM_H
#define SLV_STREAM_H

#include <stdbool.h>
#include <stddef.h>

struct slv_err;

struct slv_stream {
	const struct slv_stream_ops {
		bool (*read)(void *, void *, size_t);
		void (*del)(void *);
	} *ops;
	size_t pos;
	void (*callback)(size_t, void *, size_t);
	struct slv_err *err;
};

struct slv_stream *slv_new_fs(const char *path, struct slv_err *err);
struct slv_stream *slv_new_ms(const unsigned char *buf, size_t sz,
                              struct slv_err *err);
bool slv_read_u32(struct slv_stream *stream, unsigned long *ul);
bool slv_read_s32(struct slv_stream *stream, long *l);
bool slv_read_u16(struct slv_stream *stream, unsigned *u);
bool slv_read_s16(struct slv_stream *stream, int *i);
bool slv_read_f32(struct slv_stream *stream, float *f);
char *slv_read_str(struct slv_stream *stream);
bool slv_read_buf(struct slv_stream *stream, void *buf, size_t sz);

#define slv_read_le(stream, ptr) \
	_Generic((ptr),                         \
	         unsigned long *: slv_read_u32, \
	         long *: slv_read_s32,          \
	         unsigned *: slv_read_u16,      \
	         int *: slv_read_s16,           \
	         float *: slv_read_f32          \
	        )((stream), (ptr))

bool slv_read_u32_arr(struct slv_stream *stream, size_t num_u32,
                      unsigned long *ul_arr);
bool slv_read_s32_arr(struct slv_stream *stream, size_t num_s32, long *l_arr);
bool slv_read_f32_arr(struct slv_stream *stream, size_t num_f32, float *f_arr);

#define slv_read_le_arr(stream, num_elem, ptr) \
	_Generic((ptr),                                 \
	         unsigned long *: slv_read_u32_arr,     \
	         long *: slv_read_s32_arr,              \
	         float *: slv_read_f32_arr              \
	        )((stream), (num_elem), (ptr))

#endif // SLV_STREAM_H
