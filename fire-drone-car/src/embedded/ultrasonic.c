#include "ultrasonic.h"

#include <wiringPi.h>
#include <stdio.h>

//거리(cm) = 시간(마이크로초) x 소리 속도 (cm/마이크로초) /2 
#define SPEED_OF_SOUND_CM_PER_USEC 0.0343f //소리 속도

int ultrasonic_init(ultrasonic_t *sensor, int trig_pin, int echo_pin){
    if(sensor == NULL) return -1;

    sensor->trig_pin = trig_pin;
    sensor->echo_pin = echo_pin;

    pinMode(sensor->trig_pin, OUTPUT);
    pinMode(sensor->echo_pin, INPUT);

    digitalWrite(sensor->trig_pin, LOW);
    delay(50);

    return 0;
}
float ultrasonic_read_distance_cm(const ultrasonic_t *sensor, int timeout_usec){
    if(sensor == NULL) return -1.0f;

    int trig = sensor->trig_pin;
    int echo = sensor->echo_pin;

    digitalWrite(trig, LOW);
    delayMicroseconds(2);

    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    
    unsigned int start_wait = micros();
    while (digitalRead(echo) == LOW) {
        if ((int)(micros() - start_wait) > timeout_usec) {
            return -1.0f;
        }
    }

    unsigned int pulse_start = micros();
    while (digitalRead(echo) == HIGH) {
        if ((int)(micros() - pulse_start) > timeout_usec) {
            return -1.0f;
        }
    }
    unsigned int pulse_end = micros();

    unsigned int pulse_duration = pulse_end - pulse_start;

    float distance_cm = (pulse_duration * SPEED_OF_SOUND_CM_PER_USEC) / 2.0f;

    return distance_cm;
}

float ultrasonic_read_distance_cm_avg(const ultrasonic_t *sensor, int samples,int timeout_usec)
{
    if (sensor == NULL || samples <= 0) return -1.0f;

    float sum = 0.0f;
    int valid = 0;

    for (int i = 0; i < samples; ++i) {
        float d = ultrasonic_read_distance_cm(sensor, timeout_usec);
        if (d > 0.0f) {
            sum += d;
            valid++;
        }
        delay(50); 
    }

    if (valid == 0) return -1.0f;
    return sum / valid;
}