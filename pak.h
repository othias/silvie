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

#ifndef SLV_PAK_H
#define SLV_PAK_H

#include "asset.h"

struct slv_err;

struct slv_pak {
	struct slv_asset asset;
	unsigned long raw_hdr_sz;
	unsigned char raw_hdr[44];
	unsigned long raw_pal_sz;
	unsigned char raw_pal[768];
	unsigned long raw_buf_sz;
	unsigned char *raw_buf;
	unsigned long out_0_sz;
	unsigned char *out_0_buf;
	unsigned long out_1_sz;
	unsigned char *out_1_buf;
	unsigned long out_2_hdr_sz;
	unsigned char out_2_hdr[112];
	unsigned long out_2_buf_sz;
	unsigned char *out_2_buf;
};

void *slv_new_pak(char **argv, struct slv_err *err);

#endif // SLV_PAK_H
