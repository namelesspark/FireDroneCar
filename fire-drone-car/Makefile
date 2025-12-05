# ===== Makefile (레포 루트) =====

CC      := gcc
CFLAGS  := -Wall -Wextra -O2 \
           -I./src/common \
           -I./src/comm_ui
LDFLAGS := -lpthread

# 테스트 바이너리 이름
COMM_TEST_BIN := comm_test

# 소스 파일들
COMM_SRC      := src/comm_ui/comm.c
COMM_TEST_SRC := tests/comm_test_main.c

.PHONY: all comm_test run_comm_test clean

# 기본: comm_test 빌드
all: comm_test

# comm.c + comm_test_main.c 컴파일
comm_test: $(COMM_SRC) $(COMM_TEST_SRC)
	$(CC) $(CFLAGS) -o $(COMM_TEST_BIN) \
		$(COMM_SRC) $(COMM_TEST_SRC) \
		$(LDFLAGS)

# 테스트 실행 (서버 띄우기)
run_comm_test: comm_test
	./$(COMM_TEST_BIN)

# 정리
clean:
	rm -f $(COMM_TEST_BIN)
