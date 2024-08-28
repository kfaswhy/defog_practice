# ָ��������
CC = gcc

# ����ѡ��
CFLAGS = -Wall -Wextra -I.

# Ŀ���ִ���ļ���
TARGET = my_program

# Դ�ļ��б�
SRCS = main.c read_bmp.c

# ���ɵĶ����ļ��б�
OBJS = $(SRCS:.c=.o)

# ����Ĭ��Ŀ��
all: $(TARGET)

# �������Ӷ����ļ����ɿ�ִ���ļ�
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# ���򣺱���Դ�ļ����ɶ����ļ�
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# �����������ɵ��ļ�
clean:
	rm -f $(OBJS) $(TARGET)

# αĿ�꣬��ֹ��ʵ���ļ�����ͻ
.PHONY: all clean