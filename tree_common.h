/*
 * tree_common.h
 * 과제 02 - 이진트리 프로그램: 배열 구현 / 연결 구현이 함께 쓰는 공통 도구
 *
 *   1) 노드 이름 크기, 콘솔 설정
 *   2) 동적 메모리 사용량 추적 (mem_alloc / mem_grow / mem_release)
 *   3) 괄호 표기법을 읽기 위한 간단한 렉서(Parser)
 *   4) 결과 출력 도우미 (트리 정보, 형태 판별, 관계 조회, 메모리 사용량)
 *
 * 괄호 표기법 문법:
 *      tree := 이름 [ '(' [tree] [ ',' [tree] ] ')' ]
 *   - A(B(D,E),C)  : A의 왼쪽 자식 B, 오른쪽 자식 C
 *   - A(,C)        : 왼쪽 자식이 없고 오른쪽 자식만 C
 *   - A(B)  A(B,)  : 왼쪽 자식 B만 있음
 *   - A            : 단말 노드
 */
#ifndef TREE_COMMON_H
#define TREE_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define NAME_LEN      8         /* 노드 이름: 최대 7글자 + '\0' */
#define INDENT        4         /* 트리 출력 시 레벨당 들여쓰기 칸 수 */
#define LINE_MAX_LEN  65536     /* 한 줄 입력 최대 길이 */

/* 편향 이진트리 판별 결과 */
#define SKEW_NONE    0          /* 편향 아님 (자식이 2개인 노드가 있음) */
#define SKEW_LEFT    1          /* 모든 노드가 왼쪽 자식만 가짐 */
#define SKEW_RIGHT   2          /* 모든 노드가 오른쪽 자식만 가짐 */
#define SKEW_ZIGZAG  3          /* 자식이 1개씩이지만 방향이 섞임 */
#define SKEW_SINGLE  4          /* 노드가 1개뿐인 자명한 경우 */

/* ------------------------------------------------------------------ */
/* 1) 콘솔                                                             */
/* ------------------------------------------------------------------ */
static inline void init_console(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);    /* 한글 출력이 깨지지 않도록 */
#endif
}

static inline int read_line(char *buf, size_t size)
{
    size_t len;
    if (!fgets(buf, (int)size, stdin)) return 0;
    len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = '\0';
    return 1;
}

/* ------------------------------------------------------------------ */
/* 2) 동적 메모리 사용량 추적                                          */
/*    트리 코드는 malloc/free를 직접 부르지 않고 아래 함수를 사용하므로  */
/*    "요청한 바이트 수"를 정확히 셀 수 있다.                           */
/* ------------------------------------------------------------------ */
static size_t g_mem_bytes  = 0;     /* 현재 살아 있는 동적 할당 바이트 */
static size_t g_mem_blocks = 0;     /* 현재 살아 있는 할당 블록 수     */

static inline void *mem_alloc(size_t size)
{
    void *p = calloc(1, size);
    if (!p) { fprintf(stderr, "메모리 할당 실패\n"); exit(1); }
    g_mem_bytes += size;
    g_mem_blocks++;
    return p;
}

/* old_size -> new_size (new_size >= old_size). 늘어난 부분은 0으로 채운다. */
static inline void *mem_grow(void *p, size_t old_size, size_t new_size)
{
    void *q = realloc(p, new_size);
    if (!q) { fprintf(stderr, "메모리 할당 실패\n"); exit(1); }
    memset((char *)q + old_size, 0, new_size - old_size);
    if (!p) g_mem_blocks++;
    g_mem_bytes += new_size - old_size;
    return q;
}

static inline void mem_release(void *p, size_t size)
{
    if (!p) return;
    free(p);
    g_mem_bytes -= size;
    g_mem_blocks--;
}

/* ------------------------------------------------------------------ */
/* 3) 괄호 표기법 렉서                                                 */
/* ------------------------------------------------------------------ */
typedef struct {
    const char *s;          /* 입력 문자열 */
    int         pos;        /* 현재 위치 */
    char        err[200];   /* 첫 번째 오류 메시지 */
} Parser;

static inline void ps_error(Parser *p, const char *msg)
{
    if (!p->err[0])
        snprintf(p->err, sizeof p->err, "%d번째 문자 부근: %s", p->pos + 1, msg);
}

/* 공백을 건너뛰고 다음 문자를 (소비하지 않고) 돌려준다. 끝이면 0 */
static inline int ps_peek(Parser *p)
{
    while (isspace((unsigned char)p->s[p->pos])) p->pos++;
    return (unsigned char)p->s[p->pos];
}

/* 닫는 괄호 ')' 를 소비한다. 성공하면 1 */
static inline int ps_close(Parser *p)
{
    int c = ps_peek(p);
    if (c == ')') { p->pos++; return 1; }
    if (c == '\0') ps_error(p, "닫는 괄호 ')' 가 부족합니다");
    else           ps_error(p, "')' 가 필요합니다 (이진트리는 자식이 최대 2개)");
    return 0;
}

/* 노드 이름을 읽는다. 성공하면 1 */
static inline int ps_label(Parser *p, char *out)
{
    int c = ps_peek(p), len = 0;
    if (c == '\0' || c == '(' || c == ')' || c == ',') {
        ps_error(p, "노드 이름이 필요합니다");
        return 0;
    }
    while ((c = (unsigned char)p->s[p->pos]) != '\0' &&
           c != '(' && c != ')' && c != ',' && !isspace(c)) {
        if (len >= NAME_LEN - 1) {
            ps_error(p, "노드 이름은 최대 7글자까지 가능합니다");
            return 0;
        }
        out[len++] = (char)c;
        p->pos++;
    }
    out[len] = '\0';
    return 1;
}

/* ------------------------------------------------------------------ */
/* 4) 출력 도우미                                                      */
/* ------------------------------------------------------------------ */
static inline void print_info(long n, long leaf, int height, int degree)
{
    printf("  1. 전체 노드의 수       : %ld\n", n);
    printf("  2. 단말 노드의 수       : %ld\n", leaf);
    printf("  3. 비단말 노드의 수     : %ld\n", n - leaf);
    printf("  4. 트리의 높이(height)  : %d  (루트 레벨=1 기준, 간선 수 기준이면 %d)\n",
           height, height - 1);
    printf("  5. 트리의 차수(degree)  : %d\n", degree);
}

static inline void print_shape(int full, int complete, int skew)
{
    printf("  완전 이진트리 : %s\n", complete ? "예" : "아니오");
    printf("  포화 이진트리 : %s\n", full ? "예" : "아니오");
    switch (skew) {
    case SKEW_LEFT:   printf("  편향 이진트리 : 예 (왼쪽 편향)\n"); break;
    case SKEW_RIGHT:  printf("  편향 이진트리 : 예 (오른쪽 편향)\n"); break;
    case SKEW_SINGLE: printf("  편향 이진트리 : 예 (노드가 1개뿐인 자명한 경우)\n"); break;
    case SKEW_ZIGZAG:
        printf("  편향 이진트리 : 아니오 (노드마다 자식은 1개뿐이지만 왼쪽/오른쪽이 섞인 지그재그형)\n");
        break;
    default:          printf("  편향 이진트리 : 아니오\n"); break;
    }
}

/* 자식/부모/형제 조회 결과 */
typedef struct {
    int  found;
    int  is_root;
    long index;                 /* 배열 구현에서만 의미 있음 (연결 구현은 0) */
    char parent[NAME_LEN], left[NAME_LEN], right[NAME_LEN], sibling[NAME_LEN];
    long search_steps;          /* 이름으로 노드를 찾는 동안 접근한 슬롯/노드 수 */
    long rel_steps;             /* 부모·자식·형제를 얻는 동안 접근한 슬롯/노드 수 */
} RelInfo;

static inline void rel_print(const RelInfo *r, const char *name)
{
    if (!r->found) {
        printf("'%s' 노드를 트리에서 찾을 수 없습니다.\n", name);
        return;
    }
    printf("노드 %s\n", name);
    printf("  부모      : %s\n", r->is_root ? "없음 (루트)" : r->parent);
    printf("  왼쪽 자식 : %s\n", r->left[0]  ? r->left  : "없음");
    printf("  오른쪽 자식: %s\n", r->right[0] ? r->right : "없음");
    printf("  형제      : %s\n", r->is_root ? "없음 (루트)" : (r->sibling[0] ? r->sibling : "없음"));
    printf("  [비용] 노드 찾기 %ld회 + 관계 얻기 %ld회 = 총 %ld회 접근\n",
           r->search_steps, r->rel_steps, r->search_steps + r->rel_steps);
}

/* 메모리 사용량 */
typedef struct {
    size_t handle;      /* 트리를 가리키는 구조체 자체의 크기 */
    size_t heap;        /* 동적으로 할당한 바이트 수 */
    size_t blocks;      /* 동적 할당 블록 수 */
    size_t units;       /* 할당된 저장 단위 수 (배열: 슬롯 수, 연결: 노드 수) */
    size_t used;        /* 실제 노드 수 */
    size_t unit_size;   /* 저장 단위 1개의 크기 */
} MemInfo;

static inline void print_memory(const MemInfo *m, const char *unit_name)
{
    printf("  저장 단위(%s) 1개의 크기 : %llu B\n", unit_name, (unsigned long long)m->unit_size);
    printf("  트리 구조체 자체 크기    : %llu B\n", (unsigned long long)m->handle);
    printf("  동적 할당                : %llu %s x %llu B = %llu B (블록 %llu개)\n",
           (unsigned long long)m->units, unit_name, (unsigned long long)m->unit_size,
           (unsigned long long)m->heap, (unsigned long long)m->blocks);
    printf("  합계                     : %llu B\n", (unsigned long long)(m->handle + m->heap));
    printf("  실제 노드 수 / %s 수  : %llu / %llu (활용률 %.1f%%)\n", unit_name,
           (unsigned long long)m->used, (unsigned long long)m->units,
           m->units ? 100.0 * (double)m->used / (double)m->units : 0.0);
}

#endif /* TREE_COMMON_H */
