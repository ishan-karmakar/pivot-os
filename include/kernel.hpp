#pragma once

#define HUGEPAGE_SIZE 0x200000
#define PAGE_SIZE 0x1000
#define KERNEL_STACK_SIZE (PAGE_SIZE * 4)
#define DIV_CEIL(num, dividend) (((num) + ((dividend) - 1)) / (dividend))
#define DIV_FLOOR(num, dividend) ((num) / dividend * dividend)
