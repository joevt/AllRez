//
//  utilities.cpp
//  AllRez
//
//  Created by joevt on 2022-02-19.
//

#include "utilities.h"
#include <ApplicationServices/ApplicationServices.h>
#include <time.h>
#include <sys/time.h>
#include <mach/mach_time.h>

//uint64_t mach_continuous_time() __attribute__((weak_import));

uint64_t mach_time() {
	if (__builtin_available(macOS 10.12, *)) {
		return mach_continuous_time();
	} else {
		return mach_absolute_time();
	}
}

UInt64 MicrosecondsToAbsoluteTime(UInt64 micro) {
	/* minReplyDelay may require milliseconds instead of microseconds or nanoseconds so don't use this - use FORCEI2C instead */
	Nanoseconds nano;
	*(UInt64*)(&nano) = micro * 1000;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	AbsoluteTime absolute = NanosecondsToAbsolute(nano); // on Intel Macs this uses 1 AbsolouteTime == 1 Nanosecond
#pragma clang diagnostic pop
	return *(UInt64*)(&absolute);
}

void strftimestamp(char *buf, size_t bufsize, const char *format, int decimals, uint64_t timestamp) {
	static uint64_t nowct = 0;
	static uint64_t nowns = 0;
	if (!nowct) {
		uint64_t bestspread = -1;
		uint64_t bestct = 0;
		struct timeval besttod;

		struct timeval tod;
		for (int i = 0; i < 100; i++) {
			nowct = mach_time();
			gettimeofday(&tod, NULL);
			uint64_t spread = mach_time() - nowct;
			if (spread < bestspread) {
				bestspread = spread;
				besttod = tod;
				bestct = nowct;
			}
			if (spread < 1000)
				break;
		}
		nowct = bestct;
		nowns = (besttod.tv_sec * 1000000 + besttod.tv_usec) * 1000;
	}

	if (decimals < 0) decimals = 0;
	else if (decimals > 9) decimals = 9;
	int64_t factor = 1;
	for (int i = decimals; i < 9; i++) {
		factor *= 10;
	}
/*
	9 = 1
	8 = 10
	7 = 100
	6 = 1000
	5 = 10000
	4 = 100000
	3 = 1000000
	2 = 10000000
	1 = 100000000
	0 = 1000000000
*/
	int64_t nsoffset = timestamp - nowct;
	int64_t thenns = nowns + nsoffset + 5LL * factor / 10; // round
	time_t thentime = thenns / 1000000000;

	struct tm thentm;
	localtime_r(&thentime, &thentm);
	size_t len = strftime(buf, bufsize, format, &thentm);
	if (decimals) snprintf(buf + len, bufsize - len, ".%0*lld", decimals, (thenns % 1000000000) / factor);
}

#if 0
void testtime(void) {
	typedef struct {
		clockid_t clockid;
		const char *clockname;
		int result1;
		int result2;
		uint64_t nano;
		timespec res;
		timespec time;
	} clockitem;
	
	#define oneclock(x) { x, #x }
	clockitem clocks[] = {
		oneclock(CLOCK_REALTIME),
		oneclock(CLOCK_MONOTONIC),
		oneclock(CLOCK_MONOTONIC_RAW),
		oneclock(CLOCK_UPTIME_RAW),
		oneclock(CLOCK_MONOTONIC_RAW_APPROX),
		oneclock(CLOCK_UPTIME_RAW_APPROX),
		oneclock(CLOCK_PROCESS_CPUTIME_ID),
		oneclock(CLOCK_THREAD_CPUTIME_ID),
		{(clockid_t)-1, NULL}
	};
	
	struct timeval tod;
	struct timezone tzp;
	gettimeofday(&tod, &tzp);
	
	for (clockitem* clock = &clocks[0]; clock->clockname; clock++) {
		clock->result1 = clock_getres(clock->clockid, &clock->res);
		clock->result2 = clock_gettime(clock->clockid, &clock->time);
		clock->nano = clock_gettime_nsec_np(clock->clockid);
	} // for clock
	
	uint64_t abs = mach_absolute_time();
	uint64_t continuous = mach_continuous_time();
	uint64_t approx = mach_approximate_time();
	uint64_t contapprox = mach_continuous_approximate_time();

	iprintf("%-32s res:%ld.%09ld time:%10ld.%06ld    = %llu\n", "timeofday", (long)0, (long)0, (long)tod.tv_sec, (long)tod.tv_usec, (uint64_t)0);
	for (clockitem* clock = &clocks[0]; clock->clockname; clock++) {
		iprintf("%-32s res:%ld.%09ld time:%10ld.%09ld = %llu\n", clock->clockname, clock->res.tv_sec, clock->res.tv_nsec, clock->time.tv_sec, clock->time.tv_nsec, clock->nano);
	} // for clock
	iprintf("%-32s res:%ld.%09ld time:%10ld.%09ld = %llu\n", "mach_absolute_time", (long)0, (long)0, (long)0, (long)0, abs);
	iprintf("%-32s res:%ld.%09ld time:%10ld.%09ld = %llu\n", "mach_continuous_time", (long)0, (long)0, (long)0, (long)0, continuous);
	iprintf("%-32s res:%ld.%09ld time:%10ld.%09ld = %llu\n", "mach_approximate_time", (long)0, (long)0, (long)0, (long)0, approx);
	iprintf("%-32s res:%ld.%09ld time:%10ld.%09ld = %llu\n", "mach_continuous_approximate_time", (long)0, (long)0, (long)0, (long)0, contapprox);
} // testtime
#endif
