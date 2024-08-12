# 指定编译器
CC = gcc

# 编译选项
CFLAGS = -Wall -Wextra -I.

# 目标可执行文件名
TARGET = my_program

# 源文件列表
SRCS = main.c read_bmp.c

# 生成的对象文件列表
OBJS = $(SRCS:.c=.o)

# 规则：默认目标
all: $(TARGET)

# 规则：链接对象文件生成可执行文件
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# 规则：编译源文件生成对象文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 规则：清理生成的文件
clean:
	rm -f $(OBJS) $(TARGET)

# 伪目标，防止与实际文件名冲突
.PHONY: all clean