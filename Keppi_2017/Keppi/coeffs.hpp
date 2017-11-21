/***** coeffs.hpp *****/

#define SAMPLE_RATE 22050.0 // analog sample rate
// #define SAMPLE_RATE 44100.0 // Audio sample rate
#define sqrt2 1.41421356237
#define PI 3.14159265359



double gLowA1, gLowA2, gHighA1, gHighA2, gLowB0, gLowB1, gLowB2, gHighB0, gHighB1, gHighB2;
double gW0_low = 30.0; // 200.0;
double gW0_high = 0.3; // 20.0;

// Calculates coeffs for filters. Doesn't return anything, just sets variables.
void calculateCoeffs() {
    // Calculate:
    double t = 1 / SAMPLE_RATE;
	double t2 = t * t;
	double q = sqrt2 / 2;
	double w02_high = gW0_high * gW0_high;
	double w02_low = gW0_low * gW0_low;
	
	double high_norm = 4 + (2 * gW0_high * t)/q + 2 * w02_high * t2;
	double low_norm = 4 + (2 * gW0_low * t) / q + 2 * w02_low * t2;
	
	gLowA1 = (-8 + ( 2 * w02_low * t2)) / low_norm;
	gLowA2 = (4 - ((2 * gW0_low * t / q) + w02_low * t2)) / low_norm;
	
	gHighA1 = (-8 + (2 * w02_high * t2)) / high_norm;
	gHighA2 = (4 - ((2 * gW0_high * t / q) + w02_high*t2)) / high_norm;
	
	gLowB0 = (w02_low * t2) / low_norm;
	gLowB1 = (2 * w02_low * t2) / low_norm;
	gLowB2 = gLowB0;
	
	gHighB0 = 4 / high_norm;
	gHighB1 = -8 / high_norm;
	gHighB2 = gHighB0;
	
	rt_printf("Calculating complete!\n");
	rt_printf("lowA1: %f, lowA2: %f\n", gLowA1, gLowA2);
	rt_printf("lowB0: %f, lowB1: %f, lowB2: %f\n", gLowB0, gLowB1, gLowB2);
	rt_printf("highA1: %f, highA2: %f\n", gHighA1, gHighA2);
	rt_printf("highB0: %f, highB1: %f, highB2: %f\n", gHighB0, gHighB1, gHighB2);
	
}


