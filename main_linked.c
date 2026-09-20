/*
 * main_linked.c
 * 과제 02 - 2. 포인터를 이용한 연결 자료구조 이진트리 (메뉴 프로그램)
 *
 * 빌드:  gcc -std=c99 -Wall -Wextra -o tree_linked main_linked.c
 * 실행:  ./tree_linked                       (트리를 직접 입력)
 *        ./tree_linked "A(B(D,E),C)"         (인자로 트리 전달)
 *        ./tree_linked "A(B(D,E),C)" --all   (모든 기능을 한 번에 실행하고 종료)
 */
#include "tree_linked.h"

static int load_tree(LinkedTree *t, const char *text)
{
    char err[256];
    if (!lt_build(t, text, err, sizeof err)) {
        printf("입력 오류: %s\n", err);
        return 0;
    }
    printf("트리를 읽었습니다. (노드 %llu개)\n", (unsigned long long)lt_count(t));
    if (lt_has_duplicate(t))
        printf("경고: 이름이 같은 노드가 있습니다. 관계 조회는 처음 발견된 노드를 기준으로 합니다.\n");
    return 1;
}

/* [1] 이진트리 출력 */
static void show_tree(const LinkedTree *t)
{
    printf("[1] 이진트리 출력 (왼쪽으로 눕힌 형태: 위쪽이 오른쪽 자식, 아래쪽이 왼쪽 자식)\n");
    lt_print(t);
}

/* [2] 트리 정보 출력 */
static void show_info(const LinkedTree *t)
{
    printf("[2] 트리 정보 (연결 구현)\n");
    print_info((long)lt_count(t), (long)lt_leaf_count(t), lt_height(t), lt_degree(t));
}

/* [3] 이진트리의 형태 판별 */
static void show_shape(const LinkedTree *t)
{
    printf("[3] 이진트리의 형태 판별 (연결 구현)\n");
    print_shape(lt_is_full(t), lt_is_complete(t), lt_skew(t));
}

/* 3-[2] 자식/부모/형제 조회 */
static void show_relatives(const LinkedTree *t)
{
    char name[256];
    RelInfo r;

    printf("조회할 노드 이름: ");
    if (!read_line(name, sizeof name)) return;
    lt_relatives(t, name, &r);
    rel_print(&r, name);
    if (r.found && !r.is_root)
        printf("  (부모 포인터가 없어서 부모/형제는 루트부터 다시 탐색해서 찾았습니다)\n");
}

/* 3-[1] 메모리 사용량 */
static void show_memory(const LinkedTree *t)
{
    MemInfo m = lt_memory(t);
    printf("[메모리 사용량] 연결 구현\n");
    print_memory(&m, "노드");
    printf("  (실측: 동적 할당 추적 카운터 = %llu B, 블록 %llu개)\n",
           (unsigned long long)g_mem_bytes, (unsigned long long)g_mem_blocks);
}

static void run_all(const LinkedTree *t)
{
    show_tree(t);   printf("\n");
    show_info(t);   printf("\n");
    show_shape(t);  printf("\n");
    show_memory(t);
}

static void print_menu(void)
{
    printf("\n------ 연결 이진트리 메뉴 ------\n");
    printf(" [1] 이진트리 출력\n");
    printf(" [2] 트리 정보 출력\n");
    printf(" [3] 이진트리의 형태 판별\n");
    printf(" [4] 노드의 자식/부모/형제 조회\n");
    printf(" [5] 메모리 사용량\n");
    printf(" [6] 새 트리 입력\n");
    printf(" [0] 종료\n");
    printf("선택> ");
}

int main(int argc, char **argv)
{
    LinkedTree t;
    static char line[LINE_MAX_LEN];
    int loaded = 0;

    init_console();
    lt_init(&t);
    printf("===== 포인터를 이용한 연결 이진트리 =====\n");

    if (argc >= 2) loaded = load_tree(&t, argv[1]);
    if (loaded && argc >= 3 && strcmp(argv[2], "--all") == 0) {
        run_all(&t);
        lt_free(&t);
        return 0;
    }

    for (;;) {
        while (!loaded) {
            printf("괄호 표기법으로 이진트리를 입력하세요.\n");
            printf("  예) A(B(D,E),C)   왼쪽 자식이 없으면 A(,C)\n> ");
            if (!read_line(line, sizeof line)) { lt_free(&t); return 0; }
            loaded = load_tree(&t, line);
        }

        print_menu();
        if (!read_line(line, sizeof line)) break;
        if (line[0] == '\0') continue;

        switch (atoi(line)) {
        case 1: show_tree(&t);      break;
        case 2: show_info(&t);      break;
        case 3: show_shape(&t);     break;
        case 4: show_relatives(&t); break;
        case 5: show_memory(&t);    break;
        case 6: loaded = 0;         break;
        case 0: lt_free(&t); return 0;
        default: printf("메뉴에 없는 번호입니다.\n");
        }
    }
    lt_free(&t);
    return 0;
}
