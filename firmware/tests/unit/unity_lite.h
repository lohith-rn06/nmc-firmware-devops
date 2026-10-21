/* Tiny unit-test harness (Unity-style API). Swap for Unity/CMocka in a real project. */
#ifndef UNITY_LITE_H
#define UNITY_LITE_H
#include <stdio.h>
#include <string.h>
static int ul_fail, ul_run;
#define TEST_ASSERT(c)                                                                                                           \
    do {                                                                                                                         \
        if (!(c)) {                                                                                                              \
            printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);                                                                \
            ul_fail++;                                                                                                           \
            return;                                                                                                              \
        }                                                                                                                        \
    } while (0)
#define TEST_ASSERT_EQ(a, b)                                                                                                     \
    do {                                                                                                                         \
        long _a = (long)(a), _b = (long)(b);                                                                                     \
        if (_a != _b) {                                                                                                          \
            printf("  FAIL %s:%d: %s == %ld, expected %ld\n", __FILE__, __LINE__, #a, _a, _b);                                   \
            ul_fail++;                                                                                                           \
            return;                                                                                                              \
        }                                                                                                                        \
    } while (0)
#define TEST_ASSERT_STR(a, b)                                                                                                    \
    do {                                                                                                                         \
        if (strcmp((a), (b))) {                                                                                                  \
            printf("  FAIL %s:%d: \"%s\" != \"%s\"\n", __FILE__, __LINE__, (a), (b));                                            \
            ul_fail++;                                                                                                           \
            return;                                                                                                              \
        }                                                                                                                        \
    } while (0)
#define RUN_TEST(fn)                                                                                                             \
    do {                                                                                                                         \
        int _f = ul_fail;                                                                                                        \
        ul_run++;                                                                                                                \
        fn();                                                                                                                    \
        printf("%s %s\n", ul_fail == _f ? "PASS" : "FAIL", #fn);                                                                 \
    } while (0)
#define TEST_END() (printf("%d tests, %d failures\n", ul_run, ul_fail), ul_fail ? 1 : 0)
#endif
