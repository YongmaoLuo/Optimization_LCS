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
    int slen = (len1 < len2) ? len1 : len2;
    int llen = (len1 < len2) ? len2 : len1;
    char* shorter = slen == len1 ? s1 : s2;
    char* longer = s1 == shorter ? s2 : s1;

    int maxlen;
    int curlen;

    int tmp_len[llen];

    int i, j, k;

#pragma omp parallel for
    for (i = 0; i < llen; i++) {
        maxlen = -1;
        for (j = 0; j < slen; j++) {
            curlen = 0;
            for (k = 0; k < slen - j; k++) {
                if (i + k >= llen) break;
                if (longer[i + k] != shorter[j + k]) {
                    break;
                }
                curlen++;
            }
            if (curlen > maxlen) {
                maxlen = curlen;
            }
        }
        tmp_len[i] = maxlen;
    }
#pragma omp barrier
maxlen = -1;
#pragma omp parallel for reduction(max : maxlen)
    for (j = 0; j < llen; j++) {
        maxlen = (tmp_len[j] > maxlen) ? tmp_len[j] : maxlen;
    }

    return maxlen;
}

int lcs(char* s1, char* s2, int len1, int len2) {
    int slen = (len1 < len2) ? len1 : len2;
    int llen = (len1 < len2) ? len2 : len1;
    char* shorter = slen == len1 ? s1 : s2;
    char* longer = s1 == shorter ? s2 : s1;

    int maxlen = -1;
    int curlen;

    int i, j, k;

    for (i = 0; i < llen; i++) {
        for (j = 0; j < slen; j++) {
            curlen = 0;
            for (k = 0; k < slen - j; k++) {
                if (i + k >= llen) break;
                if (longer[i + k] != shorter[j + k]) {
                    break;
                }
                curlen++;
            }
            if (curlen > maxlen) {
                maxlen = curlen;
            }
        }
    }

    return maxlen;
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
