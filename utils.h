/*
 * utils.h -- Convenience macros and functions
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

#ifndef SLV_UTILS_H
#define SLV_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define SLV_LEN(arr) (sizeof (arr) / sizeof (arr)[0])
#define SLV_FST(arg, ...) (arg)
#define SLV_CALL(op, ...) (SLV_FST(__VA_ARGS__, ignore)->ops->op(__VA_ARGS__))
#define SLV_DEL(object) SLV_CALL(del, (object))

struct slv_err;

void *slv_malloc(size_t size, struct slv_err *err);
void *slv_realloc(void *ptr, size_t size, struct slv_err *err);
FILE *slv_fopen(const char *restrict filename, const char *restrict mode,
                struct slv_err *err);
size_t slv_fread(void *restrict ptr, size_t size, size_t nmemb,
                 FILE *restrict stream, struct slv_err *err);
int slv_fputs(const char *restrict s, FILE *restrict stream,
              struct slv_err *err);
int slv_fprintf(FILE *restrict stream, struct slv_err *err,
                const char *restrict format, ...);
size_t slv_fwrite(const void *restrict ptr, size_t size, size_t nmemb,
                  FILE *restrict stream, struct slv_err *err);
int slv_sprintf(char *restrict s, struct slv_err *err,
                const char *restrict format, ...);
void *slv_alloc(size_t num_elem, size_t sz, const void *init,
                struct slv_err *err);
bool slv_ul_to_i(unsigned long ul, int *i, struct slv_err *err);
bool slv_l_to_i(long l, int *i, struct slv_err *err);
bool slv_l_to_us(long l, unsigned short *us, struct slv_err *err);
bool slv_sz_to_us(size_t sz, unsigned short *us, struct slv_err *err);
int slv_min(int a, int b);
int slv_max(int a, int b);

struct slv_subst_ctx {
	const char *str;
	size_t str_len;
	size_t num_keys;
	char **keys;
	char **values;
	char *out;
	size_t out_sz;
	const char *pos;
	const char *from;
	char *to;
};

void slv_subst(struct slv_subst_ctx *ctx);

#endif // SLV_UTILS_H
