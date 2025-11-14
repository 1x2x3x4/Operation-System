#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#define MAX_P 10      // 最大进程数
#define MAX_R 10      // 最大资源种类数

// 全局数据结构
int n, m;                                 // n 个进程，m 类资源
int Available[MAX_R];                     // 可用资源向量
int Max[MAX_P][MAX_R];                    // 最大需求矩阵
int Allocation[MAX_P][MAX_R];             // 已分配矩阵
int Need[MAX_P][MAX_R];                   // 需求矩阵

// 打印当前系统状态
void printSystemState() {
    int i, j;

    printf("\n========== 当前系统状态 ==========\n");

    printf("进程数 n = %d, 资源种类数 m = %d\n", n, m);

    printf("\nAvailable（可用资源向量）：\n");
    for (j = 0; j < m; j++) {
        printf("R%d:%d  ", j, Available[j]);
    }
    printf("\n");

    printf("\nMax（最大需求矩阵）：\n");
    for (i = 0; i < n; i++) {
        printf("P%d: ", i);
        for (j = 0; j < m; j++) {
            printf("%3d ", Max[i][j]);
        }
        printf("\n");
    }

    printf("\nAllocation（已分配矩阵）：\n");
    for (i = 0; i < n; i++) {
        printf("P%d: ", i);
        for (j = 0; j < m; j++) {
            printf("%3d ", Allocation[i][j]);
        }
        printf("\n");
    }

    printf("\nNeed（尚需矩阵）：\n");
    for (i = 0; i < n; i++) {
        printf("P%d: ", i);
        for (j = 0; j < m; j++) {
            printf("%3d ", Need[i][j]);
        }
        printf("\n");
    }

    printf("==================================\n\n");
}

// 重新计算 Need = Max - Allocation
void calculateNeed() {
    int i, j;
    for (i = 0; i < n; i++) {
        for (j = 0; j < m; j++) {
            Need[i][j] = Max[i][j] - Allocation[i][j];
        }
    }
}

/*
 * 安全性检查：判断当前系统状态是否安全
 * safeSeq[]：若安全，返回一个安全序列
 * 返回值：1 = 安全；0 = 不安全
 */
int isSafe(int safeSeq[]) {
    int work[MAX_R];
    int finish[MAX_P];
    int i, j, count = 0;

    // 初始化
    for (j = 0; j < m; j++) {
        work[j] = Available[j];   // 工作向量 work = Available
    }
    for (i = 0; i < n; i++) {
        finish[i] = 0;            // 所有进程初始都未完成
    }

    // 尝试找到一个安全序列
    while (count < n) {
        int found = 0;

        // 寻找一个满足：Finish[i] == 0 且 Need[i] <= work 的进程
        for (i = 0; i < n; i++) {
            if (finish[i] == 0) {
                int canFinish = 1;
                for (j = 0; j < m; j++) {
                    if (Need[i][j] > work[j]) {
                        canFinish = 0;
                        break;
                    }
                }

                if (canFinish) {
                    // 假定进程 i 能执行完并释放资源
                    for (j = 0; j < m; j++) {
                        work[j] += Allocation[i][j];
                    }
                    finish[i] = 1;
                    safeSeq[count] = i;
                    count++;
                    found = 1;
                }
            }
        }

        // 如果这一轮没有找到任何可以完成的进程，说明系统不安全
        if (!found) {
            break;
        }
    }

    // 检查是否所有进程都可以完成
    for (i = 0; i < n; i++) {
        if (finish[i] == 0) {
            return 0;   // 存在无法完成的进程，系统不安全
        }
    }

    return 1;           // 所有进程均可完成，系统安全
}

/*
 * 处理一次资源请求 Request
 * 进程号 p，长度 m 的 Request 向量
 */
void handleRequest() {
    int p;
    int Request[MAX_R];
    int i;

    printf("请输入发出请求的进程号 p（0 ~ %d，输入负数结束程序）：", n - 1);
    scanf("%d", &p);
    if (p < 0) {
        // 用负数退出
        printf("结束请求处理。\n");
        return;
    }
    if (p >= n) {
        printf("进程号非法。\n");
        return;
    }

    printf("请输入进程 P%d 的请求向量 Request（共 %d 类资源）：\n", p, m);
    for (i = 0; i < m; i++) {
        printf("Request[%d] = ", i);
        scanf("%d", &Request[i]);
    }

    // 1) 检查 Request <= Need
    for (i = 0; i < m; i++) {
        if (Request[i] > Need[p][i]) {
            printf("请求非法：Request[%d] = %d > Need[P%d][%d] = %d\n",
                i, Request[i], p, i, Need[p][i]);
            printf("原因：进程请求的资源数量超过其最大剩余需求，直接拒绝。\n\n");
            return;
        }
    }

    // 2) 检查 Request <= Available
    for (i = 0; i < m; i++) {
        if (Request[i] > Available[i]) {
            printf("请求暂时不能满足：Request[%d] = %d > Available[%d] = %d\n",
                i, Request[i], i, Available[i]);
            printf("原因：系统当前可用资源不足，暂不分配。\n\n");
            return;
        }
    }

    // 3) 模拟分配：修改 Available、Allocation、Need
    printf("请求合法，进入模拟分配阶段...\n");

    for (i = 0; i < m; i++) {
        Available[i] -= Request[i];
        Allocation[p][i] += Request[i];
        Need[p][i] -= Request[i];
    }

    // 4) 检查模拟分配后的系统状态是否安全
    int safeSeq[MAX_P];
    if (isSafe(safeSeq)) {
        int k;
        printf("系统处于安全状态，请求可以被满足。\n");
        printf("一种安全序列为：");
        for (k = 0; k < n; k++) {
            printf("P%d", safeSeq[k]);
            if (k != n - 1) printf(" -> ");
        }
        printf("\n\n");

        printSystemState();
    }
    else {
        int k;
        printf("警告：若按该请求分配资源，系统将进入不安全状态，请求被拒绝！\n");

        // 回滚：撤销刚才的模拟分配
        for (i = 0; i < m; i++) {
            Available[i] += Request[i];
            Allocation[p][i] -= Request[i];
            Need[p][i] += Request[i];
        }

        printf("系统状态已回滚到请求前：\n");
        printSystemState();
    }
}

int main() {
    int i, j;

    printf("=========== 银行家算法模拟 ===========\n");
    printf("请输入进程数 n（<= %d）：", MAX_P);
    scanf("%d", &n);
    if (n <= 0 || n > MAX_P) {
        printf("进程数非法。\n");
        return 1;
    }

    printf("请输入资源种类数 m（<= %d）：", MAX_R);
    scanf("%d", &m);
    if (m <= 0 || m > MAX_R) {
        printf("资源种类数非法。\n");
        return 1;
    }

    // 输入 Available
    printf("\n请输入各类资源的初始可用数量 Available（共 %d 类资源）：\n", m);
    for (j = 0; j < m; j++) {
        printf("Available[%d] = ", j);
        scanf("%d", &Available[j]);
    }

    // 输入 Max
    printf("\n请输入 Max 矩阵（每个进程对各类资源的最大需求）：\n");
    for (i = 0; i < n; i++) {
        printf("进程 P%d 的最大需求：\n", i);
        for (j = 0; j < m; j++) {
            printf("Max[%d][%d] = ", i, j);
            scanf("%d", &Max[i][j]);
        }
    }

    // 输入 Allocation
    printf("\n请输入 Allocation 矩阵（当前已分配给各进程的资源数量）：\n");
    for (i = 0; i < n; i++) {
        printf("进程 P%d 的已分配资源：\n", i);
        for (j = 0; j < m; j++) {
            printf("Allocation[%d][%d] = ", i, j);
            scanf("%d", &Allocation[i][j]);
        }
    }

    // 根据 Max 和 Allocation 计算 Need
    calculateNeed();

    // 初始状态打印一次
    printSystemState();

    // 检查初始状态是否安全
    {
        int safeSeq[MAX_P];
        if (isSafe(safeSeq)) {
            int k;
            printf("初始系统状态是安全的。\n");
            printf("一种安全序列为：");
            for (k = 0; k < n; k++) {
                printf("P%d", safeSeq[k]);
                if (k != n - 1) printf(" -> ");
            }
            printf("\n\n");
        }
        else {
            printf("注意：初始系统状态已经不安全！\n\n");
        }
    }

    // 循环处理多次请求
    while (1) {
        int choice;
        printf("========== 菜单 ==========\n");
        printf("1. 发出一次资源请求\n");
        printf("2. 打印当前系统状态\n");
        printf("0. 退出程序\n");
        printf("请输入你的选择：");
        scanf("%d", &choice);

        if (choice == 1) {
            handleRequest();
        }
        else if (choice == 2) {
            printSystemState();
        }
        else if (choice == 0) {
            printf("程序结束，再见。\n");
            break;
        }
        else {
            printf("无效选项，请重试。\n");
        }
    }

    return 0;
}
