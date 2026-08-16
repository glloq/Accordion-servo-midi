#ifndef TEST_ASSERT_H
#define TEST_ASSERT_H

#include <cstdio>

static int testFailures = 0;

static inline void check(const char *name, bool ok) {
    printf("  %-64s %s\n", name, ok ? "PASS" : "*** FAIL ***");
    if (!ok) testFailures++;
}

static inline int testSummary(const char *suite) {
    printf("\n%s : %s (%d echec(s))\n\n",
           suite, testFailures ? "ECHEC" : "OK", testFailures);
    return testFailures ? 1 : 0;
}

#endif
