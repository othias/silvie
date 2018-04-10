/*
 * eng.c -- ENG dialog text format
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asset.h"
#include "eng.h"
#include "error.h"
#include "stream.h"
#include "utils.h"

static bool check_args(const void *me)
{
	return slv_check_argc(me, 4, SLV_ERR_ENG_ARGS);
}

static bool load_event(struct slv_eng_event *event, struct slv_stream *stream)
{
	if (!slv_read_le(stream, &event->num_topic_names)
	    || !(event->topic_names = slv_alloc(event->num_topic_names,
	                                        sizeof event->topic_names[0],
	                                        &(char *) {NULL},
	                                        stream->err))
	    || !slv_read_le(stream, &event->unk_0)
	    || !slv_read_le(stream, &event->unk_1)
	    || !slv_read_le(stream, &event->unk_2)
	    || !slv_read_le(stream, &event->unk_3)
	    || !slv_read_le(stream, &event->unk_4)
	    || !(event->name = slv_read_str(stream))
	    || !(event->trigger = slv_read_str(stream)))
		return false;
	for (size_t i = 0; i < event->num_topic_names; ++i)
		if (!(event->topic_names[i] = slv_read_str(stream)))
			return false;
	return true;
}

static bool load_topic(struct slv_eng_topic *topic, struct slv_stream *stream)
{
	if (!slv_read_le(stream, &topic->num_reply_names)
	    || !(topic->reply_names = slv_alloc(topic->num_reply_names,
	                                        sizeof topic->reply_names[0],
	                                        &(char *) {NULL},
	                                        stream->err))
	    || !slv_read_le(stream, &topic->unk_0)
	    || !slv_read_le(stream, &topic->unk_1)
	    || !slv_read_le(stream, &topic->unk_2)
	    || !slv_read_le(stream, &topic->unk_3)
	    || !(topic->name = slv_read_str(stream)))
		return false;
	for (size_t i = 0; i < topic->num_reply_names; ++i)
		if (!(topic->reply_names[i] = slv_read_str(stream)))
			return false;
	return true;
}

static bool load_reply(struct slv_eng_reply *reply, struct slv_stream *stream)
{
	if (!(reply->name = slv_read_str(stream))
	    || !(reply->character = slv_read_str(stream))
	    || !(reply->color = slv_read_str(stream))
	    || !(reply->anim = slv_read_str(stream))
	    || !(reply->text = slv_read_str(stream)))
		return false;
	return true;
}

static void unxor(size_t pos, void *buf, size_t sz)
{
	for (size_t i = 0; i < sz; ++i, ++pos) {
		unsigned char *bytes = buf;
		unsigned char key[] = {0x52, 0xa6, 0xfa};
		bytes[i] ^= (unsigned char)
		            (key[pos % sizeof key] - 4 * (pos / sizeof key));
	}
}

static bool load(void *me, struct slv_stream *stream)
{
	struct slv_eng *eng = me;
	struct slv_eng_hdr *hdr = &eng->hdr;
	stream->cb = unxor;
	if (!slv_read_le(stream, &hdr->file_sz)
	    || !slv_read_le(stream, &hdr->num_events)
	    || !(eng->events = slv_alloc(hdr->num_events,
	                                 sizeof eng->events[0],
	                                 &(struct slv_eng_event) {0},
	                                 stream->err))
	    || !slv_read_le(stream, &hdr->num_topics)
	    || !(eng->topics = slv_alloc(hdr->num_topics,
	                                 sizeof eng->topics[0],
	                                 &(struct slv_eng_topic) {0},
	                                 stream->err))
	    || !slv_read_le(stream, &hdr->num_replies)
	    || !(eng->replies = slv_alloc(hdr->num_replies,
	                                  sizeof eng->replies[0],
	                                  &(struct slv_eng_reply) {0},
	                                  stream->err))
	    || !slv_read_le(stream, &hdr->unk_0)
	    || !slv_read_le(stream, &hdr->unk_1)
	    || !slv_read_le(stream, &hdr->unk_2)
	    || !slv_read_le(stream, &hdr->unk_3))
		return false;
	for (size_t i = 0; i < hdr->num_events; ++i)
		if (!load_event(&eng->events[i], stream))
			return false;
	for (size_t i = 0; i < hdr->num_topics; ++i)
		if (!load_topic(&eng->topics[i], stream))
			return false;
	for (size_t i = 0; i < hdr->num_replies; ++i)
		if (!load_reply(&eng->replies[i], stream))
			return false;
	if (hdr->file_sz != stream->pos) {
		slv_set_err(stream->err, SLV_LIB_SLV, SLV_ERR_FILE_SZ);
		return false;
	}
	return true;
}

static bool save_reply(const struct slv_eng_reply *reply, FILE *xml,
                       struct slv_err *err)
{
	if (slv_fprintf(xml, err,
	                "\t\t<reply name=\"%s\" character=\"%s\" color=\"%s\">",
	                reply->name, reply->character, reply->color) < 0)
		return false;
	const char *text = reply->text;
	// Print "<f/>" and "<r/>" instead of '\f' and '\r'
	for (const char *ch = text; *ch; ++ch) {
		char tag[] = "<f/>";
		if (!(*ch == '\f' || (*ch == '\r' && (tag[1] = 'r'))))
			continue;
		size_t len = (size_t)(ch - text);
		if (slv_fwrite(text, 1, len, xml, err) != len
		    || slv_fputs(tag, xml, err) < 0)
			return false;
		text = ch + 1;
	}
	if (slv_fprintf(xml, err, "%s</reply>\n", text) < 0)
		return false;
	return true;
}

static bool save_topic(const struct slv_eng_topic *topic,
		       const struct slv_eng *eng, FILE *xml)
{
	const struct slv_eng_hdr *hdr = &eng->hdr;
	if (slv_fprintf(xml, eng->asset.err, "\t<topic name=\"%s\">\n",
			topic->name) < 0)
		return false;
	for (size_t i = 0; i < topic->num_reply_names; ++i) {
		const struct slv_eng_reply *r = SLV_FIND(slv_eng_reply, name,
		                                         eng->replies,
		                                         hdr->num_replies,
		                                         topic->reply_names[i],
		                                         slv_fnd_strcmp);
		if (r ? !save_reply(r, xml, eng->asset.err)
		      : slv_fprintf(xml, eng->asset.err,
		                    "\t\t<action name=\"%s\"/>\n",
		                    topic->reply_names[i]) < 0)
			return false;
	}
	if (slv_fputs("\t</topic>\n", xml, eng->asset.err) < 0)
		return false;
	return true;
}

static bool save_event(const struct slv_eng_event *event,
                       const struct slv_eng *eng, FILE *xml)
{
	const struct slv_eng_hdr *hdr = &eng->hdr;
	if (slv_fprintf(xml, eng->asset.err,
	                "<event name=\"%s\" trigger=\"%s\">\n", event->name,
	                event->trigger) < 0)
		return false;
	for (size_t i = 0; i < event->num_topic_names; ++i) {
		const struct slv_eng_topic *t = SLV_FIND(slv_eng_topic, name,
		                                         eng->topics,
		                                         hdr->num_topics,
		                                         event->topic_names[i],
		                                         slv_fnd_strcmp);
		if (!t) {
			slv_set_err(eng->asset.err, SLV_LIB_SLV,
			            SLV_ERR_ENG_TOPIC);
			return false;
		}
		if (!save_topic(t, eng, xml))
			return false;
	}
	if (slv_fputs("</event>\n", xml, eng->asset.err) < 0)
		return false;
	return true;
}

static bool save(const void *me)
{
	const struct slv_eng *eng = me;
	const struct slv_eng_hdr *hdr = &eng->hdr;
	FILE *xml = slv_fopen(eng->asset.argv[eng->asset.out_idx], "w",
	                      eng->asset.err);
	if (!xml)
		return false;
	bool ret = false;
	for (size_t i = 0; i < hdr->num_events; ++i)
		if (!save_event(&eng->events[i], eng, xml))
			goto close_xml;
	ret = true;
close_xml:
	fclose(xml);
	return ret;
}

static void del_event(const struct slv_eng_event *event)
{
	free(event->name);
	free(event->trigger);
	if (!event->topic_names)
		return;
	for (size_t i = 0; i < event->num_topic_names; ++i)
		free(event->topic_names[i]);
	free(event->topic_names);
}

static void del_topic(const struct slv_eng_topic *topic)
{
	free(topic->name);
	if (!topic->reply_names)
		return;
	for (size_t i = 0; i < topic->num_reply_names; ++i)
		free(topic->reply_names[i]);
	free(topic->reply_names);
}

static void del(void *me)
{
	struct slv_eng *eng = me;
	const struct slv_eng_hdr *hdr = &eng->hdr;
	if (eng->events) {
		for (size_t i = 0; i < hdr->num_events; ++i)
			del_event(&eng->events[i]);
		free(eng->events);
	}
	if (eng->topics) {
		for (size_t i = 0; i < hdr->num_topics; ++i)
			del_topic(&eng->topics[i]);
		free(eng->topics);
	}
	if (eng->replies) {
		for (size_t i = 0; i < hdr->num_replies; ++i) {
			const struct slv_eng_reply *reply = &eng->replies[i];
			free(reply->name);
			free(reply->character);
			free(reply->color);
			free(reply->anim);
			free(reply->text);
		}
		free(eng->replies);
	}
	free(eng);
}

static const struct slv_asset_ops ops = {
	.check_args = check_args,
	.load = load,
	.save = save,
	.del = del,
};

struct slv_asset *slv_new_eng(int argc, char **argv, struct slv_err *err)
{
	return slv_alloc(1, sizeof (struct slv_eng), &(struct slv_eng) {
		.asset = {
			.ops = &ops,
			.argc = argc,
			.argv = argv,
			.out_idx = 3,
			.err = err,
		},
	}, err);
}
