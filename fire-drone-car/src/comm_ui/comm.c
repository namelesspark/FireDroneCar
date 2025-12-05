// comm.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>

// 프로젝트 공통 헤더 (shared_state_t 정의, enum, 상수 등)
#include "common.h"
#include "comm_ui.h"

#define COMM_PORT 5000              // 라즈베리파이에서 열 포트
#define COMM_STATUS_INTERVAL_MS 200 // 상태 전송 주기 (ms)
#define COMM_RECV_BUF_SIZE 1024

// ----------------------
// 컨텍스트 구조체
// ----------------------
typedef struct
{
    shared_state_t *state;
    pthread_mutex_t *state_mutex;
    volatile bool *shutdown_flag;
} comm_ctx_t;

// ----------------------
// 내부 함수 프로토타입
// ----------------------
static int comm_setup_server_socket(int port);
static void comm_format_status_line(char *buf, size_t size,
                                    const shared_state_t *st);
static void comm_handle_command_line(const char *line,
                                     shared_state_t *st,
                                     pthread_mutex_t *mtx);
static long long now_ms(void);

// ----------------------
// 외부에서 부를 스레드 함수
//  - pthread_create 에 넘길 함수
//  - arg 에 comm_ctx_t* 를 넣어서 호출
// ----------------------
void *comm_thread(void *arg)
{
    comm_ctx_t *ctx = (comm_ctx_t *)arg;
    if (!ctx || !ctx->state || !ctx->state_mutex)
    {
        fprintf(stderr, "[comm] invalid context\n");
        return NULL;
    }

    int server_fd = comm_setup_server_socket(COMM_PORT);
    if (server_fd < 0)
    {
        fprintf(stderr, "[comm] failed to open server socket\n");
        return NULL;
    }

    printf("[comm] listening on port %d\n", COMM_PORT);

    int client_fd = -1;
    char recv_buf[COMM_RECV_BUF_SIZE];
    size_t recv_len = 0;
    long long last_status_ms = now_ms();

    while (!*(ctx->shutdown_flag))
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_fd = server_fd;

        if (client_fd >= 0)
        {
            FD_SET(client_fd, &readfds);
            if (client_fd > max_fd)
                max_fd = client_fd;
        }

        // select 타임아웃 설정
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000; // 100ms

        int ret = select(max_fd + 1, &readfds, NULL, NULL, &tv);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("[comm] select");
            break;
        }

        // 새로운 클라이언트 접속
        if (FD_ISSET(server_fd, &readfds))
        {
            struct sockaddr_in cliaddr;
            socklen_t clilen = sizeof(cliaddr);
            int new_fd = accept(server_fd, (struct sockaddr *)&cliaddr, &clilen);
            if (new_fd < 0)
            {
                perror("[comm] accept");
            }
            else
            {
                if (client_fd >= 0)
                {
                    // 이미 연결 중이면 기존 클라이언트 닫고 교체
                    close(client_fd);
                }
                client_fd = new_fd;
                printf("[comm] client connected: %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
            }
        }

        // 클라이언트로부터 명령 수신
        if (client_fd >= 0 && FD_ISSET(client_fd, &readfds))
        {
            char buf[256];
            ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
            if (n <= 0)
            {
                if (n < 0)
                    perror("[comm] recv");
                printf("[comm] client disconnected\n");
                close(client_fd);
                client_fd = -1;
                recv_len = 0;
            }
            else
            {
                // 수신 버퍼에 누적
                if (recv_len + (size_t)n > sizeof(recv_buf))
                {
                    // 오버플로 방지: 버퍼 리셋
                    recv_len = 0;
                }
                memcpy(recv_buf + recv_len, buf, n);
                recv_len += (size_t)n;

                // '\n' 단위로 파싱
                size_t start = 0;
                for (size_t i = 0; i < recv_len; ++i)
                {
                    if (recv_buf[i] == '\n' || recv_buf[i] == '\r')
                    {
                        recv_buf[i] = '\0';
                        if (i > start)
                        {
                            const char *line = recv_buf + start;
                            comm_handle_command_line(line, ctx->state, ctx->state_mutex);
                        }
                        start = i + 1;
                    }
                }
                // 남은 데이터 앞쪽으로 당기기
                if (start > 0 && start < recv_len)
                {
                    memmove(recv_buf, recv_buf + start, recv_len - start);
                    recv_len -= start;
                }
                else if (start >= recv_len)
                {
                    recv_len = 0;
                }
            }
        }

        // 상태 전송
        long long now = now_ms();
        if (client_fd >= 0 &&
            now - last_status_ms >= COMM_STATUS_INTERVAL_MS)
        {

            char line[256];

            pthread_mutex_lock(ctx->state_mutex);
            shared_state_t snapshot = *(ctx->state); // 잠깐 복사
            pthread_mutex_unlock(ctx->state_mutex);

            comm_format_status_line(line, sizeof(line), &snapshot);
            size_t len = strlen(line);
            line[len++] = '\n';
            ssize_t s = send(client_fd, line, len, 0);
            if (s < 0)
            {
                perror("[comm] send");
                close(client_fd);
                client_fd = -1;
            }
            last_status_ms = now;
        }

        // TODO: 종료 조건 필요 시 break 처리
    }

    if (client_fd >= 0)
    {
        close(client_fd);
    }
    close(server_fd);
    return NULL;
}

// ----------------------
// 서버 소켓 생성
// ----------------------
static int comm_setup_server_socket(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        perror("[comm] socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("[comm] setsockopt");
        close(fd);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("[comm] bind");
        close(fd);
        return -1;
    }

    if (listen(fd, 1) < 0)
    {
        perror("[comm] listen");
        close(fd);
        return -1;
    }

    return fd;
}

// ----------------------
// 상태 문자열 포맷
//   예) MODE=EXT;WLV=2;T_FIRE=123.4;DT=30.2;D=0.35;HOT=4,7
// ----------------------
static void comm_format_status_line(char *buf, size_t size,
                                    const shared_state_t *st)
{
    // 아래 필드/enum 이름은 예시이므로
    // 네가 정의한 shared_state_t에 맞게 수정하면 됨.

    const char *mode_str = "UNKNOWN";
    switch (st->mode)
    {
    case MODE_IDLE:
        mode_str = "IDLE";
        break;
    case MODE_SEARCH:
        mode_str = "SEARCH";
        break;
    case MODE_APPROACH:
        mode_str = "APPROACH";
        break;
    case MODE_EXTINGUISH:
        mode_str = "EXT";
        break;
    case MODE_SAFE_STOP:
        mode_str = "SAFE";
        break;
    default:
        break;
    }

    snprintf(buf, size, "MODE=%s;WLV=%d;T_FIRE=%.1f;DT=%.1f;D=%.2f;HOT=%d,%d;ESTOP=%d",
             mode_str,
             st->water_level,
             st->t_fire,
             st->dT,
             st->distance,
             st->hot_row,
             st->hot_col,
             st->emergency_stop ? 1 : 0);
}

// ----------------------
// 한 줄 명령 처리
//   ex)
//     "START"
//     "STOP"
//     "ESTOP"
//     "SET_WLV 2"
// ----------------------
static void comm_handle_command_line(const char *line,
                                     shared_state_t *st,
                                     pthread_mutex_t *mtx)
{
    printf("[comm] recv cmd: '%s'\n", line);

    // 앞뒤 공백 제거
    while (*line == ' ' || *line == '\t')
        line++;
    if (*line == '\0')
        return;

    pthread_mutex_lock(mtx);

    if (strcasecmp(line, "START") == 0)
    {
        st->cmd_mode = CMD_MODE_START; // 실제 enum 이름에 맞게 수정
    }
    else if (strcasecmp(line, "STOP") == 0)
    {
        st->cmd_mode = CMD_MODE_STOP;
    }
    else if (strcasecmp(line, "MANUAL") == 0)
    {
        st->cmd_mode = CMD_MODE_MANUAL;
    }
    else if (strcasecmp(line, "ESTOP") == 0 || strcasecmp(line, "E-STOP") == 0)
    {
        st->emergency_stop = true;
    }
    else if (strncasecmp(line, "SET_WLV", 7) == 0)
    {
        int lv = 0;
        if (sscanf(line + 7, "%d", &lv) == 1)
        {
            if (lv < 0)
                lv = 0;
            if (lv > 5)
                lv = 5; // 물 강도 단계 최대 5단 예시
            st->water_level = lv;
            st->water_level_override = true; // 이런 플래그 두면 좋음
        }
    }
    else
    {
        // TODO: 필요하면 더 많은 명령 추가 (예: MODE=SEARCH 등)
    }

    pthread_mutex_unlock(mtx);
}

// ----------------------
// 현재 시각(ms 단위)
// ----------------------
static long long now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}
