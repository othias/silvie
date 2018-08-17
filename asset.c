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
#include "asset.h"
#include "error.h"

bool slv_check_args(const struct slv_asset *asset, size_t num_args, int code)
{
	size_t i;
	for (i = 0; asset->args[i]; ++i);
	if (i != num_args) {
		slv_set_err(asset->err, SLV_LIB_SLV, code);
		return false;
	}
	return true;
}
