#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
    int i;

    // ============ 测试1：不设置种子（默认种子=1）============
    printf("=== 测试1：不调用srand()（默认种子=1）===\n");
    printf("第一次生成5个随机数：");
    for (i = 0; i < 5; i++) {
        printf("%d ", rand() % 100);
    }
    printf("\n");

    // 重新设置种子为1，再生成一次
    srand(1);  // 等同于程序刚启动时的默认状态
    printf("srand(1)后再生成5个：");
    for (i = 0; i < 5; i++) {
        printf("%d ", rand() % 100);
    }
    printf("\n");
    // 你会发现：两次输出完全一样！

    // ============ 测试2：使用固定种子 ============
    printf("\n=== 测试2：使用固定种子 srand(12345) ===\n");
    srand(12345);
    printf("第一次：");
    for (i = 0; i < 5; i++) {
        printf("%d ", rand() % 100);
    }
    printf("\n");

    srand(12345);  // 重新设置相同种子
    printf("第二次：");
    for (i = 0; i < 5; i++) {
        printf("%d ", rand() % 100);
    }
    printf("\n");
    // 两次输出也完全一样！

    // ============ 测试3：使用时间作为种子 ============
    printf("\n=== 测试3：使用 srand(time(NULL)) ===\n");
    printf("每次运行程序，下面的数字都会不同：\n");
    srand((unsigned int)time(NULL));  // 用当前时间作为种子
    printf("随机数：");
    for (i = 0; i < 5; i++) {
        printf("%d ", rand() % 100);
    }
    printf("\n");

    return 0;
}