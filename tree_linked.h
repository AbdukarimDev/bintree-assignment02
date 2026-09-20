/*
 * tree_linked.h
 * 과제 02 - [2] 포인터를 이용한 연결 자료구조 구현
 *
 * 표현 방법
 *   - 노드 하나 = { 이름, 왼쪽 자식 포인터, 오른쪽 자식 포인터 }
 *   - 자식이 없으면 해당 포인터는 NULL
 *   - 노드는 필요할 때마다 하나씩 동적 할당한다 (노드 n개 = 할당 n번)
 *   - 부모 포인터는 두지 않는다 (부모를 알려면 루트부터 다시 탐색해야 함)
 */
#ifndef TREE_LINKED_H
#define TREE_LINKED_H

#include "tree_common.h"

typedef struct Node {
    char         name[NAME_LEN];
    struct Node *left;
    struct Node *right;
} Node;

typedef struct { Node *root; } LinkedTree;

/* ------------------------------ 기본 연산 ------------------------------ */
static inline void lt_init(LinkedTree *t) { t->root = NULL; }

static inline Node *lt_new_node(const char *name)
{
    Node *n = (Node *)mem_alloc(sizeof(Node));
    strcpy(n->name, name);
    n->left = n->right = NULL;
    return n;
}

static void lt_free_node(Node *n)
{
    if (!n) return;
    lt_free_node(n->left);
    lt_free_node(n->right);
    mem_release(n, sizeof(Node));
}

static inline void lt_free(LinkedTree *t) { lt_free_node(t->root); t->root = NULL; }

/* ------------------------------ 입력(파싱) ------------------------------ */
/* 노드 하나(와 그 아래 서브트리)를 읽어 만든다. 실패하면 NULL */
static Node *lt_parse_node(Parser *p)
{
    char  name[NAME_LEN];
    Node *n;

    if (!ps_label(p, name)) return NULL;
    n = lt_new_node(name);

    if (ps_peek(p) == '(') {
        p->pos++;
        if (ps_peek(p) != ',' && ps_peek(p) != ')') {           /* 왼쪽 자식 */
            n->left = lt_parse_node(p);
            if (!n->left) { lt_free_node(n); return NULL; }
        }
        if (ps_peek(p) == ',') {
            p->pos++;
            if (ps_peek(p) != ')') {                            /* 오른쪽 자식 */
                n->right = lt_parse_node(p);
                if (!n->right) { lt_free_node(n); return NULL; }
            }
        }
        if (!ps_close(p)) { lt_free_node(n); return NULL; }
    }
    return n;
}

static int lt_build(LinkedTree *t, const char *text, char *err, size_t errsz)
{
    Parser p;
    Node  *root;

    lt_free(t);
    p.s = text; p.pos = 0; p.err[0] = '\0';

    root = lt_parse_node(&p);
    if (root && ps_peek(&p) != '\0') {
        ps_error(&p, "트리가 끝난 뒤에 불필요한 문자가 있습니다");
        lt_free_node(root);
        root = NULL;
    }
    if (!root) {
        snprintf(err, errsz, "%s", p.err);
        return 0;
    }
    t->root = root;
    return 1;
}

/* ------------------------------ 트리 정보 ------------------------------ */
static size_t lt_count_rec(const Node *n)
{
    return n ? 1 + lt_count_rec(n->left) + lt_count_rec(n->right) : 0;
}

static size_t lt_leaf_rec(const Node *n)
{
    if (!n) return 0;
    if (!n->left && !n->right) return 1;
    return lt_leaf_rec(n->left) + lt_leaf_rec(n->right);
}

static int lt_height_rec(const Node *n)
{
    int l, r;
    if (!n) return 0;
    l = lt_height_rec(n->left);
    r = lt_height_rec(n->right);
    return 1 + (l > r ? l : r);
}

static void lt_degree_rec(const Node *n, int *deg)
{
    int c;
    if (!n) return;
    c = (n->left != NULL) + (n->right != NULL);
    if (c > *deg) *deg = c;
    lt_degree_rec(n->left, deg);
    lt_degree_rec(n->right, deg);
}

static inline size_t lt_count(const LinkedTree *t)      { return lt_count_rec(t->root); }
static inline size_t lt_leaf_count(const LinkedTree *t) { return lt_leaf_rec(t->root); }
static inline int    lt_height(const LinkedTree *t)     { return lt_height_rec(t->root); }
static inline int    lt_degree(const LinkedTree *t)
{
    int d = 0;
    lt_degree_rec(t->root, &d);
    return d;
}

/* ------------------------------ 형태 판별 ------------------------------ */
/* 포화: 모든 노드에서 왼쪽/오른쪽 서브트리의 높이가 같고 자식이 0개 또는 2개.
 * 포화이면 높이(>=1)를, 아니면 -1 을 돌려준다. */
static int lt_full_rec(const Node *n)
{
    int l, r;
    if (!n) return 0;
    l = lt_full_rec(n->left);
    r = lt_full_rec(n->right);
    if (l < 0 || r < 0 || l != r) return -1;
    return l + 1;
}

static inline int lt_is_full(const LinkedTree *t)
{
    return t->root != NULL && lt_full_rec(t->root) > 0;
}

/* 완전: 레벨 순서(BFS)로 방문하면서 NULL 자식을 한 번 본 뒤에는
 *       더 이상 진짜 노드가 나오지 않아야 한다. */
static int lt_is_complete(const LinkedTree *t)
{
    Node **q;
    size_t head = 0, tail = 0, n;
    int seen_null = 0, ok = 1;

    if (!t->root) return 0;
    n = lt_count(t);
    q = (Node **)malloc(n * sizeof *q);
    if (!q) return 0;

    q[tail++] = t->root;
    while (head < tail && ok) {
        Node *cur = q[head++];
        Node *ch[2];
        int k;
        ch[0] = cur->left; ch[1] = cur->right;
        for (k = 0; k < 2; k++) {
            if (!ch[k])         seen_null = 1;
            else if (seen_null) { ok = 0; break; }
            else                q[tail++] = ch[k];
        }
    }
    free(q);
    return ok;
}

/* 편향: 자식이 2개인 노드가 없고, 자식 방향이 모두 같음 */
static int lt_skew_rec(const Node *n, int *saw_l, int *saw_r)
{
    if (!n) return 1;
    if (n->left && n->right) return 0;
    if (n->left)  *saw_l = 1;
    if (n->right) *saw_r = 1;
    return lt_skew_rec(n->left, saw_l, saw_r) && lt_skew_rec(n->right, saw_l, saw_r);
}

static inline int lt_skew(const LinkedTree *t)
{
    int saw_l = 0, saw_r = 0;
    if (!lt_skew_rec(t->root, &saw_l, &saw_r)) return SKEW_NONE;
    if (saw_l && saw_r) return SKEW_ZIGZAG;
    if (saw_l)          return SKEW_LEFT;
    if (saw_r)          return SKEW_RIGHT;
    return SKEW_SINGLE;
}

/* ------------------------------ 출력 ------------------------------ */
/* 왼쪽으로 90도 눕힌 모양: 오른쪽 서브트리(위) -> 자신 -> 왼쪽 서브트리(아래) */
static void lt_print_rec(const Node *n, int depth)
{
    if (!n) return;
    lt_print_rec(n->right, depth + 1);
    printf("%*s%s\n", depth * INDENT, "", n->name);
    lt_print_rec(n->left, depth + 1);
}

static inline void lt_print(const LinkedTree *t) { lt_print_rec(t->root, 0); }

/* ------------------------------ 노드 찾기 / 관계 조회 ------------------------------ */
/* 이름으로 노드를 찾는다 (전위 순회). steps 는 방문한 노드 수 */
static const Node *lt_find_rec(const Node *n, const char *name, long *steps)
{
    const Node *f;
    if (!n) return NULL;
    (*steps)++;
    if (strcmp(n->name, name) == 0) return n;
    f = lt_find_rec(n->left, name, steps);
    if (f) return f;
    return lt_find_rec(n->right, name, steps);
}

/* target 을 자식으로 가진 노드(부모)를 루트부터 탐색해서 찾는다.
 * 부모 포인터가 없으므로 이 탐색이 추가로 필요하다. */
static const Node *lt_find_parent_rec(const Node *n, const Node *target, long *steps)
{
    const Node *f;
    if (!n) return NULL;
    (*steps)++;
    if (n->left == target || n->right == target) return n;
    f = lt_find_parent_rec(n->left, target, steps);
    if (f) return f;
    return lt_find_parent_rec(n->right, target, steps);
}

static void lt_relatives(const LinkedTree *t, const char *name, RelInfo *r)
{
    const Node *x, *par, *sib;

    memset(r, 0, sizeof *r);
    x = lt_find_rec(t->root, name, &r->search_steps);
    if (!x) return;
    r->found = 1;

    r->rel_steps += 2;                                     /* left/right 포인터 읽기 */
    if (x->left)  strcpy(r->left,  x->left->name);
    if (x->right) strcpy(r->right, x->right->name);

    if (x == t->root) { r->is_root = 1; return; }

    par = lt_find_parent_rec(t->root, x, &r->rel_steps);   /* 부모: 루트부터 다시 탐색 */
    strcpy(r->parent, par->name);
    sib = (par->left == x) ? par->right : par->left;      /* 형제: 부모의 다른 쪽 자식 */
    r->rel_steps++;
    if (sib) strcpy(r->sibling, sib->name);
}

/* 같은 이름이 둘 이상 있는가? */
static inline int lt_has_duplicate(const LinkedTree *t)
{
    size_t i, j, n = lt_count(t), tail = 0, head = 0;
    const Node **all;
    int dup = 0;

    if (!t->root) return 0;
    all = (const Node **)malloc(n * sizeof *all);
    if (!all) return 0;
    all[tail++] = t->root;                                 /* BFS 로 모든 노드를 모은다 */
    while (head < tail) {
        const Node *cur = all[head++];
        if (cur->left)  all[tail++] = cur->left;
        if (cur->right) all[tail++] = cur->right;
    }
    for (i = 0; i < n && !dup; i++)
        for (j = i + 1; j < n; j++)
            if (strcmp(all[i]->name, all[j]->name) == 0) { dup = 1; break; }
    free(all);
    return dup;
}

/* ------------------------------ 메모리 ------------------------------ */
static inline MemInfo lt_memory(const LinkedTree *t)
{
    MemInfo m;
    m.used      = lt_count(t);
    m.handle    = sizeof(LinkedTree);
    m.heap      = m.used * sizeof(Node);
    m.blocks    = m.used;                 /* 노드마다 malloc 1번 */
    m.units     = m.used;
    m.unit_size = sizeof(Node);
    return m;
}

#endif /* TREE_LINKED_H */
