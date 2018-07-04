/*
 * stream.c -- File / memory input stream
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

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "stream.h"
#include "utils.h"

static_assert(CHAR_BIT == 8, "CHAR_BIT must be exactly 8");
static_assert(CHAR_MIN <= -128, "CHAR_MIN must be at most -128");
static_assert(INT_MIN <= -32768, "INT_MIN must be at most -32768");
static_assert(LONG_MIN <= -2147483648, "LONG_MIN must be at most -2147483648");

struct fs {
	struct slv_stream stream;
	FILE *file;
};

static bool fs_read(void *me, void *buf, size_t sz)
{
	struct fs *fs = me;
	if (slv_fread(buf, sz, 1, fs->file, fs->stream.err) != 1)
		return false;
	if (fs->stream.callback)
		fs->stream.callback(fs->stream.pos, buf, sz);
	fs->stream.pos += sz;
	return true;
}

static void fs_del(void *me)
{
	struct fs *fs = me;
	fclose(fs->file);
	free(fs);
}

static const struct slv_stream_ops fs_ops = {
	.read = fs_read,
	.del = fs_del,
};

struct slv_stream *slv_new_fs(const char *path, struct slv_err *err)
{
	struct fs *fs = slv_malloc(sizeof *fs, err);
	if (!fs)
		return NULL;
	if (!(fs->file = slv_fopen(path, "rb", err)))
		goto free_fs;
	fs->stream.ops = &fs_ops;
	fs->stream.pos = 0;
	fs->stream.callback = NULL;
	fs->stream.err = err;
	return &fs->stream;
free_fs:
	free(fs);
	return NULL;
}

struct ms {
	struct slv_stream stream;
	const unsigned char *buf;
	size_t sz;
};

static bool ms_read(void *me, void *buf, size_t sz)
{
	struct ms *ms = me;
	if (ms->stream.pos + sz > ms->sz) {
		slv_set_err(ms->stream.err, SLV_LIB_SLV, SLV_ERR_READ);
		return false;
	}
	memcpy(buf, &ms->buf[ms->stream.pos], sz);
	if (ms->stream.callback)
		ms->stream.callback(ms->stream.pos, buf, sz);
	ms->stream.pos += sz;
	return true;
}

static const struct slv_stream_ops ms_ops = {
	.read = ms_read,
	.del = free,
};

struct slv_stream *slv_new_ms(const void *buf, size_t sz, struct slv_err *err)
{
	struct ms *ms = slv_malloc(sizeof *ms, err);
	if (!ms)
		return NULL;
	ms->stream.ops = &ms_ops;
	ms->stream.pos = 0;
	ms->stream.callback = NULL;
	ms->stream.err = err;
	ms->buf = buf;
	ms->sz = sz;
	return &ms->stream;
}

bool slv_read_buf(struct slv_stream *stream, void *buf, size_t sz)
{
	return SLV_CALL(read, stream, buf, sz);
}

char *slv_read_str(struct slv_stream *stream)
{
	size_t i = 0;
	size_t buf_sz = 32;
	char *tmp;
	char *str = NULL;
	do
		if ((!(i % buf_sz) && (!(tmp = slv_realloc(str, i + buf_sz,
		                                           stream->err))
		                       || !(str = tmp)))
		    || !slv_read_buf(stream, &str[i], 1)) {
			free(str);
			return NULL;
		}
	while (str[i++]);
	return str;
}

bool slv_read_le_u32(struct slv_stream *stream, unsigned long *ul)
{
	unsigned char buf[4];
	if (!slv_read_buf(stream, buf, sizeof buf))
		return false;
	*ul = 0;
	for (size_t i = 0; i < sizeof buf; ++i)
		*ul |= (unsigned long)buf[i] << i * CHAR_BIT;
	return true;
}

bool slv_read_le_s32(struct slv_stream *stream, long *l)
{
	unsigned long ul;
	if (!slv_read_le_u32(stream, &ul))
		return false;
	*l = ul & 0x80000000UL ? -(long)(~ul & 0xffffffffUL) - 1L : (long)ul;
	return true;
}

bool slv_read_le_u16(struct slv_stream *stream, unsigned *u)
{
	unsigned char buf[2];
	if (!slv_read_buf(stream, buf, sizeof buf))
		return false;
	*u = 0;
	for (size_t i = 0; i < sizeof buf; ++i)
		*u |= (unsigned)buf[i] << i * CHAR_BIT;
	return true;
}

bool slv_read_le_s16(struct slv_stream *stream, int *i)
{
	unsigned u;
	if (!slv_read_le_u16(stream, &u))
		return false;
	*i = u & 0x8000U ? -(int)(~u & 0xffffU) - 1 : (int)u;
	return true;
}

bool slv_read_le_f32(struct slv_stream *stream, float *f)
{
	unsigned char buf[4];
	if (!slv_read_buf(stream, buf, sizeof buf))
		return false;
	double sign = buf[3] & 0x80 ? -1. : 1.;
	int exponent = (buf[3] & 0x7f) << 1 | (buf[2] & 0x80) >> 7;
	unsigned long sig_ul = (unsigned long)(buf[2] & 0x7f) << 2 * CHAR_BIT
	                       | (unsigned long)buf[1] << CHAR_BIT
	                       | (unsigned long)buf[0];
	double sig = sig_ul / 8388608.;
	if (!exponent)
		if (!sig_ul)
			*f = (float)sign * 0.f;
		else
			*f = (float)(sign * ldexp(sig, -126));
	else if (exponent == 0xff)
		if (!sig_ul)
			*f = (float)sign * INFINITY;
		else
			*f = NAN;
	else
		*f = (float)(sign * ldexp(sig + 1., exponent - 127));
	return true;
}

bool slv_read_le_u32_arr(struct slv_stream *stream, size_t num_u32,
                         unsigned long *arr)
{
	for (size_t i = 0; i < num_u32; ++i)
		if (!slv_read_le(stream, &arr[i]))
			return false;
	return true;
}

bool slv_read_le_s32_arr(struct slv_stream *stream, size_t num_s32, long *arr)
{
	for (size_t i = 0; i < num_s32; ++i)
		if (!slv_read_le(stream, &arr[i]))
			return false;
	return true;
}

bool slv_read_le_f32_arr(struct slv_stream *stream, size_t num_f32, float *arr)
{
	for (size_t i = 0; i < num_f32; ++i)
		if (!slv_read_le(stream, &arr[i]))
			return false;
	return true;
}

bool slv_read_be_u32(struct slv_stream *stream, unsigned long *ul)
{
	unsigned char buf[4];
	if (!slv_read_buf(stream, buf, sizeof buf))
		return false;
	*ul = 0;
	for (size_t i = 0; i < sizeof buf; ++i)
		*ul |= (unsigned long)buf[sizeof buf - i - 1] << i * CHAR_BIT;
	return true;
}
