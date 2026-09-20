/*
 * compare.c
 * 과제 02 - 3. 배열 구현과 연결 구현의 비교
 *
 *   [1] 메모리 사용량 실측 (일반 / 완전 / 편향 이진트리)
 *   [2] 두 구현이 같은 결과를 내는지 검증 + 자식/부모/형제 조회 비용 측정
 *
 * 빌드:  gcc -std=c99 -Wall -Wextra -o tree_compare compare.c
 * 실행:  ./tree_compare
 */
#include "tree_array.h"
#include "tree_linked.h"

/* ------------------------- 테스트 트리 문자열 생성기 ------------------------- */
static void put(char *buf, size_t *len, const char *s)
{
    size_t l = strlen(s);
    memcpy(buf + *len, s, l + 1);
    *len += l;
}

static void put_num(char *buf, size_t *len, int v)
{
    char tmp[16];
    snprintf(tmp, sizeof tmp, "%d", v);
    put(buf, len, tmp);
}

/* 노드 n개짜리 완전 이진트리: 레벨 순서로 1..n 번호를 붙인다 */
static void gen_complete_rec(char *buf, size_t *len, int i, int n)
{
    put_num(buf, len, i);
    if (2 * i <= n) {
        put(buf, len, "(");
        gen_complete_rec(buf, len, 2 * i, n);
        if (2 * i + 1 <= n) {
            put(buf, len, ",");
            gen_complete_rec(buf, len, 2 * i + 1, n);
        }
        put(buf, len, ")");
    }
}

static char *gen_complete(int n)
{
    char *buf = (char *)malloc((size_t)n * 12 + 16);
    size_t len = 0;
    buf[0] = '\0';
    gen_complete_rec(buf, &len, 1, n);
    return buf;
}

/* 노드 n개짜리 편향 트리. right=0 이면 왼쪽 편향, 1 이면 오른쪽 편향 */
static char *gen_skewed(int n, int right)
{
    char *buf = (char *)malloc((size_t)n * 12 + 16);
    size_t len = 0;
    int i;
    buf[0] = '\0';
    for (i = 1; i <= n; i++) {
        put_num(buf, &len, i);
        if (i < n) put(buf, &len, right ? "(," : "(");
    }
    for (i = 1; i < n; i++) put(buf, &len, ")");
    return buf;
}

/* ------------------------- 측정 도구 ------------------------- */
typedef struct {
    size_t n;
    int    height;
    size_t a_slots, a_total, l_total;
    int    tracked_ok;          /* 추적 카운터와 계산값이 일치하는가 */
} MemRow;

/* 같은 문자열로 두 구조를 각각 만들어 메모리를 잰다 */
static int measure(const char *text, MemRow *row)
{
    ArrayTree  at;
    LinkedTree lt;
    char err[256];
    size_t base, a_heap_tracked, l_heap_tracked;
    MemInfo am, lm;

    at_init(&at);
    lt_init(&lt);

    base = g_mem_bytes;
    if (!at_build(&at, text, err, sizeof err)) {
        printf("  (배열 구현 실패: %s)\n", err);
        return 0;
    }
    a_heap_tracked = g_mem_bytes - base;
    am = at_memory(&at);

    base = g_mem_bytes;
    if (!lt_build(&lt, text, err, sizeof err)) { at_free(&at); return 0; }
    l_heap_tracked = g_mem_bytes - base;
    lm = lt_memory(&lt);

    row->n        = am.used;
    row->height   = at_height(&at);
    row->a_slots  = am.units;
    row->a_total  = am.handle + am.heap;
    row->l_total  = lm.handle + lm.heap;
    row->tracked_ok = (a_heap_tracked == am.heap) && (l_heap_tracked == lm.heap);

    at_free(&at);
    lt_free(&lt);
    return 1;
}

static void print_row(const char *label, const char *text)
{
    MemRow r;
    if (!measure(text, &r)) return;
    printf("%5llu %6d %9llu %11llu %11llu %9.2f %8.1f%%  %s%s\n",
           (unsigned long long)r.n, r.height, (unsigned long long)r.a_slots,
           (unsigned long long)r.a_total, (unsigned long long)r.l_total,
           (double)r.a_total / (double)r.l_total,
           100.0 * (double)r.n / (double)r.a_slots, label,
           r.tracked_ok ? "" : "  [!추적값 불일치]");
}

static void print_header(void)
{
    printf("%5s %6s %9s %11s %11s %9s %9s  %s\n",
           "n", "height", "slots", "array(B)", "linked(B)", "arr/link", "fill", "type");
}

/* ------------------------- 두 구현 결과 일치 검증 ------------------------- */
static int check_same(const char *label, const char *text)
{
    ArrayTree  at;
    LinkedTree lt;
    char err[256];
    int ok = 1;

    at_init(&at);
    lt_init(&lt);
    if (!at_build(&at, text, err, sizeof err) || !lt_build(&lt, text, err, sizeof err)) {
        printf("  %-28s 빌드 실패\n", label);
        at_free(&at); lt_free(&lt);
        return 0;
    }
    ok &= at_count(&at)      == lt_count(&lt);
    ok &= at_leaf_count(&at) == lt_leaf_count(&lt);
    ok &= at_height(&at)     == lt_height(&lt);
    ok &= at_degree(&at)     == lt_degree(&lt);
    ok &= at_is_full(&at)     == lt_is_full(&lt);
    ok &= at_is_complete(&at) == lt_is_complete(&lt);
    ok &= at_skew(&at)        == lt_skew(&lt);

    printf("  %-28s n=%-4llu leaf=%-4llu h=%-3d deg=%d  full=%d complete=%d skew=%d  -> %s\n",
           label, (unsigned long long)at_count(&at), (unsigned long long)at_leaf_count(&at),
           at_height(&at), at_degree(&at), at_is_full(&at), at_is_complete(&at), at_skew(&at),
           ok ? "두 구현 결과 일치" : "불일치!");
    at_free(&at);
    lt_free(&lt);
    return ok;
}

/* ------------------------- 자식/부모/형제 조회 비용 ------------------------- */
static void cost_row(const char *label, const char *text, const char *node)
{
    ArrayTree  at;
    LinkedTree lt;
    RelInfo ra, rl;
    char err[256];

    at_init(&at);
    lt_init(&lt);
    if (!at_build(&at, text, err, sizeof err) || !lt_build(&lt, text, err, sizeof err)) {
        at_free(&at); lt_free(&lt);
        return;
    }
    at_relatives(&at, node, &ra);
    lt_relatives(&lt, node, &rl);
    printf("  %-16s %-5s | %6ld %6ld %6ld | %6ld %6ld %6ld | %s\n", label, node,
           ra.search_steps, ra.rel_steps, ra.search_steps + ra.rel_steps,
           rl.search_steps, rl.rel_steps, rl.search_steps + rl.rel_steps,
           (ra.found && rl.found &&
            strcmp(ra.parent, rl.parent) == 0 && strcmp(ra.left, rl.left) == 0 &&
            strcmp(ra.right, rl.right) == 0 && strcmp(ra.sibling, rl.sibling) == 0)
               ? "결과 동일" : "결과 다름!");
    at_free(&at);
    lt_free(&lt);
}

int main(void)
{
    /* 손으로 만든 일반 이진트리 (노드 15개, 높이 5) */
    const char *general = "1(2(4(8,9(,15)),5(10)),3(6(,11(12,13)),7(14)))";
    char *complete15 = gen_complete(15);
    char *complete12 = gen_complete(12);
    char *left15     = gen_skewed(15, 0);
    char *right15    = gen_skewed(15, 1);
    int i;

    init_console();
    printf("===== 배열 vs 연결 구현 비교 =====\n\n");
    printf("sizeof(Slot)=%llu  sizeof(Node)=%llu  sizeof(ArrayTree)=%llu  sizeof(LinkedTree)=%llu\n\n",
           (unsigned long long)sizeof(Slot), (unsigned long long)sizeof(Node),
           (unsigned long long)sizeof(ArrayTree), (unsigned long long)sizeof(LinkedTree));

    /* ---- 3-[1] 메모리: 노드 수를 비슷하게 맞춘 세 종류의 트리 ---- */
    printf("[3-1-a] 트리 종류별 메모리 (노드 수 12~15개)\n");
    print_header();
    print_row("일반 이진트리",          general);
    print_row("완전 이진트리 (포화)",    complete15);
    print_row("완전 이진트리 (비포화)",  complete12);
    print_row("왼쪽 편향 이진트리",      left15);
    print_row("오른쪽 편향 이진트리",    right15);

    /* ---- 3-[1] 메모리: 노드 수를 늘려 가며 ---- */
    printf("\n[3-1-b] 완전 이진트리: 노드 수가 늘어날 때\n");
    print_header();
    {
        int sizes[] = { 7, 15, 63, 255, 1023 };
        for (i = 0; i < 5; i++) {
            char *s = gen_complete(sizes[i]);
            print_row("완전 이진트리", s);
            free(s);
        }
    }

    printf("\n[3-1-c] 왼쪽 편향 이진트리: 노드 수가 늘어날 때 (배열은 슬롯이 2^n 으로 증가)\n");
    print_header();
    {
        int sizes[] = { 5, 10, 15, 20 };
        for (i = 0; i < 4; i++) {
            char *s = gen_skewed(sizes[i], 0);
            print_row("왼쪽 편향", s);
            free(s);
        }
    }
    {
        char *s = gen_skewed(23, 0);
        printf("  (노드 23개짜리 편향 트리를 배열에 넣으면?)\n  ");
        {
            ArrayTree at;
            char err[256];
            at_init(&at);
            if (!at_build(&at, s, err, sizeof err)) printf("배열 구현: %s\n", err);
            at_free(&at);
        }
        free(s);
    }

    /* ---- 두 구현이 같은 답을 내는지 ---- */
    printf("\n[검증] 같은 입력에 대해 두 구현의 트리 정보/형태 판별 결과 비교\n");
    {
        int ok = 1;
        ok &= check_same("general(15)", general);
        ok &= check_same("complete(15)", complete15);
        ok &= check_same("complete(12)", complete12);
        ok &= check_same("left-skew(15)", left15);
        ok &= check_same("right-skew(15)", right15);
        ok &= check_same("single node", "A");
        ok &= check_same("zigzag 1(,2(3))", "1(,2(3))");
        ok &= check_same("A(B,)", "A(B,)");
        printf("  (skew 코드: 0=아님 1=왼쪽 2=오른쪽 3=지그재그 4=노드 1개)\n");
        printf("  => %s\n", ok ? "모든 경우 일치" : "불일치가 있습니다");
    }

    /* ---- 3-[2] 자식/부모/형제 조회 비용 ---- */
    printf("\n[3-2] 자식/부모/형제 조회 비용 (접근한 슬롯/노드 수)\n");
    printf("  %-16s %-5s | %-20s | %-20s |\n", "tree", "node", "array: find rel total", "linked: find rel total");
    cost_row("general(15)",  general,    "1");
    cost_row("general(15)",  general,    "3");
    cost_row("general(15)",  general,    "15");
    cost_row("complete(15)", complete15, "15");
    cost_row("left-skew(15)", left15,    "8");
    cost_row("left-skew(15)", left15,    "15");
    cost_row("right-skew(15)", right15,  "15");
    {
        char *big = gen_complete(1023);
        cost_row("complete(1023)", big, "1023");
        cost_row("complete(1023)", big, "511");
        free(big);
        big = gen_skewed(20, 0);
        cost_row("left-skew(20)", big, "20");
        free(big);
    }
    printf("  (find = 이름으로 노드를 찾는 비용, rel = 그 노드의 부모/자식/형제를 얻는 비용)\n");

    free(complete15); free(complete12); free(left15); free(right15);
    return 0;
}
