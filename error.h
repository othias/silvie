/*
 * error.h -- Error handling
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

#ifndef SLV_ERROR_H
#define SLV_ERROR_H

#include <GL/gl.h>

#define EXP "Expected arguments:\n\n\t"

#define SLV_ERRS(X) \
	X(SLV_ERR_NONE, "No error")                                     \
	X(SLV_ERR_UNK, "Unknown error")                                 \
	X(SLV_ERR_CHR_ARGS, EXP "silvie chr chr_file pal_file prefix")  \
	X(SLV_ERR_CHR_CHUNK_ID, "Unknown chunk identifier")             \
	X(SLV_ERR_CHR_CHUNK_SZ, "Chunk size mismatch")                  \
	X(SLV_ERR_CHR_GROUP_TYPE, "Unknown mesh group type")            \
	X(SLV_ERR_CHR_MAT, "Unknown material")                          \
	X(SLV_ERR_CHR_MESH_ID, "Unknown mesh id")                       \
	X(SLV_ERR_ENG_ARGS, EXP "silvie eng eng_file xml_file")         \
	X(SLV_ERR_ENG_TOPIC, "Unknown topic")                           \
	X(SLV_ERR_PAK_ARGS, EXP "silvie pak pak_file prefix")           \
	X(SLV_ERR_RAW_ARGS, EXP "silvie raw raw_file gif_file")         \
	X(SLV_ERR_SPR_ARGS, EXP "silvie spr spr_file pal_file prefix")  \
	X(SLV_ERR_SPR_FORMAT, "Unknown frame format")                   \
	X(SLV_ERR_SPR_FRAME, "Frame size mismatch")                     \
	X(SLV_ERR_SPR_MASK, "Mask size mismatch")                       \
	X(SLV_ERR_SPR_RES, "Frame resolution mismatch")                 \
	X(SLV_ERR_READ, "Error reading from stream")                    \
	X(SLV_ERR_OVERFLOW, "Overflow error")                           \
	X(SLV_ERR_FILE_SZ, "File size mismatch")

enum slv_lib {
	SLV_LIB_SLV,
	SLV_LIB_STD,
	SLV_LIB_GIF,
	SLV_LIB_GLU,
	SLV_LIB_RNC,
};

enum {
#define SLV_X(name, msg) name,
	SLV_ERRS(SLV_X)
#undef SLV_X
};

struct slv_err {
	enum slv_lib lib;
	union {
		int code;
		GLenum glu_code;
		long rnc_code;
	};
};

void slv_set_err(struct slv_err *err, enum slv_lib lib, int code);
void slv_set_errno(struct slv_err *err);
const char *slv_err_msg(const struct slv_err *err);

#endif // SLV_ERROR_H
