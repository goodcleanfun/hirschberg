#ifndef HIRSCHBERG_H
#define HIRSCHBERG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include "vector/vector.h"

typedef struct {
    const char *s1;
    size_t m;
    const char *s2;
    size_t n;
} string_subproblem_t;

VECTOR_INIT(string_subproblem_array, string_subproblem_t);

typedef enum {
    DISTANCE,
    SIMILARITY
} hirschberg_metric_t;

typedef enum {
    HIRSCHBERG_COST_STANDARD,
    HIRSCHBERG_COST_OPTIONS,
    HIRSCHBERG_COST_VARARGS
} hirschberg_cost_function_type_t;

typedef struct {
    bool utf8;
    bool allow_transpose;
    void *costs;
    void *rev_costs;
    size_t costs_size;
    hirschberg_metric_t metric;
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

static inline bool subproblem_border_transpose(string_subproblem_t sub, size_t split) {
    if (sub.m == 0 || sub.n == 0 || split == 0) return false;
    char split_left = sub.s1[split - 1];
    char split_right = sub.s1[split];

    for (size_t j = 1; j < sub.n; j++) {
        if (sub.s2[j - 1] == split_right && sub.s2[j] == split_left && sub.s2[j - 1] != sub.s2[j]) {
            return true;
        }
    }
    return false;
}

static inline bool subproblem_border_transpose_utf8(string_subproblem_t  sub, size_t split, size_t offset) {
    if (sub.m == 0 || sub.n == 0 || split == 0) return false;
    size_t left_len = utf8_prev(sub.s1, offset);
    size_t left_pos = offset - left_len;
    size_t right_pos = offset;
    size_t right_len = utf8_next(sub.s1 + offset);
    // If the characters are equal, then it's not a transpose and we can return early
    if (left_len == right_len && memcmp(sub.s1 + left_pos, sub.s1 + right_pos, left_len) == 0) return false;

    size_t prev_start = 0;
    size_t prev_len = utf8_next(sub.s2);
    size_t start = prev_len;
    for (size_t j = 1; j < sub.n; j++) {
        size_t cur_len = utf8_next(sub.s2 + start);

        if (prev_len == right_len && cur_len == left_len && (
                (memcmp(sub.s2 + prev_start, sub.s1 + right_pos, right_len) == 0)
             && (memcmp(sub.s2 + start, sub.s1 + left_pos, left_len) == 0))
        ) {
            return true;
        }
        prev_start = start;
        prev_len = cur_len;
        start += cur_len;
    }

    return false;
}


static inline bool subproblem_is_transpose_utf8(string_subproblem_t sub) {
    if (sub.m != 2 || sub.n != 2) return false;
    size_t s1_c1_len = utf8_next(sub.s1);
    size_t s1_c2_len = utf8_next(sub.s1 + s1_c1_len);
    size_t s2_c1_len = utf8_next(sub.s2);
    size_t s2_c2_len = utf8_next(sub.s2 + s2_c1_len);
    if (s1_c1_len == 0 || s1_c2_len == 0 || s2_c1_len == 0 || s2_c2_len == 0) return false;
    return (s1_c1_len == s2_c2_len && s1_c2_len == s2_c1_len
        && (memcmp(sub.s1, sub.s2 + s2_c1_len, s1_c1_len) == 0)
        && (memcmp(sub.s1 + s1_c1_len, sub.s2, s1_c2_len) == 0)
        && !(s1_c1_len == s1_c2_len && memcmp(sub.s1, sub.s1 + s1_c1_len, s1_c1_len) == 0));
}

static inline bool subproblem_is_transpose(string_subproblem_t sub) {
    return (sub.m == 2 && sub.n == 2
        && sub.s1[0] == sub.s2[1]
        && sub.s1[1] == sub.s2[0]
        && sub.s1[0] != sub.s1[1]);
}


#define HIRSCHBERG_INIT(name, cost_type)                                                        \
    typedef void (*hirschberg_##name##_cost_function_t)(                                        \
        const char *s1, size_t m, const char *s2, size_t n, bool rev, cost_type *costs);        \
    typedef void (*hirschberg_##name##_cost_function_options_t)(                                \
        const char *s1, size_t m, const char *s2, size_t n, bool rev, cost_type *costs, void *options);   \
    typedef void (*hirschberg_##name##_cost_function_varargs_t)(                                \
        const char *s1, size_t m, const char *s2, size_t n, bool rev, cost_type *costs, va_list args);   \
    typedef struct {                                                                            \
        hirschberg_cost_function_type_t type;                                                   \
        union {                                                                                 \
            hirschberg_##name##_cost_function_t standard;                                       \
            hirschberg_##name##_cost_function_options_t options;                                \
            hirschberg_##name##_cost_function_varargs_t varargs;                                \
        } function;                                                                             \
    } hirschberg_##name##_cost_function_wrapper_t;                                              \
    bool hirschberg_subproblems_##name##_core(                                                  \
                                const char *s1, size_t m, const char *s2, size_t n,             \
                                hirschberg_##name##_cost_function_wrapper_t cost_function,      \
                                hirschberg_context_t context, void *options, va_list args) {    \
        if (m == 0 || n == 0) return false;                                                     \
        if (m < n) {                                                                            \
            const char *tmp = s1;                                                               \
            s1 = s2;                                                                            \
            s2 = tmp;                                                                           \
            size_t tmp_n = m;                                                                   \
            m = n;                                                                              \
            n = tmp_n;                                                                          \
        }                                                                                       \
                                                                                                \
        bool utf8 = context.utf8;                                                               \
        bool allow_transpose = context.allow_transpose;                                         \
        cost_type *costs = (cost_type *)context.costs;                                          \
        cost_type *rev_costs = (cost_type *)context.rev_costs;                                  \
        size_t costs_size = context.costs_size;                                                 \
        string_subproblem_array *stack = context.stack;                                         \
        string_subproblem_array *result = context.result;                                       \
        hirschberg_metric_t metric = context.metric;                                            \
                                                                                                \
        size_t cost = 0;                                                                        \
        if (stack->n > 0) {                                                                     \
            string_subproblem_array_clear(stack);                                               \
        }                                                                                       \
        if (result->n > 0) {                                                                    \
            string_subproblem_array_clear(result);                                              \
        }                                                                                       \
                                                                                                \
        string_subproblem_t prob = (string_subproblem_t) {                                      \
            .s1 = s1,                                                                           \
            .m = m,                                                                             \
            .s2 = s2,                                                                           \
            .n = n                                                                              \
        };                                                                                      \
        string_subproblem_array_push(stack, prob);                                              \
                                                                                                \
        string_subproblem_t sub;                                                                \
        while (string_subproblem_array_pop(stack, &sub)) {                                      \
            if (sub.m == 0 || sub.n == 0 || (sub.m == 1 && sub.n == 1)) {                       \
                string_subproblem_array_push(result, sub);                                      \
                continue;                                                                       \
            } else if (allow_transpose && sub.m == 2 && sub.n == 2 && (                         \
                      (utf8 && subproblem_is_transpose_utf8(sub))                               \
                   || (!utf8 && subproblem_is_transpose(sub)))                                  \
            ) {                                                                                 \
                string_subproblem_array_push(result, sub);                                      \
                continue;                                                                       \
            }                                                                                   \
                                                                                                \
            size_t sub_m = floor((double)sub.m / 2.0);                                          \
            size_t sub_m_offset = sub_m;                                                        \
            if (utf8) {                                                                         \
                sub_m_offset = utf8_offset(sub.s1, sub_m);                                      \
            }                                                                                   \
            if (allow_transpose && sub.m > 1 && (                                               \
                      (utf8 && subproblem_border_transpose_utf8(sub, sub_m, sub_m_offset))      \
                   || (!utf8 && subproblem_border_transpose(sub, sub_m))                        \
            )) {                                                                                \
                sub_m++;                                                                        \
                if (utf8) {                                                                     \
                    sub_m_offset += utf8_next(sub.s1 + sub_m_offset);                           \
                } else {                                                                        \
                    sub_m_offset++;                                                             \
                }                                                                               \
            }                                                                                   \
                                                                                                \
            memset(costs, 0, sizeof(cost_type) * costs_size);                                   \
            bool rev = false;                                                                   \
            if (cost_function.type == HIRSCHBERG_COST_STANDARD) {                               \
                cost_function.function.standard(sub.s1, sub_m, sub.s2, sub.n, rev, costs);      \
            } else if (cost_function.type == HIRSCHBERG_COST_OPTIONS) {                         \
                cost_function.function.options(sub.s1, sub_m, sub.s2, sub.n, rev, costs, options); \
            } else if (cost_function.type == HIRSCHBERG_COST_VARARGS) {                         \
                cost_function.function.varargs(sub.s1, sub_m, sub.s2, sub.n, rev, costs, args); \
            }                                                                                   \
            cost_type lc = costs[sub.n];                                                        \
            memset(rev_costs, 0, sizeof(cost_type) * costs_size);                               \
            rev = true;                                                                         \
            if (cost_function.type == HIRSCHBERG_COST_STANDARD) {                               \
                cost_function.function.standard(sub.s1 + sub_m_offset, sub.m - sub_m,           \
                                                sub.s2, sub.n, rev, rev_costs);                 \
            } else if (cost_function.type == HIRSCHBERG_COST_OPTIONS) {                         \
                cost_function.function.options(sub.s1 + sub_m_offset, sub.m - sub_m,            \
                                                sub.s2, sub.n, rev, rev_costs, options);        \
            } else if (cost_function.type == HIRSCHBERG_COST_VARARGS) {                         \
                cost_function.function.varargs(sub.s1 + sub_m_offset, sub.m - sub_m,            \
                                                sub.s2, sub.n, rev, rev_costs, args);           \
            }                                                                                   \
            cost_type rc = rev_costs[sub.n];                                                    \
            cost_type best_sum = (cost_type) 0;                                                 \
            size_t sub_n = 0;                                                                   \
            if (metric == DISTANCE) {                                                           \
                for (size_t j = 0; j < sub.n + 1; j++) {                                        \
                    costs[j] += rev_costs[sub.n - j];                                           \
                    if (costs[j] < best_sum || (sub_n == 0 && j > 0 && costs[j] == best_sum)) { \
                        sub_n = j;                                                              \
                        best_sum = costs[j];                                                    \
                    }                                                                           \
                }                                                                               \
            } else if (metric == SIMILARITY) {                                                  \
                for (size_t j = 0; j < sub.n + 1; j++) {                                        \
                    costs[j] += rev_costs[sub.n - j];                                           \
                    if (costs[j] > best_sum || (sub_n == 0 && j > 0 && costs[j] == best_sum)) { \
                        sub_n = j;                                                              \
                        best_sum = costs[j];                                                    \
                    }                                                                           \
                }                                                                               \
            }                                                                                   \
            if ((sub_n == 0 && sub_m == 0) || (sub_n == sub.n && sub_m == sub.m)){              \
                sub_m = 1;                                                                      \
                sub_n = 1;                                                                      \
            }                                                                                   \
            size_t sub_n_offset = sub_n;                                                        \
            if (utf8) {                                                                         \
                sub_n_offset = utf8_offset(sub.s2, sub_n);                                      \
            }                                                                                   \
                                                                                                \
            string_subproblem_t left_sub = (string_subproblem_t) {                              \
                .s1 = sub.s1,                                                                   \
                .m = sub_m,                                                                     \
                .s2 = sub.s2,                                                                   \
                .n = sub_n                                                                      \
            };                                                                                  \
            string_subproblem_t right_sub = (string_subproblem_t) {                             \
                .s1 = sub.s1 + sub_m_offset,                                                    \
                .m = sub.m - sub_m,                                                             \
                .s2 = sub.s2 + sub_n_offset,                                                    \
                .n = sub.n - sub_n                                                              \
            };                                                                                  \
            string_subproblem_array_push(stack, right_sub);                                     \
            string_subproblem_array_push(stack, left_sub);                                      \
                                                                                                \
        }                                                                                       \
                                                                                                \
        return true;                                                                            \
    }                                                                                           \
    bool hirschberg_subproblems_##name(                                                         \
                                const char *s1, size_t m, const char *s2, size_t n,             \
                                hirschberg_##name##_cost_function_t cost_function,              \
                                hirschberg_context_t context) {                                 \
        hirschberg_##name##_cost_function_wrapper_t cost_wrapper = {                            \
            .type = HIRSCHBERG_COST_STANDARD,                                                   \
            .function = {                                                                       \
                .standard = cost_function                                                       \
            }                                                                                   \
        };                                                                                      \
        return hirschberg_subproblems_##name##_core(s1, m, s2, n,                               \
                                                    cost_wrapper, context, NULL, NULL);         \
    }                                                                                           \
    bool hirschberg_subproblems_##name##_options(                                               \
                                const char *s1, size_t m, const char *s2, size_t n,             \
                                hirschberg_##name##_cost_function_options_t cost_function,      \
                                hirschberg_context_t context, void *options) {                  \
        hirschberg_##name##_cost_function_wrapper_t cost_wrapper = {                            \
            .type = HIRSCHBERG_COST_OPTIONS,                                                    \
            .function = {                                                                       \
                .options = cost_function                                                        \
            }                                                                                   \
        };                                                                                      \
        return hirschberg_subproblems_##name##_core(s1, m, s2, n,                               \
                                                       cost_wrapper, context, options, NULL);   \
    }                                                                                           \
    bool hirschberg_subproblems_##name##_varargs(                                               \
                                const char *s1, size_t m, const char *s2, size_t n,             \
                                hirschberg_##name##_cost_function_varargs_t cost_function,      \
                                hirschberg_context_t context, ...) {                            \
        va_list args;                                                                           \
        va_start(args, context);                                                                \
        hirschberg_##name##_cost_function_wrapper_t cost_wrapper = {                            \
            .type = HIRSCHBERG_COST_VARARGS,                                                    \
            .function = {                                                                       \
                .varargs = cost_function                                                        \
            }                                                                                   \
        };                                                                                      \
        bool result = hirschberg_subproblems_##name##_core(s1, m, s2, n,                        \
                                                           cost_wrapper, context, NULL, args);  \
        va_end(args);                                                                           \
        return result;                                                                          \
    }


#endif
