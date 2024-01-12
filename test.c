#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "greatest/greatest.h"
#include "uint64_cost.h"
#include "utf8/utf8.h"

typedef struct {
    const char *s1;
    const char *s2;
    const char *expected_lcs;
} lcs_test_t;


lcs_test_t test_data_lcs[] = {
    {
        .s1="AGTACGCA",
        .s2="TATGC",
        .expected_lcs="ATGC"
    }
};



void test_hirschberg_lcs_cost(const char *s1, size_t m, const char *s2, size_t n, bool reverse, uint64_t *costs) {
    uint64_t diag = 0;
    uint64_t up = 0;
    uint64_t left = 0;
    uint64_t best_cost = m + n;
    uint64_t *cur_lcs = costs;
    uint64_t *prev_lcs = costs + n + 1;
    for (size_t i = 1; i < m + 1; i++) {
        char c1 = !reverse ? s1[i - 1] : s1[m - i];
        for (size_t j = 1; j < n + 1; j++) {
            char c2 = !reverse ? s2[j - 1] : s2[n - j];
            uint64_t val = 0;
            if (c1 == c2) {
                val = prev_lcs[j - 1] + 1;
            } else if (j > 1 && i > 1 && !reverse && c1 != c2 && c1 == s2[j - 2] && s1[i - 2] == c2) {
                val = prev_lcs[j - 2] + 2;
            } else if (prev_lcs[j] > cur_lcs[j - 1]) {
                val = prev_lcs[j];
            } else {
                val = cur_lcs[j - 1];
            }
            cur_lcs[j] = val;
        }
        for (size_t j = 0; j < n + 1; j++) {
            prev_lcs[j] = cur_lcs[j];
        }
    }
}


void test_hirschberg_lcs_utf8_cost(const char *s1, size_t m, const char *s2, size_t n, bool reverse, uint64_t *costs) {
    uint64_t diag = 0;
    uint64_t up = 0;
    uint64_t left = 0;
    uint64_t best_cost = m + n;
    uint64_t *cur_lcs = costs;
    uint64_t *prev_lcs = costs + n + 1;
    const unsigned char *s1_ptr = (const unsigned char *) s1;
    const unsigned char *s2_ptr;
    size_t s1_len, s2_len;
    size_t s1_consumed = 0;
    size_t s2_consumed = 0;
    if (reverse) {
        s1_len = strlen(s1);
        s2_len = strlen(s2);
    }
    int32_t c1;
    int32_t c2;
    int32_t prev_c1;
    int32_t prev_c2;
    for (size_t i = 1; i < m + 1; i++) {
        c1 = 0;
        ssize_t c1_len;
        if (!reverse) {
            c1_len = utf8proc_iterate(s1_ptr, -1, &c1);
        } else {
            c1_len = utf8proc_iterate_reversed(s1_ptr, s1_len - s1_consumed, &c1);
        }
        if (c1 == 0 || c1_len <= 0) break;
        s2_ptr = (const unsigned char *) s2;
        s2_consumed = 0;
        for (size_t j = 1; j < n + 1; j++) {
            c2 = 0;
            ssize_t c2_len;
            if (!reverse) {
                c2_len = utf8proc_iterate(s2_ptr, -1, &c2);
            } else {
                c2_len = utf8proc_iterate_reversed(s2_ptr, s2_len - s2_consumed, &c2);
            }
            if (c2 == 0 || c2_len == 0) break;

            uint64_t val = 0;
            if (c1 == c2) {
                val = prev_lcs[j - 1] + 1;
            } else if (j > 1 && i > 1 && !reverse && c1 != c2 && c1 == prev_c2 && prev_c1 == c2) {
                val = prev_lcs[j - 2] + 2;
            } else if (prev_lcs[j] > cur_lcs[j - 1]) {
                val = prev_lcs[j];
            } else {
                val = cur_lcs[j - 1];
            }
            cur_lcs[j] = val;
            if (!reverse) {
                s2_ptr += c2_len;
            } else {
                s2_consumed += c2_len;
            }
            prev_c2 = c2;
        }
        for (size_t j = 0; j < n + 1; j++) {
            prev_lcs[j] = cur_lcs[j];
        }
        if (!reverse) {
            s1_ptr += c1_len;
        } else {
            s1_consumed += c1_len;
        }

        prev_c1 = c1;
    }
}

char *hirschberg_alignment_lcs(string_subproblem_array *result, size_t max_len) {
    char *alignment = malloc(max_len + 1);
    size_t idx = 0;

    ssize_t c1_len, c2_len;
    int32_t c1, c2;
    const unsigned char *s1_ptr;
    const unsigned char *s2_ptr;

    for (size_t i = 0; i < result->n; i++) {
        string_subproblem_t sub = result->a[i];
        if (sub.n == 1) {
            c2_len = utf8proc_iterate((const unsigned char *) sub.s2, -1, &c2);
            const unsigned char *s1_ptr = (const unsigned char *) sub.s1;
            for (size_t j = 0; j < sub.m; j++) {
                c1_len = utf8proc_iterate(s1_ptr, -1, &c1);
                if (c2 == c1) {
                    for (size_t k = 0; k < c2_len; k++) {
                        alignment[idx++] = *(sub.s2 + k);
                    }
                    break;
                }
                s1_ptr += c1_len;
            }
        } else if (sub.n == 2 && sub.m == 2) {
            alignment[idx++] = '/';
            alignment[idx++] = '\\';
        } else if (sub.m == 1) {
            c2_len = utf8proc_iterate((const unsigned char *) sub.s1, -1, &c2);
            const unsigned char *s2_ptr = (const unsigned char *) sub.s2;
            for (size_t j = 0; j < sub.n; j++) {
                c1_len = utf8proc_iterate(s2_ptr, -1, &c1);
                if (c2 == c1) {
                    for (size_t k = 0; k < c2_len; k++) {
                        alignment[idx++] = *(sub.s1 + k);
                    }
                    break;
                }
                s2_ptr += c1_len;
            }
        }
    }
    alignment[idx] = '\0';
    return alignment;
}


bool test_hirschberg_subproblem_lcs(lcs_test_t test) {
    const char *s1 = test.s1;
    const char *s2 = test.s2;
    size_t m = strlen(s1);
    size_t n = strlen(s2);
    size_t max_len = m + n;
    if (n > m) {
        const char *tmp = s1;
        s1 = s2;
        s2 = tmp;
        size_t tmp_len = m;
        m = n;
        n = tmp_len;
    }

    bool is_utf8 = false;
    size_t um = utf8_len(s1, m);
    size_t un = utf8_len(s2, n);
    if (um != m || un != n) {
        is_utf8 = true;
        m = um;
        n = un;
    }

    string_subproblem_array *stack = string_subproblem_array_new();
    string_subproblem_array *result = string_subproblem_array_new();

    size_t cost_size = (n + 1) * 2;

    uint64_t *costs = malloc(sizeof(size_t) * cost_size * 2);
    uint64_t *rev_costs = costs + cost_size;

    bool allow_transpose = true;

    bool success = hirschberg_subproblems_uint64(s1, m, s2, n, !is_utf8 ? test_hirschberg_lcs_cost : test_hirschberg_lcs_utf8_cost,
                                                 is_utf8, allow_transpose,
                                                 costs, rev_costs, cost_size, stack, result);

    char *alignment = hirschberg_alignment_lcs(result, max_len);
    if (strncmp(alignment, test.expected_lcs, strlen(test.expected_lcs)) != 0) {
        printf("s1: %s\n", s1);
        printf("s2: %s\n", s2);
        printf("alignment: %s\n", alignment);
        printf("expected: %s\n", test.expected_lcs);
        success = false;
    }

    free(alignment);
    free(costs);
    string_subproblem_array_destroy(stack);
    string_subproblem_array_destroy(result);

    return success;
}



TEST test_hirschberg_lcs_subproblem_correctness(void) {
    size_t num_test_cases = sizeof(test_data_lcs) / sizeof(lcs_test_t);
    for (size_t i = 0; i < num_test_cases; i++) {
        lcs_test_t test = test_data_lcs[i];
        ASSERT(test_hirschberg_subproblem_lcs(test));
    }
    PASS();
}

SUITE(test_lcs_alignment_suite) {
    RUN_TEST(test_hirschberg_lcs_subproblem_correctness);
}


GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(test_lcs_alignment_suite);

    GREATEST_MAIN_END();
}