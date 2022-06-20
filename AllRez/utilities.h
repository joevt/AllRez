//
//  utilities.h
//  AllRez
//
//  Created by joevt on 2022-02-19.
//

#ifndef utilities_h
#define utilities_h

#include <MacTypes.h>
#include <mach/mach_time.h>

#ifdef __cplusplus
extern "C" {
#endif

UInt64 MicrosecondsToAbsoluteTime(UInt64 micro);
void strftimestamp(char *buf, size_t bufsize, const char *format, int decimals, uint64_t timestamp);
void testtime(void);

#ifdef __cplusplus
}
#endif

#endif /* utilities_h */
