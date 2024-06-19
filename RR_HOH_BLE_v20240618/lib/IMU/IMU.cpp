#include <Arduino.h>
#include "IMU.h"

IMU::IMU(ICM_20948_I2C *icm) {
    _icm = icm;
    _icm->enableDebugging(Serial);
};

void IMU::init(uint8_t addr, uint8_t ad0) {
    _addr = addr;
    _ad0 = ad0;
    reset();
}

void IMU::reset() {
    _icm->begin(Wire, _ad0);
    set_has_offset_acc(false);
    set_filters_all();
    clear_all();

    if (_icm->status == ICM_20948_Stat_Ok) {
        Serial.println("\n>> ICM20948 connection SUCCESS!!!");
        set_working(true);
    }
    else {
        Serial.println("\n>> Error in ICM20948 connection!");
        set_working(false);
    }
}

void IMU::set_working(bool val) {
    _is_working = val;
}

bool IMU::get_working() {
    return _is_working;
}

void IMU::set_has_offset_acc(bool val) {
    _has_offset_acc = val;
    _offset_acc_count = 0;
    _accel_offset_sum.x = 0;
    _accel_offset_sum.y = 0;
    _accel_offset_sum.z = 0;
}

bool IMU::get_has_offset_acc() {
    return _has_offset_acc;
}

template <typename any>
void IMU::set_filters(any *sensor) {
    sensor->x.butterworth_lowpass(CUTOFF_FREQ, SAMPLE_TIME);
    sensor->y.butterworth_lowpass(CUTOFF_FREQ, SAMPLE_TIME);
    sensor->z.butterworth_lowpass(CUTOFF_FREQ, SAMPLE_TIME);
    sensor->x.butterworth_highpass(CUTOFF_FREQ, SAMPLE_TIME);
    sensor->y.butterworth_highpass(CUTOFF_FREQ, SAMPLE_TIME);
    sensor->z.butterworth_highpass(CUTOFF_FREQ, SAMPLE_TIME);
}

void IMU::set_filters_all() {
    set_filters(&acc);
    set_filters(&gyr);
    set_filters(&mag);
    set_filters(&ang);
    set_filters(&accel);
    set_filters(&vel);
    set_filters(&pos);
}

template <typename any>
void IMU::clear(any *sensor) {
    sensor->x.clear();
    sensor->y.clear();
    sensor->z.clear();
}

void IMU::clear_all() {
    clear(&acc);
    clear(&gyr);
    clear(&mag);
    clear(&ang);
    clear(&accel);
    clear(&vel);
    clear(&pos);
}

void IMU::update() {
    if (_icm->dataReady()) {
        set_working(true);
        _icm->getAGMT();
        acquire();
        compute_tilt_angles();
        compute_head_angle();
        compute_linear_acc();
        compute_linear_vel();
        compute_linear_pos();
        offset_accel();
    }
}

void IMU::acquire() {
    acc.x.update(_icm->accX());
    acc.y.update(_icm->accY());
    acc.z.update(_icm->accZ());
    gyr.x.update(_icm->gyrX());
    gyr.y.update(_icm->gyrY());
    gyr.z.update(_icm->gyrZ());
    mag.x.update(_icm->magX());
    mag.y.update(_icm->magY());
    mag.z.update(_icm->magZ());
}

void IMU::lowpass_filter() {
    acc.x.filter_lowpass(acc.x.array, acc.x.filt);
    acc.y.filter_lowpass(acc.y.array, acc.y.filt);
    acc.z.filter_lowpass(acc.z.array, acc.z.filt);
    gyr.x.filter_lowpass(acc.x.array, acc.x.filt);
    gyr.y.filter_lowpass(acc.y.array, acc.y.filt);
    gyr.z.filter_lowpass(acc.z.array, acc.z.filt);
    mag.x.filter_lowpass(acc.x.array, acc.x.filt);
    mag.y.filter_lowpass(acc.y.array, acc.y.filt);
    mag.z.filter_lowpass(acc.z.array, acc.z.filt);
}

void IMU::compute_tilt_angles() {
    if (acc.z.array[0] <= 0) {
        // the device should not be overturned, i.e., up-side-down
        return;
    }

    // compute the ROLL tilting angle, along the x direction
    float ang_x_ax = atan(abs(acc.x.array[0]) / acc.z.array[0]) * RADIAN_TO_DEGREE;
    float ang_x_dy = gyr.y.array[0] * SAMPLE_TIME / ONE_SECOND;
    // increase accuracy using gyroscope data in the complementary filter
    ang_x_ax = (acc.x.array[0] > 0) ? -ang_x_ax : ang_x_ax;
    ang.x.update(ang_x_ax + FILTER_WEIGHT * (ang.x.array[0] + ang_x_dy - ang_x_ax));

    // compute the PITCH tilting angle, along the y direction
    float ang_y_ay = atan(abs(acc.y.array[0]) / acc.z.array[0]) * RADIAN_TO_DEGREE;
    float ang_y_dx = gyr.x.array[0] * SAMPLE_TIME / ONE_SECOND;
    // increase accuracy using gyroscope data in the complementary filter
    ang_y_ay = (acc.y.array[0] > 0) ? ang_y_ay : -ang_y_ay;
    ang.y.update(ang_y_ay + FILTER_WEIGHT * (ang.y.array[0] + ang_y_dx - ang_y_ay));
}

void IMU::compute_head_angle() {
    // convert the tilt angles to radian
    float ang_x = ang.x.array[0] * DEGREE_TO_RADIAN;
    float ang_y = ang.y.array[0] * DEGREE_TO_RADIAN;
    // compute the magnetic field strength adjusted to tilting angles
    float mag_xh = mag.x.array[0] * cos(ang_x) + mag.y.array[0] * sin(ang_y) * sin(ang_x) - mag.z.array[0] * cos(ang_y) * sin(ang_x);
    float mag_yh = mag.y.array[0] * cos(ang_y) + mag.z.array[0] * sin(ang_y);
    
    // compute the YAW angle, along the z direction
    // This is the compass orientation of the device
    float ang_z_m = atan(mag_yh / mag_xh) * RADIAN_TO_DEGREE;
    if (mag_xh < 0) {
        ang_z_m = 180 - ang_z_m;
    }
    else if ((mag_xh > 0) && (mag_yh < 0)) {
        ang_z_m = - ang_z_m;
    }
    else if ((mag_xh > 0) && (mag_yh > 0)) {
        ang_z_m = 360 - ang_z_m;
    }
    else if ((mag_xh == 0) && (mag_yh < 0)) {
        ang_z_m = 90;
    }
    else if ((mag_xh == 0) && (mag_yh > 0)) {
        ang_z_m = 270;
    }

    // increase accuracy using gyroscope data in the complementary filter
    float ang_z_dt = - gyr.z.array[0] * SAMPLE_TIME / ONE_SECOND;
    if (abs(ang_z_m - ang.z.array[0]) < 90) {
        ang.z.update(ang_z_m + FILTER_WEIGHT * (ang.z.array[0] + ang_z_dt - ang_z_m));
    }
    else {
        // when the compass angle passed the zero
        // do not use the complementary filter
        ang.z.update(ang_z_m);
    }
}

void IMU::compute_linear_acc() {
    // convert the tilt angles to radian
    float ang_x = ang.x.array[0] * DEGREE_TO_RADIAN;
    float ang_y = ang.y.array[0] * DEGREE_TO_RADIAN;
    // compute the magnetic field strength adjusted to tilting angles
    accel.x.update((acc.x.array[0] * cos(ang_x) + acc.y.array[0] * sin(ang_y) * sin(ang_x) + acc.z.array[0] * cos(ang_y) * sin(ang_x)) * GRAVITY_TO_ACC);
    accel.y.update((acc.y.array[0] * cos(ang_y) - acc.z.array[0] * sin(ang_y)) * GRAVITY_TO_ACC);
    accel.z.update(((- acc.x.array[0] * sin(ang_x) + acc.y.array[0] * cos(ang_x) * sin(ang_y) + acc.z.array[0] * cos(ang_x) * cos(ang_y)) - 1000) * GRAVITY_TO_ACC);
    accel.x.filter_lowpass(accel.x.array, accel.x.filt);
    accel.y.filter_lowpass(accel.y.array, accel.y.filt);
    accel.z.filter_lowpass(accel.z.array, accel.z.filt);
    accel_filt.x.update(accel.x.filt[0] - _accel_offset.x); 
    accel_filt.y.update(accel.y.filt[0] - _accel_offset.y); 
    accel_filt.z.update(accel.z.filt[0] - _accel_offset.z); 
}

void IMU::compute_linear_vel() {
    // set threshold for velocity in X
    vel.x.update(vel.x.array[0] + accel_filt.x.array[0] * SAMPLE_TIME / ONE_SECOND * 1000.0);

    // set threshold for velocity in Y
    vel.y.update(vel.y.array[0] + accel_filt.y.array[0] * SAMPLE_TIME / ONE_SECOND * 1000.0);

    // set threshold for velocity in Z
    vel.z.update(vel.z.array[0] + accel_filt.z.array[0] * SAMPLE_TIME / ONE_SECOND * 1000.0);

}

void IMU::compute_linear_pos() {
    pos.x.update(pos.x.array[0] + vel.x.array[0] * SAMPLE_TIME / ONE_SECOND);
    pos.y.update(pos.y.array[0] + vel.y.array[0] * SAMPLE_TIME / ONE_SECOND);
    pos.z.update(pos.z.array[0] + vel.z.array[0] * SAMPLE_TIME / ONE_SECOND);
}

void IMU::offset_accel() {
    if (!get_has_offset_acc()) {
        if (_offset_acc_count < OFFSET_ACC_COUNT) {
            _accel_offset_sum.x += accel.x.filt[0];
            _accel_offset_sum.y += accel.y.filt[0];
            _accel_offset_sum.z += accel.z.filt[0];
            _offset_acc_count++;
        }
        else {
            _accel_offset.x = _accel_offset_sum.x / _offset_acc_count;
            _accel_offset.y = _accel_offset_sum.y / _offset_acc_count;
            _accel_offset.z = _accel_offset_sum.z / _offset_acc_count;
            set_has_offset_acc(true);
            
            Serial.print(">> Accel Offset: "); Serial.print('\t');
            Serial.print(_accel_offset.x); Serial.print('\t');
            Serial.print(_accel_offset.y); Serial.print('\t');
            Serial.print(_accel_offset.z); Serial.print('\t');
            Serial.println("");
        }
    }
}

void IMU::set_zero_pos() {
    clear(&vel);
    clear(&pos);
}
