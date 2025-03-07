#ifndef __CC_UTIL_CC_ATOMIC_H__
#define __CC_UTIL_CC_ATOMIC_H__

int atomic_add(int* value, int increment);
int compare_and_set(int* value, int old_value, int new_value);
int compare_and_swap(int* lhs, int* rhs, int value);

#endif