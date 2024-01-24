#ifndef HIRSCHBERG_H
#define HIRSCHBERG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include "utf8proc/utf8proc.h"

typedef struct {
    const char *s1;
    size_t m;
    const char *s2;
    size_t n;
} string_subproblem_t;

#define ARRAY_NAME string_subproblem_array
#define ARRAY_TYPE string_subproblem_t
#include "array/array.h"
#undef ARRAY_NAME
#undef ARRAY_TYPE

typedef enum {
    HIRSCHBERG_VALUE_STANDARD,
    HIRSCHBERG_VALUE_OPTIONS,
    HIRSCHBERG_VALUE_VARARGS
} hirschberg_value_function_type_t;

typedef struct {
    bool utf8;
    bool allow_transpose;
    string_subproblem_array *stack;
    string_subproblem_array *result;
} hirschberg_context_t;

static inline size_t utf8_next(const char *str) {
    size_t i = 0;
    if (*str == '\0') return 0;
    do {
        i++;
    } while ((str[i] & 0xC0) == 0x80);
    return i;
}

static inline size_t utf8_prev(const char *str, size_t start) {
    size_t i = 0;
    const char *ptr = str + start;
    do {
        if (ptr <= str) break;
        ptr--; i++;
    } while ((*ptr & 0xC0) == 0x80);
    return i;
}

static inline size_t utf8_offset(const char *s, size_t n) {
    size_t pos = 0;
    for (size_t i = 0; i < n; i++) {
        pos += utf8_next(s + pos);
    }
    return pos;
}


#ifndef HIRSCHBERG_CHAR_EQUAL
#ifndef HIRSCHBERG_CASE_SENSITIVE
#define HIRSCHBERG_CHAR_EQUAL(a, b) (tolower(a) == tolower(b))
#else
#define HIRSCHBERG_CHAR_EQUAL(a, b) ((a) == (b))
#endif
#endif

static inline bool subproblem_border_transpose(string_subproblem_t sub, size_t split) {
    if (sub.m == 0 || sub.n == 0 || split == 0) return false;
    char split_left = sub.s1[split - 1];
    char split_right = sub.s1[split];

    for (size_t j = 1; j < sub.n; j++) {
        if (HIRSCHBERG_CHAR_EQUAL(sub.s2[j - 1], split_right) && HIRSCHBERG_CHAR_EQUAL(sub.s2[j], split_left) && !(HIRSCHBERG_CHAR_EQUAL(sub.s2[j - 1], sub.s2[j]))) {
            return true;
        }
    }
    return false;
}

static inline bool subproblem_is_transpose(string_subproblem_t sub) {
    return sub.m == 2 && sub.n == 2
        && HIRSCHBERG_CHAR_EQUAL(sub.s1[0], sub.s2[1])
        && HIRSCHBERG_CHAR_EQUAL(sub.s1[1], sub.s2[0])
        && !(HIRSCHBERG_CHAR_EQUAL(sub.s1[0], sub.s1[1]));
}

#ifndef HIRSCHBERG_UTF8_CHAR_EQUAL
#ifndef HIRSCHBERG_CASE_SENSITIVE
#define HIRSCHBERG_UTF8_CHAR_EQUAL(a, b) (utf8proc_tolower(a) == utf8proc_tolower(b))
#else
#define HIRSCHBERG_UTF8_CHAR_EQUAL(a, b) ((a) == (b))
#endif
#endif

static inline bool subproblem_border_transpose_utf8(string_subproblem_t  sub, size_t split, size_t offset) {
    if (sub.m == 0 || sub.n == 0 || split == 0) return false;
    int32_t left_ch = 0;
    size_t prev_utf8_len = utf8_prev(sub.s1, offset);
    size_t left_pos = offset - prev_utf8_len;
    ssize_t left_len = utf8proc_iterate((const uint8_t *)sub.s1 + left_pos, -1, &left_ch);
    size_t right_pos = offset;
    int32_t right_ch = 0;
    ssize_t right_len = utf8proc_iterate((const uint8_t *)sub.s1 + offset, -1, &right_ch);
    // If the characters are equal, then it's not a transpose and we can return early
    if (left_len == right_len && (HIRSCHBERG_UTF8_CHAR_EQUAL(left_ch, right_ch))) return false;

    int32_t ch = 0;
    int32_t prev_ch = 0;

    size_t prev_start = 0;
    ssize_t prev_len = utf8proc_iterate((const uint8_t *)sub.s2, -1, &prev_ch);
    if (prev_len < 0) return false;
    size_t start = prev_len;
    for (size_t j = 1; j < sub.n; j++) {
        ssize_t cur_len = utf8proc_iterate((const uint8_t *)sub.s2 + start, -1, &ch);
        if (cur_len < 0) return false;

        if (HIRSCHBERG_UTF8_CHAR_EQUAL(prev_ch, right_ch)
         && HIRSCHBERG_UTF8_CHAR_EQUAL(ch, left_ch)
        ) {
            return true;
        }
        prev_start = start;
        prev_len = cur_len;
        prev_ch = ch;
        start += cur_len;
    }

    return false;
}

static inline bool subproblem_is_transpose_utf8(string_subproblem_t sub) {
    if (sub.m != 2 || sub.n != 2) return false;
    int32_t s1_c1 = 0, s1_c2 = 0, s2_c1 = 0, s2_c2 = 0;
    ssize_t s1_c1_len = utf8proc_iterate((const uint8_t *)sub.s1, -1, &s1_c1);
    if (s1_c1_len < 0) return false;
    ssize_t s1_c2_len = utf8proc_iterate((const uint8_t *)sub.s1 + s1_c1_len, -1, &s1_c2);
    if (s1_c2_len < 0) return false;
    ssize_t s2_c1_len = utf8proc_iterate((const uint8_t *)sub.s2, -1, &s2_c1);
    if (s2_c1_len < 0) return false;
    ssize_t s2_c2_len = utf8proc_iterate((const uint8_t *)sub.s2 + s2_c1_len, -1, &s2_c2);
    if (s2_c2_len < 0) return false;
    return ((HIRSCHBERG_UTF8_CHAR_EQUAL(s1_c1, s2_c2))
        && (HIRSCHBERG_UTF8_CHAR_EQUAL(s1_c2, s2_c1))
        && !(HIRSCHBERG_UTF8_CHAR_EQUAL(s1_c1, s1_c2)));
}

#endif // HIRSCHBERG_H

#ifndef VALUE_TYPE
#error "Must define VALUE_TYPE"
#endif

#ifndef VALUE_NAME
#error "Must define VALUE_NAME"
#endif

#define CONCAT3_(a, b, c) a ## b ## c
#define CONCAT3(a, b, c) CONCAT3_(a, b, c)
#define HIRSCHBERG_TYPED(name) CONCAT3(hirschberg_, VALUE_NAME, _##name)
// e.g. HIRSCHBERG_TYPED(foo) for double would = hirschberg_double_foo


typedef void (*HIRSCHBERG_TYPED(value_function_t))(const char *s1, size_t m, const char *s2, size_t n, bool reverse, VALUE_TYPE *values);
typedef void (*HIRSCHBERG_TYPED(value_function_options_t))(const char *s1, size_t m, const char *s2, size_t n, bool reverse, VALUE_TYPE *values, void *options);
typedef void (*HIRSCHBERG_TYPED(value_function_varargs_t))(const char *s1, size_t m, const char *s2, size_t n, bool reverse, VALUE_TYPE *values, va_list args);

typedef struct {
    hirschberg_value_function_type_t type;
    union {
        HIRSCHBERG_TYPED(value_function_t) standard;
        HIRSCHBERG_TYPED(value_function_options_t) options;
        HIRSCHBERG_TYPED(value_function_varargs_t) varargs;
    } function;
} HIRSCHBERG_TYPED(value_function_generic_t);


static bool HIRSCHBERG_TYPED(subproblems_core)(const char *s1, size_t m, const char *s2, size_t n, hirschberg_context_t context,
                                               HIRSCHBERG_TYPED(value_function_generic_t) value_function,
                                               VALUE_TYPE *values, VALUE_TYPE *rev_values, size_t values_len,
                                               void *options, va_list args) {
    if (m == 0 || n == 0) return false;
    if (m < n) {
        const char *tmp = s1;
        s1 = s2;
        s2 = tmp;
        size_t tmp_n = m;
        m = n;
        n = tmp_n;
    }
    bool utf8 = context.utf8;
    bool allow_transpose = context.allow_transpose;
    string_subproblem_array *stack = context.stack;
    string_subproblem_array *result = context.result;

    if (stack->n > 0) {
        string_subproblem_array_clear(stack);
    }
    if (result->n > 0) {
        string_subproblem_array_clear(result);
    }

    string_subproblem_t prob = (string_subproblem_t) {
        .s1 = s1,
        .m = m,
        .s2 = s2,
        .n = n
    };
    string_subproblem_array_push(stack, prob);

    string_subproblem_t sub;
    while (string_subproblem_array_pop(stack, &sub)) {
        if (sub.m == 0 || sub.n == 0 || (sub.m == 1 && sub.n == 1)) {
            string_subproblem_array_push(result, sub);
            continue;
        } else if (allow_transpose && sub.m == 2 && sub.n == 2 && (
                    (utf8 && subproblem_is_transpose_utf8(sub))
                || (!utf8 && subproblem_is_transpose(sub)))
        ) {
            string_subproblem_array_push(result, sub);
            continue;
        }

        size_t sub_m = floor((double)sub.m / 2.0);
        size_t sub_m_offset = sub_m;
        if (utf8) {
            sub_m_offset = utf8_offset(sub.s1, sub_m);
        }
        if (allow_transpose && sub.m > 1 && (
                    (utf8 && subproblem_border_transpose_utf8(sub, sub_m, sub_m_offset))
                || (!utf8 && subproblem_border_transpose(sub, sub_m))
        )) {
            sub_m++;
            if (utf8) {
                sub_m_offset += utf8_next(sub.s1 + sub_m_offset);
            } else {
                sub_m_offset++;
            }
        }

        memset(values, 0, sizeof(VALUE_TYPE) * values_len);
        memset(rev_values, 0, sizeof(VALUE_TYPE) * values_len);

        // reverse flag is false on the forward pass and true on the reverse pass
        static const bool FORWARD = false;
        static const bool REVERSE = true;
        if (value_function.type == HIRSCHBERG_VALUE_STANDARD) {
            value_function.function.standard(sub.s1, sub_m, sub.s2, sub.n, FORWARD, values);
            value_function.function.standard(sub.s1 + sub_m_offset, sub.m - sub_m,
                                            sub.s2, sub.n, REVERSE, rev_values);
        } else if (value_function.type == HIRSCHBERG_VALUE_OPTIONS) {
            value_function.function.options(sub.s1, sub_m, sub.s2, sub.n, FORWARD, values, options);
            value_function.function.options(sub.s1 + sub_m_offset, sub.m - sub_m,
                                            sub.s2, sub.n, REVERSE, rev_values, options);
        } else if (value_function.type == HIRSCHBERG_VALUE_VARARGS) {
            value_function.function.varargs(sub.s1, sub_m, sub.s2, sub.n, FORWARD, values, args);
            value_function.function.varargs(sub.s1 + sub_m_offset, sub.m - sub_m,
                                            sub.s2, sub.n, REVERSE, rev_values, args);
        } else {
            return false;
        }

        VALUE_TYPE opt_sum = (VALUE_TYPE) 0;

        size_t sub_n = 0;

        // 
        #ifdef HIRSCHBERG_SIMILARITY
        #define IMPROVES >
        #else
        #define IMPROVES <
        #endif

        for (size_t j = 0; j < sub.n + 1; j++) {
            values[j] += rev_values[sub.n - j];

            if (values[j] IMPROVES opt_sum || (sub_n == 0 && j > 0 && values[j] == opt_sum)) {
                sub_n = j;
                opt_sum = values[j];
            }
        }

        if ((sub_n == 0 && sub_m == 0) || (sub_n == sub.n && sub_m == sub.m)){
            sub_m = 1;
            sub_n = 1;
        }
        size_t sub_n_offset = sub_n;
        if (utf8) {
            sub_n_offset = utf8_offset(sub.s2, sub_n);
        }

        string_subproblem_t left_sub = (string_subproblem_t) {
            .s1 = sub.s1,
            .m = sub_m,
            .s2 = sub.s2,
            .n = sub_n
        };
        string_subproblem_t right_sub = (string_subproblem_t) {
            .s1 = sub.s1 + sub_m_offset,
            .m = sub.m - sub_m,
            .s2 = sub.s2 + sub_n_offset,
            .n = sub.n - sub_n
        };
        string_subproblem_array_push(stack, right_sub);
        string_subproblem_array_push(stack, left_sub);
    }
    return true;
}


bool HIRSCHBERG_TYPED(subproblems)(const char *s1, size_t m, const char *s2, size_t n,
                                   hirschberg_context_t context, HIRSCHBERG_TYPED(value_function_t) value_function,
                                   VALUE_TYPE *values, VALUE_TYPE *rev_values, size_t values_len) {
    return HIRSCHBERG_TYPED(subproblems_core)(s1, m, s2, n, context,
                            (HIRSCHBERG_TYPED(value_function_generic_t)){
                                .type = HIRSCHBERG_VALUE_STANDARD,
                                .function = {
                                    .standard = value_function
                                }
                            }, values, rev_values, values_len, NULL, NULL);
}


bool HIRSCHBERG_TYPED(subproblems_options)(const char *s1, size_t m, const char *s2, size_t n,
                                           hirschberg_context_t context, HIRSCHBERG_TYPED(value_function_options_t) value_function,
                                           VALUE_TYPE *values, VALUE_TYPE *rev_values, size_t values_len, void *options) {
    return HIRSCHBERG_TYPED(subproblems_core)(s1, m, s2, n, context,
                            (HIRSCHBERG_TYPED(value_function_generic_t)){
                                .type = HIRSCHBERG_VALUE_OPTIONS,
                                .function = {
                                    .options = value_function
                                }
                            }, values, rev_values, values_len, options, NULL);
}

bool HIRSCHBERG_TYPED(subproblems_varargs)(const char *s1, size_t m, const char *s2, size_t n,
                                           hirschberg_context_t context, HIRSCHBERG_TYPED(value_function_varargs_t) value_function,
                                           VALUE_TYPE *values, VALUE_TYPE *rev_values, size_t values_len, va_list args) {
    bool result = HIRSCHBERG_TYPED(subproblems_core)(s1, m, s2, n, context,
                            (HIRSCHBERG_TYPED(value_function_generic_t)){
                                .type = HIRSCHBERG_VALUE_VARARGS,
                                .function = {
                                    .varargs = value_function
                                }
                            }, values, rev_values, values_len, NULL, args);
    return result;
}

#undef CONCAT3_
#undef CONCAT3
#undef HIRSCHBERG_TYPED
#undef IMPROVES
