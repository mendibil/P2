#include <math.h> 
#include <stdio.h>

#include "pav_analysis.h"

float compute_power(const float *x, unsigned int N) { 
    float power = 0.0;

    for(int i = 0; i < N; i++) {
        power = power + x[i]*x[i]; 
    }
    power = 10*log10((1.0/N)*power);

    return power;

} 

float compute_am(const float *x, unsigned int N) { 
    float am = 0;

    for(int i = 0; i < N; i++) {
        am = am + fabs(x[i]); 
    }

    return am/N;
}

float compute_zcr(const float *x, unsigned int N, float fm) { 
    float zcr = 0;

    for(int i = 1; i < N; i++) {
        if(x[i] >= 0 && x[i-1] < 0  || x[i] < 0 && x[i-1] >= 0) {
            zcr++;
        }
    }
    zcr = fm/(2* (N-1)) * zcr;
    return zcr;
}

float hamming_window(int n, int N){
	return (0.54 - 0.46 * cos(2 * n * N * M_PI));
}

float compute_power_hamming(const float *x, unsigned int N, float fm) {
    float numerador = 0;
	float denominador = 0;

	for(int i = 0; i < N; i++){
		numerador   = numerador + pow(x[i] * hamming_window(i, N),2);
		denominador = denominador + pow(hamming_window(i, N),2);
	}    
	return 10*log10(numerador/denominador);
}

