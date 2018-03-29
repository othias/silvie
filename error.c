/*
 * error.c -- Error handling
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

#include <errno.h>
#include <gif_lib.h>
#include <GL/glu.h>
#include <stddef.h>
#include <string.h>
#include "error.h"

void slv_set_err(struct slv_err *err, enum slv_lib lib, int code)
{
	err->lib = lib;
	err->code = code;
}

void slv_set_errno(struct slv_err *err)
{
	if (errno)
		slv_set_err(err, SLV_LIB_STD, errno);
	else
		slv_set_err(err, SLV_LIB_SLV, SLV_ERR_UNK);
}

static const char *const err_msgs[] = {
#define X(name, msg) msg,
	SLV_ERRS
#undef X
};

const char *slv_err_msg(const struct slv_err *err)
{
	switch (err->lib) {
	case SLV_LIB_SLV:
		return err_msgs[err->code];
	case SLV_LIB_STD:
		return strerror(err->code);
	case SLV_LIB_GIF:
		return GifErrorString(err->code);
	case SLV_LIB_GLU:
		return (const char *)gluErrorString(err->glu_code);
	}
	return NULL;
}
