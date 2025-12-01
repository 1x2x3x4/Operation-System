#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>

#define MAX_PROCESSES 100

typedef struct {
    int pid;                // 进程 ID
    int arrival_time;       // 到达时间
    int service_time;       // 服务时间（CPU 总需求）
} ProcessBase;

typedef struct {
    int pid;
    int arrival_time;
    int service_time;
    int remaining_time;
    int start_time;         // 第一次开始执行的时间
    int finish_time;        // 完成时间
    double turnaround_time;         // 周转时间 = 完成 - 到达
    double weighted_turnaround_time; // 带权周转时间 = 周转 / 服务
} ProcessRun;

// 按到达时间排序，方便后面按时间推进
void sort_by_arrival(ProcessBase p[], int n) {
    for (int i = 0; i < n - 1; ++i) {
        for (int j = 0; j < n - 1 - i; ++j) {
            if (p[j].arrival_time > p[j + 1].arrival_time) {
                ProcessBase tmp = p[j];
                p[j] = p[j + 1];
                p[j + 1] = tmp;
            }
        }
    }
}

// 就绪队列（循环队列实现）
typedef struct {
    int data[MAX_PROCESSES];
    int front;
    int rear;
    int count;
} Queue;

void init_queue(Queue* q) {
    q->front = 0;
    q->rear = 0;
    q->count = 0;
}

int is_empty(Queue* q) {
    return q->count == 0;
}

void enqueue(Queue* q, int x) {
    if (q->count == MAX_PROCESSES) {
        printf("队列已满，无法入队\n");
        return;
    }
    q->data[q->rear] = x;
    q->rear = (q->rear + 1) % MAX_PROCESSES;
    q->count++;
}

int dequeue(Queue* q) {
    if (is_empty(q)) {
        printf("队列为空，无法出队\n");
        return -1;
    }
    int x = q->data[q->front];
    q->front = (q->front + 1) % MAX_PROCESSES;
    q->count--;
    return x;
}

// 核心：对给定时间片 time_quantum 做一次完整调度，并输出结果
void simulate_rr(ProcessBase base[], int n, int time_quantum) {
    // 复制一份运行时数据，避免多次实验相互影响
    ProcessRun proc[MAX_PROCESSES];
    for (int i = 0; i < n; ++i) {
        proc[i].pid = base[i].pid;
        proc[i].arrival_time = base[i].arrival_time;
        proc[i].service_time = base[i].service_time;
        proc[i].remaining_time = base[i].service_time;
        proc[i].start_time = -1;
        proc[i].finish_time = -1;
        proc[i].turnaround_time = 0.0;
        proc[i].weighted_turnaround_time = 0.0;
    }

    int current_time = 0;
    int completed = 0;
    int next_arrival_index = 0;   // 指向下一个还没进入系统的进程

    Queue ready;
    init_queue(&ready);

    double sum_turnaround = 0.0;
    double sum_weighted_turnaround = 0.0;

    while (completed < n) {
        // 把所有到达时间 <= 当前时间的进程放入就绪队列
        while (next_arrival_index < n &&
            proc[next_arrival_index].arrival_time <= current_time) {
            enqueue(&ready, next_arrival_index);
            next_arrival_index++;
        }

        // 如果就绪队列为空，但还有进程没到达，直接跳到下一个进程到达的时间
        if (is_empty(&ready)) {
            if (next_arrival_index < n) {
                current_time = proc[next_arrival_index].arrival_time;
                // 把这个到达的进程入队（以及同一时刻到达的）
                while (next_arrival_index < n &&
                    proc[next_arrival_index].arrival_time <= current_time) {
                    enqueue(&ready, next_arrival_index);
                    next_arrival_index++;
                }
            }
            else {
                // 理论上不会走到这里：没有就绪，没有未到达，但没完成？
                break;
            }
        }

        int idx = dequeue(&ready);
        ProcessRun* p = &proc[idx];

        // 第一次执行时记录开始时间
        if (p->start_time == -1) {
            p->start_time = current_time;
        }

        // 本次实际执行时间
        int exec_time = (p->remaining_time <= time_quantum)
            ? p->remaining_time
            : time_quantum;

        current_time += exec_time;
        p->remaining_time -= exec_time;

        // 执行过程中，可能又有新进程到达了，把他们加入就绪队列
        while (next_arrival_index < n &&
            proc[next_arrival_index].arrival_time <= current_time) {
            enqueue(&ready, next_arrival_index);
            next_arrival_index++;
        }

        if (p->remaining_time == 0) {
            // 该进程完成
            p->finish_time = current_time;
            p->turnaround_time = p->finish_time - p->arrival_time;
            p->weighted_turnaround_time = p->turnaround_time / p->service_time;

            sum_turnaround += p->turnaround_time;
            sum_weighted_turnaround += p->weighted_turnaround_time;
            completed++;
        }
        else {
            // 没完成，重新入队
            enqueue(&ready, idx);
        }
    }

    // 输出结果表
    printf("\n=============================\n");
    printf("  时间片大小 = %d\n", time_quantum);
    printf("=============================\n");
    printf("PID\t到达\t服务\t开始\t完成\t周转\t带权周转\n");
    for (int i = 0; i < n; ++i) {
        printf("%d\t%d\t%d\t%d\t%d\t%.1f\t%.2f\n",
            proc[i].pid,
            proc[i].arrival_time,
            proc[i].service_time,
            proc[i].start_time,
            proc[i].finish_time,
            proc[i].turnaround_time,
            proc[i].weighted_turnaround_time);
    }

    printf("---------------------------------------------\n");
    printf("平均周转时间 = %.2f\n", sum_turnaround / n);
    printf("平均带权周转时间 = %.2f\n", sum_weighted_turnaround / n);
}

int main() {
    int n;
    ProcessBase base[MAX_PROCESSES];

    printf("请输入进程数量 n (<= %d): ", MAX_PROCESSES);
    if (scanf("%d", &n) != 1 || n <= 0 || n > MAX_PROCESSES) {
        printf("进程数量非法\n");
        return 1;
    }

    printf("按顺序输入每个进程的信息：PID 到达时间 服务时间\n");
    printf("(例如: 1 0 5 表示 PID=1, t=0 到达, 需要 5 个时间单位)\n");

    for (int i = 0; i < n; ++i) {
        printf("进程 %d: ", i + 1);
        if (scanf("%d %d %d",
            &base[i].pid,
            &base[i].arrival_time,
            &base[i].service_time) != 3) {
            printf("输入错误\n");
            return 1;
        }
        if (base[i].service_time <= 0) {
            printf("服务时间必须 > 0\n");
            return 1;
        }
    }

    // 为了应对“没有按到达时间排序输入”的情况，这里按到达时间排序
    sort_by_arrival(base, n);

    int m;
    printf("\n准备测试不同时间片大小。\n");
    printf("请输入要测试的时间片个数 m: ");
    if (scanf("%d", &m) != 1 || m <= 0) {
        printf("m 非法\n");
        return 1;
    }

    for (int i = 0; i < m; ++i) {
        int tq;
        printf("请输入第 %d 个时间片大小: ", i + 1);
        if (scanf("%d", &tq) != 1 || tq <= 0) {
            printf("时间片必须为正整数\n");
            return 1;
        }
        simulate_rr(base, n, tq);
    }

    return 0;
}
