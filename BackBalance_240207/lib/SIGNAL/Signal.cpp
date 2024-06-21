#include "Signal.h"
MPU6050 IMU(MPU6050_DEFAULT_ADDRESS);                            //
                                                                 //
Signal::Signal() {};                                             //
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Signal::init_imu(int samplrt_div, int dlpf_conf){           // Initialize I2C MPU6050 sensor
    Wire.begin();                                                // Initialize I2C
    IMU.initialize();                                            // Initialize to +/-2g & +/-250 deg/s
                                                                 // Default: sample rate division = 4
                                                                 // Default: digital LPF configuration = 6
    IMU.setRate(samplrt_div);                                    // Sampling rate (Default=200Hz)
    IMU.setDLPFMode(dlpf_conf);                                  // Digital LPF (Default cutoff=5Hz)

}                                                                //
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Signal::butterworth(double cutoff, double sample_time) {    // 2nd Order IIRF Polynomial Equation:              //
    for (int i = 0; i < N_CHANNEL; i++) {                        // filt[n] = b0 raw[n] + b1 raw[n-1] + b2 raw[n-2]  //
        for (int j = 0; j < N_BUFFER; j++) {                     //           + a1 filt[n-1] + a2 filt[n-2]          //
            raw[i][j]  = 0.0;                                    // Initialize input buffer and output buffer with   //
            filt[i][j] = 0.0;                                    // zeros 2nd order filter needs to save last two    //
        }                                                        // data frames need to make a buffer with size = 3  //
    }                                                            //
                                                                 //
    const double ita = 1.0 / tan(PI / COE*cutoff*sample_time);   // Compute the polynomial coefficient of 
    const double q = sqrt(2.0);                                  // 2nd order Butterworth filter
    b0 = 1.0 / (1.0 + q * ita + ita * ita);                      //
    b1 = 2 * b0;                                                 //
    b2 = b0;                                                     //
    a1 = 2.0 * (ita * ita - 1.0) * b0;                           //
    a2 = -(1.0 - q * ita + ita * ita) * b0;                      //
}                                                                //
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Signal::phase_shift(double arr[N_BUFFER]) {                 // Shift phase delay of raw data by one time unit,
    arr[2] = arr[1];                                             // ready for new data    
    arr[1] = arr[0];                                             // t(n-1) -> t(n-2)
}                                                                // t(n)   -> t(n-1)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Signal::phase_shift(double dta[N_CHANNEL][N_BUFFER],int n){ // Shift phase delay of raw data by one time unit, 
    for (int i = 0; i < n; i++){                                 // ready for new data
        dta[i][2] = dta[i][1];                                   // t(n-1) -> t(n-2)
        dta[i][1] = dta[i][0];                                   // t(n  ) -> t(n-1)
    }                                                            //
}                                                                //
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Signal::acquire() {                                         //                                                  //
    phase_shift(raw, N_CHANNEL);                                 // shift previous data (n0 > n1; n1 > n2;)          //
    IMU.getMotion6(&a[0], &a[1], &a[2], &g[0], &g[1], &g[2]);    // Read IMU signals from I2C devices to data buffers//
                                                                 // ax ay az gx gy gz                                // 
    for (int i = 0; i < N_BUFFER; i++) {                         // Read IMU signals from data buffers               //
        raw[i  ][0] = A_FACTOR * (double) a[i];                  // ax ay az                                         //
        raw[i+3][0] = G_FACTOR * (double) g[i];                  // gx gy gz                                         //
    }                                                            //                                                  //
}                                                                //                                                  //
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Signal::filter() {                                          //                                                  //
    phase_shift(filt, N_CHANNEL);                                // prepare to add new data, first one first out     //
                                                                 //    
    for (int i = 0; i < N_CHANNEL; i++) {                        // Filter signals using 2nd order Butterworth LPF
        filt[i][0] = b0*raw[i][0] + b1*raw[i][1]+ b2*raw[i][2] + a1*filt[i][1]+ a2*filt[i][2];             //
    }                                                            //
}                                                                //
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Signal::tilting(double dt) {                                //
    double acc_x = filt[0][0];                                   // get a copy of imu data
    double acc_y = filt[1][0];                                   //
    double acc_z = filt[2][0];                                   //
    double gyr_x = filt[3][0];                                   //
    double gyr_y = filt[4][0];                                   //
    double gyr_z = filt[5][0];                                   //
                                                                 //
    double ang_x, ang_y, ang_z;                                  //
    double ang_zy, ang_xy;                                       //
    double ang_x_diff, ang_z_diff;                               //
                                                                 //
    // AngleRoll  = atan(acc_y/sqrt(acc_x * acc_x + acc_z * acc_z)) * 1/(3.142/180);
    // AnglePitch = atan(acc_x/sqrt(acc_y * acc_y + acc_z * acc_z)) * 1/(3.142/180);

    // if(AngleRoll >0){
    // AnglePitch = AnglePitch + AngleRoll;
    // }else{
    // AnglePitch = AnglePitch - AngleRoll;
    // }
    
    phase_shift(tilt_x);                                         // t(n-1) -> t(n-2); t(n)   -> t(n-1)
    phase_shift(tilt_z);                                         // 
                                                                 // compute the tilt angle in x-direction 
                                                                 // (forward rotation as +ve)
    ang_z=R_FACTOR*atan(abs(acc_z)/sqrt(sq(acc_x)+sq(acc_y)));   // assume sensor orientation is upright,  
                                                                 // x pointing to the right,
    ang_y=0;                                                     // y pointing downward,
    ang_zy=(acc_z >= 0) ? (ang_z + ang_y) : (- ang_z - ang_y);   //   z facing in forward direction.
    ang_x_diff = dt * gyr_x / COE;                               //
    tilt_x[0]=ang_zy+FILTER_WEIGH*(tilt_x[1]+ang_x_diff-ang_zy); // compute using acceleromter signal
                                                                 // negative z value imply tilting backward
    ang_x=R_FACTOR*atan(abs(acc_x)/sqrt(sq(acc_z)+sq(acc_y)));   // compute using gyroscope signal in x axis
                                                                 // complementary filter
    ang_y=0;
    ang_xy=(acc_x >= 0) ? (ang_x + ang_y) : (- ang_x - ang_y);   // tilting angle in forward-backward plane
    ang_z_diff = dt * gyr_z / COE;                               // compute using acceleromter signal
    tilt_z[0]=ang_xy+FILTER_WEIGH*(tilt_z[1]+ang_z_diff-ang_xy); // negative x value imply tilting to the left   
                                                                 // compute using gyroscope signal in z axis
                                                                 // complementary filter
                                                                 // tilting angle in lateral-medial plane
    tile_x_final = tilt_x[0] - tile_x_cal;
    tile_z_final = tilt_z[0] - tile_z_cal;

    // a_z     = ang_z;
    // a_x     = ang_x;
    // a_zy    = ang_zy;
    // a_xy    = ang_xy;
}                                                                //
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Signal::float2bytes(float data1, float data2){
    union float_bytes{
        float val;
        unsigned char bytes[sizeof(float)];
    }data;

    data.val = data1;
    for(int i=3; i>-1; i--){
        
        tiltX2Byte[i] = data.bytes[i];                           //Serial.printf("%x\n", data.bytes[i]);
    }

    data.val = data2;
    for(int i=3; i>-1; i--){
        //Serial.printf("%x\n", data.bytes[i]);
        tiltZ2Byte[i] = data.bytes[i];
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Signal::getData(){
    acquire();
    filter();
    tilting(delta_t);
    float2bytes(tilt_x[0], tilt_z[0]);
    // float2bytes(tile_x_final, tile_z_final);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
void Signal::setZero(){
    tile_x_cal = 0;
    tile_z_cal = 0;
    for(int i=0; i<100; i++){
        getData();
        tile_x_cal += tilt_x[0];
        tile_z_cal += tilt_z[0];
        delay(1);
    }
    tile_x_cal = tile_x_cal / 100;
    tile_z_cal = tile_z_cal / 100;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - // - - - - - - - - - - - - - - - - - - - - - - - -  //
