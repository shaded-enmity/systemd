/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

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

#include <math.h>

#include "util.h"
#include "json.h"

static void test_one(const char *data, ...) {
        void *state = NULL;
        va_list ap;

        va_start(ap, data);

        for (;;) {
                _cleanup_free_ char *str = NULL;
                union json_value v = {};
                int t, tt;

                t = json_tokenize(&data, &str, &v, &state, NULL);
                tt = va_arg(ap, int);

                assert_se(t == tt);

                if (t == JSON_END || t < 0)
                        break;

                else if (t == JSON_STRING) {
                        const char *nn;

                        nn = va_arg(ap, const char *);
                        assert_se(streq_ptr(nn, str));

                } else if (t == JSON_REAL) {
                        double d;

                        d = va_arg(ap, double);
                        assert_se(fabs(d - v.real) < 0.001);

                } else if (t == JSON_INTEGER) {
                        intmax_t i;

                        i = va_arg(ap, intmax_t);
                        assert_se(i == v.integer);

                } else if (t == JSON_BOOLEAN) {
                        bool b;

                        b = va_arg(ap, int);
                        assert_se(b == v.boolean);
                }
        }

        va_end(ap);
}

static char *value_string(json_variant *v) {
        char *r = NULL;
        switch (v->type) {
        case JSON_VARIANT_STRING:
                if(0 > asprintf(&r, "\"%s\"", v->string))
                      return NULL;
                break;
        case JSON_VARIANT_INTEGER:
                if(0 > asprintf(&r, "%"PRIi64, v->value.integer))
                      return NULL;
                break;
        case JSON_VARIANT_REAL:
                if(0 > asprintf(&r, "%f", v->value.real))
                      return NULL;
                break;
        case JSON_VARIANT_BOOLEAN:
                if(0 > asprintf(&r, "%s", v->value.boolean ? "true" : "false"))
                      return NULL;
                break;
        case JSON_VARIANT_NULL:
                if(0 > asprintf(&r, "null"))
                      return NULL;
                break;
        }
        return r;
}

static void echo_variant(json_variant *v, unsigned i) {

        switch(v->type) {
       case JSON_VARIANT_ARRAY:
                log_info("[");
                char *p = NULL;

                for (unsigned j = 0; j < v->size; ++j) {
                      json_variant *s = json_variant_element(v, j);
                      if (s->type == JSON_VARIANT_ARRAY || s->type == JSON_VARIANT_OBJECT) {
                             if (p) {
                                  log_info("%s, ", p);
                                  free(p);
                                  p = NULL;
                             }
                             echo_variant(s, i);
                      } else {
                             _cleanup_free_ char *x = value_string(s);
                             char *t;
                             if (p && 0 > asprintf(&t, "%s, %s", p, x))
                                   return;
                             else if (!p && 0 > asprintf(&t, "%s", x))
                                   return;
                             free(p);
                             p = t;
                      }
                }

                if (p) {
                     log_info(p);
                     free(p);
                     p = NULL;
                }

                log_info("]");
                break;
        case JSON_VARIANT_OBJECT:
                log_info("{");

                for (unsigned j = 0; j < v->size; j+=2) {
                      json_variant *key = json_variant_element(v, j);
                      json_variant *val = json_variant_element(v, j+1);
                      _cleanup_free_ char *x = value_string(key);
                      printf("%s: ", x);
                      if (val->type == JSON_VARIANT_ARRAY || val->type == JSON_VARIANT_OBJECT) {
                             printf("\n");
                             echo_variant(val, i);
                             printf(",\n");
                      } else {
                             _cleanup_free_ char *x = value_string(val);
                             printf("%s, \n", x);
                      }
                }

                log_info("}");
                break;
        }

}

static void test_file(const char *data) {
        json_variant *v = NULL;
        int t = json_parse(data, &v);

        assert_se(t > 0);
        assert_se(v != NULL);
        assert_se(v->type == JSON_VARIANT_OBJECT);
        echo_variant(v, 0);

        json_variant_unref(v);
}

int main(int argc, char *argv[]) {

        test_one("x", -EINVAL);
        test_one("", JSON_END);
        test_one(" ", JSON_END);
        test_one("0", JSON_INTEGER, (intmax_t) 0, JSON_END);
        test_one("1234", JSON_INTEGER, (intmax_t) 1234, JSON_END);
        test_one("3.141", JSON_REAL, 3.141, JSON_END);
        test_one("0.0", JSON_REAL, 0.0, JSON_END);
        test_one("7e3", JSON_REAL, 7e3, JSON_END);
        test_one("-7e-3", JSON_REAL, -7e-3, JSON_END);
        test_one("true", JSON_BOOLEAN, true, JSON_END);
        test_one("false", JSON_BOOLEAN, false, JSON_END);
        test_one("null", JSON_NULL, JSON_END);
        test_one("{}", JSON_OBJECT_OPEN, JSON_OBJECT_CLOSE, JSON_END);
        test_one("\t {\n} \n", JSON_OBJECT_OPEN, JSON_OBJECT_CLOSE, JSON_END);
        test_one("[]", JSON_ARRAY_OPEN, JSON_ARRAY_CLOSE, JSON_END);
        test_one("\t [] \n\n", JSON_ARRAY_OPEN, JSON_ARRAY_CLOSE, JSON_END);
        test_one("\"\"", JSON_STRING, "", JSON_END);
        test_one("\"foo\"", JSON_STRING, "foo", JSON_END);
        test_one("\"foo\\nfoo\"", JSON_STRING, "foo\nfoo", JSON_END);
        test_one("{\"foo\" : \"bar\"}", JSON_OBJECT_OPEN, JSON_STRING, "foo", JSON_COLON, JSON_STRING, "bar", JSON_OBJECT_CLOSE, JSON_END);
        test_one("{\"foo\" : [true, false]}", JSON_OBJECT_OPEN, JSON_STRING, "foo", JSON_COLON, JSON_ARRAY_OPEN, JSON_BOOLEAN, true, JSON_COMMA, JSON_BOOLEAN, false, JSON_ARRAY_CLOSE, JSON_OBJECT_CLOSE, JSON_END);
        test_one("\"\xef\xbf\xbd\"", JSON_STRING, "\xef\xbf\xbd", JSON_END);
        test_one("\"\\ufffd\"", JSON_STRING, "\xef\xbf\xbd", JSON_END);
        test_one("\"\\uf\"", -EINVAL);
        test_one("\"\\ud800a\"", -EINVAL);
        test_one("\"\\udc00\\udc00\"", -EINVAL);
        test_one("\"\\ud801\\udc37\"", JSON_STRING, "\xf0\x90\x90\xb7", JSON_END);

        test_one("[1, 2]", JSON_ARRAY_OPEN, JSON_INTEGER, 1, JSON_COMMA, JSON_INTEGER, 2, JSON_ARRAY_CLOSE, JSON_END);

        test_file("{\"k\": \"v\", \"foo\": [1, 2, 3], \"bar\": {\"zap\": null}}");
        test_file("{\"mutant\": [1, null, \"1\", {\"1\": [1, \"1\"]}], \"blah\": 1.27}");

        return 0;
}
