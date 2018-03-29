/*
 * asset.c -- Generic asset interface
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
#include <string.h>
#include "error.h"
#include "asset.h"
#include "utils.h"

bool slv_check_argc(const struct slv_asset *asset, int argc, int err_code)
{
	if (asset->argc != argc) {
		slv_set_err(asset->err, SLV_LIB_SLV, err_code);
		return false;
	}
	return true;
}

char *slv_suf(const struct slv_asset *asset, char **suf, size_t suf_sz)
{
	const char *out = asset->argv[asset->out_idx];
	size_t out_len = strlen(out);
	char *path = slv_malloc(out_len + suf_sz, asset->err);
	if (!path)
		return NULL;
	strcpy(path, out);
	*suf = &path[out_len];
	return path;
}
