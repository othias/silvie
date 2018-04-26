/*
 * chr.c -- CHR 3D model format
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
#include <GL/gl.h>
#include <GL/glu.h>
#include <lib3ds/file.h>
#include <lib3ds/material.h>
#include <lib3ds/mesh.h>
#include <lib3ds/node.h>
#include <lib3ds/tracks.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "asset.h"
#include "chr.h"
#include "error.h"
#include "gif.h"
#include "stream.h"
#include "utils.h"

static bool check_args(const void *me)
{
	return slv_check_argc(me, 5, SLV_ERR_CHR_ARGS);
}

static bool load_face(struct slv_chr_face *face, struct slv_stream *stream)
{
	if (!slv_read_buf(stream, &face->flags, 1)
	    || !slv_read_buf(stream, &face->idx, 1)
	    || !slv_read_buf(stream, &face->num_vertices, 1)
	    || !(face->vertices = slv_malloc(face->num_vertices *
	                                     sizeof face->vertices[0],
	                                     stream->err))
	    || !slv_read_buf(stream, &face->mat_off_idx, 1)
	    || !slv_read_le(stream, &face->unk))
		return false;
	for (size_t i = 0; i < face->num_vertices; ++i) {
		struct slv_chr_face_vtx *vtx = &face->vertices[i];
		if (!slv_read_le(stream, &vtx->idx)
		    || !slv_read_le(stream, &vtx->unk_0)
		    || !slv_read_le(stream, &vtx->color_idx)
		    || !slv_read_le(stream, &vtx->texel_u)
		    || !slv_read_le(stream, &vtx->unk_1)
		    || !slv_read_le(stream, &vtx->texel_v))
			return false;
	}
	return true;
}

static bool load_mesh(struct slv_chr_mesh *mesh, struct slv_stream *stream)
{
	if (!slv_read_le(stream, &mesh->idx)
	    || !slv_read_le(stream, &mesh->id)
	    || !slv_read_le(stream, &mesh->unk_0)
	    || !slv_read_le(stream, &mesh->num_vertices)
	    || !(mesh->vertices = slv_malloc(3 * mesh->num_vertices *
	                                     sizeof mesh->vertices[0],
	                                     stream->err))
	    || !slv_read_le(stream, &mesh->num_normals)
	    || !(mesh->normals = slv_malloc(3 * mesh->num_normals *
	                                    sizeof mesh->normals[0],
	                                    stream->err))
	    || !slv_read_le(stream, &mesh->num_faces)
	    || !(mesh->faces = slv_alloc(mesh->num_faces, sizeof mesh->faces[0],
	                                 &(struct slv_chr_face) {0},
	                                 stream->err))
	    || !slv_read_le(stream, &mesh->unk_1)
	    || !slv_read_le(stream, &mesh->unk_2)
	    || !slv_read_le_arr(stream, SLV_LEN(mesh->origin), &mesh->origin[0])
	    || !slv_read_le_arr(stream, SLV_LEN(mesh->matrix), &mesh->matrix[0])
	    || !slv_read_le_arr(stream, 3 * mesh->num_vertices, mesh->vertices)
	    || !slv_read_le_arr(stream, 3 * mesh->num_normals, mesh->normals))
		return false;
	for (size_t i = 0; i < mesh->num_faces; ++i)
		if (!load_face(&mesh->faces[i], stream))
			return false;
	return true;
}

static bool load_chunk(struct slv_chr *chr, struct slv_stream *stream,
                       size_t mesh_idx);

static bool load_meshes(struct slv_chr *chr, struct slv_stream *stream)
{
	struct slv_chr_meshes *meshes = &chr->root.meshes;
	if (!slv_read_le(stream, &meshes->num_meshes)
	    || !(meshes->meshes = slv_alloc(meshes->num_meshes,
	                                    sizeof meshes->meshes[0],
	                                    &(struct slv_chr_mesh) {0},
	                                    stream->err))
	    || !slv_read_le(stream, &meshes->fst_mesh_id))
		return false;
	for (size_t i = 0; i < meshes->num_meshes; ++i)
		if (!load_chunk(chr, stream, i))
			return false;
	return true;
}

static bool load_nodes(struct slv_chr_nodes *nodes, struct slv_stream *stream)
{
	if (!slv_read_le(stream, &nodes->unk)
	    || !slv_read_le(stream, &nodes->num_nodes)
	    || !(nodes->nodes = slv_malloc(nodes->num_nodes *
	                                   sizeof nodes->nodes[0],
	                                   stream->err)))
		return false;
	for (size_t i = 0; i < nodes->num_nodes; ++i) {
		struct slv_chr_node *node = &nodes->nodes[i];
		if (!slv_read_le(stream, &node->parent_idx)
		    || !slv_read_le_arr(stream, SLV_LEN(node->pos),
		                        &node->pos[0]))
			return false;
	}
	return true;
}

static bool load_mesh_group(struct slv_chr_mesh_group *group,
                            struct slv_stream *stream)
{
	if (!slv_read_le(stream, &group->type))
		return false;
	switch (group->type) {
	case SLV_CHR_GROUP_TYPE_NONE:
		return true;
	case SLV_CHR_GROUP_TYPE_SNGL:
		if (!slv_read_le(stream, &group->mesh_id))
			return false;
		return true;
	case SLV_CHR_GROUP_TYPE_LIST:
		if (!slv_read_le(stream, &group->num_mesh_ids)
		    || !(group->mesh_ids = slv_malloc(group->num_mesh_ids *
		                                      sizeof group->mesh_ids[0],
		                                      stream->err))
		    || !slv_read_le_arr(stream, group->num_mesh_ids,
		                        group->mesh_ids))
			return false;
		return true;
	}
	slv_set_err(stream->err, SLV_LIB_SLV, SLV_ERR_CHR_GROUP_TYPE);
	return false;
}

static bool load_mesh_groups(struct slv_chr_root *root,
                             struct slv_stream *stream)
{
	struct slv_chr_mesh_groups *mesh_groups = &root->mesh_groups;
	unsigned long num_nodes = root->nodes.num_nodes;
	if (!(mesh_groups->groups = slv_alloc(num_nodes,
	                                      sizeof mesh_groups->groups[0],
	                                      &(struct slv_chr_mesh_group) {0},
	                                      stream->err)))
		return false;
	for (size_t i = 0; i < num_nodes; ++i)
		if (!load_mesh_group(&mesh_groups->groups[i], stream))
			return false;
	return true;
}

static bool load_chunk(struct slv_chr *chr, struct slv_stream *stream,
                       size_t mesh_idx)
{
	struct slv_chr_chunk chunk;
	if (!slv_read_le(stream, &chunk.id)
	    || !slv_read_le(stream, &chunk.sz))
		return false;
	struct slv_chr_root *root = &chr->root;
	size_t pos = stream->pos;
	switch (chunk.id) {
	case 0x7f01: {
		struct slv_chr_mat_offsets *o = &root->mat_offsets;
		o->chunk = chunk;
		if (!slv_read_le(stream, &o->num_offsets)
		    || !(o->offsets = slv_malloc(o->num_offsets *
		                                 sizeof o->offsets[0],
		                                 stream->err))
		    || !slv_read_le_arr(stream, o->num_offsets, o->offsets))
			return false;
		break;
	}
	case 0x7f02: {
		struct slv_chr_mesh *mesh = &root->meshes.meshes[mesh_idx];
		mesh->chunk = chunk;
		if (!load_mesh(mesh, stream))
			return false;
		/*
		 * Some models have broken mesh ids (BOOTS2.CHR, EROCK1.CHR,
		 * EROCK2.CHR, EROCK3.CHR, Ringb.chr, RING2.CHR, VILLAGR2.CHR)
		 */
		mesh->id = root->meshes.fst_mesh_id + mesh_idx;
		break;
	}
	case 0x7f03:
		root->meshes.chunk = chunk;
		if (!load_meshes(chr, stream))
			return false;
		break;
	case 0x7f04: {
		struct slv_chr_tex *tex = &root->tex;
		tex->chunk = chunk;
		if (!slv_read_le(stream, &tex->unk_0)
		    || !slv_read_le(stream, &tex->width_0)
		    || !slv_read_le(stream, &tex->height)
		    || !(tex->unks = slv_malloc(tex->height *
		                                sizeof tex->unks[0],
		                                stream->err))
		    || !slv_read_le(stream, &tex->unk_1)
		    || !slv_read_le(stream, &tex->buf_sz)
		    || !(tex->buf = slv_alloc(tex->buf_sz, 1,
		                              &(unsigned char) {0},
		                              stream->err))
		    || !slv_read_le(stream, &tex->width_1)
		    || !slv_read_le(stream, &tex->buf_off)
		    || !slv_read_le(stream, &tex->unk_2)
		    || !slv_read_le(stream, &tex->unk_3)
		    || !slv_read_le(stream, &tex->unk_4)
		    || !slv_read_le(stream, &tex->unk_5)
		    || !slv_read_le(stream, &tex->unk_6)
		    || !slv_read_le_arr(stream, tex->height, tex->unks)
		    /*
		     * The last 8 pixels of every CHR texture are missing (the
		     * game mistakenly overreads them into the next chunk)
		     */
		    || !slv_read_buf(stream, tex->buf, tex->buf_sz - 8))
			return false;
		break;
	}
	case 0x7f05: {
		struct slv_chr_nodes *nodes = &root->nodes;
		nodes->chunk = chunk;
		if (!load_nodes(nodes, stream))
			return false;
		break;
	}
	case 0x7f06:
		root->mesh_groups.chunk = chunk;
		if (!load_mesh_groups(root, stream))
			return false;
		break;
	case 0x8000:
		chr->chunk = chunk;
		if (!load_chunk(chr, stream, 0))
			return false;
		break;
	case 0x8001:
		root->chunk = chunk;
		if (!load_chunk(chr, stream, 0) // Material offsets
		    || !load_chunk(chr, stream, 0) // Meshes
		    || !load_chunk(chr, stream, 0) // Texture
		    || !load_chunk(chr, stream, 0) // Nodes
		    || !load_chunk(chr, stream, 0)) // Mesh groups
			return false;
		break;
	default:
		slv_set_err(stream->err, SLV_LIB_SLV, SLV_ERR_CHR_CHUNK_ID);
		return false;
	}
	if (pos + chunk.sz != stream->pos) {
		slv_set_err(stream->err, SLV_LIB_SLV, SLV_ERR_CHR_CHUNK_SZ);
		return false;
	}
	return true;
}

static bool load(void *me, struct slv_stream *stream)
{
	return load_chunk(me, stream, 0);
}

struct tess_ctx {
	struct GLUtesselator *tess;
	void (*tesselate)(size_t, struct tess_ctx *);
	size_t prev_vtx_indices[2];
	size_t cur_vtx;
	size_t cur_tess_tri;
	size_t num_tris;
	struct tess_tri {
		size_t indices[3];
	} *tess_tris;
	bool odd_winding;
	const struct slv_chr_root *root;
	const struct slv_chr_mesh *chr_mesh;
	struct Lib3dsFile *file;
	struct Lib3dsMesh *lib3ds_mesh;
	bool has_err;
	struct slv_err *err;
};

static void add_tri(const size_t *tri, struct tess_ctx *ctx)
{
	struct tess_tri *tess_tri = &ctx->tess_tris[ctx->cur_tess_tri++];
	memcpy(tess_tri->indices, tri, sizeof tess_tri->indices);
}

static void tess_tris(size_t idx, struct tess_ctx *ctx)
{
	add_tri((size_t[]) {
		ctx->prev_vtx_indices[0],
		ctx->prev_vtx_indices[1],
		idx,
	}, ctx);
	ctx->cur_vtx = 0;
}

static void tess_tri_strip(size_t idx, struct tess_ctx *ctx)
{
	if (ctx->odd_winding)
		add_tri((size_t[]) {
			ctx->prev_vtx_indices[0],
			idx,
			ctx->prev_vtx_indices[1],
		}, ctx);
	else
		add_tri((size_t[]) {
			ctx->prev_vtx_indices[0],
			ctx->prev_vtx_indices[1],
			idx,
		}, ctx);
	ctx->prev_vtx_indices[0] = ctx->prev_vtx_indices[1];
	ctx->prev_vtx_indices[1] = idx;
	ctx->odd_winding = !ctx->odd_winding;
}

static void tess_tri_fan(size_t idx, struct tess_ctx *ctx)
{
	add_tri((size_t[]) {
		ctx->prev_vtx_indices[0],
		ctx->prev_vtx_indices[1],
		idx,
	}, ctx);
	ctx->prev_vtx_indices[1] = idx;
}

static void APIENTRY tess_begin_cb(GLenum type, void *user)
{
	struct tess_ctx *ctx = user;
	switch (type)
	{
	case GL_TRIANGLES:
		ctx->tesselate = tess_tris;
		break;
	case GL_TRIANGLE_STRIP:
		ctx->tesselate = tess_tri_strip;
		break;
	case GL_TRIANGLE_FAN:
		ctx->tesselate = tess_tri_fan;
	}
	ctx->cur_vtx = 0;
	ctx->cur_tess_tri = 0;
	ctx->odd_winding = false;
}

struct tess_vtx {
	GLdouble pos[3];
	size_t idx;
};

static void APIENTRY tess_vtx_cb(void *data, void *user)
{
	struct tess_ctx *ctx = user;
	const struct tess_vtx *vtx = data;
	if (ctx->cur_vtx == 2)
		ctx->tesselate(vtx->idx, ctx);
	else
		ctx->prev_vtx_indices[ctx->cur_vtx++] = vtx->idx;
}

static void APIENTRY tess_err_cb(GLenum code, void *user)
{
	struct tess_ctx *ctx = user;
	ctx->err->lib = SLV_LIB_GLU;
	ctx->err->glu_code = code;
	ctx->has_err = true;
}

static double find_v(unsigned idx, unsigned long off,
                     const struct slv_chr_tex *tex)
{
	unsigned long width = tex->width_0;
	double v = .0;
	for (size_t i = off; i + width < tex->buf_sz; i += width, ++v)
		if (tex->buf[i] == idx)
			return v;
	return .0;
}

static bool save_tri(const struct slv_chr_face *chr_face, struct tess_ctx *ctx)
{
	struct Lib3dsMesh *lib3ds_mesh = ctx->lib3ds_mesh;
	struct Lib3dsFace *lib3ds_face = &lib3ds_mesh->faceL[ctx->num_tris];
	for (size_t i = 0; i < 3; ++i) {
		size_t idx = 3 * ctx->num_tris + i;
		const struct slv_chr_face_vtx *vtx = &chr_face->vertices[2 - i];
		/*
		 * CHR textures are not unwraps, therefore vertices have to be
		 * duplicated for every triangle (they cannot be shared)
		 */
		memcpy(lib3ds_mesh->pointL[idx].pos,
		       &ctx->chr_mesh->vertices[3 * vtx->idx],
		       sizeof lib3ds_mesh->pointL[0].pos);
		if (!slv_sz_to_us(idx, &lib3ds_face->points[i], ctx->err))
			return false;
		double u;
		double v;
		if (chr_face->flags & SLV_CHR_FLAG_COLOR_IDX) {
			const unsigned long *o = ctx->root->mat_offsets.offsets;
			u = 1.0;
			v = find_v(vtx->color_idx, o[chr_face->mat_off_idx],
			           &ctx->root->tex);
		}
		else {
			u = vtx->texel_u;
			v = vtx->texel_v;
		}
		float *texel = lib3ds_mesh->texelL[idx];
		const struct slv_chr_tex *tex = &ctx->root->tex;
		texel[0] = (float)(u / tex->width_0);
		texel[1] = (float)(1 - v / tex->height);
	}
	if (slv_sprintf(lib3ds_face->material, ctx->err, "mat%hhu",
	                chr_face->mat_off_idx) < 0)
		return false;
	++ctx->num_tris;
	return true;
}

static bool tesselate(const struct slv_chr_face *face, struct tess_ctx *ctx)
{
	struct tess_vtx *vertices = slv_malloc(face->num_vertices *
	                                       sizeof vertices[0], ctx->err);
	if (!vertices)
		return false;
	size_t num_tess_tris = face->num_vertices;
	num_tess_tris -= 2;
	bool ret = false;
	if (!(ctx->tess_tris = slv_malloc(num_tess_tris *
	                                  sizeof ctx->tess_tris[0], ctx->err)))
		goto free_vertices;
	gluTessBeginPolygon(ctx->tess, ctx);
	gluTessBeginContour(ctx->tess);
	for (size_t i = 0; i < face->num_vertices; ++i) {
		GLdouble *pos = vertices[i].pos;
		for (size_t j = 0; j < 3; ++j) {
			size_t idx = 3 * face->vertices[i].idx + j;
			pos[j] = ctx->chr_mesh->vertices[idx];
		}
		vertices[i].idx = i;
		gluTessVertex(ctx->tess, pos, pos);
	}
	gluTessEndContour(ctx->tess);
	gluTessEndPolygon(ctx->tess);
	if (ctx->has_err)
		goto free_tess_tris;
	struct slv_chr_face tess_face = *face;
	for (size_t i = 0; i < num_tess_tris; ++i) {
		const struct tess_tri *tri = &ctx->tess_tris[i];
		tess_face.vertices = (struct slv_chr_face_vtx[]) {
			face->vertices[tri->indices[0]],
			face->vertices[tri->indices[1]],
			face->vertices[tri->indices[2]],
		};
		if (!save_tri(&tess_face, ctx))
			goto free_tess_tris;
	}
	ret = true;
free_tess_tris:
	free(ctx->tess_tris);
free_vertices:
	free(vertices);
	return ret;
}

static bool save_mesh(struct tess_ctx *ctx)
{
	const struct slv_chr_mesh *chr_mesh = ctx->chr_mesh;
	for (size_t i = 0; i < chr_mesh->num_faces; ++i) {
		const struct slv_chr_face *face = &chr_mesh->faces[i];
		if ((face->num_vertices > 3 && !tesselate(face, ctx))
		    || (face->num_vertices == 3 && !save_tri(face, ctx)))
			return false;
	}
	return true;
}

static struct Lib3dsNode *save_node(size_t idx, const struct tess_ctx *ctx)
{
	struct Lib3dsNode *lib3ds_node = lib3ds_node_new_object();
	if (!lib3ds_node) {
		slv_set_errno(ctx->err);
		return NULL;
	}
	struct Lib3dsLin3Key *key = lib3ds_lin3_key_new();
	if (!key) {
		slv_set_errno(ctx->err);
		goto free_node;
	}
	unsigned short idx_us;
	if (!slv_sz_to_us(idx, &idx_us, ctx->err)
	    || slv_sprintf(lib3ds_node->name, ctx->err, "%hu", idx_us) < 0)
		goto free_key;
	const struct slv_chr_node *chr_node = &ctx->root->nodes.nodes[idx];
	memcpy(key->value, chr_node->pos, sizeof key->value);
	lib3ds_node->node_id = idx_us;
	if (chr_node->parent_idx == idx_us
	    || chr_node->parent_idx == -1)
		lib3ds_node->parent_id = LIB3DS_NO_PARENT;
	else if (!slv_l_to_us(chr_node->parent_idx, &lib3ds_node->parent_id,
	                      ctx->err))
		goto free_key;
	lib3ds_lin3_track_insert(&lib3ds_node->data.object.pos_track, key);
	lib3ds_file_insert_node(ctx->file, lib3ds_node);
	return lib3ds_node;
free_key:
	lib3ds_lin3_key_free(key);
free_node:
	lib3ds_node_free(lib3ds_node);
	return NULL;
}

static unsigned get_num_tris(const struct slv_chr_mesh *mesh)
{
	unsigned num_tris = 0;
	for (size_t i = 0; i < mesh->num_faces; ++i) {
		const struct slv_chr_face *face = &mesh->faces[i];
		if (face->num_vertices > 2) {
			num_tris += face->num_vertices;
			num_tris -= 2;
		}
	}
	return num_tris;
}

static bool save_group(size_t idx, struct tess_ctx *ctx)
{
	const struct slv_chr_root *root = ctx->root;
	const struct slv_chr_mesh_group *group = &root->mesh_groups.groups[idx];
	const unsigned long *ids;
	size_t num_ids;
	switch (group->type) {
	case SLV_CHR_GROUP_TYPE_NONE:
		return true;
	case SLV_CHR_GROUP_TYPE_SNGL:
		ids = &group->mesh_id;
		num_ids = 1;
		break;
	case SLV_CHR_GROUP_TYPE_LIST:
		ids = group->mesh_ids;
		num_ids = group->num_mesh_ids;
		break;
	default:
		slv_set_err(ctx->err, SLV_LIB_SLV, SLV_ERR_CHR_GROUP_TYPE);
		return false;
	}
	struct Lib3dsNode *node = save_node(idx, ctx);
	if (!node)
		return false;
	const struct slv_chr_mesh **chr_mshes = slv_malloc(num_ids *
	                                                   sizeof chr_mshes[0],
	                                                   ctx->err);
	if (!chr_mshes)
		return false;
	unsigned num_tris = 0;
	bool ret = false;
	for (size_t i = 0; i < num_ids; ++i) {
		const struct slv_chr_mesh *m = SLV_FIND(slv_chr_mesh, id,
		                                        root->meshes.meshes,
		                                        root->meshes.num_meshes,
		                                        &ids[i], memcmp);
		if (!m) {
			slv_set_err(ctx->err, SLV_LIB_SLV, SLV_ERR_CHR_MESH_ID);
			goto free_chr_mshes;
		}
		chr_mshes[i] = m;
		num_tris += get_num_tris(m);
	}
	struct Lib3dsMesh *lib3ds_mesh = lib3ds_mesh_new(node->name);
	if (!lib3ds_mesh) {
		slv_set_errno(ctx->err);
		goto free_chr_mshes;
	}
	unsigned num_vertices = 3 * num_tris;
	if (!lib3ds_mesh_new_point_list(lib3ds_mesh, num_vertices)
	    || !lib3ds_mesh_new_texel_list(lib3ds_mesh, num_vertices)
	    || !lib3ds_mesh_new_face_list(lib3ds_mesh, num_tris)) {
		slv_set_errno(ctx->err);
		lib3ds_mesh_free(lib3ds_mesh);
		goto free_chr_mshes;
	}
	ctx->num_tris = 0;
	ctx->lib3ds_mesh = lib3ds_mesh;
	for (size_t i = 0; i < num_ids; ++i) {
		ctx->chr_mesh = chr_mshes[i];
		if (!save_mesh(ctx))
			goto free_chr_mshes;
	}
	lib3ds_file_insert_mesh(ctx->file, lib3ds_mesh);
	ret = true;
free_chr_mshes:
	free(chr_mshes);
	return ret;
}

static bool save(const void *me)
{
	const struct slv_chr *chr = me;
	const struct slv_chr_root *root = &chr->root;
	char *path;
	char *extension;
	if (!(path = slv_suf(&chr->asset, &extension, sizeof ".xxx")))
		return false;
	struct GifColorType pal_colors[SLV_NUM_PAL_COLORS];
	const struct slv_chr_tex *tex = &root->tex;
	int width;
	int height;
	bool ret = false;
	if (!slv_read_pal(chr->asset.argv[3], pal_colors, chr->asset.err)
	    || !slv_ul_to_i(tex->width_0, &width, chr->asset.err)
	    || !slv_ul_to_i(tex->height, &height, chr->asset.err))
		goto free_path;
	strcpy(extension, ".gif");
	struct GifFileType *gif = slv_open_gif(&(struct slv_gif_opts) {
		.file_path = path,
		.num_colors = SLV_NUM_PAL_COLORS,
		.colors = pal_colors,
		.width = width,
		.height = height,
	}, chr->asset.err);
	if (!gif)
		goto free_path;
	if (!slv_fill_gif(gif, &(struct slv_gif_buf_info) {
		.width = width,
		.height = height,
		.buf = tex->buf,
	}, chr->asset.err))
		goto close_gif;
	struct Lib3dsFile *file = lib3ds_file_new();
	if (!file)
		goto close_gif;
	const char *gif_name = strrchr(path, '/');
	gif_name = gif_name ? gif_name + 1 : path;
	const struct slv_chr_mat_offsets *offsets = &root->mat_offsets;
	for (size_t i = 0; i < offsets->num_offsets; ++i) {
		struct Lib3dsMaterial *mat = lib3ds_material_new();
		if (!mat) {
			slv_set_errno(chr->asset.err);
			goto free_file;
		}
		if (slv_sprintf(mat->name, chr->asset.err, "mat%zu", i) < 0) {
			lib3ds_material_free(mat);
			goto free_file;
		}
		struct Lib3dsTextureMap *map = &mat->texture1_map;
		strcpy(map->name, gif_name);
		unsigned long off = offsets->offsets[i];
		double off_u = (double)(off % tex->width_0) / tex->width_0;
		double off_v = (double)(off / tex->width_0) / tex->height;
		// 3ds Max 5 seems to use reversed axes for material offsets
		map->offset[0] = (float)-off_u;
		map->offset[1] = (float)-off_v;
		lib3ds_file_insert_material(file, mat);
	}
	struct GLUtesselator *tess = gluNewTess();
	if (!tess) {
		slv_set_errno(chr->asset.err);
		goto free_file;
	}
	gluTessCallback(tess, GLU_TESS_BEGIN_DATA, tess_begin_cb);
	gluTessCallback(tess, GLU_TESS_VERTEX_DATA, tess_vtx_cb);
	gluTessCallback(tess, GLU_TESS_ERROR_DATA, tess_err_cb);
	gluTessCallback(tess, GLU_TESS_END, glEnd);
	gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
	gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, GLU_FALSE);
	struct tess_ctx ctx = {
		.tess = tess,
		.root = root,
		.file = file,
		.err = chr->asset.err,
	};
	for (size_t i = 0; i < root->nodes.num_nodes; ++i)
		if (!save_group(i, &ctx))
			goto del_tess;
	strcpy(extension, ".3ds");
	lib3ds_file_save(file, path);
	ret = true;
del_tess:
	gluDeleteTess(tess);
free_file:
	lib3ds_file_free(file);
close_gif:
	EGifCloseFile(gif, NULL);
free_path:
	free(path);
	return ret;
}

static void del_mesh(const struct slv_chr_mesh *mesh)
{
	free(mesh->vertices);
	free(mesh->normals);
	if (!mesh->faces)
		return;
	for (size_t i = 0; i < mesh->num_faces; ++i)
		free(mesh->faces[i].vertices);
	free(mesh->faces);
}

static void del_group(const struct slv_chr_mesh_group *group)
{
	if (group->type == SLV_CHR_GROUP_TYPE_LIST)
		free(group->mesh_ids);
}

static void del(void *me)
{
	struct slv_chr *chr = me;
	const struct slv_chr_root *root = &chr->root;
	free(root->mat_offsets.offsets);
	const struct slv_chr_meshes *meshes = &root->meshes;
	if (meshes->meshes) {
		for (size_t i = 0; i < meshes->num_meshes; ++i)
			del_mesh(&meshes->meshes[i]);
		free(meshes->meshes);
	}
	free(root->tex.unks);
	free(root->tex.buf);
	free(root->nodes.nodes);
	const struct slv_chr_mesh_groups *groups = &root->mesh_groups;
	if (groups->groups) {
		for (size_t i = 0; i < root->nodes.num_nodes; ++i)
			del_group(&groups->groups[i]);
		free(groups->groups);
	}
	free(chr);
}

static const struct slv_asset_ops ops = {
	.check_args = check_args,
	.load = load,
	.save = save,
	.del = del,
};

struct slv_asset *slv_new_chr(int argc, char **argv, struct slv_err *err)
{
	return slv_alloc(1, sizeof (struct slv_chr), &(struct slv_chr) {
		.asset = {
			.ops = &ops,
			.argc = argc,
			.argv = argv,
			.out_idx = 4,
			.err = err,
		},
	}, err);
}
