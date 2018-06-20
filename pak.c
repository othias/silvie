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
#include <stdlib.h>
#include "asset.h"
#include "dernc.h"
#include "error.h"
#include "pak.h"
#include "stream.h"
#include "utils.h"

static bool check_args(const void *me)
{
	return slv_check_args(me, 2, SLV_ERR_PAK_ARGS);
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
	struct rnc_hdr hdr;
	unsigned char *packed = slv_malloc(sizeof hdr, stream->err);
	if (!packed)
		return false;
	bool ret = false;
	struct slv_stream *hdr_stream;
	if (!slv_read_buf(stream, packed, sizeof hdr)
	    || !(hdr_stream = slv_new_ms(packed, sizeof hdr, stream->err)))
		goto free_packed;
	if (!slv_read_buf(hdr_stream, &hdr.magic[0], sizeof hdr.magic)
	    || !slv_read_buf(hdr_stream, &hdr.method, 1)
	    || !slv_read_be(hdr_stream, &hdr.unpacked_sz)
	    || !slv_read_be(hdr_stream, &hdr.packed_sz))
		goto del_hdr_stream;
	unsigned char *tmp;
	size_t pak_size = sizeof hdr + hdr.packed_sz;
	if (!(tmp = slv_realloc(packed, pak_size, stream->err)))
		goto del_hdr_stream;
	packed = tmp;
	unsigned char *unpacked;
	if (!slv_read_buf(stream, &packed[sizeof hdr], hdr.packed_sz)
	    || !(unpacked = slv_malloc(hdr.unpacked_sz, stream->err)))
		goto del_hdr_stream;
	long rnc_ret = rnc_unpack(packed, unpacked);
	if (rnc_ret < 0) {
		stream->err->lib = SLV_LIB_RNC;
		stream->err->rnc_code = rnc_ret;
		goto free_unpacked;
	}
	struct slv_stream *unpacked_stream;
	if (!(unpacked_stream = slv_new_ms(unpacked, pak_size, stream->err)))
		goto free_unpacked;
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

static bool save(const void *me)
{
	return false;
}

static void del(void *me)
{
}

static const struct slv_asset_ops ops = {
	.check_args = check_args,
	.load = load,
	.save = save,
	.del = del,
};

struct slv_asset *slv_new_pak(char **args, struct slv_err *err)
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
