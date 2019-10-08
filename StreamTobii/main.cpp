//Joe Snider
//10/18
//
//Stream Tobii data over LSL.

#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include <Windows.h>

#include "Tobii.h"
#include "lsl_cpp.h"

using namespace std;

//hack in a global interface for non-ui version.
//Set the channels. Must match (note the utter lack of a UI).
const int NCHANNELS = 2;
shared_ptr<lsl::stream_info> info;
shared_ptr<lsl::stream_outlet> outlet;

//code for the quit key (spec to windows)
const int QUIT_KEY = 0x51; //0x51 is the letter 'q'

void StreamLSL(double x, double y) {
	float sample[NCHANNELS];
	sample[0] = x;
	sample[1] = y;
	outlet->push_sample(sample);
}

//hackish callback test
int N = 0;
void f(double x, double y) {
	cout << x << " " << y << "\n" << flush;
	++N;
}

int main() {
	cout << "Create Tobii object ... " << flush;
	CTobii T1;
	T1.Start();
	cout << "done\n" << flush;

	/*cout << "Stream 100 samples ... \n" << flush;
	double x = T1.eyeX;
	double y = T1.eyeY;
	for (int i = 0; i < 100; ) {
		if (x != T1.eyeX || y != T1.eyeY) {
			cout << "  " << T1.eyeX << " " << T1.eyeY << "\n" << flush;
			++i;
		}
	}
	cout << "done\n" << flush;*/

	//add lsl to the callback list
	cout << "Add LSL to the callback list ... \n" << flush;
	info.reset( new lsl::stream_info("Tobii4C", "Gaze", NCHANNELS));
	outlet.reset(new lsl::stream_outlet(*info, 0, 1));
	T1.AddCallback(StreamLSL);
	cout << "done\n" << flush;

	cout << "Streaming Tobii data on Gaze stream. Press q to quit ... " << flush;
	//T1.AddCallback(f);
	while (GetAsyncKeyState(QUIT_KEY) >= 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	T1.ClearCallbacks();
	cout << "done\n" << flush;

	cout << "Shutting down Tobii (takes a few seconds, automatic)\n" << flush;

	return 0;
}



/*
#include <tobii/tobii.h>
#include <tobii/tobii_streams.h>
#include <stdio.h>
#include <assert.h>
#include <cstring>

void gaze_point_callback(tobii_gaze_point_t const* gaze_point, void* user_data)
{
	if (gaze_point->validity == TOBII_VALIDITY_VALID)
		printf("Gaze point: %f, %f\n",
			gaze_point->position_xy[0],
			gaze_point->position_xy[1]);
}

static void url_receiver(char const* url, void* user_data)
{
	char* buffer = (char*)user_data;
	if (*buffer != '\0') return; // only keep first value

	if (strlen(url) < 256)
		strcpy_s(buffer, 256, url);
}

int main()
{
	tobii_api_t* api;
	tobii_error_t error = tobii_api_create(&api, NULL, NULL);
	assert(error == TOBII_ERROR_NO_ERROR);

	char url[256] = { 0 };
	error = tobii_enumerate_local_device_urls(api, url_receiver, url);
	assert(error == TOBII_ERROR_NO_ERROR && *url != '\0');

	tobii_device_t* device;
	error = tobii_device_create(api, url, &device);
	assert(error == TOBII_ERROR_NO_ERROR);

	error = tobii_gaze_point_subscribe(device, gaze_point_callback, 0);
	assert(error == TOBII_ERROR_NO_ERROR);

	int is_running = 1000; // in this sample, exit after some iterations
	while (--is_running > 0)
	{
		error = tobii_wait_for_callbacks(NULL, 1, &device);
		assert(error == TOBII_ERROR_NO_ERROR || error == TOBII_ERROR_TIMED_OUT);

		error = tobii_device_process_callbacks(device);
		assert(error == TOBII_ERROR_NO_ERROR);
	}

	error = tobii_gaze_point_unsubscribe(device);
	assert(error == TOBII_ERROR_NO_ERROR);

	error = tobii_device_destroy(device);
	assert(error == TOBII_ERROR_NO_ERROR);

	error = tobii_api_destroy(api);
	assert(error == TOBII_ERROR_NO_ERROR);
	return 0;
}*/