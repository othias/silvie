/*
 * eng.h -- ENG dialog text format
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

#ifndef SLV_ENG_H
#define SLV_ENG_H

#include "asset.h"

struct slv_err;

struct slv_eng_hdr {
	unsigned long file_sz;
	unsigned long num_events;
	unsigned long num_topics;
	unsigned long num_replies;
	long unk_0;
	long unk_1;
	long unk_2;
	long unk_3;
};

struct slv_eng_event {
	unsigned long num_topic_names;
	long unk_0;
	long unk_1;
	long unk_2;
	long unk_3;
	long unk_4;
	char *name;
	char *trigger;
	char **topic_names;
};

struct slv_eng_topic {
	unsigned long num_reply_names;
	long unk_0;
	long unk_1;
	long unk_2;
	long unk_3;
	char *name;
	char **reply_names;
};

struct slv_eng_reply {
	char *name;
	char *character;
	char *color;
	char *anim; // Empty string
	char *text;
};

struct slv_eng {
	struct slv_asset asset;
	struct slv_eng_hdr hdr;
	struct slv_eng_event *events;
	struct slv_eng_topic *topics;
	struct slv_eng_reply *replies;
};

struct slv_asset *slv_new_eng(char **args, struct slv_err *err);

#endif // SLV_ENG_H
