#pragma once

//Joe Snider
//10/18
//
//Stream data from a Tobii. Uses Tobii's StreamEngine wrapper (on nuget)
//Starts a worker thread that syncs the public atomic values (x, y, userPresent).
//Interface should be through those values.
//Note that all calibration and etc.. is done through Tobii's app.
//
//Must call CTobii.Start() after creation:
//   CTobii instance;
//   instance.Start();
//
//Add callbacks of the form void F(double& x, double& y) that are called whenever there is new data.

#include <tobii/tobii.h>
#include <tobii/tobii_streams.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <functional>
#include <list>
#include <mutex>

using namespace std;

#ifndef TOBII_READER
#define TOBII_READER

class CTobii {
public:
	//typedefs
	typedef function<void(double, double)> CallbackType;
	//constructors
public:
	CTobii() {
		eyeX = 0.0;
		eyeY = 0.0;
		userPresent = false;
		tobiiStarted = false;
	}
	//call this after construction so the thread is kosher
	void Start() { Init(); }

	~CTobii() {
		if (tobiiStarted) {
			tobiiError = tobii_gaze_point_unsubscribe(device);
			CheckError(tobiiError);

			tobiiError = tobii_device_destroy(device);
			CheckError(tobiiError);

			tobiiError = tobii_api_destroy(api);
			CheckError(tobiiError);
		}

		tobiiStarted = false;
		if (worker.joinable()) {
			worker.join();
		}
	}

public:
	//gets and sets

	string GetURL() const { return url; }
	bool IsTobiiStarted() const { return tobiiStarted; }

public:
	//interface

	//These are updated by default. 
	atomic<double> eyeX;
	atomic<double> eyeY;
	atomic<bool> userPresent;

	//Callbacks
	//The functions on the list are called whenever there is a successful acquisition.
	//May modify the inputs (if you want to store it).
	//Anything castable to a std::function<void(double&,double&)
	//Note that std::bind wraps member types kind of nicely (for c++ at least)
	//   e.g. for a class like: class foo{ public: void f(double x, double y) }; you can pass in
	//     std::bind(&Foo::f, &foo, _1, _2);
	void AddCallback(CallbackType f) {
		callbacks.push_back(f);
	}

	void ClearCallbacks() {
		lock_guard<mutex> lock(callbackMutex);
		callbacks.clear();
	}

private:
	//helpers

	//evaluate all the callbacks in the list with the current eyeX and Y
	void EvaluateCallbacks() {
		lock_guard<mutex> lock(callbackMutex);
		for (auto it = callbacks.begin(); it != callbacks.end(); ++it) {
			(*it)(eyeX, eyeY);
		}
	}

	//Error handling. Pretty rude for now.
	void CheckError(const tobii_error_t& err) const {
		if (err != TOBII_ERROR_NO_ERROR) {
			cerr << "Error in CTobii: " << tobii_error_message(err) << " ... fatal\n" << flush;
			exit(err);
		}
	}

	//Tobii callback to parse the data. userData is a pointer the calling instance. (c-style, sigh)
	static void GazePointCallback(tobii_gaze_point_t const* gazePoint, void* userData) {
		CTobii* obj = (CTobii*)userData;
		if (gazePoint->validity == TOBII_VALIDITY_VALID) {
			obj->eyeX = gazePoint->position_xy[0];
			obj->eyeY = gazePoint->position_xy[1];
			obj->userPresent = true;
			obj->EvaluateCallbacks();
		}
		else {
			obj->userPresent = false;
		}
	}

	//The static is forced on us by Tobii.
	//pass in the whole class as user_data.
	static void URLReceiver(char const* url, void* user_data) {
		CTobii* obj = (CTobii*)user_data;
		//keep the first one found
		if (obj->url.length() > 0) { return; }
		obj->url = url;
	}

	//Start up the interface. Reports some errors.
	void Init() {
		tobiiError = tobii_api_create(&api, NULL, NULL);
		CheckError(tobiiError);

		tobiiError = tobii_enumerate_local_device_urls(api, URLReceiver, this);
		CheckError(tobiiError);
		if (url.length() == 0) {
			cerr << "Error: no Tobii was found ... exiting (more errors may show)." << flush;
			exit(-1);
		}

		tobiiError = tobii_device_create(api, url.c_str(), &device);
		CheckError(tobiiError);

		tobiiError = tobii_gaze_point_subscribe(device, CTobii::GazePointCallback, this);
		CheckError(tobiiError);

		if (tobiiStarted || worker.joinable()) {
			tobiiStarted = false;
			worker.join();
		}
		tobiiStarted = true;
		worker = thread(&CTobii::DoWork, this);
	}

	void DoWork() {
		while (tobiiStarted) {
			tobiiError = tobii_wait_for_callbacks(NULL, 1, &device);
			if(tobiiError == TOBII_ERROR_TIMED_OUT) {}
			else if (tobiiError == TOBII_ERROR_NO_ERROR) {
				CheckError(tobii_device_process_callbacks(device));
			}
			else {
				CheckError(tobiiError);
			}
		}
	}

private:
	//data members

	//tobii stuff is internal access only
	tobii_api_t * api;
	tobii_error_t tobiiError;
	tobii_device_t * device;

	bool tobiiStarted;
	string url;

	thread worker;

	list<CallbackType> callbacks;
	mutex callbackMutex;
};

#endif // !TOBII_READER
