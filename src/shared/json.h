/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#pragma once

/***
  This file is part of systemd.

  Copyright 2014 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <stdbool.h>

enum {
        JSON_END,
        JSON_COLON,
        JSON_COMMA,
        JSON_OBJECT_OPEN,
        JSON_OBJECT_CLOSE,
        JSON_ARRAY_OPEN,
        JSON_ARRAY_CLOSE,
        JSON_STRING,
        JSON_REAL,
        JSON_INTEGER,
        JSON_BOOLEAN,
        JSON_NULL,
};

enum {
	JSON_VARIANT_CONTROL,
	JSON_VARIANT_STRING,
	JSON_VARIANT_INTEGER,
	JSON_VARIANT_BOOLEAN,
	JSON_VARIANT_REAL,
	JSON_VARIANT_ARRAY,
	JSON_VARIANT_OBJECT,
	JSON_VARIANT_NULL
};

union json_value {
        bool boolean;
        double real;
        intmax_t integer;
};

typedef struct json_variant {
	union {
		char *string;
		struct json_variant *obj;
		union json_value value;
	};
	int type;
	unsigned size;
} json_variant;

json_variant *json_variant_new(int json_type, unsigned size);
json_variant *json_variant_unref(json_variant *);
DEFINE_TRIVIAL_CLEANUP_FUNC(json_variant *, json_variant_unref);

char *json_variant_string(json_variant *);
bool json_variant_bool(json_variant *);
intmax_t json_variant_integer(json_variant *);
double json_variant_real(json_variant *);

json_variant *json_variant_element(json_variant *, unsigned index);
json_variant *json_variant_value(json_variant *, const char *key);

#define JSON_VALUE_NULL ((union json_value) {})

int json_tokenize(const char **p, char **ret_string, union json_value *ret_value, void **state, unsigned *line);
int json_parse(const char *string, json_variant **ret_variant);
