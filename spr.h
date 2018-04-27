/*
 * spr.h -- SPR spritesheet format
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

#ifndef SLV_SPR_H
#define SLV_SPR_H

#include "asset.h"

struct slv_err;

enum slv_spr_format {
	SLV_SPR_RLE,
	SLV_SPR_WITH_MASKS,
	SLV_SPR_UNCOMPRESSED,
};

struct slv_spr_hdr {
	unsigned long file_sz;
	unsigned long file_id;
	unsigned long format;
	unsigned long num_frames;
	long unk_0;
	unsigned long num_anims;
	long unk_1;
};

struct slv_spr_frame_info {
	unsigned long sz;
	unsigned long width;
	unsigned long height;
	long left;
	long top;
};

struct slv_spr_anim {
	unsigned long num_frames;
	unsigned long delay;
	unsigned long num_indices;
	long fst_frame_idx;
	long unk;
	long *indices;
};

struct slv_spr_frame {
	unsigned long sz;
	unsigned char *data;
};

struct slv_spr {
	struct slv_asset asset;
	struct slv_spr_hdr hdr;
	struct slv_spr_frame_info *frame_infos;
	long *unks;
	struct slv_spr_anim *anims;
	struct slv_spr_frame *frames;
};

struct slv_asset *slv_new_spr(int argc, char **argv, struct slv_err *err);

#endif // SLV_SPR_H
