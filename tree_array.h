/*
 * tree_array.h
 * 과제 02 - [1] 배열을 이용한 이진트리 구현
 *
 * 표현 방법 (1-based 인덱스, 0번 슬롯은 사용하지 않음)
 *   - 루트               : slot[1]
 *   - 노드 i 의 왼쪽 자식 : slot[2*i]
 *   - 노드 i 의 오른쪽 자식: slot[2*i + 1]
 *   - 노드 i 의 부모      : slot[i / 2]
 *   - 빈 슬롯은 name[0] == '\0' 으로 표시한다.
 *   - 배열 크기(cap)는 "가장 큰 인덱스 + 1" 로 딱 맞게 잡는다.
 */
#ifndef TREE_ARRAY_H
#define TREE_ARRAY_H

#include "tree_common.h"

/* 편향 트리처럼 깊은 트리는 슬롯 수가 2^h 로 폭발한다. 안전장치로 한도를 둔다. */
#define AT_MAX_SLOTS ((size_t)1 << 22)      /* 4,194,304 슬롯 (= 32 MB) */

typedef struct { char name[NAME_LEN]; } Slot;

typedef struct {
    Slot  *slot;    /* slot[0..cap-1] */
    size_t cap;     /* 할당된 슬롯 수 (0번 포함) */
} ArrayTree;

/* ------------------------------ 기본 연산 ------------------------------ */
static inline void at_init(ArrayTree *t) { t->slot = NULL; t->cap = 0; }

static inline void at_free(ArrayTree *t)
{
    mem_release(t->slot, t->cap * sizeof(Slot));
    at_init(t);
}

/* i번 슬롯에 노드가 있는가? (범위 밖이면 없는 것으로 본다) */
static inline int at_has(const ArrayTree *t, size_t i)
{
    return i >= 1 && i < t->cap && t->slot[i].name[0] != '\0';
}

static inline void at_reserve(ArrayTree *t, size_t need)
{
    if (need <= t->cap) return;
    t->slot = (Slot *)mem_grow(t->slot, t->cap * sizeof(Slot), need * sizeof(Slot));
    t->cap = need;
}

/* ------------------------------ 입력(파싱) ------------------------------ */
/* idx 번 슬롯에 노드를 저장하며 재귀적으로 내려간다. 성공하면 1 */
static int at_parse_node(ArrayTree *t, Parser *p, size_t idx)
{
    char name[NAME_LEN];

    if (!ps_label(p, name)) return 0;

    if (idx >= AT_MAX_SLOTS) {
        char msg[160];
        snprintf(msg, sizeof msg,
                 "배열로 표현하기에 트리가 너무 깊습니다 (이 노드의 인덱스가 %llu 이상이며, 약 %llu MB 필요)",
                 (unsigned long long)idx,
                 (unsigned long long)((idx + 1) * sizeof(Slot) / (1024 * 1024)));
        ps_error(p, msg);
        return 0;
    }
    at_reserve(t, idx + 1);
    strcpy(t->slot[idx].name, name);

    if (ps_peek(p) == '(') {
        p->pos++;
        if (ps_peek(p) != ',' && ps_peek(p) != ')') {           /* 왼쪽 자식 */
            if (!at_parse_node(t, p, 2 * idx)) return 0;
        }
        if (ps_peek(p) == ',') {
            p->pos++;
            if (ps_peek(p) != ')') {                            /* 오른쪽 자식 */
                if (!at_parse_node(t, p, 2 * idx + 1)) return 0;
            }
        }
        if (!ps_close(p)) return 0;
    }
    return 1;
}

/* 괄호 표기법 문자열로 트리를 만든다. 실패하면 0을 돌려주고 err에 이유를 적는다. */
static int at_build(ArrayTree *t, const char *text, char *err, size_t errsz)
{
    Parser p;
    int ok;

    at_free(t);
    p.s = text; p.pos = 0; p.err[0] = '\0';

    ok = at_parse_node(t, &p, 1);
    if (ok && ps_peek(&p) != '\0') {
        ps_error(&p, "트리가 끝난 뒤에 불필요한 문자가 있습니다");
        ok = 0;
    }
    if (!ok) {
        snprintf(err, errsz, "%s", p.err);
        at_free(t);
        return 0;
    }
    return 1;
}

/* ------------------------------ 트리 정보 ------------------------------ */
static inline size_t at_count(const ArrayTree *t)
{
    size_t i, n = 0;
    for (i = 1; i < t->cap; i++) if (t->slot[i].name[0]) n++;
    return n;
}

static inline int at_children(const ArrayTree *t, size_t i)
{
    return at_has(t, 2 * i) + at_has(t, 2 * i + 1);
}

static inline size_t at_leaf_count(const ArrayTree *t)
{
    size_t i, n = 0;
    for (i = 1; i < t->cap; i++)
        if (at_has(t, i) && at_children(t, i) == 0) n++;
    return n;
}

/* 인덱스 i 가 속한 레벨 (루트=1) : i 의 이진수 자릿수 = floor(log2 i) + 1 */
static inline int at_level_of(size_t i)
{
    int l = 0;
    while (i) { l++; i >>= 1; }
    return l;
}

/* 높이 = 가장 깊은 노드의 레벨 (= 가장 큰 인덱스의 레벨) */
static inline int at_height(const ArrayTree *t)
{
    size_t i;
    for (i = t->cap; i-- > 1; )
        if (at_has(t, i)) return at_level_of(i);
    return 0;
}

/* 차수 = 노드가 가진 자식 수의 최댓값 */
static inline int at_degree(const ArrayTree *t)
{
    size_t i;
    int d = 0;
    for (i = 1; i < t->cap; i++)
        if (at_has(t, i) && at_children(t, i) > d) d = at_children(t, i);
    return d;
}

/* ------------------------------ 형태 판별 ------------------------------ */
/* 포화: 높이가 h 이고 노드 수가 정확히 2^h - 1 */
static inline int at_is_full(const ArrayTree *t)
{
    int h = at_height(t);
    return h > 0 && at_count(t) == (((size_t)1 << h) - 1);
}

/* 완전: 노드가 n개일 때 slot[1..n] 이 모두 차 있음 (사이에 빈 칸이 없음) */
static inline int at_is_complete(const ArrayTree *t)
{
    size_t i, n = at_count(t);
    if (n == 0) return 0;
    for (i = 1; i <= n; i++)
        if (!at_has(t, i)) return 0;
    return 1;
}

/* 편향: 모든 노드의 자식이 1개 이하이며, 그 방향이 모두 같음 */
static inline int at_skew(const ArrayTree *t)
{
    size_t i;
    int saw_l = 0, saw_r = 0;
    for (i = 1; i < t->cap; i++) {
        int l, r;
        if (!at_has(t, i)) continue;
        l = at_has(t, 2 * i);
        r = at_has(t, 2 * i + 1);
        if (l && r) return SKEW_NONE;
        if (l) saw_l = 1;
        if (r) saw_r = 1;
    }
    if (saw_l && saw_r) return SKEW_ZIGZAG;
    if (saw_l)          return SKEW_LEFT;
    if (saw_r)          return SKEW_RIGHT;
    return SKEW_SINGLE;
}

/* ------------------------------ 출력 ------------------------------ */
/* 왼쪽으로 90도 눕힌 모양: 오른쪽 서브트리(위) -> 자신 -> 왼쪽 서브트리(아래) */
static void at_print_rec(const ArrayTree *t, size_t i, int depth)
{
    if (!at_has(t, i)) return;
    at_print_rec(t, 2 * i + 1, depth + 1);
    printf("%*s%s\n", depth * INDENT, "", t->slot[i].name);
    at_print_rec(t, 2 * i, depth + 1);
}

static inline void at_print(const ArrayTree *t) { at_print_rec(t, 1, 0); }

/* 배열 내용을 [인덱스]=이름 형태로 보여준다 (앞쪽 max_show 개만) */
static inline void at_dump(const ArrayTree *t, size_t max_show)
{
    size_t i, last = t->cap < max_show + 1 ? t->cap : max_show + 1;
    for (i = 1; i < last; i++) {
        if (t->slot[i].name[0]) printf("[%llu]=%s ", (unsigned long long)i, t->slot[i].name);
        else                    printf("[%llu]=- ", (unsigned long long)i);
    }
    if (last < t->cap) printf("... (전체 %llu 슬롯 중 %llu개만 표시)",
                              (unsigned long long)(t->cap - 1), (unsigned long long)max_show);
    printf("\n");
}

/* ------------------------------ 노드 찾기 / 관계 조회 ------------------------------ */
/* 이름으로 인덱스를 찾는다 (슬롯을 1번부터 순서대로 확인). 못 찾으면 0 */
static inline size_t at_find(const ArrayTree *t, const char *name, long *steps)
{
    size_t i;
    for (i = 1; i < t->cap; i++) {
        (*steps)++;
        if (t->slot[i].name[0] && strcmp(t->slot[i].name, name) == 0) return i;
    }
    return 0;
}

/* 부모/자식/형제 조회: 인덱스 계산만으로 끝난다.
 *   부모 i/2, 왼쪽 자식 2i, 오른쪽 자식 2i+1, 형제 i^1 (짝수면 i+1, 홀수면 i-1) */
static void at_relatives(const ArrayTree *t, const char *name, RelInfo *r)
{
    size_t i;
    memset(r, 0, sizeof *r);
    i = at_find(t, name, &r->search_steps);
    if (!i) return;

    r->found = 1;
    r->index = (long)i;

    r->rel_steps += 2;                                     /* 왼쪽/오른쪽 자식 슬롯 확인 */
    if (at_has(t, 2 * i))     strcpy(r->left,  t->slot[2 * i].name);
    if (at_has(t, 2 * i + 1)) strcpy(r->right, t->slot[2 * i + 1].name);

    if (i == 1) { r->is_root = 1; return; }

    r->rel_steps += 2;                                     /* 부모 슬롯, 형제 슬롯 확인 */
    strcpy(r->parent, t->slot[i / 2].name);
    if (at_has(t, i ^ 1)) strcpy(r->sibling, t->slot[i ^ 1].name);
}

/* 같은 이름이 둘 이상 있는가? */
static inline int at_has_duplicate(const ArrayTree *t)
{
    size_t i, j, n = 0, *idx;
    int dup = 0;
    idx = (size_t *)malloc((t->cap + 1) * sizeof *idx);
    if (!idx) return 0;
    for (i = 1; i < t->cap; i++) if (at_has(t, i)) idx[n++] = i;
    for (i = 0; i < n && !dup; i++)
        for (j = i + 1; j < n; j++)
            if (strcmp(t->slot[idx[i]].name, t->slot[idx[j]].name) == 0) { dup = 1; break; }
    free(idx);
    return dup;
}

/* ------------------------------ 메모리 ------------------------------ */
static inline MemInfo at_memory(const ArrayTree *t)
{
    MemInfo m;
    m.handle    = sizeof(ArrayTree);
    m.heap      = t->cap * sizeof(Slot);
    m.blocks    = t->slot ? 1 : 0;
    m.units     = t->cap;                 /* 0번 슬롯(미사용)도 포함 */
    m.used      = at_count(t);
    m.unit_size = sizeof(Slot);
    return m;
}

#endif /* TREE_ARRAY_H */
