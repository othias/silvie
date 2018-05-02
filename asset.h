/*
 * asset.h -- Generic asset interface
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

#ifndef SLV_ASSET_H
#define SLV_ASSET_H

#include <stdbool.h>
#include <stddef.h>

struct slv_err;
struct slv_stream;

struct slv_asset {
	const struct slv_asset_ops {
		bool (*check_args)(const void *);
		bool (*load)(void *, struct slv_stream *);
		bool (*save)(const void *);
		void (*del)(void *);
	} *ops;
	int argc;
	char **argv;
	const char *out;
	struct slv_err *err;
};

typedef struct slv_asset *slv_asset_ctor(int, char **, struct slv_err *);

bool slv_check_argc(const struct slv_asset *asset, int argc, int err_code);
char *slv_suf(const struct slv_asset *asset, char **suf, size_t suf_sz);

#endif // SLV_ASSET_H
