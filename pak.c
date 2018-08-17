/*
 * pak.c -- PAK level archive format
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
#include <stdlib.h>
#include "asset.h"
#include "dernc.h"
#include "error.h"
#include "pak.h"
#include "stream.h"
#include "utils.h"

static bool check_args(const void *me)
{
	return slv_check_args(me, 5, SLV_ERR_PAK_ARGS);
}

struct rnc_hdr {
	char magic[3];
	unsigned char method;
	unsigned long unpacked_sz;
	unsigned long packed_sz;
	unsigned unpacked_crc;
	unsigned packed_crc;
	unsigned char leeway;
	unsigned char num_chunks;
};

static bool load(void *me, struct slv_stream *stream)
{
	size_t hdr_sz = 18;
	unsigned char *packed = slv_malloc(hdr_sz, stream->err);
	if (!packed)
		return false;
	bool ret = false;
	struct slv_stream *hdr_stream;
	if (!slv_read_buf(stream, packed, hdr_sz)
	    || !(hdr_stream = slv_new_ms(packed, hdr_sz, stream->err)))
		goto free_packed;
	struct rnc_hdr hdr;
	if (!slv_read_buf(hdr_stream, &hdr.magic[0], sizeof hdr.magic)
	    || !slv_read_buf(hdr_stream, &hdr.method, 1)
	    || !slv_read_be(hdr_stream, &hdr.unpacked_sz)
	    || !slv_read_be(hdr_stream, &hdr.packed_sz))
		goto del_hdr_stream;
	size_t pak_sz = hdr_sz + hdr.packed_sz;
	// The RNC library needs 8 extra bytes for packed and unpacked buffers
	unsigned char *tmp = slv_realloc(packed, pak_sz + 8, stream->err);
	if (!tmp)
		goto del_hdr_stream;
	packed = tmp;
	unsigned char *unpacked;
	size_t unpacked_sz = hdr.unpacked_sz;
	if (!slv_read_buf(stream, &packed[hdr_sz], hdr.packed_sz)
	    || !(unpacked = slv_malloc(unpacked_sz + 8, stream->err)))
		goto del_hdr_stream;
	long rnc_ret = rnc_unpack(packed, unpacked);
	if (rnc_ret < 0) {
		stream->err->lib = SLV_LIB_RNC;
		stream->err->rnc_code = rnc_ret;
		goto free_unpacked;
	}
	struct slv_stream *unpacked_stream;
	if (!(unpacked_stream = slv_new_ms(unpacked, unpacked_sz, stream->err)))
		goto free_unpacked;
	struct slv_pak *pak = me;
	if (!slv_read_le(unpacked_stream, &pak->raw_hdr_sz)
	    || !slv_read_buf(unpacked_stream, pak->raw_hdr, sizeof pak->raw_hdr)
	    || !slv_read_le(unpacked_stream, &pak->raw_pal_sz)
	    || !slv_read_buf(unpacked_stream, pak->raw_pal, sizeof pak->raw_pal)
	    || !slv_read_le(unpacked_stream, &pak->raw_buf_sz)
	    || !(pak->raw_buf = slv_malloc(pak->raw_buf_sz, stream->err))
	    || !slv_read_buf(unpacked_stream, pak->raw_buf, pak->raw_buf_sz)
	    || !slv_read_le(unpacked_stream, &pak->out_0_sz)
	    || !(pak->out_0_buf = slv_malloc(pak->out_0_sz, stream->err))
	    || !slv_read_buf(unpacked_stream, pak->out_0_buf, pak->out_0_sz)
	    || !slv_read_le(unpacked_stream, &pak->out_1_sz)
	    || !(pak->out_1_buf = slv_malloc(pak->out_1_sz, stream->err))
	    || !slv_read_buf(unpacked_stream, pak->out_1_buf, pak->out_1_sz)
	    || !slv_read_le(unpacked_stream, &pak->out_2_hdr_sz)
	    || !slv_read_buf(unpacked_stream, pak->out_2_hdr,
	                     sizeof pak->out_2_hdr)
	    || !slv_read_le(unpacked_stream, &pak->out_2_buf_sz)
	    || !(pak->out_2_buf = slv_malloc(pak->out_2_buf_sz, stream->err))
	    || !slv_read_buf(unpacked_stream, pak->out_2_buf,
	                     pak->out_2_buf_sz))
		goto del_unpacked_stream;
	ret = true;
del_unpacked_stream:
	SLV_DEL(unpacked_stream);
free_unpacked:
	free(unpacked);
del_hdr_stream:
	SLV_DEL(hdr_stream);
free_packed:
	free(packed);
	return ret;
}

static bool save_file(const char *path, const unsigned char *buf, size_t sz,
                      struct slv_err *err)
{
	FILE *file = slv_fopen(path, "wb", err);
	if (!file)
		return false;
	bool ret = false;
	if (!slv_fwrite(buf, sz, 1, file, err))
		goto close_file;
	ret = true;
close_file:
	fclose(file);
	return ret;
}

static bool save(const void *me)
{
	const struct slv_pak *pak = me;
	struct slv_err *err = pak->asset.err;
	char **args = pak->asset.args;
	FILE *bg = slv_fopen(args[1], "wb", err);
	bool ret = false;
	if (!bg)
		return false;
	FILE *out_2 = slv_fopen(args[4], "wb", err);
	if (!out_2)
		goto close_bg;
	if (!slv_fwrite(pak->raw_hdr, sizeof pak->raw_hdr, 1, bg, err)
	    || !slv_fwrite(pak->raw_pal, sizeof pak->raw_pal, 1, bg, err)
	    || !slv_fwrite(pak->raw_buf, pak->raw_buf_sz, 1, bg, err)
	    || !save_file(args[2], pak->out_0_buf, pak->out_0_sz, err)
	    || !save_file(args[3], pak->out_1_buf, pak->out_1_sz, err)
	    || !slv_fwrite(pak->out_2_hdr, sizeof pak->out_2_hdr, 1, out_2, err)
	    || !slv_fwrite(pak->out_2_buf, pak->out_2_buf_sz, 1, out_2, err))
		goto close_out_2;
	ret = true;
close_out_2:
	fclose(out_2);
close_bg:
	fclose(bg);
	return ret;
}

static void del(void *me)
{
	struct slv_pak *pak = me;
	free(pak->raw_buf);
	free(pak->out_0_buf);
	free(pak->out_1_buf);
	free(pak->out_2_buf);
	free(pak);
}

static const struct slv_asset_ops ops = {
	.check_args = check_args,
	.load = load,
	.save = save,
	.del = del,
};

void *slv_new_pak(char **args, struct slv_err *err)
{
	return slv_alloc(1, sizeof (struct slv_pak), &(struct slv_pak) {
		.asset = {
			.ops = &ops,
			.args = args,
			.out = args[1],
			.err = err,
		},
	}, err);
}
