#pragma once
#ifndef angle_H
#define angle_H
#include <Arduino.h>
//#include <cmath>
#include <Adafruit_LSM6DSO32.h>
#define M_PI 3.14159265358979323846
class angle {
public:
    angle(Adafruit_LSM6DSOX* acc,String modele);
    void initangle(float ax, float ay, float az, float gx, float gy, float gz, unsigned long time);
    float correctionangle(float alpha, float ax, float ay, float az, float gx, float gy, float gz, unsigned long time);
    float wheelRot2(void);
    float updateangle(unsigned long t_new);
    float acfx;  //filtre accel
    float acfy;
    float anglef;
    //float acfz;
private:
    void applyrot(float* ax, float* ay, float* az, float* gx, float* gy, float* gz);
    float kalman_p;
    float kalman_q;
    float kalman_r;
    double w;
    float wy;
    float wz;
    float dtt;
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    Adafruit_LSM6DSOX* dsox = nullptr; // accelerometer
    unsigned long tangle0;
    unsigned long t;
    float rotmat[9] ={0,0,0,0,0,0,0,0,0};
};
#endif

