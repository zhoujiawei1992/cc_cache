
#ifndef __CC_UTIL_CC_TEST_TOOLS_H__
#define __CC_UTIL_CC_TEST_TOOLS_H__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RUN_TEST_FUNC(func)                  \
  do {                                       \
    printf("Running function: %s\n", #func); \
    func();                                  \
  } while (0)

#endif