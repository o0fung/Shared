#ifndef __SIGNAL_H__
#define __SIGNAL_H__

template <typename ANY, uint8_t N>
class Data {
    private:
        ANY _mean;
        struct coeff {
            float _b0;
            float _b1;
            float _b2;
            float _a1;
            float _a2;
        } lowpass, highpass;

    public:
        Data();

        ANY array[N];
        ANY filt[N];
        ANY filt2[N];

        void clear();
        void shift(ANY *ptr_array);
        void update(ANY val);

        ANY set_mean();
        ANY get_mean();
  
        void butterworth_lowpass(float cutoff, float sample_time);
        void butterworth_highpass(float cutoff, float sample_time);
        void filter_lowpass(ANY *array_in, ANY *array_out);
        void filter_highpass(ANY *array_in, ANY *array_out);
        void detrend(ANY *array_in, ANY *array_out);
};

template <typename ANY, uint8_t N>
Data<ANY, N>::Data() {};

template <typename ANY, uint8_t N>
void Data<ANY, N>::clear() {
    // clear all array data
    for (uint8_t i = 0; i < N; i++) {
        array[i] = 0;
        filt[i] = 0;
        filt2[i] = 0;
    }
}

template <typename ANY, uint8_t N>
void Data<ANY, N>::shift(ANY *ptr_array) {
    // shift all the array values to up one cell
    // update the first cell as the incoming data
    for (uint8_t i = 0; i < N - 1; i++) {
        ptr_array[N - i - 1] = ptr_array[N - i - 2];
    }
}

template <typename ANY, uint8_t N>
void Data<ANY, N>::update(ANY val) {
    shift(array);
    array[0] = val;
}

template <typename ANY, uint8_t N>
ANY Data<ANY, N>::set_mean() {
    ANY result = 0;
    for (uint8_t i = 0; i < N; i++) {
        // get the sum of all data in the array
        result += array[i];
    }
    // take the average of all data in the array
    _mean = result / N;
    return _mean;
}

template <typename ANY, uint8_t N>
ANY Data<ANY, N>::get_mean() {
    return _mean;
}

template <typename ANY, uint8_t N>
void Data<ANY, N>::butterworth_lowpass(float cutoff, float sample_time) {
    float ita = 1.0 / tan(PI / 1000000.0 * cutoff * sample_time);
    float q = sqrt(2.0);
    lowpass._b0 = 1.0 / (1.0 + q * ita + ita * ita);
    lowpass._b1 = 2 * lowpass._b0;
    lowpass._b2 = lowpass._b0;
    lowpass._a1 = 2.0 * (ita * ita - 1.0) * lowpass._b0;
    lowpass._a2 = - (1.0 - q * ita + ita * ita) * lowpass._b0;
}

template <typename ANY, uint8_t N>
void Data<ANY, N>::butterworth_highpass(float cutoff, float sample_time) {
    float ita = 1.0 / tan(PI / 1000000.0 * cutoff * sample_time);
    float q = sqrt(2.0);
    highpass._b0 = lowpass._b0 * ita * ita;
    highpass._b1 = - lowpass._b1 * ita * ita;
    highpass._b2 = lowpass._b2 * ita * ita;
    highpass._a1 = lowpass._a1;
    highpass._a2 = lowpass._a2;
}

template <typename ANY, uint8_t N>
void Data<ANY, N>::filter_lowpass(ANY *array_in, ANY *array_out) {
    shift(array_out);
    array_out[0] = lowpass._b0 * array_in[0] + lowpass._b1 * array_in[1] + lowpass._b2 * array_in[2] + lowpass._a1 * array_out[1] + lowpass._a2 * array_out[2];
}

template <typename ANY, uint8_t N>
void Data<ANY, N>::filter_highpass(ANY *array_in, ANY *array_out) {
    shift(array_out);
    array_out[0] = highpass._b0 * array_in[0] + highpass._b1 * array_in[1] + highpass._b2 * array_in[2] + highpass._a1 * array_out[1] + highpass._a2 * array_out[2];
}

template <typename ANY, uint8_t N>
void Data<ANY, N>::detrend(ANY *array_in, ANY *array_out) {
    shift(array_out);
    array_out[0] = array_in[0] - array_in[1];
}

#endif
