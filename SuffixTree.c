#include <immintrin.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CHAR 256

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


struct SuffixTreeNode {
    struct SuffixTreeNode *children[MAX_CHAR];
    struct SuffixTreeNode *suffixLink;
    int start;
    int *end;
    int suffixIndex;
};

typedef struct SuffixTreeNode Node;

char *text;
Node *root = NULL;
Node *lastNewNode = NULL;
Node *activeNode = NULL;
int activeEdge = -1;
int activeLength = 0;


int remainingSuffixCount = 0;
int leafEnd = -1;
int *rootEnd = NULL;
int *splitEnd = NULL;
int size = -1;
size_t size1 = 0;

Node *newNode(int start, int *end) {
    Node *node =(Node*) malloc(sizeof(Node));
    int i;
    for (i = 0; i < MAX_CHAR; i++)
        node->children[i] = NULL;

    node->suffixLink = root;
    node->start = start;
    node->end = end;

    node->suffixIndex = -1;
    return node;
}

int edgeLength(Node *n) {
    if(n == root)
        return 0;
    return *(n->end) - (n->start) + 1;
}

int walkDown(Node *currNode) {
    if (activeLength >= edgeLength(currNode)) {
        activeEdge += edgeLength(currNode);
        activeLength -= edgeLength(currNode);
        activeNode = currNode;
        return 1;
    }
    return 0;
}

void extendSuffixTree(int pos) {
    leafEnd = pos;
    remainingSuffixCount++;
    lastNewNode = NULL;

    while(remainingSuffixCount > 0) {

        if (activeLength == 0)
            activeEdge = pos; 

        if (activeNode->children[text[activeEdge]] == NULL) {
            activeNode->children[text[activeEdge]] = newNode(pos, &leafEnd);

            if (lastNewNode != NULL) {
                lastNewNode->suffixLink = activeNode;
                lastNewNode = NULL;
            }
        }
        else {
            Node *next = activeNode->children[text[activeEdge]];
            if (walkDown(next)) {
                continue;
            }
            if (text[next->start + activeLength] == text[pos]) {
                if(lastNewNode != NULL && activeNode != root) {
                    lastNewNode->suffixLink = activeNode;
                    lastNewNode = NULL;
                }
                activeLength++;
                break;
            }

            splitEnd = (int*) malloc(sizeof(int));
            *splitEnd = next->start + activeLength - 1;

            Node *split = newNode(next->start, splitEnd);
            activeNode->children[text[activeEdge]] = split;

            split->children[text[pos]] = newNode(pos, &leafEnd);
            next->start += activeLength;
            split->children[text[next->start]] = next;

            if (lastNewNode != NULL) {
                lastNewNode->suffixLink = split;
            }

            lastNewNode = split;
        }

        remainingSuffixCount--;
        if (activeNode == root && activeLength > 0) {
            activeLength--;
            activeEdge = pos - remainingSuffixCount + 1;
        }
        else if (activeNode != root) {
            activeNode = activeNode->suffixLink;
        }
    }
}

void print(int i, int j) {
    int k;
    for (k=i; k<=j && text[k] != '#'; k++)
        printf("%c", text[k]);
    if(k<=j) 
        printf("#");
}

void setSuffixIndexByDFS(Node *n, int labelHeight) {
    if (n == NULL) return;
    int leaf = 1;
    int i;
    for (i = 0; i < MAX_CHAR; i++) {
        if (n->children[i] != NULL) {
            leaf = 0;
            setSuffixIndexByDFS(n->children[i], labelHeight + edgeLength(n->children[i]));
        }
    }
    if (leaf == 1) {
        for(i= n->start; i<= *(n->end); i++) {
            if(text[i] == '#') {
                n->end = (int*) malloc(sizeof(int));
                *(n->end) = i;
            }
        }
        n->suffixIndex = size - labelHeight;
    }
}

void freeSuffixTreeByPostOrder(Node *n) {
    if (n == NULL) return;
    int i;
    for (i = 0; i < MAX_CHAR; i++) {
        if (n->children[i] != NULL) {
            freeSuffixTreeByPostOrder(n->children[i]);
        }
    }
    if (n->suffixIndex == -1) free(n->end);
    free(n);
}


void buildSuffixTree() {
    size = strlen(text);
    int i;
    rootEnd = (int*) malloc(sizeof(int));
    *rootEnd = - 1;

    root = newNode(-1, rootEnd);

    activeNode = root;
    for (i=0; i<size; i++)
        extendSuffixTree(i);
    int labelHeight = 0;
    setSuffixIndexByDFS(root, labelHeight);
}

int doTraversal(Node *n, int labelHeight, int* maxHeight, int* substringStartIndex) {
    if(n == NULL) return;
    int i=0;
    int ret = -1;
    if(n->suffixIndex < 0) {
        for (i = 0; i < MAX_CHAR; i++) {
            if(n->children[i] != NULL) {
                ret = doTraversal(n->children[i], labelHeight +
                    edgeLength(n->children[i]),
                    maxHeight, substringStartIndex);
                
                if(n->suffixIndex == -1) n->suffixIndex = ret;
                else if((n->suffixIndex == -2 && ret == -3) ||
                        (n->suffixIndex == -3 && ret == -2) ||
                         n->suffixIndex == -4) {
                    
                    n->suffixIndex = -4;

                    if(*maxHeight < labelHeight) {
                        *maxHeight = labelHeight;
                        *substringStartIndex = *(n->end) - labelHeight + 1;
                    }
                }
            }
        }
    }
    else if(n->suffixIndex > -1 && n->suffixIndex < size1)
        return -2;
    else if(n->suffixIndex >= size1)
        return -3;
    return n->suffixIndex;
}

int getLongestCommonSubstring() {
    int maxHeight = 0;
    int substringStartIndex = 0;
    doTraversal(root, 0, &maxHeight, &substringStartIndex);
    return maxHeight;
}

int main(int argc, char *argv[])
{
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
    size_t sz3 = sz1 + sz2 + 2;

    fseek(f1, 0, SEEK_SET);
    fseek(f2, 0, SEEK_SET);

    char* s1 = malloc(sz1+1);
    char* s2 = malloc(sz2+1);
    text = malloc(sz3);

    fread(s1, sizeof(char), sz1, f1);
    fread(s2, sizeof(char), sz2, f2);
    s1[sz1] = '#';
    s2[sz2] = '$';



    size1 = sz1;
    strcpy(text, s1);
    strcat(text, s2);
    struct timespec st = timer_start();
    buildSuffixTree();
    int len_lcs = getLongestCommonSubstring();
    uint64_t dt = timer_end(st);
    printf("length of lcs is %d\n", len_lcs);
    printf("time taken %f ms\n", dt / 1000000.0);
    freeSuffixTreeByPostOrder(root);
    free(s1);
    free(s2);

    return 0;
}
