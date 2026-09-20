/*
 * 과제 01 - 괄호 표기법으로 입력된 트리의 정보 출력
 * ---------------------------------------------------------------
 * 요구사항 요약:
 *   1) 괄호 표기법 문자열을 입력받는다.
 *   2) 문법에 맞지 않으면 오류 메시지를 출력하고 종료한다.
 *   3) 올바른 트리면 다음을 출력한다.
 *        - 전체 노드 수 / 단말 노드 수 / 비단말 노드 수
 *        - 트리의 높이
 *        - 트리의 차수 (모든 노드의 자식 수 중 최댓값)
 *        - 노드 C의 부모 노드 (노드 스택 이용)
 *        - 노드 C의 자식 노드 (카운터 스택 이용)
 *        - 트리를 왼쪽으로 눕힌 형태로 '+', '-', '|' 로 출력
 *
 *   주의: 트리를 연결 리스트(포인터) 구조로 별도로 만들지 않고,
 *         입력 문자열을 그대로 스캔하면서 스택만으로 모든 정보를 계산한다.
 *
 * 컴파일:  gcc -o tree tree.c
 * 실행  :  ./tree
 * 입력 예:  A(B(E,F),C,D(G))
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXLEN    100000   /* 입력 문자열 최대 길이 */
#define MAXNODES  30       /* 노드는 A~Z, 최대 26개 -> 스택 크기 여유있게 30 */

/* ------------------------------------------------------------------ */
/* 전역 데이터                                                          */
/* ------------------------------------------------------------------ */
static char input[MAXLEN];
static int  inputLen;

/* ------------------------------------------------------------------ */
/* 1단계. 문법 검증 (재귀 하향 파서)                                     */
/*    Node     := UpperLetter Children?                                */
/*    Children := '(' Node (',' Node)* ')'                             */
/* ------------------------------------------------------------------ */
static int seenLabel[26];   /* 같은 알파벳이 두 번 쓰이면 잘못된 입력으로 간주 */

static int parseNodeValidate(int *p) {
    if (*p >= inputLen) return 0;                 /* 문자열이 예상보다 일찍 끝남 */

    char c = input[*p];
    if (!isupper((unsigned char)c)) return 0;      /* 영문 대문자가 아님 */
    if (seenLabel[c - 'A']) return 0;              /* 이미 나온 노드 이름 재사용 */
    seenLabel[c - 'A'] = 1;
    (*p)++;

    if (*p < inputLen && input[*p] == '(') {
        (*p)++;                                    /* '(' 소비 */
        if (!parseNodeValidate(p)) return 0;        /* 자식이 최소 1개 있어야 함 */
        while (*p < inputLen && input[*p] == ',') {
            (*p)++;
            if (!parseNodeValidate(p)) return 0;
        }
        if (*p >= inputLen || input[*p] != ')') return 0; /* 짝이 맞는 ')' 없음 */
        (*p)++;                                     /* ')' 소비 */
    }
    return 1;
}

static int isValidTree(void) {
    if (inputLen == 0) return 0;
    memset(seenLabel, 0, sizeof(seenLabel));
    int p = 0;
    if (!parseNodeValidate(&p)) return 0;
    return (p == inputLen);   /* 문자열 전체를 정확히 다 사용해야 함 (군더더기 문자 금지) */
}

/* ------------------------------------------------------------------ */
/* 2단계. 통계 계산 - 문자열을 한 번 왼쪽에서 오른쪽으로 스캔               */
/*    nodeStack    : 현재 "부모"가 누구인지 기억 (C의 부모 찾기용)          */
/*    counterStack : 그 부모가 지금까지 몇 명의 자식을 읽었는지 (차수, C의 자식 찾기용) */
/* ------------------------------------------------------------------ */
static char nodeStack[MAXNODES];
static int  counterStack[MAXNODES];
static int  top = -1;

static int  totalNodes = 0, leafNodes = 0;
static int  maxDepth   = 0;   /* 루트 레벨 = 0 이라고 두고, 도달한 최대 레벨 */
static int  maxDegree  = 0;

static int  foundC = 0, hasParentC = 0;
static char parentOfC = '\0';
static char childrenOfC[MAXNODES];
static int  numChildrenOfC = 0;

static void computeStats(void) {
    int level = 0;

    for (int i = 0; i < inputLen; i++) {
        char c = input[i];

        if (isupper((unsigned char)c)) {
            totalNodes++;
            if (level > maxDepth) maxDepth = level;

            if (top >= 0) {
                /* --- 노드 스택으로 부모 찾기 --- */
                if (c == 'C') {
                    parentOfC  = nodeStack[top];
                    hasParentC = 1;
                }
                /* --- 카운터 스택으로 C의 자식 목록 모으기 & 차수 갱신 --- */
                if (nodeStack[top] == 'C') {
                    childrenOfC[numChildrenOfC++] = c;
                }
                counterStack[top]++;
                if (counterStack[top] > maxDegree) maxDegree = counterStack[top];
            }

            if (c == 'C') foundC = 1;

            /* 바로 뒤에 '(' 가 없으면 단말 노드 */
            if (i + 1 >= inputLen || input[i + 1] != '(') {
                leafNodes++;
            }

        } else if (c == '(') {
            level++;
            top++;
            nodeStack[top]    = input[i - 1];  /* 바로 앞 글자가 이 괄호의 "부모" 노드 */
            counterStack[top] = 0;

        } else if (c == ')') {
            level--;
            top--;
        }
        /* ',' 는 별도 처리 불필요 */
    }
}

/* ------------------------------------------------------------------ */
/* 3단계. 왼쪽으로 눕힌 트리 출력                                         */
/*                                                                      */
/*   핵심 아이디어:                                                     */
/*   어떤 노드의 자손을 출력할 때, 그 노드의 "조상들" 각각이 자기 형제      */
/*   목록에서 마지막이었는지 아닌지에 따라 들여쓰기 칸에 '|' 를 그릴지     */
/*   공백을 그릴지가 결정된다. 그래서 단순히 "현재 깊이"라는 숫자 하나만  */
/*   가지고는 부족하고, 각 조상 레벨별로 "이 레벨은 계속 형제가 있어서    */
/*   막대를 이어야 하는가?"를 기억하는 문자열(스택처럼 누적되는 prefix)이 */
/*   필요하다.                                                          */
/*                                                                      */
/*   또한, 어떤 노드가 "마지막 형제"인지는 그 노드 뒤에 오는 문자(','     */
/*   인지 ')' 인지)를 봐야 알 수 있는데, 그 문자는 이 노드의 서브트리      */
/*   전체(중첩된 괄호 포함)를 건너뛴 다음에야 나온다. 그래서 한 글자씩만  */
/*   보는 단순 스캔으로는 부족하고, 괄호 짝을 맞춰 가며 "이 노드 표현이   */
/*   어디서 끝나는지" 미리 건너뛰어 보는(peek) 보조 함수가 필요하다.      */
/* ------------------------------------------------------------------ */

/* p 위치에서 시작하는 노드 하나(글자 + 있다면 (자식들))를 건너뛰고
   그 바로 다음 위치를 반환한다. 실제로 출력하거나 값을 바꾸지 않는
   "미리 보기" 전용 함수다. */
static int skipNode(int p) {
    p++;  /* 글자 하나 건너뜀 */
    if (p < inputLen && input[p] == '(') {
        int depth = 0;
        do {
            if (input[p] == '(') depth++;
            else if (input[p] == ')') depth--;
            p++;
        } while (depth > 0);
    }
    return p;
}

/* p 위치의 노드를 prefix를 붙여 한 줄 출력하고, 자식이 있으면 재귀적으로
   출력한다. 이 노드를 다 출력한 뒤의 위치를 반환한다. */
static int printNode(int p, const char *prefix) {
    /* 이 노드가 자기 형제들 중 마지막인지 미리 확인 (peek) */
    int endPos = skipNode(p);
    int isLast = (endPos >= inputLen || input[endPos] == ')');

    char label = input[p];
    printf("%s+---%c\n", prefix, label);
    p++;

    if (p < inputLen && input[p] == '(') {
        p++;  /* '(' 소비 */

        char childPrefix[1024];
        snprintf(childPrefix, sizeof(childPrefix), "%s%s",
                 prefix, isLast ? "    " : "|   ");

        while (1) {
            p = printNode(p, childPrefix);
            if (p < inputLen && input[p] == ',') { p++; continue; }
            break;
        }
        if (p < inputLen && input[p] == ')') p++;
    }
    return p;
}

static void printTree(void) {
    printf("%c\n", input[0]);  /* 루트는 접두어 없이 그대로 출력 */

    int p = 1;
    if (p < inputLen && input[p] == '(') {
        p++;
        while (1) {
            p = printNode(p, "");  /* 루트 자식들의 기본 prefix는 빈 문자열 */
            if (p < inputLen && input[p] == ',') { p++; continue; }
            break;
        }
        if (p < inputLen && input[p] == ')') p++;
    }
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */
int main(void) {
    printf("트리의 괄호 표기법을 입력하세요: ");
    if (!fgets(input, sizeof(input), stdin)) {
        printf("오류: 입력을 읽을 수 없습니다.\n");
        return 1;
    }

    int len = (int)strlen(input);
    while (len > 0 &&
           (input[len - 1] == '\n' || input[len - 1] == '\r' ||
            input[len - 1] == ' '  || input[len - 1] == '\t')) {
        input[--len] = '\0';
    }
    inputLen = len;

    if (!isValidTree()) {
        printf("오류: 올바른 트리의 괄호 표기법이 아닙니다.\n");
        return 1;
    }

    computeStats();

    printf("\n=== 트리 정보 ===\n");
    printf("전체 노드의 수   : %d\n", totalNodes);
    printf("단말 노드의 수   : %d\n", leafNodes);
    printf("비단말 노드의 수 : %d\n", totalNodes - leafNodes);
    printf("트리의 높이      : %d\n", maxDepth + 1);
    printf("트리의 차수      : %d\n", maxDegree);

    if (!foundC) {
        printf("노드 C           : 트리에 존재하지 않습니다.\n");
    } else {
        if (hasParentC)
            printf("노드 C의 부모 노드 : %c\n", parentOfC);
        else
            printf("노드 C의 부모 노드 : 없음 (C가 루트입니다)\n");

        printf("노드 C의 자식 노드 : ");
        if (numChildrenOfC == 0) {
            printf("없음 (단말 노드)\n");
        } else {
            for (int i = 0; i < numChildrenOfC; i++) {
                printf("%c", childrenOfC[i]);
                if (i < numChildrenOfC - 1) printf(", ");
            }
            printf("\n");
        }
    }

    printf("\n=== 트리 구조 (왼쪽으로 눕힌 형태) ===\n");
    printTree();

    return 0;
}
