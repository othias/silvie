/*
 * raw.h -- RAW image format
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

#ifndef SLV_RAW_H
#define SLV_RAW_H

#include "asset.h"

struct slv_err;

struct slv_raw_hdr {
	unsigned long cst_0;
	unsigned long width_0;
	unsigned long height;
	unsigned long cst_1;
	unsigned long buf_sz;
	unsigned long width_1;
	long unk_0;
	long unk_1;
	long unk_2;
	long unk_3;
	long unk_4;
};

#define SLV_NUM_RAW_COLORS 256

struct slv_raw {
	struct slv_asset asset;
	struct slv_raw_hdr hdr;
	unsigned char colors[3 * SLV_NUM_RAW_COLORS];
	unsigned char *buf;
};

struct slv_asset *slv_new_raw(char **args, struct slv_err *err);

#endif // SLV_RAW_H
