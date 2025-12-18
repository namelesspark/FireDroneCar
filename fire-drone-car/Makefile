# ===== Makefile (fire-drone-car) =====

CC      := gcc
CFLAGS  := -Wall -Wextra -O2 \
           -I./src/common \
           -I./src/comm_ui \
           -I./src/algo \
           -I./src/embedded \
           -I./src/embedded/mlx90641
LDFLAGS := -lpthread -lwiringPi -lm

# 메인 바이너리 이름
TARGET := fire_drone_car

# 소스 파일들
MAIN_SRC := src/main/main.c

ALGO_SRC := src/algo/state_machine.c \
            src/algo/navigation.c \
            src/algo/extinguish_logic.c \
            src/algo/shared_state.c

EMBEDDED_SRC := src/embedded/motor_control.c \
                src/embedded/pump_control.c \
                src/embedded/servo_control.c \
                src/embedded/thermal_sensor.c \
                src/embedded/ultrasonic.c \
                src/embedded/mlx90641/MLX90641_API.c \
                src/embedded/mlx90641/MLX90641_I2C_Driver.c

COMM_SRC := src/comm_ui/comm.c

ALL_SRC := $(MAIN_SRC) $(ALGO_SRC) $(EMBEDDED_SRC) $(COMM_SRC)

# 테스트용
COMM_TEST_BIN := comm_test
COMM_TEST_SRC := tests/comm_test_main.c

.PHONY: all clean comm_test run_comm_test

# 기본: 메인 프로그램 빌드
all: $(TARGET)

$(TARGET): $(ALL_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 통신 테스트 빌드
comm_test: $(COMM_SRC) $(COMM_TEST_SRC)
	$(CC) $(CFLAGS) -o $(COMM_TEST_BIN) $^ $(LDFLAGS)

# 테스트 실행 (서버 띄우기)
run_comm_test: comm_test
	./$(COMM_TEST_BIN)

# 정리
clean:
	rm -f $(TARGET) $(COMM_TEST_BIN)
