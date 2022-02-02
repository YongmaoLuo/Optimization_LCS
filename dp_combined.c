#include <inttypes.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// call this function to start a nanosecond-resolution timer
struct timespec timer_start() {
    struct timespec start_time;
    timespec_get(&start_time, TIME_UTC);
    return start_time;
}

// call this function to end a timer, returning nanoseconds elapsed as a long
uint64_t timer_end(struct timespec start_time) {
    struct timespec end_time;
    timespec_get(&end_time, TIME_UTC);
    uint64_t diffInNanos =
        (uint64_t)(end_time.tv_sec - start_time.tv_sec) * (uint64_t)1e9 +
        (uint64_t)(end_time.tv_nsec - start_time.tv_nsec);
    return diffInNanos;
}

int lcs_optimized(char* s1, char* s2, int len1, int len2) {
    int dp0[len2 + 1], dp1[len2 + 1];
    int i, j;
    int res = 0;
    int tmp;

#pragma omp parallel for private(i, j, tmp)
    for (i = 0; i <= len2; i++) {
        dp0[i] = 0;
        dp1[i] = 0;
    }

    // omp_set_num_threads(8);

    for (i = 1; i <= len1; i++) {
#pragma omp parallel for private(j, tmp) shared(dp0, dp1, s1, s2, len2) \
    reduction(max                                                       \
              : res)
        for (j = 1; j <= len2; j++) {
            if (s1[i - 1] == s2[j - 1]) {
                if (i % 2 == 0) {
                    tmp = dp1[j - 1] + 1;
                    dp0[j] = tmp;
                } else {
                    tmp = dp0[j - 1] + 1;
                    dp1[j] = tmp;
                }
                res = (tmp > res) ? tmp : res;
            } else {
                if (i % 2 == 0) {
                    dp0[j] = 0;
                } else {
                    dp1[j] = 0;
                }
            }
        }
        // #pragma omp barrier
        //         tmp = -1;
        //         tmp_dp = (i % 2 == 0) ? dp0 : dp1;
        // #pragma omp parallel for shared(tmp_dp) reduction(max : tmp)
        //         for (j = 0; j <= len2; j++) {
        //             tmp = (tmp > tmp_dp[j]) ? tmp : tmp_dp[j];
        //         }
        //         res = (tmp > res) ? tmp : res;
    }
    return res;
}

int lcs(char* s1, char* s2, int len1, int len2) {
    int dp[2][len2 + 1];
    int i, j;
    int res = 0;
    int tmp;

    for (i = 1; i <= len1; i++) {
        for (j = 1; j <= len2; j++) {
            if (s1[i - 1] == s2[j - 1]) {
                tmp = dp[(i - 1) % 2][j - 1] + 1;
                dp[i % 2][j] = tmp;
                res = (tmp > res) ? tmp : res;
            } else {
                dp[i % 2][j] = 0;
            }
        }
    }
    return res;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Wrong number of arguments");
        return 1;
    }
    FILE* f1 = fopen(argv[1], "r");
    FILE* f2 = fopen(argv[2], "r");

    fseek(f1, 0, SEEK_END);
    fseek(f2, 0, SEEK_END);

    size_t sz1 = ftell(f1);
    size_t sz2 = ftell(f2);

    fseek(f1, 0, SEEK_SET);
    fseek(f2, 0, SEEK_SET);

    char* s1 = malloc(sz1);
    char* s2 = malloc(sz2);

    fread(s1, sizeof(char), sz1, f1);
    fread(s2, sizeof(char), sz2, f2);
    s1[sz1] = 0;
    s2[sz2] = 0;

    printf("length of files: %ld, %ld\n", sz1, sz2);

    struct timespec st = timer_start();
    int len_lcs = lcs(s1, s2, sz1, sz2);
    uint64_t dt = timer_end(st);
    printf("length of lcs is %d\n", len_lcs);
    printf("time taken %f ms\n", dt / 1000000.0);

    st = timer_start();
    len_lcs = lcs_optimized(s1, s2, sz1, sz2);
    dt = timer_end(st);
    printf("optimized: length of lcs is %d\n", len_lcs);
    printf("optimized: time taken %f ms\n", dt / 1000000.0);

    free(s1);
    free(s2);
    return 0;
}