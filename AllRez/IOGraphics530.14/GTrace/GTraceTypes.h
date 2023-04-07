//
//  GTraceTypes.h
//  IOGraphics
//
//  Created by Jeremy Tran on 8/8/17.
//

#ifndef GTrace_h
#define GTrace_h

#include "MacOSMacros.h"
#include <stdint.h>
#include <sys/cdefs.h>

#pragma mark - Header Revision
#define         GTRACE_REVISION                             0x1

#pragma mark - Constants
#define         kGTraceMaximumLineCount                     8192    // @64b == 512k
#define         kGTraceDefaultLineCount                     2048    // @64b == 128K

__BEGIN_DECLS

#pragma mark - GTrace Structures
/*
 These structures must be power of 2 for performance, alignment, and the buffer-to-line calculations
 */
#pragma pack(push, 1)
typedef struct _GTraceID {
    union {
        struct {
            uint64_t    line:16;
            uint64_t    component:48;
        };
        uint64_t    u64;
    } ID;
} sGTraceID;

typedef struct _GTraceThreadInfo {
    union {
        struct {
            uint64_t    cpu:8;
            uint64_t    threadID:24;
            uint64_t    registryID:32;
        };
        uint64_t    u64;
    } TI;
} sGTraceThreadInfo;
extern int assertx00[(sizeof(sGTraceThreadInfo) == 8) - 1];

typedef struct _GTraceArgsTag {
    union {
        struct {
            uint16_t    targ[4];
        };
        uint64_t    u64;
    } TAG;
} sGTraceArgsTag;
extern int assertx01[(sizeof(sGTraceArgsTag) == 8) - 1];

typedef struct _GTraceArgs {
    union {
        uint64_t    u64s[4];
        uint32_t    u32s[8];
        uint16_t    u16s[16];
        uint8_t     u8s[32];
        char        str[32];
    } ARGS;
} sGTraceArgs;
extern int assertx02[(sizeof(sGTraceArgs) == 32) - 1];


typedef struct _GTrace {
    union {
        struct {
            uint64_t            timestamp;      // mach absolute time
            sGTraceID           traceID;        // unique ID to entry
            sGTraceThreadInfo   threadInfo;     // CPU, thread info
            sGTraceArgsTag      argsTag;        // Argument tags
            sGTraceArgs         args;           // Argument data
        };
        uint64_t            entry[8];
    } traceEntry;
} sGTrace;
#pragma pack(pop)

#if defined(__cplusplus) && defined(__clang__) && MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_14
    static_assert(sizeof(sGTrace) == 64, "sGTrace != 64 bytes");
#else
    #if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_6
        #include <AssertMacros.h>
    #else
        #ifndef __Check_Compile_Time
            #ifdef __GNUC__ 
                #define __Check_Compile_Time( expr )    \
                    extern int compile_time_assert_failed[ ( expr ) ? 1 : -1 ] __attribute__( ( unused ) )
            #else
                #define __Check_Compile_Time( expr )    \
                    extern int compile_time_assert_failed[ ( expr ) ? 1 : -1 ]
            #endif
        #endif
    #endif
    __Check_Compile_Time(sizeof(sGTrace) == 64);
#endif

__END_DECLS

#endif /* GTraceTypes_h */
