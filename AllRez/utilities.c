//
//  utilities.c
//  AllRez
//
//  Created by joevt on 2022-02-19.
//

#include "utilities.h"

UInt64 MicrosecondsToAbsoluteTime(UInt64 micro) {
	/* minReplyDelay may require milliseconds instead of microseconds or nanoseconds so don't use this - use FORCEI2C instead */
	Nanoseconds nano;
	*(UInt64*)(&nano) = micro * 1000;
	AbsoluteTime absolute = NanosecondsToAbsolute(nano); // on Intel Macs this uses 1 AbsolouteTime == 1 Nanosecond
	return *(UInt64*)(&absolute);
}
