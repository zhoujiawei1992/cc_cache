#include "cc_util/cc_atomic.h"
#include "cc_util/cc_test_tools.h"

// 测试 atomic_add 函数
void test_atomic_add() {
  int value = 10;
  int increment = 5;
  int result = atomic_add(&value, increment);
  printf("atomic_add test: ");
  if (result == 15 && value == 15) {
    printf("Passed\n");
  } else {
    printf("Failed\n");
  }
}

// 测试 compare_and_set 函数
void test_compare_and_set() {
  int a = 10;
  int old_value = 10;
  int new_value = 20;

  int result = compare_and_set(&a, old_value, new_value);
  printf("compare_and_set test 1: ");
  if (result == 1 && a == 20) {
    printf("Passed\n");
  } else {
    printf("Failed\n");
  }

  result = compare_and_set(&a, old_value, 30);
  printf("compare_and_set test 2: ");
  if (result == 0 && a == 20) {
    printf("Passed\n");
  } else {
    printf("Failed\n");
  }
}

// 测试 compare_and_swap 函数
void test_compare_and_swap() {
  int lhs = 10;
  int rhs = 20;
  int value = 10;

  int result = compare_and_swap(&lhs, &rhs, value);
  printf("compare_and_swap test 1: ");
  if (result == 1 && lhs == 20) {
    printf("Passed\n");
  } else {
    printf("Failed\n");
  }

  value = 30;
  result = compare_and_swap(&lhs, &rhs, value);
  printf("compare_and_swap test 2: ");
  if (result == 0 && lhs == 20) {
    printf("Passed\n");
  } else {
    printf("Failed\n");
  }
}

int main() {
  RUN_TEST_FUNC(test_atomic_add);
  RUN_TEST_FUNC(test_compare_and_set);
  RUN_TEST_FUNC(test_compare_and_swap);
  return 0;
}