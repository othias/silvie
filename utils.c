/*
 * utils.c -- Convenience macros and functions
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

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "utils.h"

int slv_fnd_strcmp(const void *field_val, const void *val, size_t field_sz)
{
	(void)field_sz;
	const char *const *str = field_val;
	return strcmp(*str, val);
}

const void *slv_find(const void *arr, size_t num_structs, size_t struct_sz,
                     const void *val, size_t field_off, size_t field_sz,
                     slv_comparer compare)
{
	for (size_t i = 0; i < num_structs * struct_sz; i += struct_sz) {
		const unsigned char *arr_bytes = arr;
		const void *field_val = &arr_bytes[i + field_off];
		const void *structure = &arr_bytes[i];
		if (!compare(field_val, val, field_sz))
			return structure;
	}
	return NULL;
}

void *slv_malloc(size_t size, struct slv_err *err)
{
	void *buf = malloc(size);
	if (!buf)
		slv_set_errno(err);
	return buf;
}

void *slv_realloc(void *ptr, size_t size, struct slv_err *err)
{
	void *buf = realloc(ptr, size);
	if (!buf)
		slv_set_errno(err);
	return buf;
}

FILE *slv_fopen(const char *restrict filename, const char *restrict mode,
                struct slv_err *err)
{
	FILE *file = fopen(filename, mode);
	if (!file)
		slv_set_errno(err);
	return file;
}

size_t slv_fread(void *restrict ptr, size_t size, size_t nmemb,
                 FILE *restrict stream, struct slv_err *err)
{
	size_t ret = fread(ptr, size, nmemb, stream);
	if (ret != nmemb)
		slv_set_err(err, SLV_LIB_SLV, SLV_ERR_READ);
	return ret;
}

int slv_fputs(const char *restrict s, FILE *restrict stream,
              struct slv_err *err)
{
	int ret = fputs(s, stream);
	if (ret < 0)
		slv_set_errno(err);
	return ret;
}

int slv_fprintf(FILE *restrict stream, struct slv_err *err,
                const char *restrict format, ...)
{
	va_list args;
	va_start(args, format);
	int ret = vfprintf(stream, format, args);
	va_end(args);
	if (ret < 0)
		slv_set_errno(err);
	return ret;
}

size_t slv_fwrite(const void *restrict ptr, size_t size, size_t nmemb,
                  FILE *restrict stream, struct slv_err *err)
{
	size_t ret = fwrite(ptr, size, nmemb, stream);
	if (ret != nmemb)
		slv_set_errno(err);
	return ret;
}

int slv_sprintf(char *restrict s, struct slv_err *err,
                const char *restrict format, ...)
{
	va_list args;
	va_start(args, format);
	int ret = vsprintf(s, format, args);
	va_end(args);
	if (ret < 0)
		slv_set_errno(err);
	return ret;
}

void *slv_alloc(size_t num_elem, size_t sz, const void *init,
                struct slv_err *err)
{
	if (sz && num_elem > SIZE_MAX / sz) {
		slv_set_err(err, SLV_LIB_SLV, SLV_ERR_OVERFLOW);
		return NULL;
	}
	unsigned char *buf = slv_malloc(num_elem * sz, err);
	if (buf)
		for (size_t i = 0; i < num_elem; ++i)
			memcpy(&buf[i * sz], init, sz);
	return buf;
}

bool slv_ul_to_i(unsigned long ul, int *i, struct slv_err *err)
{
	if (ul > INT_MAX) {
		slv_set_err(err, SLV_LIB_SLV, SLV_ERR_OVERFLOW);
		return false;
	}
	*i = (int)ul;
	return true;
}

bool slv_l_to_i(long l, int *i, struct slv_err *err)
{
	if (l < INT_MIN || l > INT_MAX) {
		slv_set_err(err, SLV_LIB_SLV, SLV_ERR_OVERFLOW);
		return false;
	}
	*i = (int)l;
	return true;
}

bool slv_l_to_us(long l, unsigned short *us, struct slv_err *err)
{
	if (l < 0 || l > USHRT_MAX) {
		slv_set_err(err, SLV_LIB_SLV, SLV_ERR_OVERFLOW);
		return false;
	}
	*us = (unsigned short)l;
	return true;
}

bool slv_sz_to_us(size_t sz, unsigned short *us, struct slv_err *err)
{
	if (sz > USHRT_MAX) {
		slv_set_err(err, SLV_LIB_SLV, SLV_ERR_OVERFLOW);
		return false;
	}
	*us = (unsigned short)sz;
	return true;
}

int slv_min(int a, int b)
{
	return a < b ? a : b;
}

int slv_max(int a, int b)
{
	return a > b ? a : b;
}
