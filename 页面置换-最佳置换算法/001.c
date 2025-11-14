#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#define MAX_REF_LEN   100     // 页引用串最大长度
#define MAX_FRAMES    10      // 物理块最大个数
#define MAX_VPAGES    100     // 虚拟页最大数量（页表大小）

/* 页表项结构 */
typedef struct {
    int frame_no;   // 所在物理块号
    int valid;      // 有效位：1=在内存中，0=不在
} PageTableEntry;

/* 在物理块中查找某页，找到返回帧号下标，没找到返回 -1 */
int findInFrames(int frames[], int frame_count, int page) {
    for (int i = 0; i < frame_count; i++) {
        if (frames[i] == page) {
            return i;
        }
    }
    return -1;
}

/* 打印当前物理块内容 */
void printFrames(int frames[], int frame_count) {
    printf("当前物理块：");
    for (int i = 0; i < frame_count; i++) {
        if (frames[i] == -1)
            printf("[ ] ");
        else
            printf("[%d] ", frames[i]);
    }
    printf("\n");
}

/*
 * OPT 预测函数：
 * ref_str     ：页引用串
 * ref_len     ：引用串总长度
 * frames      ：当前物理块中存的页号
 * frame_count ：物理块数
 * current_idx ：当前访问的引用串下标
 *
 * 返回：应该被替换的物理块下标
 */
int predictOPT(int ref_str[], int ref_len,
    int frames[], int frame_count,
    int current_idx) {
    int farthest = current_idx;   // 记录“最远下次使用位置”的下标
    int victim_index = -1;        // 要被置换的块号下标

    for (int i = 0; i < frame_count; i++) {
        int page = frames[i];
        int j;
        int found_in_future = 0;

        // 在未来的引用中查找该页
        for (j = current_idx + 1; j < ref_len; j++) {
            if (ref_str[j] == page) {
                found_in_future = 1;
                // 距离越远越“好”作为牺牲品
                if (j > farthest) {
                    farthest = j;
                    victim_index = i;
                }
                break;
            }
        }

        // 如果该页以后再也不会被访问，是最好的牺牲品
        if (!found_in_future) {
            return i;
        }
    }

    // 如果所有页都还会被访问，但 victim_index 仍然没被更新，
    // 那就随便选一个（这里选 0）（理论上极少出现）
    if (victim_index == -1) {
        victim_index = 0;
    }

    return victim_index;
}

/* 模拟逻辑地址到物理地址的转换 */
void translateAddress(PageTableEntry page_table[], int page_size) {
    int logical_addr;

    printf("\n===== 地址转换演示 =====\n");
    printf("输入一个逻辑地址（十进制，负数结束）：");

    while (scanf("%d", &logical_addr) == 1 && logical_addr >= 0) {
        int page = logical_addr / page_size;   // 页号
        int offset = logical_addr % page_size; // 页内偏移

        printf("逻辑地址 %d => 页号=%d, 偏移=%d\n", logical_addr, page, offset);

        if (page < 0 || page >= MAX_VPAGES) {
            printf("该页号超出页表范围，无法转换。\n");
        }
        else if (!page_table[page].valid) {
            printf("页 %d 不在内存中（页表有效位=0），发生缺页。\n", page);
        }
        else {
            int frame_no = page_table[page].frame_no;
            int physical_addr = frame_no * page_size + offset;
            printf("页表：page %d -> frame %d\n", page, frame_no);
            printf("物理地址 = 帧号 * 页大小 + 偏移 = %d * %d + %d = %d\n",
                frame_no, page_size, offset, physical_addr);
        }

        printf("\n继续输入逻辑地址（负数结束）：");
    }

    printf("\n地址转换演示结束。\n");
}

int main() {
    int ref_len;                   // 页引用串长度
    int ref_str[MAX_REF_LEN];      // 页引用串
    int frame_count;               // 物理块个数
    int frames[MAX_FRAMES];        // 物理块中的页号
    PageTableEntry page_table[MAX_VPAGES]; // 页表
    int page_size = 1024;          // 页大小（字节），可根据需要修改

    // 初始化页表
    for (int i = 0; i < MAX_VPAGES; i++) {
        page_table[i].valid = 0;
        page_table[i].frame_no = -1;
    }

    // 输入
    printf("===== OPT 页面置换算法模拟 =====\n");
    printf("请输入页引用串长度（<=%d）：", MAX_REF_LEN);
    scanf("%d", &ref_len);

    if (ref_len <= 0 || ref_len > MAX_REF_LEN) {
        printf("引用串长度非法。\n");
        return 1;
    }

    printf("请输入页引用串（依次输入页号，用空格分隔）：\n");
    for (int i = 0; i < ref_len; i++) {
        scanf("%d", &ref_str[i]);
        if (ref_str[i] < 0 || ref_str[i] >= MAX_VPAGES) {
            printf("页号 %d 超出支持范围 [0, %d)，可修改 MAX_VPAGES 后重试。\n",
                ref_str[i], MAX_VPAGES);
            return 1;
        }
    }

    printf("请输入物理块个数（<=%d）：", MAX_FRAMES);
    scanf("%d", &frame_count);

    if (frame_count <= 0 || frame_count > MAX_FRAMES) {
        printf("物理块个数非法。\n");
        return 1;
    }

    // 初始化物理块为空
    for (int i = 0; i < frame_count; i++) {
        frames[i] = -1;  // -1 表示该帧为空
    }

    int page_faults = 0;  // 缺页次数
    int used_frames = 0;  // 已经占用的物理块数量

    printf("\n开始模拟 OPT 页面置换过程...\n\n");

    for (int i = 0; i < ref_len; i++) {
        int page = ref_str[i];
        printf("访问第 %d 次：页 %d\n", i + 1, page);

        // 1. 查找该页是否已经在物理块中
        int frame_index = findInFrames(frames, frame_count, page);

        if (frame_index != -1) {
            // 命中
            printf("-> 页 %d 已在物理块 %d 中，命中。\n", page, frame_index);
        }
        else {
            // 缺页
            page_faults++;
            printf("-> 页 %d 不在内存，发生缺页。\n", page);

            // 2. 如果还有空闲物理块，直接装入
            if (used_frames < frame_count) {
                frames[used_frames] = page;
                page_table[page].valid = 1;
                page_table[page].frame_no = used_frames;

                printf("   使用空闲物理块 %d 装入页 %d。\n", used_frames, page);
                used_frames++;
            }
            else {
                // 3. 没有空闲物理块，使用 OPT 算法选择一个牺牲页
                int victim = predictOPT(ref_str, ref_len, frames, frame_count, i);
                int victim_page = frames[victim];

                printf("   使用 OPT 算法选择牺牲页：页 %d（在物理块 %d）。\n",
                    victim_page, victim);

                // 更新页表：原页失效
                page_table[victim_page].valid = 0;
                page_table[victim_page].frame_no = -1;

                // 新页装入
                frames[victim] = page;
                page_table[page].valid = 1;
                page_table[page].frame_no = victim;

                printf("   替换后：物理块 %d 中装入页 %d。\n", victim, page);
            }
        }

        printFrames(frames, frame_count);
        printf("----------------------------------------\n");
    }

    printf("\n===== 模拟结束 =====\n");
    printf("总访问次数：%d\n", ref_len);
    printf("缺页次数  ：%d\n", page_faults);
    printf("缺页率    ：%.2f%%\n", (page_faults * 100.0) / ref_len);

    // 地址转换演示
    translateAddress(page_table, page_size);

    return 0;
}
