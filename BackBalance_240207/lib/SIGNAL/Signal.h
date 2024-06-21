
#ifndef _SIGNAL_
#define _SIGNAL_

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include "parameter.h"

class Signal {                                                   // Difference Equation for 2nd Order Butterworth IIRF
    private:                                                     // y0 = b0*x0 + b1*x1 + b2*x2 + a1*y1 + a2*y2
        int16_t a[N_BUFFER], g[N_BUFFER];                        // Data buffer for IMU signal acquisition
    public:                                                      //
                                                                 //
        Signal();                                                //
        void init_imu(int samplrt_div, int dlpf_conf);           //
        void butterworth(double cutoff, double sample_time);     //
        void phase_shift(double arr[N_BUFFER]);                  //
        void phase_shift(double dta[N_CHANNEL][N_BUFFER], int n);//
        void acquire();
        void filter();
        void tilting(double dt);
        void float2bytes(float data1, float data2);
        void getData();
        void setZero();
};
#endif /* _SIGNAL_ */