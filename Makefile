CC=gcc

DIRS = $(shell find . -type d -path ./.git -prune -o -type d -print )
INCLUDE = $(addprefix -I, $(DIRS))

# 默认编译选项
CFLAGS_DEFAULT=$(INCLUDE)

# 允许用户通过命令行传递额外的编译选项
CFLAGS_EXTRA=-g -lpthread
ifeq ($(CC_MEMCHECK),1)  # 通过 make CC_MEMCHECK=1 触发
CFLAGS_EXTRA += -DCC_MEMCHECK
endif

CFLAGS=$(CFLAGS_DEFAULT) $(CFLAGS_EXTRA)

# 找到所有非测试源文件（包括 main.c）
SRC = $(shell find . -name "*.c" ! -name "test_*.c" ! -name "main.c")
# 找到所有测试源文件
TEST_SRC = $(shell find . -name "test_*.c")
# 生成对应的测试二进制文件名，放在 cc_test 目录
TEST_BIN = $(patsubst %.c,cc_test/%, $(TEST_SRC:test_%.c=test_%))

MAIN_OBJ = main.o

HTTP_PARSER_OBJ = third/http_parser/http_parser.o

# 生成对应的对象文件
OBJ = $(SRC:.c=.o)
# 生成对应的测试对象文件
TEST_OBJ = $(patsubst %.c,cc_test/%.o, $(TEST_SRC))

# 默认目标
all: format cc_cache test

# 编译主程序
cc_cache: $(OBJ) $(MAIN_OBJ) $(HTTP_PARSER_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

# 测试目标
test: $(TEST_BIN)
	@for bin in $(TEST_BIN); do \
		echo "Running $$bin..."; \
		./$$bin; \
	done

# 编译测试二进制文件
cc_test/%: cc_test/%.o $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

# 编译测试对象文件
cc_test/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -rf cc_test
	rm -f $(OBJ) $(MAIN_OBJ) $(TEST_OBJ) cc_cache

.PHONY: format
format:
	clang-format -style=file --verbose -i `find . -type f  | grep -v  './third/' | grep -v 'lang-format'| grep -E '\.c|\.h'`