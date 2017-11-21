/***** play.hpp *****/
int startPlayingSample(int sensor, float piezoValue);

extern int gReadPointers[NUM_VOICES];
extern int gPointerState[NUM_VOICES];
extern int gReadPointerBufferID[NUM_VOICES];
extern int gReadPointerAge[NUM_VOICES];
extern float gSampleVelocity[NUM_VOICES];
extern int gSampleCount;

int startPlayingSample(int sensor, float piezoValue) {
	
	
	// See if we have a free pointer.
	for (int i = 0; i < NUM_VOICES; i++) {
		if (gPointerState[i] == 0) {
			gReadPointerBufferID[i] = sensor;
			gPointerState[i] = 1;
			gReadPointerAge[i] = gSampleCount;
			// gSampleVelocity[i] = piezoValue * sampleScalers[sensor];
			gSampleVelocity[i] = piezoValue;
			gReadPointers[i] = 0; // Set read to the beginning of the sample
			rt_printf("Found a loose voice! It was index %d\n", i);

			return 1; // Return 1 
		}
	}
	int oldest = gReadPointerAge[0];
	int oldestPointerIndex = 0;
	for (int j = 0; j < NUM_VOICES; j++) {
		if (gReadPointerAge[j] < oldest) {
			oldest = gReadPointerAge[j];
			oldestPointerIndex = j;
		}
	}
	gPointerState[oldestPointerIndex] = 0;
	rt_printf("Stole a voice!\n");

	return 0;
}
