#include "cc_util/cc_atomic.h"

// 原子加法
int atomic_add(int* value, int increment) {
  int old_value;
  __asm__ __volatile__("lock; xaddl %0, %1"             // 原子加法
                       : "=r"(old_value), "+m"(*value)  // 输出操作数
                       : "0"(increment)                 // 输入操作数
                       : "memory"                       // 告诉编译器内存可能被修改
  );
  return old_value + increment;  // 返回增加后的值
}

// 比较并设置
int compare_and_set(int* a, int old_value, int new_value) {
  int result;
  __asm__ __volatile__(
      "lock; cmpxchgl %2, %1;"
      "setz %%al;"
      "movzx %%al, %0;"
      : "=r"(result), "+m"(*a)
      : "r"(new_value), "a"(old_value)
      : "memory", "cc");
  return result;
}

// 比较并交换
int compare_and_swap(int* lhs, int* rhs, int value) {
  int result;
  __asm__ __volatile__(
      "movl (%2), %%ebx\n"           // Move the value at rhs into ebx
      "lock cmpxchgl %%ebx, (%1)\n"  // Compare lhs with eax (value), if equal, swap with ebx (rhs)
      "setz %%al\n"                  // Set result based on zero flag
      "movzbl %%al, %0"              // Move the result into the return register
      : "=r"(result)
      : "r"(lhs), "r"(rhs), "a"(value)
      : "%ebx", "memory", "cc");
  return result;
}