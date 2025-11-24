#include <stdio.h>
#include <stdlib.h>

#define MAX_PAGES 20      // 最大页面数
#define MAX_FRAMES 10     // 最大物理块数
#define PAGE_SIZE 1024    // 页面大小
#define MEMORY_SIZE 4096  // 内存大小

// 用int代替bool
#define TRUE 1
#define FALSE 0

// 页表项结构
typedef struct {
    int frame_number;     // 物理块号
    int valid;           // 有效位
    int modified;        // 修改位
    int time_loaded;      // 装入时间（用于FIFO）
} PageTableEntry;

// 物理块结构
typedef struct {
    int page_number;      // 存放的页面号
    int occupied;        // 是否被占用
    int load_time;        // 装入时间
} Frame;

// 全局变量
PageTableEntry page_table[MAX_PAGES];  // 页表
Frame physical_memory[MAX_FRAMES];     // 物理内存
int page_fault_count = 0;              // 缺页次数
int memory_access_count = 0;           // 内存访问次数
int current_time = 0;                  // 当前时间

// 函数声明
void initialize_system();
void print_page_table();
void print_physical_memory();
int logical_to_physical(int logical_address);
void handle_page_fault(int page_number);
int find_victim_page();
void simulate_memory_access();

int main() {
    printf("========== FIFO页面置换算法模拟系统 ==========\n\n");

    // 初始化系统
    initialize_system();

    // 显示初始状态
    printf("系统初始化完成:\n");
    printf("页面大小: %d bytes\n", PAGE_SIZE);
    printf("物理内存: %d bytes (%d个物理块)\n", MEMORY_SIZE, MEMORY_SIZE / PAGE_SIZE);
    printf("逻辑地址空间: %d pages\n\n", MAX_PAGES);

    // 模拟内存访问
    simulate_memory_access();

    printf("按任意键退出...");
    getchar();
    return 0;
}

// 初始化系统
void initialize_system() {
    int i;

    // 初始化页表
    for (i = 0; i < MAX_PAGES; i++) {
        page_table[i].frame_number = -1;  // 未分配物理块
        page_table[i].valid = FALSE;      // 无效
        page_table[i].modified = FALSE;   // 未修改
        page_table[i].time_loaded = -1;   // 未装入
    }

    // 初始化物理内存
    for (i = 0; i < MAX_FRAMES; i++) {
        physical_memory[i].page_number = -1;  // 空闲
        physical_memory[i].occupied = FALSE;  // 未占用
        physical_memory[i].load_time = -1;    // 未装入
    }
}

// 打印页表
void print_page_table() {
    int i;
    printf("当前页表状态:\n");
    printf("页号\t块号\t有效位\t修改位\t装入时间\n");
    printf("----\t----\t----\t----\t--------\n");

    for (i = 0; i < MAX_PAGES; i++) {
        printf("%2d\t", i);
        if (page_table[i].valid) {
            printf("%2d\t", page_table[i].frame_number);
            printf("%s\t", page_table[i].valid ? "是" : "否");
            printf("%s\t", page_table[i].modified ? "是" : "否");
            printf("%4d\n", page_table[i].time_loaded);
        }
        else {
            printf("无\t否\t否\t  无\n");
        }
    }
    printf("\n");
}

// 打印物理内存
void print_physical_memory() {
    int i;
    printf("当前物理内存状态:\n");
    printf("块号\t页号\t占用\t装入时间\n");
    printf("----\t----\t----\t--------\n");

    for (i = 0; i < MAX_FRAMES; i++) {
        printf("%2d\t", i);
        if (physical_memory[i].occupied) {
            printf("%2d\t", physical_memory[i].page_number);
            printf("是\t");
            printf("%4d\n", physical_memory[i].load_time);
        }
        else {
            printf("无\t否\t  无\n");
        }
    }
    printf("\n");
}

// 逻辑地址到物理地址的转换
int logical_to_physical(int logical_address) {
    int page_number, offset, frame_number, physical_address;

    memory_access_count++;
    current_time++;

    page_number = logical_address / PAGE_SIZE;
    offset = logical_address % PAGE_SIZE;

    printf("访问逻辑地址: %d (页号: %d, 页内偏移: %d)\n",
        logical_address, page_number, offset);

    // 检查页号是否有效
    if (page_number >= MAX_PAGES) {
        printf("错误: 页号 %d 超出范围!\n", page_number);
        return -1;
    }

    // 检查页面是否在内存中
    if (!page_table[page_number].valid) {
        printf("发生缺页中断! 页面 %d 不在内存中\n", page_number);
        handle_page_fault(page_number);
        page_fault_count++;
    }
    else {
        printf("页面命中! 页面 %d 在物理块 %d 中\n",
            page_number, page_table[page_number].frame_number);
    }

    // 计算物理地址
    frame_number = page_table[page_number].frame_number;
    physical_address = frame_number * PAGE_SIZE + offset;

    printf("物理地址: %d (块号: %d, 块内偏移: %d)\n\n",
        physical_address, frame_number, offset);

    return physical_address;
}

// 处理缺页中断
void handle_page_fault(int page_number) {
    int free_frame = -1;
    int i;
    int victim_page;

    printf("正在处理页面 %d 的缺页...\n", page_number);

    // 查找空闲物理块
    free_frame = -1;
    for (i = 0; i < MAX_FRAMES; i++) {
        if (!physical_memory[i].occupied) {
            free_frame = i;
            break;
        }
    }

    if (free_frame != -1) {
        // 有空闲块，直接分配
        printf("找到空闲物理块 %d，分配页面 %d\n", free_frame, page_number);
    }
    else {
        // 没有空闲块，使用FIFO选择置换页面
        free_frame = find_victim_page();
        victim_page = physical_memory[free_frame].page_number;

        printf("使用FIFO算法置换: 页面 %d (块 %d) -> 页面 %d\n",
            victim_page, free_frame, page_number);

        // 更新被置换页面的页表项
        if (page_table[victim_page].modified) {
            printf("页面 %d 被修改过，需要写回磁盘\n", victim_page);
        }
        page_table[victim_page].valid = FALSE;
        page_table[victim_page].frame_number = -1;
    }

    // 装入新页面
    physical_memory[free_frame].page_number = page_number;
    physical_memory[free_frame].occupied = TRUE;
    physical_memory[free_frame].load_time = current_time;

    // 更新页表
    page_table[page_number].frame_number = free_frame;
    page_table[page_number].valid = TRUE;
    page_table[page_number].modified = FALSE;  // 假设新装入的页面未被修改
    page_table[page_number].time_loaded = current_time;

    printf("页面 %d 已装入物理块 %d\n", page_number, free_frame);
}

// 使用FIFO算法选择被置换的页面
int find_victim_page() {
    int oldest_time = current_time + 1;
    int victim_frame = -1;
    int i;

    // 查找最早装入的页面
    for (i = 0; i < MAX_FRAMES; i++) {
        if (physical_memory[i].occupied &&
            physical_memory[i].load_time < oldest_time) {
            oldest_time = physical_memory[i].load_time;
            victim_frame = i;
        }
    }

    return victim_frame;
}

// 模拟内存访问序列
void simulate_memory_access() {
    int access_sequence[] = {
        0,   512, 1024, 1536,  // 页面0,0,1,1
        2048, 2560, 3072,      // 页面2,2,3
        1024, 512, 4096,       // 页面1,0,4 (缺页)
        5120, 6144, 7168,      // 页面5,6,7 (连续缺页)
        1024, 2048, 3072       // 页面1,2,3
    };

    int sequence_length = sizeof(access_sequence) / sizeof(access_sequence[0]);
    int i;
    int used_frames;

    printf("开始模拟内存访问序列...\n\n");

    // 预装入一些页面
    printf("=== 预装入阶段 ===\n");
    int initial_pages[] = { 0, 1, 2, 3 };
    for (i = 0; i < 4; i++) {
        int logical_addr = initial_pages[i] * PAGE_SIZE;
        logical_to_physical(logical_addr);
    }

    printf("=== 预装入完成 ===\n");
    print_page_table();
    print_physical_memory();

    // 模拟访问序列
    printf("=== 内存访问阶段 ===\n");

    for (i = 0; i < sequence_length; i++) {
        printf("步骤 %d: ", i + 1);
        logical_to_physical(access_sequence[i]);

        // 每4次访问显示一次状态
        if ((i + 1) % 4 == 0) {
            print_page_table();
            print_physical_memory();
        }
    }

    // 显示最终结果
    printf("=== 模拟完成 ===\n");
    print_page_table();
    print_physical_memory();

    // 统计信息
    printf("=== 性能统计 ===\n");
    printf("总内存访问次数: %d\n", memory_access_count);
    printf("缺页次数: %d\n", page_fault_count);
    printf("缺页率: %.2f%%\n", (float)page_fault_count / memory_access_count * 100);

    // 计算内存利用率
    used_frames = 0;
    for (i = 0; i < MAX_FRAMES; i++) {
        if (physical_memory[i].occupied) {
            used_frames++;
        }
    }
    printf("内存利用率: %.2f%%\n", (float)used_frames / MAX_FRAMES * 100);
}
