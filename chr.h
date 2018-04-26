/*
 * chr.h -- CHR 3D model format
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

#ifndef SLV_CHR_H
#define SLV_CHR_H

#include "asset.h"

struct slv_err;

struct slv_chr_chunk {
	unsigned long id;
	unsigned long sz;
};

struct slv_chr_mat_offsets {
	struct slv_chr_chunk chunk;
	unsigned long num_offsets;
	unsigned long *offsets;
};

enum slv_chr_flag {
	SLV_CHR_FLAG_NONE = 0x00,
	SLV_CHR_FLAG_COLOR_IDX = 0x08,
};

struct slv_chr_face_vtx {
	unsigned long idx;
	long unk_0;
	unsigned color_idx;
	unsigned texel_u;
	int unk_1;
	unsigned texel_v;
};

struct slv_chr_face {
	unsigned char flags;
	unsigned char idx;
	unsigned char num_vertices;
	unsigned char mat_off_idx;
	long unk;
	struct slv_chr_face_vtx *vertices;
};

struct slv_chr_mesh {
	struct slv_chr_chunk chunk;
	unsigned long idx;
	unsigned long id;
	unsigned long unk_0;
	unsigned long num_vertices;
	unsigned long num_normals;
	unsigned long num_faces; // Same as num_normals
	long unk_1;
	long unk_2;
	float origin[3];
	float matrix[16];
	float *vertices;
	float *normals;
	struct slv_chr_face *faces;
};

struct slv_chr_meshes {
	struct slv_chr_chunk chunk;
	unsigned long num_meshes;
	unsigned long fst_mesh_id;
	struct slv_chr_mesh *meshes;
};

struct slv_chr_tex {
	struct slv_chr_chunk chunk;
	long unk_0;
	unsigned long width_0;
	unsigned long height;
	long unk_1;
	unsigned long buf_sz;
	unsigned long width_1;
	unsigned long buf_off;
	long unk_2;
	long unk_3;
	long unk_4;
	long unk_5;
	long unk_6;
	long *unks;
	unsigned char *buf;
};

struct slv_chr_node {
	long parent_idx;
	float pos[3];
};

struct slv_chr_nodes {
	struct slv_chr_chunk chunk;
	long unk;
	unsigned long num_nodes;
	struct slv_chr_node *nodes;
};

enum SLV_CHR_GROUP_TYPE {
	SLV_CHR_GROUP_TYPE_NONE = 0,
	SLV_CHR_GROUP_TYPE_SNGL = 1,
	SLV_CHR_GROUP_TYPE_LIST = 2,
};

struct slv_chr_mesh_group {
	unsigned long type;
	union {
		unsigned long mesh_id;
		struct {
			unsigned long num_mesh_ids;
			unsigned long *mesh_ids;
		};
	};
};

struct slv_chr_mesh_groups {
	struct slv_chr_chunk chunk;
	struct slv_chr_mesh_group *groups;
};

struct slv_chr_root {
	struct slv_chr_chunk chunk;
	struct slv_chr_mat_offsets mat_offsets;
	struct slv_chr_meshes meshes;
	struct slv_chr_tex tex;
	struct slv_chr_nodes nodes;
	struct slv_chr_mesh_groups mesh_groups;
};

struct slv_chr {
	struct slv_asset asset;
	struct slv_chr_chunk chunk;
	struct slv_chr_root root;
};

struct slv_asset *slv_new_chr(int argc, char **argv, struct slv_err *err);

#endif // SLV_CHR_H
