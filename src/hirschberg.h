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
    size_t x;
    size_t m;
    size_t y;
    size_t n;
} string_subproblem_t;

typedef struct {
    const char *s1;
    size_t m;
    const char *s2;
    size_t n;
} string_pair_input_t;

#define NULL_SUBPROBLEM ((string_subproblem_t){ .x = 0, .m = 0, .y = 0, .n = 0})

#define ARRAY_NAME string_subproblem_array
#define ARRAY_TYPE string_subproblem_t
#include "array/array.h"
#undef ARRAY_NAME
#undef ARRAY_TYPE

typedef enum {
    VALUE_FUNCTION_STANDARD = 0,
    VALUE_FUNCTION_OPTIONS = 1,
    VALUE_FUNCTION_VARARGS = 2
} hirschberg_value_function_type_t;

typedef struct {
    bool utf8;
    bool allow_transpose;
    bool zero_out_memory;
} hirschberg_options_t;

static inline bool utf8_is_continuation(char c) {
    return (c & 0xC0) == 0x80;
}

static inline size_t utf8_next(const char *str) {
    size_t i = 0;
    if (*str == '\0') return 0;
    do {
        i++;
    } while (utf8_is_continuation(str[i]));
    return i;
}

static inline size_t utf8_prev(const char *str, size_t start) {
    size_t i = 0;
    const char *ptr = str + start;
    do {
        if (ptr <= str) break;
        ptr--; i++;
    } while (utf8_is_continuation(*ptr));
    return i;
}


#ifndef CHAR_EQUAL
#define CHAR_EQUAL_DEFINED
#ifndef HIRSCHBERG_CASE_SENSITIVE
#define CHAR_EQUAL(a, b) (tolower(a) == tolower(b))
#else
#define CHAR_EQUAL(a, b) ((a) == (b))
#endif
#endif

static inline bool subproblem_border_transpose(const char *s1, const char *s2, string_subproblem_t sub, size_t split) {
    if (sub.m == 0 || sub.n == 0 || split == 0) return false;
    char split_left = s1[split - 1];
    char split_right = s1[split];

    for (size_t j = 1; j < sub.n; j++) {
        if (CHAR_EQUAL(s2[j - 1], split_right) && CHAR_EQUAL(s2[j], split_left) && !(CHAR_EQUAL(s2[j - 1], s2[j]))) {
            return true;
        }
    }
    return false;
}

#ifndef UTF8_CHAR_EQUAL
#define UTF8_CHAR_EQUAL_DEFINED
#ifndef HIRSCHBERG_CASE_SENSITIVE
#define UTF8_CHAR_EQUAL(a, b) (utf8proc_tolower(a) == utf8proc_tolower(b))
#else
#define UTF8_CHAR_EQUAL(a, b) ((a) == (b))
#endif
#endif

static inline bool subproblem_border_transpose_utf8(const char *s1, const char *s2, string_subproblem_t sub, size_t split) {
    if (sub.m == 0 || sub.n == 0 || split == 0) return false;
    int32_t left_ch = 0;
    size_t prev_utf8_len = utf8_prev(s1, split);
    size_t left_pos = split - prev_utf8_len;
    utf8proc_ssize_t left_len = utf8proc_iterate((const uint8_t *)s1 + left_pos, -1, &left_ch);
    size_t right_pos = split;
    int32_t right_ch = 0;
    utf8proc_ssize_t right_len = utf8proc_iterate((const uint8_t *)s1 + split, -1, &right_ch);
    // If the characters are equal, then it's not a transpose and we can return early
    if (left_len == right_len && (UTF8_CHAR_EQUAL(left_ch, right_ch))) return false;

    int32_t ch = 0;
    int32_t prev_ch = 0;

    size_t prev_start = 0;
    utf8proc_ssize_t prev_len = utf8proc_iterate((const uint8_t *)s2, -1, &prev_ch);
    if (prev_len < 0) return false;
    size_t cur = prev_len;
    while (cur < sub.n) {
        utf8proc_ssize_t cur_len = utf8proc_iterate((const uint8_t *)s2 + cur, -1, &ch);
        if (cur_len < 0) return false;

        if (UTF8_CHAR_EQUAL(prev_ch, right_ch)
         && UTF8_CHAR_EQUAL(ch, left_ch)
        ) {
            return true;
        }
        prev_start = cur;
        prev_len = cur_len;
        prev_ch = ch;
        cur += cur_len;
    }

    return false;
}

#endif // HIRSCHBERG_H

#ifndef VALUE_TYPE
#error "Must define VALUE_TYPE"
#endif

#ifndef VALUE_NAME
#error "Must define VALUE_NAME"
#endif

#if !defined(HIRSCHBERG_SIMILARITY) && !defined(MAX_VALUE)
#error "Must define MAX_VALUE for distance functions"
#endif

#define CONCAT3_(a, b, c) a ## b ## c
#define CONCAT3(a, b, c) CONCAT3_(a, b, c)
#define HIRSCHBERG_TYPED(name) CONCAT3(hirschberg_, VALUE_NAME, _##name)
// e.g. HIRSCHBERG_TYPED(foo) for double would = hirschberg_double_foo

typedef size_t (*HIRSCHBERG_TYPED(function_standard))(const char *s1, size_t m, const char *s2, size_t n, bool reverse, VALUE_TYPE *values, size_t values_size);
typedef size_t (*HIRSCHBERG_TYPED(function_options))(const char *s1, size_t m, const char *s2, size_t n, bool reverse, VALUE_TYPE *values, size_t values_size, void *options);
typedef size_t (*HIRSCHBERG_TYPED(function_varargs))(const char *s1, size_t m, const char *s2, size_t n, bool reverse, VALUE_TYPE *values, size_t values_size, size_t num_args, va_list args);

typedef struct {
    hirschberg_value_function_type_t type;
    union {
        HIRSCHBERG_TYPED(function_standard) standard;
        HIRSCHBERG_TYPED(function_options) options;
        HIRSCHBERG_TYPED(function_varargs) varargs;
    } func;
    void *options;
    size_t num_args;
    va_list args;
} HIRSCHBERG_TYPED(function);

typedef struct {
    VALUE_TYPE *values;
    size_t size;
} HIRSCHBERG_TYPED(values);

typedef struct {
    string_pair_input_t input;
    hirschberg_options_t options;
    HIRSCHBERG_TYPED(values) *values;
    HIRSCHBERG_TYPED(function) *values_function;
    string_subproblem_array *stack;
    string_subproblem_t sub;
    bool is_result;
} HIRSCHBERG_TYPED(iter);

HIRSCHBERG_TYPED(values) *HIRSCHBERG_TYPED(values_new)(size_t size) {
    HIRSCHBERG_TYPED(values) *self = malloc(sizeof(HIRSCHBERG_TYPED(values)));
    if (self == NULL) return NULL;
    VALUE_TYPE *values = malloc(sizeof(VALUE_TYPE) * 2 * size);
    if (values == NULL) {
        free(self);
        return NULL;
    }
    self->values = values;
    self->size = size;
    return self;
}

static bool HIRSCHBERG_TYPED(values_resize)(HIRSCHBERG_TYPED(values) *self, size_t size) {
    if (self == NULL) return false;
    if (size == self->size) return true;
    VALUE_TYPE *new_values = realloc(self->values, sizeof(VALUE_TYPE) * 2 * size);
    if (new_values == NULL) return false;
    self->values = new_values;
    self->size = size;
    return true;
}

static inline VALUE_TYPE *HIRSCHBERG_TYPED(forward_values)(HIRSCHBERG_TYPED(values) *self) {
    if (self == NULL || self->values == NULL) return NULL;
    return self->values;
}

static inline VALUE_TYPE *HIRSCHBERG_TYPED(reverse_values)(HIRSCHBERG_TYPED(values) *self) {
    if (self == NULL || self->values == NULL) return NULL;
    return self->values + self->size;
}

static inline void HIRSCHBERG_TYPED(zero_values)(HIRSCHBERG_TYPED(values) *self) {
    if (self == NULL || self->values == NULL) return;
    memset(self->values, 0, sizeof(VALUE_TYPE) * 2 * self->size);
}

static inline void HIRSCHBERG_TYPED(values_destroy)(HIRSCHBERG_TYPED(values) *self) {
    if (self == NULL) return;
    free(self->values);
    free(self);
}

static HIRSCHBERG_TYPED(function) *HIRSCHBERG_TYPED(function_new)(HIRSCHBERG_TYPED(function_standard) standard_func) {
    HIRSCHBERG_TYPED(function) *function = malloc(sizeof(HIRSCHBERG_TYPED(function)));
    if (function == NULL) return NULL;
    function->type = VALUE_FUNCTION_STANDARD;
    function->func.standard = standard_func;
    return function;
}

static HIRSCHBERG_TYPED(function) *HIRSCHBERG_TYPED(function_new_options)(HIRSCHBERG_TYPED(function_options) options_func, void *options) {
    HIRSCHBERG_TYPED(function) *function = malloc(sizeof(HIRSCHBERG_TYPED(function)));
    if (function == NULL) return NULL;
    function->type = VALUE_FUNCTION_OPTIONS;
    function->func.options = options_func;
    function->options = options;
    return function;
}

static HIRSCHBERG_TYPED(function) *HIRSCHBERG_TYPED(function_new_varargs)(HIRSCHBERG_TYPED(function_varargs) varargs_func, size_t num_args, ...) {
    HIRSCHBERG_TYPED(function) *function = malloc(sizeof(HIRSCHBERG_TYPED(function)));
    if (function == NULL) return NULL;
    function->type = VALUE_FUNCTION_VARARGS;
    va_list args;
    va_start(args, num_args);
    function->func.varargs = varargs_func;
    function->num_args = num_args;
    va_copy(function->args, args);
    va_end(args);
    return function;
}

HIRSCHBERG_TYPED(iter) *HIRSCHBERG_TYPED(iter_new)(string_pair_input_t input,
                                                   hirschberg_options_t options,
                                                   HIRSCHBERG_TYPED(values) *values,
                                                   HIRSCHBERG_TYPED(function) *values_function) {
    if (values == NULL || values_function == NULL) return NULL;
    HIRSCHBERG_TYPED(iter) *iter = malloc(sizeof(HIRSCHBERG_TYPED(iter)));
    if (iter == NULL) return NULL;
    string_subproblem_array *stack = string_subproblem_array_new();
    if (stack == NULL) {
        free(iter);
        return NULL;
    }

    iter->input = input;
    iter->options = options;
    iter->values = values;
    iter->values_function = values_function;
    iter->stack = stack;
    iter->sub = NULL_SUBPROBLEM;
    iter->is_result = false;

    string_subproblem_array_push(iter->stack, (string_subproblem_t) {
        .x = 0,
        .m = input.m,
        .y = 0,
        .n = input.n
    });
    return iter;
}


static bool HIRSCHBERG_TYPED(iter_next)(HIRSCHBERG_TYPED(iter) *iter) {
    if (iter == NULL || iter->stack == NULL || iter->values == NULL || iter->values_function == NULL) return false;
    string_pair_input_t input = iter->input;
    if (input.m == 0 || input.n == 0) return false;

    hirschberg_options_t options = iter->options;
    bool utf8 = options.utf8;
    bool allow_transpose = options.allow_transpose;
    string_subproblem_array *stack = iter->stack;

    if (!string_subproblem_array_pop(stack, &iter->sub)) return false;
    string_subproblem_t sub = iter->sub;

    const char *s1 = input.s1 + sub.x;
    const char *s2 = input.s2 + sub.y;
    size_t m = sub.m;
    size_t n = sub.n;

    bool single_char_one_side = false;

    if (m > 0 && n > 0) {
        if (utf8) {
            int32_t s1_c1 = 0, s1_c2 = 0, s2_c1 = 0, s2_c2 = 0;
            utf8proc_ssize_t s1_c1_len = utf8proc_iterate((const uint8_t *)s1, -1, &s1_c1);
            utf8proc_ssize_t s2_c1_len = utf8proc_iterate((const uint8_t *)s2, -1, &s2_c1);
            if (s1_c1_len == m && s2_c1_len == n) {
                iter->is_result = true;
                return true;
            } else if (s1_c1_len == m || s2_c1_len == n) {
                single_char_one_side = true;
            }
            utf8proc_ssize_t s1_c2_len = utf8proc_iterate((const uint8_t *)s1 + s1_c1_len, -1, &s1_c2);
            utf8proc_ssize_t s2_c2_len = utf8proc_iterate((const uint8_t *)s2 + s2_c1_len, -1, &s2_c2);
            if (s1_c1_len + s1_c2_len == m && s2_c1_len + s2_c2_len == n
                && UTF8_CHAR_EQUAL(s1_c1, s2_c2)
                && UTF8_CHAR_EQUAL(s1_c2, s2_c1)
                && !(UTF8_CHAR_EQUAL(s1_c1, s1_c2))
            ) {
                iter->is_result = true;
                return true;
            }
        } else {
            if (m == 1 && n == 1) {
                iter->is_result = true;
                return true;
            } else if (m == 1 || n == 1) {
                single_char_one_side = true;
            } else if (m == 2 && n == 2 && CHAR_EQUAL(s1[0], s2[1])
                        && CHAR_EQUAL(s1[1], s2[0])
                        && !(CHAR_EQUAL(s1[0], s1[1]))
            ) {
                iter->is_result = true;
                return true;
            }
        }
    } else if (m == 0 || n == 0) {
        iter->is_result = true;
        return true;
    }

    iter->is_result = false;

    size_t sub_m = floor((double)m / 2.0);
    if (utf8 && utf8_is_continuation(s1[sub_m])) {
        sub_m -= utf8_prev(s1, sub_m);
    }
    if (allow_transpose) {
        if (utf8 && subproblem_border_transpose_utf8(s1, s2, sub, sub_m)) {
            sub_m += utf8_next(s1 + sub_m);
        } else if (!utf8 && m > 1 && subproblem_border_transpose(s1, s2, sub, sub_m)) {
            sub_m++;
        }
    }

    if (options.zero_out_memory) {
        HIRSCHBERG_TYPED(zero_values)(iter->values);
    }

    VALUE_TYPE *forward_values = HIRSCHBERG_TYPED(forward_values)(iter->values);
    VALUE_TYPE *reverse_values = HIRSCHBERG_TYPED(reverse_values)(iter->values);
    size_t values_len = iter->values->size;

    HIRSCHBERG_TYPED(function) *values_function = iter->values_function;

    // reverse flag is false on the forward pass and true on the reverse pass
    static const bool FORWARD = false;
    static const bool REVERSE = true;
    size_t size_used = 0;
    size_t rev_size_used = 0;
    if (values_function->type == VALUE_FUNCTION_STANDARD) {
        size_used = values_function->func.standard(s1, sub_m, s2, n, FORWARD, forward_values, values_len);
        rev_size_used = values_function->func.standard(s1 + sub_m, m - sub_m,
                                                       s2, n, REVERSE, reverse_values, values_len);
    } else if (values_function->type == VALUE_FUNCTION_OPTIONS) {
        size_used = values_function->func.options(s1, sub_m, s2, n, FORWARD, forward_values, values_len, values_function->options);
        rev_size_used = values_function->func.options(s1 + sub_m, m - sub_m,
                                                  s2, n, REVERSE, reverse_values, values_len, values_function->options);
    } else if (values_function->type == VALUE_FUNCTION_VARARGS) {
        size_used = values_function->func.varargs(s1, sub_m, s2, n, FORWARD, forward_values, values_len, values_function->num_args, values_function->args);
        rev_size_used = values_function->func.varargs(s1 + sub_m, m - sub_m,
                                                  s2, n, REVERSE, reverse_values, values_len, values_function->num_args, values_function->args);
    } else {
        return false;
    }

    size_t sub_n = 0;

    // IMPROVES encodes whether to maximize similarity or minimize distance
    #ifdef HIRSCHBERG_SIMILARITY
    #define IMPROVES >
    VALUE_TYPE opt_sum = (VALUE_TYPE) 0;
    #else
    #define IMPROVES <
    VALUE_TYPE opt_sum = (VALUE_TYPE) MAX_VALUE;
    #endif

    bool opt_sum_improved = false;

    #ifndef VALUE_EQUALS
    #define VALUE_EQUALS_DEFINED
    #define VALUE_EQUALS(a, b) ((a) == (b))
    #endif

    if (utf8) {
        const char *s2_ptr = s2;
        size_t s2_consumed = 0;
        for (size_t j = 0; j < size_used; j++) {
            size_t c_len = utf8_next(s2_ptr);
            forward_values[j] += reverse_values[size_used - j - 1];
            if (forward_values[j] IMPROVES opt_sum || (!opt_sum_improved && VALUE_EQUALS(forward_values[j], opt_sum))) {
                opt_sum_improved = !single_char_one_side; // handles m = 1 and n = 1
                sub_n = s2_consumed;
                opt_sum = forward_values[j];
            }
            s2_consumed += c_len;
            s2_ptr += c_len;
        }
    } else {
        for (size_t j = 0; j < size_used; j++) {
            forward_values[j] += reverse_values[size_used - j - 1];
            if (forward_values[j] IMPROVES opt_sum || (!opt_sum_improved && VALUE_EQUALS(forward_values[j], opt_sum))) {
                opt_sum_improved = !single_char_one_side; // handles m = 1 and n = 1
                sub_n = j;
                opt_sum = forward_values[j];
            }
        }
    }

    if ((sub_n == 0 && sub_m == 0) || (sub_n == n && sub_m == m)){
        if (!utf8) {
            sub_m = 1;
            sub_n = 1;
        } else {
            sub_m = utf8_next(s1);
            sub_n = utf8_next(s2);
        }
    }

    string_subproblem_t left_sub = (string_subproblem_t) {
        .x = sub.x,
        .m = sub_m,
        .y = sub.y,
        .n = sub_n
    };
    string_subproblem_t right_sub = (string_subproblem_t) {
        .x = sub.x + sub_m,
        .m = sub.m - sub_m,
        .y = sub.y + sub_n,
        .n = sub.n - sub_n
    };
    string_subproblem_array_push(stack, right_sub);
    string_subproblem_array_push(stack, left_sub);
    return true;
}

static inline void HIRSCHBERG_TYPED(iter_destroy)(HIRSCHBERG_TYPED(iter) *iter) {
    if (iter == NULL) return;
    if (iter->stack != NULL) string_subproblem_array_destroy(iter->stack);
    if (iter->values != NULL) HIRSCHBERG_TYPED(values_destroy)(iter->values);
    if (iter->values_function != NULL) free(iter->values_function);
    free(iter);
}


#undef CONCAT3_
#undef CONCAT3
#undef HIRSCHBERG_TYPED
#undef IMPROVES
#ifdef CHAR_EQUAL_DEFINED
#undef CHAR_EQUAL
#undef CHAR_EQUAL_DEFINED
#endif
#ifdef UTF8_CHAR_EQUAL_DEFINED
#undef UTF8_CHAR_EQUAL
#undef UTF8_CHAR_EQUAL_DEFINED
#endif
#ifdef VALUE_EQUALS_DEFINED
#undef VALUE_EQUALS
#undef VALUE_EQUALS_DEFINED
#endif
