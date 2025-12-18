#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>

typedef struct {
    int trig_pin; //output
    int echo_pin; //input
}ultrasonic_t;

typedef enum {
    US_OK = 0,
    US_ERR_NULL = -10,
    US_ERR_WAIT_ECHO_HIGH_TIMEOUT = -11,
    US_ERR_WAIT_ECHO_LOW_TIMEOUT  = -12,
} us_err_t;

float ultrasonic_read_distance_cm_dbg(const ultrasonic_t *sensor,
                                      int timeout_usec,
                                      us_err_t *out_err);

                                      
//초음파 센서 세팅 함수 (return 정상:0, 실패: <0)
int ultrasonic_init(ultrasonic_t *sensor, int trig_pin, int echo_pin);

// 거리 1회 측정 함수 , 이걸 여러번 써서 평균 값으로 측정할거임! 
float ultrasonic_read_distance_cm(const ultrasonic_t *sensor, int timeout_usec);


// 거리의 평균을 내주는 함수
float ultrasonic_read_distance_cm_avg(const ultrasonic_t *senseor, int samples, int timeout_usec);



#endif

