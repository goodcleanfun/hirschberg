#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "greatest/greatest.h"
#include "uint64_sim.h"
#include "utf8/utf8.h"

typedef struct {
    const char *s1;
    const char *s2;
    const char *expected_lcs;
} lcs_test_t;


lcs_test_t test_data_lcs[] = {
    {
        .s1="GTCGTAGAATA",
        .s2="CACGTAGTA",
        .expected_lcs="CGTAGTA"
    },
    // address with abbreviations at token boundaries
    {
        .s1="bam 30 lafyette ave bk new yORk 11217",
        .s2="Brooklyn Academy of Music 30 Lafayette Avenue Brooklyn New York",
        .expected_lcs="bam 30 lafyette ave bk new yORk"
    },
    // name
    {
        .s1="William Edward Burghardt Du Bois",
        .s2="WEB DuBois",
        .expected_lcs="WEB DuBois"
    },
    // abbreviations not at token boundaries
    {
        .s1="evidence lower bound",
        .s2="elbo",
        .expected_lcs="elbo"
    },
    // With punctuation
    {
        .s1="ca$h rules everything around me",
        .s2="c.r.e.a.m.",
        .expected_lcs="cream"
    },
    // hashtag speak
    {
        .s1="#throwbackthursdays",
        .s2="#tbt",
        .expected_lcs="#tbt"
    },
    // Spanish with unicode gaps
    {
        .s1="Hernández",
        .s2="hdez",
        .expected_lcs="hdez"
    }
};

size_t test_hirschberg_lcs_cost(const char *s1, size_t m, const char *s2, size_t n, bool reverse, uint64_t *costs, size_t costs_size) {
    uint64_t diag = 0;
    uint64_t up = 0;
    uint64_t left = 0;
    uint64_t best_cost = m + n;
    uint64_t *cur_lcs = costs;
    uint64_t *prev_lcs = costs + n + 1;
    for (size_t i = 1; i < m + 1; i++) {
        char c1 = !reverse ? tolower(s1[i - 1]) : tolower(s1[m - i]);
        for (size_t j = 1; j < n + 1; j++) {
            char c2 = !reverse ? tolower(s2[j - 1]) : tolower(s2[n - j]);
            uint64_t val = 0;
            if (c1 == c2) {
                val = prev_lcs[j - 1] + 1;
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
    return n + 1;
}


size_t test_hirschberg_lcs_utf8_cost(const char *s1, size_t m, const char *s2, size_t n, bool reverse, uint64_t *costs, size_t costs_size) {
    uint64_t best_cost = m + n;
    uint64_t *cur_lcs = costs;
    uint64_t *prev_lcs = costs + (costs_size / 2);
    const unsigned char *s1_ptr = (const unsigned char *) s1;
    const unsigned char *s2_ptr;
    size_t s1_consumed = 0;
    size_t s2_consumed = 0;
    int32_t c1;
    int32_t c2;
    size_t i = 1;
    size_t used = 0;
    while (s1_consumed < m) {
        c1 = 0;
        ssize_t c1_len;
        if (!reverse) {
            c1_len = utf8proc_iterate(s1_ptr, -1, &c1);
        } else {
            c1_len = utf8proc_iterate_reversed(s1_ptr, m - s1_consumed, &c1);
        }
        if (c1 == 0 || c1_len <= 0) break;
        c1 = utf8proc_tolower(c1);
        s2_ptr = (const unsigned char *) s2;
        s2_consumed = 0;
        size_t j = 1;
        while (s2_consumed < n) {
            c2 = 0;
            ssize_t c2_len;
            if (!reverse) {
                c2_len = utf8proc_iterate(s2_ptr, -1, &c2);
            } else {
                c2_len = utf8proc_iterate_reversed(s2_ptr, n - s2_consumed, &c2);
            }
            if (c2 == 0 || c2_len == 0) break;
            c2 = utf8proc_tolower(c2);
            uint64_t val = 0;
            if (c1 == c2) {
                val = prev_lcs[j - 1] + 1;
            } else if (prev_lcs[j] > cur_lcs[j - 1]) {
                val = prev_lcs[j];
            } else {
                val = cur_lcs[j - 1];
            }
            cur_lcs[j] = val;
            if (!reverse) {
                s2_ptr += c2_len;
            }
            s2_consumed += c2_len;
            j++;
        }
        used = j;
        for (size_t k = 0; k < used; k++) {
            prev_lcs[k] = cur_lcs[k];
        }
        if (!reverse) {
            s1_ptr += c1_len;
        }
        s1_consumed += c1_len;
        i++;
    }

    return used;
}


char *hirschberg_alignment_lcs(hirschberg_uint64_sim_iter *iter, size_t max_len) {
    char *alignment = malloc(sizeof(char) * (max_len + 1));
    size_t idx = 0;
    ssize_t c1_len, c2_len;
    int32_t c1, c2;
    const unsigned char *s1_ptr;
    const unsigned char *s2_ptr;

    const char *s1 = iter->input.s1;
    const char *s2 = iter->input.s2;

    bool result = true;
    size_t i = 0;
    while (hirschberg_uint64_sim_iter_next(iter)) {
        if (iter->is_result) {
            string_subproblem_t sub = iter->sub;
            size_t um = utf8_len(s1 + sub.x, sub.m);
            size_t un = utf8_len(s2 + sub.y, sub.n);
            if (un == 1) {
                c2_len = utf8proc_iterate((const unsigned char *) s2 + sub.y, -1, &c2);
                const unsigned char *s1_ptr = (const unsigned char *) s1 + sub.x;
                for (size_t j = 0; j < um; j++) {
                    c1_len = utf8proc_iterate(s1_ptr, -1, &c1);
                    if (utf8proc_tolower(c2) == utf8proc_tolower(c1)) {
                        for (size_t k = 0; k < c2_len; k++) {
                            alignment[idx++] = *(s2 + sub.y + k);
                        }
                        break;
                    }
                }
            } else if (um == 2 && un == 2) {
                alignment[idx++] = '/';
                alignment[idx++] = '\\';
            } else if (um == 1) {
                c2_len = utf8proc_iterate((const unsigned char *) s1 + sub.x, -1, &c2);
                const unsigned char *s2_ptr = (const unsigned char *) s2 + sub.y;
                for (size_t j = 0; j < un; j++) {
                    c1_len = utf8proc_iterate(s2_ptr, -1, &c1);
                    if (c2 == c1) {
                        for (size_t k = 0; k < c2_len; k++) {
                            alignment[idx++] = *(s1 + sub.x + k);
                        }
                        break;
                    }
                }
            }
            i++;
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
    size_t max_len = m;
    if (n > m) {
        max_len = n;
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
    }

    size_t values_size = (un + 1) * 2;

    hirschberg_uint64_sim_iter *iter = hirschberg_uint64_sim_iter_new(
        (string_pair_input_t){.s1 = s1, .m = m, .s2 = s2, .n = n},
        (hirschberg_options_t){.utf8 = is_utf8, .allow_transpose = false, .zero_out_memory = true},
        hirschberg_uint64_sim_values_new(values_size),
        hirschberg_uint64_sim_function_new(is_utf8 ? test_hirschberg_lcs_utf8_cost : test_hirschberg_lcs_cost)
    );

    char *alignment = hirschberg_alignment_lcs(iter, max_len);

    bool success = strncmp(alignment, test.expected_lcs, strlen(test.expected_lcs)) == 0;
    if (!success) {
        printf("s1: %s\n", s1);
        printf("s2: %s\n", s2);
        printf("alignment: %s\n", alignment);
        printf("expected: %s\n", test.expected_lcs);
    }

    hirschberg_uint64_sim_iter_destroy(iter);
    free(alignment);

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