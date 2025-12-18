#ifndef PUMP_CONTROL_H
#define PUMP_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

// 네 코드 그대로: PCA9685 채널 4 사용
#define PUMP_CHANNEL 4  // 서보 포트 4번

// 초기화/제어 API
int pump_init(void);
void pump_on(void);
void pump_off(void);

#ifdef __cplusplus
}
#endif

#endif // PUMP_CONTROL_H
