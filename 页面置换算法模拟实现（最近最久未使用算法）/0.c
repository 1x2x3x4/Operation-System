#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>

#define MAX_PAGES   1024   // 最大页数（根据需要可调大）
#define MAX_FRAMES  32     // 最大物理帧数
#define MAX_REF     1000   // 最大访问次数

typedef struct {
    int frame;      // 该页所在的物理帧号
    int valid;      // 有效位：1=在内存，0=不在
    int last_used;  // 最近一次访问的时间戳
} PageTableEntry;

PageTableEntry page_table[MAX_PAGES];
int phys_mem[MAX_FRAMES];      // phys_mem[frame] = page_no（-1 表示该帧空闲）

int frame_count;               // 实际使用的帧数
int page_size;                 // 页面大小（字节）
int time_counter = 0;          // 全局“时间”，每次访问+1

// 查找空闲帧，返回帧号；若无空闲帧，返回 -1
int find_free_frame() {
    for (int i = 0; i < frame_count; i++) {
        if (phys_mem[i] == -1) {
            return i;
        }
    }
    return -1;
}

// LRU：在所有 valid==1 的页中，找 last_used 最小的页号
int find_victim_lru(int max_page_index) {
    int victim_page = -1;
    int min_time = 0x7fffffff;

    for (int p = 0; p <= max_page_index && p < MAX_PAGES; p++) {
        if (page_table[p].valid == 1) {
            if (page_table[p].last_used < min_time) {
                min_time = page_table[p].last_used;
                victim_page = p;
            }
        }
    }
    return victim_page;
}

// 打印当前物理内存中各个帧的内容
void print_frames() {
    printf("  物理内存帧: ");
    for (int i = 0; i < frame_count; i++) {
        if (phys_mem[i] == -1) {
            printf("[  ] ");
        }
        else {
            printf("[%2d] ", phys_mem[i]);
        }
    }
    printf("\n");
}

int main() {
    int ref_count;
    int logical_addrs[MAX_REF];

    // 初始化页表和物理内存
    for (int i = 0; i < MAX_PAGES; i++) {
        page_table[i].frame = -1;
        page_table[i].valid = 0;
        page_table[i].last_used = 0;
    }
    for (int i = 0; i < MAX_FRAMES; i++) {
        phys_mem[i] = -1;   // -1 表示该帧为空
    }

    printf("===== LRU 页面置换算法模拟 =====\n");
    printf("请输入页面大小（字节）：");
    if (scanf("%d", &page_size) != 1 || page_size <= 0) {
        printf("页面大小输入错误！\n");
        return 1;
    }

    printf("请输入物理帧数（<= %d）：", MAX_FRAMES);
    if (scanf("%d", &frame_count) != 1 || frame_count <= 0 || frame_count > MAX_FRAMES) {
        printf("物理帧数输入错误！\n");
        return 1;
    }

    printf("请输入要访问的逻辑地址个数（<= %d）：", MAX_REF);
    if (scanf("%d", &ref_count) != 1 || ref_count <= 0 || ref_count > MAX_REF) {
        printf("访问次数输入错误！\n");
        return 1;
    }

    printf("请依次输入每个逻辑地址（以空格或换行分隔）：\n");
    for (int i = 0; i < ref_count; i++) {
        if (scanf("%d", &logical_addrs[i]) != 1) {
            printf("逻辑地址输入错误！\n");
            return 1;
        }
        if (logical_addrs[i] < 0) {
            printf("逻辑地址不能为负数！\n");
            return 1;
        }
    }

    int page_faults = 0;
    int hits = 0;
    int max_page_index_seen = 0;  // 记录访问过的最大页号，用于加速 LRU 搜索

    printf("\n===== 开始模拟 =====\n\n");
    printf("步次 | 逻辑地址 | 页号 | 偏移量 | 结果    | 被淘汰页 | 物理地址\n");
    printf("---------------------------------------------------------------\n");

    for (int i = 0; i < ref_count; i++) {
        int logical_addr = logical_addrs[i];
        int page = logical_addr / page_size;
        int offset = logical_addr % page_size;

        if (page >= MAX_PAGES) {
            printf("访问的页号 %d 超出 MAX_PAGES=%d，程序终止。\n", page, MAX_PAGES);
            break;
        }
        if (page > max_page_index_seen) {
            max_page_index_seen = page;
        }

        time_counter++;  // 模拟时间流逝
        int frame = -1;
        int victim_page = -1;
        int is_hit = 0;

        if (page_table[page].valid == 1) {
            // 命中
            is_hit = 1;
            hits++;
            frame = page_table[page].frame;
            page_table[page].last_used = time_counter;
        }
        else {
            // 缺页
            page_faults++;
            int free_frame = find_free_frame();
            if (free_frame != -1) {
                // 有空闲帧，直接装入
                frame = free_frame;
                victim_page = -1; // 没有被淘汰页
            }
            else {
                // 没有空闲帧，需要置换
                victim_page = find_victim_lru(max_page_index_seen);
                if (victim_page == -1) {
                    printf("内部错误：未找到可置换的页！\n");
                    return 1;
                }
                frame = page_table[victim_page].frame;
                // 淘汰 victim_page
                page_table[victim_page].valid = 0;
                page_table[victim_page].frame = -1;
                page_table[victim_page].last_used = 0;
            }

            // 装入新页
            page_table[page].valid = 1;
            page_table[page].frame = frame;
            page_table[page].last_used = time_counter;
            phys_mem[frame] = page;
        }

        int phys_addr = frame * page_size + offset;

        // 输出本次访问信息
        printf("%3d  | %8d | %3d | %6d | ", i + 1, logical_addr, page, offset);
        if (is_hit) {
            printf("命中   |   --    | %10d\n", phys_addr);
        }
        else {
            if (victim_page == -1) {
                printf("缺页   |  空闲帧 | %10d\n", phys_addr);
            }
            else {
                printf("缺页   | %7d | %10d\n", victim_page, phys_addr);
            }
        }

        // 打印当前物理内存帧情况
        print_frames();
        printf("\n");
    }

    printf("===== 模拟结束 =====\n");
    printf("总访问次数: %d\n", ref_count);
    printf("命中次数  : %d\n", hits);
    printf("缺页次数  : %d\n", page_faults);
    double hit_rate = ref_count > 0 ? (double)hits / ref_count : 0.0;
    double miss_rate = ref_count > 0 ? (double)page_faults / ref_count : 0.0;
    printf("命中率    : %.4f\n", hit_rate);
    printf("缺页率    : %.4f\n", miss_rate);

    return 0;
}
