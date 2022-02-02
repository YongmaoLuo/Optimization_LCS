#include <inttypes.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static inline void GetNext(char t[], int next[], int tlen) {
    int scan, num;
    scan = 0;
    num = -1;  // j扫描t，即模式串，k记录t[j]之前与t[0]开头相同的字符个数。
    //除此之外，t[k]的位置恰好是从头开始已经匹配的字符串的下一个字符。
    //所以判断t[j]和t[k]的关系，可以迅速判断k值是否需要增加。如果不相等，则说明无法继续拓展字符串长度，
    //需要利用更短的可匹配子串来试探是否可以匹配，所以k=next[k]。这里的t[k]仍然指向更短子串的下一个字符。
    next[0] = num;
    while (scan < tlen - 1) {
        if (num == -1 || t[scan] == t[num]) {
            scan++;
            num++;
            next[scan] = num;  // num既是指针，也是相同串的长度。
        } else
            num = next[num];  //在前头的匹配的串中，也用一次KMP思路减少工作量。
    }
}

int kmp_lcs_optimized(char* s1, char* s2, int len1, int len2) {
    int slen = (len1 < len2) ? len1 : len2;
    int llen = (len1 < len2) ? len2 : len1;
    char* shorter = slen == len1 ? s1 : s2;
    char* longer = shorter == s1 ? s2 : s1;

    // printf("%s\n",shorter);
    // printf("%s\n",longer);

    int maxlen = 0, fakeMax[slen];

    memset(fakeMax, 0, sizeof(fakeMax));

#pragma omp parallel for shared(longer, shorter, llen, slen)
    for (int i = 0; i < slen; i++) {
        int* next = malloc((slen - i) * sizeof(int));
        GetNext(shorter + i, next, slen - i);

        // for(int j=0;j<slen-i;j++)
        //    printf("%d ",next[j]);
        // printf("\n");
        int scan = 0, num = 0;

        while (scan < llen - fakeMax[i]) {
            if (longer[scan] == shorter[num + i]) {
                scan++;
                num++;
                if (num >= slen - i) break;
            } else {
                if (!num)
                    scan++;
                else {
                    if (num > fakeMax[i]) fakeMax[i] = num;

                    num = next
                        [num];  //在前头的匹配的串中，也用一次KMP思路减少工作量。
                }
            }
        }

        if (num > fakeMax[i]) fakeMax[i] = num;

        // if(fakeMax[i]>=slen-i){
        //    break;
        //}
    }
#pragma omp barrier
    maxlen = -1;
#pragma omp parallel for reduction(max : maxlen)
    for (int i = 0; i < slen; i++) {
        maxlen = (fakeMax[i] > maxlen) ? fakeMax[i] : maxlen;
    }

    return maxlen;
}

int kmp_lcs(char* s1, char* s2, int len1, int len2, uint64_t* GNTime) {
    int slen = (len1 < len2) ? len1 : len2;
    int llen = (len1 < len2) ? len2 : len1;
    char* shorter = slen == len1 ? s1 : s2;
    char* longer = shorter == s1 ? s2 : s1;

    // printf("%s\n",shorter);
    // printf("%s\n",longer);

    int maxlen = 0;
    int* next = malloc(slen * sizeof(int));

    for (int i = 0; i < slen; i++) {
        // struct timespec st = timer_start();

        GetNext(shorter + i, next, slen - i);

        // uint64_t dt = timer_end(st);
        // *GNTime += dt;
        // for(int j=0;j<slen-i;j++)
        //    printf("%d ",next[j]);
        // printf("\n");
        int scan = 0, num = 0;

        while (scan < llen - maxlen) {
            if (longer[scan] == shorter[num + i]) {
                scan++;
                num++;
                if (num >= slen - i) break;
            } else {
                if (!num)
                    scan++;
                else {
                    if (num > maxlen) maxlen = num;

                    num = next
                        [num];  //在前头的匹配的串中，也用一次KMP思路减少工作量。
                }
            }
        }

        if (num > maxlen) maxlen = num;

        if (maxlen >= slen - i) {
            break;
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

    uint64_t GNTime = 0;

    struct timespec st = timer_start();
    int len_lcs = kmp_lcs(s1, s2, sz1, sz2, &GNTime);
    uint64_t dt = timer_end(st);

    printf("length of lcs is %d\n", len_lcs);
    printf("time taken %f ms\n", dt / 1000000.0);

    st = timer_start();
    len_lcs = kmp_lcs_optimized(s1, s2, sz1, sz2);
    dt = timer_end(st);

    printf("optimized: length of lcs is %d\n", len_lcs);
    printf("optimized: time taken %f ms\n", dt / 1000000.0);
    // printf("Get Next Time:%f ms\n", GNTime / 1000000.0);

    free(s1);
    free(s2);
    return 0;
}