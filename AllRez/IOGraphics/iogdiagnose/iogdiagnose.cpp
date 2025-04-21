//
//  iogdiagnose.cpp
//  iogdiagnose
//
//  Created by bparke on 12/16/16.
//  Modified by joevt on 2022-06-04.
//
/*
 State bits documentation:

 OP: Opened (bool)
 ON: Online (bool)
 US: Usable (bool)
 PS: Paging (bool)
 CS: Clamshell (bool)
 CC: Clamshell Current (bool)
 CL: Clamshell Last (bool)
 DS: System Dark (bool)
 MR: Mirrored (bool)
 SG: System Gated (bool)
 WL: IOG Workloop Gated (bool)
 NA: FB Notifications Active (bool)
 SN: WindowServer (CoreDisplay) Notified (bool)
 SS: WindowServer (CoreDisplay) Notification State (bool)
 SA: WindowServer (CoreDisplay) Pending Acknowledgement (bool)
 PP: Pending Power Change (bool)
 SP: System Power AckTo (bool)
 MS: Mux Switching (bool)               // version 6+
 MP: Mux Power Change Pending (bool)    // version 6+
 MU: Muted (bool)                       // version 6+
 MX: Pending Mux Change (bool)          // version 5-
 PPS: Pending Power State (ordinal)
 NOTIFIED: Active FB Notification Group ID (ID)
 WSAA: Active WSAA State (enum/bitfield)

 A-State (External API-State)

 CreateSharedCursor                 0x00 00 00 01
 GetPixelInformation                0x00 00 00 02
 GetCurrentDisplayMode              0x00 00 00 04
 SetStartupDisplayMode              0x00 00 00 08

 SetDisplayMode                     0x00 00 00 10
 GetInformationForDisplayMode       0x00 00 00 20
 GetDisplayModeCount                0x00 00 00 40
 GetDisplayModes                    0x00 00 00 80

 GetVRAMMapOffset                   0x00 00 01 00
 SetBounds                          0x00 00 02 00
 SetNewCursor                       0x00 00 04 00
 SetGammaTable                      0x00 00 08 00

 SetCursorVisible                   0x00 00 10 00
 SetCursorPosition                  0x00 00 20 00
 AcknowledgeNotification            0x00 00 40 00
 SetColorConvertTable               0x00 00 80 00

 SetCLUTWithEntries                 0x00 01 00 00
 ValidateDetailedTiming             0x00 02 00 00
 GetAttribute                       0x00 04 00 00
 SetAttribute                       0x00 08 00 00

 WSStartAttribute                   0x00 10 00 00
 EndConnectionChange                0x00 20 00 00
 ProcessConnectionChange            0x00 40 00 00
 RequestProbe                       0x00 80 00 00

 Close                              0x01 00 00 00
 SetProperties                      0x02 00 00 00
 SetHibernateGamma                  0x04 00 00 00

 */

// C++ headers
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>

extern char **environ;


#include <mach/mach_error.h>
#include <mach/mach_time.h>
#include <getopt.h>
#include <spawn.h>
#include <sys/wait.h>

#include "iokit.h"

#include "IOGraphicsDiagnose.h"
#include "IOGDiagnoseUtils.hpp"

#include "IOKit/graphics/GTraceTypes.hpp"
#include "IOKit/graphics/IOGraphicsTypesPrivate.h"
#include "IOKit/pwr_mgt/IOPM.h"
#include <pexpert/pexpert.h>

#include "printf.h"
#include <time.h>
#include <sys/time.h>
#include "AGDCDiagnose.h"
#include "AppleMisc.h"


#define ENABLE_TELEMETRY 1
//#define __APPLE_API_UNSTABLE

#define __kernel_data_semantics

#ifndef __options_decl
#define __options_decl(newtype, oldtype, enums...) \
typedef enum : oldtype enums __attribute__((__enum_extensibility__(open))) __attribute__((__flag_enum__)) newtype
#endif

#ifndef __kpi_deprecated
#define __kpi_deprecated(x)
#endif

#include <sys/kdebug_private.h>
#include <sys/kdebug_kernel.h>
#include <sys/kdebug.h>

#if !defined(IOGD576_1)
unsigned int kdebug_enable = -1;
void kernel_debug(uint32_t debugid, uintptr_t arg1, uintptr_t arg2,
				  uintptr_t arg3, uintptr_t arg4, uintptr_t arg5) {}
#endif

#include "IOGraphicsKTrace.h"
#include "utilities.h"

//#ifdef TARGET_CPU_X86_64

#define kFILENAME_LENGTH                64

using std::string;
using std::vector;

namespace {

#define COUNT_OF(x) \
    ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#if 0
void print_usage(const char* name)
{
    fprintf(stdout, "%s options:\n", name);

    fprintf(stdout, "\t--file | -f\n");
    fprintf(stdout, "\t\tWrite diag to file /tmp/com.apple.iokit.IOGraphics_YYYY_MM__DD_HH-MM-SS.txt (instead of stdout)\n");

    fprintf(stdout, "\n\t--binary | -b binary_file\n");

    fprintf(stdout, "\n");
    fflush(stdout);
}
#endif


void foutsep(FILE* outfile, const string title)
{
    static const char kSep[] = "---------------------------------------------------------------------------------------";
    static const int kSepLen = static_cast<int>(sizeof(kSep) - 1);
    if (title.empty()) {
        fprintf(outfile, "\n%s\n", kSep);
        return;
    }
    // Add room for spaces
    const int titlelen = static_cast<int>(title.length()) + 2;
    assert(titlelen < kSepLen);
    const int remainsep = kSepLen - titlelen;
    const int prefixsep = remainsep / 2;
    const int sufficsep = prefixsep + (remainsep & 1);

    fprintf(outfile, "%.*s %s %.*s\n\n",
            prefixsep, kSep, title.c_str(), sufficsep, kSep);
}
inline void foutsep(FILE* outfile) { foutsep(outfile, string()); }

#if 0
string stringf(const char *fmt, ...)
{
    va_list args;
    char buffer[1024]; // pretty big, but userland stacks are big

    va_start(args, fmt);
    const auto len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    assert(len < sizeof(buffer)); (void) len;
    return buffer;
}

string formatEntry(const int currentLine, const GTraceEntry& entry)
{
    return stringf(
            "\t\tTkn: %06u\tTS: %llu\tLn: %u\tC: %#llx\tCTID: %u-%#llx\t"
            "OID: %#llx\tTag: %#llx\tA: %#llx-%#llx-%#llx-%#llx\n",
            currentLine, entry.timestamp(), entry.line(), entry.component(),
            entry.cpu(), entry.threadID(), entry.registryID(),
            entry.tag(), entry.arg64(0), entry.arg64(1),
            entry.arg64(2), entry.arg64(3));
}
#endif

void agdcdiagnose(FILE * outfile)
{
#define AGCKEXT      "/System/Library/Extensions/AppleGraphicsControl.kext/"
#define BNDLBINDIR   "Contents/MacOS/"
#define AGDCDIAGEXEC "AGDCDiagnose"
    static const char* kDiagArgs[]
        = { AGCKEXT BNDLBINDIR AGDCDIAGEXEC, "-a", nullptr };

    if (static_cast<bool>(outfile)) {
        pid_t       pid = 0;
        foutsep(outfile, "AGDC REPORT"); fflush(outfile);

        auto agdcArgs = const_cast<char* const *>(kDiagArgs);
        int status = posix_spawn(&pid, kDiagArgs[0], nullptr, nullptr,
                                 agdcArgs, environ);
        if (0 == status)
            (void) waitpid(pid, &status, 0);
        else
            fprintf(outfile, "\tAGDCDiagnose failed to launch\n");
        foutsep(outfile);
    }
}

int entrycompare(const void *entry1, const void *entry2) {
	if (((GTraceEntry*)entry1)->timestamp() < ((GTraceEntry*)entry2)->timestamp())
		return -1;
	else if (((GTraceEntry*)entry1)->timestamp() > ((GTraceEntry*)entry2)->timestamp())
		return 1;
	return 0;
}

#define DBG_IOG_SOURCE_DIMENGINE_37              37
#define DBG_IOG_SOURCE_INIT_FB                   38

#define DBG_IOG_SOURCE_DO_SETUP                  40
#define DBG_IOG_SOURCE_DO_SET_DISPLAY_MODE       41
#define DBG_IOG_SOURCE_SYSWORK_RESETCLAMSHELL_V2 43
#define DBG_IOG_SOURCE_VENDOR                    44
#define DBG_IOG_SOURCE_SET_TRANSFORM             45

#define DBG_IOG_SOURCE_SET_ATTR_FOR_CONN_EXT     47
#define DBG_IOG_SOURCE_OVERSCAN                  48


const char *GetGTraceSourceStr(char *buf, int bufsize, uint64_t src) {
	const char *str;
	switch (src) {
		case DBG_IOG_SOURCE_MATCH_FRAMEBUFFER         /*  1 */ : str = "MATCH_FRAMEBUFFER"; break;
		case DBG_IOG_SOURCE_PROCESS_CONNECTION_CHANGE /*  2 */ : str = "PROCESS_CONNECTION_CHANGE"; break;
		case DBG_IOG_SOURCE_EXT_SET_DISPLAY_MODE      /*  3 */ : str = "EXT_SET_DISPLAY_MODE"; break;
		case DBG_IOG_SOURCE_EXT_SET_PROPERTIES        /*  4 */ : str = "EXT_SET_PROPERTIES"; break;
		case DBG_IOG_SOURCE_IODISPLAYWRANGLER         /*  5 */ : str = "IODISPLAYWRANGLER"; break;
		case DBG_IOG_SOURCE_IODISPLAY                 /*  6 */ : str = "IODISPLAY"; break;
		case DBG_IOG_SOURCE_STOP                      /*  7 */ : str = "STOP"; break;
		case DBG_IOG_SOURCE_CHECK_POWER_WORK          /*  8 */ : str = "CHECK_POWER_WORK"; break;
		case DBG_IOG_SOURCE_SET_AGGRESSIVENESS        /*  9 */ : str = "SET_AGGRESSIVENESS"; break;
		case DBG_IOG_SOURCE_APPLEBACKLIGHTDISPLAY     /* 10 */ : str = "APPLEBACKLIGHTDISPLAY"; break;
		case DBG_IOG_SOURCE_NEWUSERCLIENT             /* 11 */ : str = "NEWUSERCLIENT"; break;
		case DBG_IOG_SOURCE_OPEN                      /* 12 */ : str = "OPEN"; break;
		case DBG_IOG_SOURCE_CLOSE                     /* 13 */ : str = "CLOSE"; break;
		case DBG_IOG_SOURCE_FINDCONSOLE               /* 14 */ : str = "FINDCONSOLE"; break;
		case DBG_IOG_SOURCE_SUSPEND                   /* 15 */ : str = "SUSPEND"; break;
		case DBG_IOG_SOURCE_UPDATE_GAMMA_TABLE        /* 16 */ : str = "UPDATE_GAMMA_TABLE"; break;
		case DBG_IOG_SOURCE_DEFERRED_CLUT             /* 17 */ : str = "DEFERRED_CLUT"; break;
		case DBG_IOG_SOURCE_RESTORE_FRAMEBUFFER       /* 18 */ : str = "RESTORE_FRAMEBUFFER"; break;
		case DBG_IOG_SOURCE_EXT_GET_ATTRIBUTE         /* 19 */ : str = "EXT_GET_ATTRIBUTE"; break;
		case DBG_IOG_SOURCE_GET_ATTRIBUTE             /* 20 */ : str = "GET_ATTRIBUTE"; break;
		case DBG_IOG_SOURCE_POSTWAKE                  /* 21 */ : str = "POSTWAKE"; break;
		case DBG_IOG_SOURCE_END_CONNECTION_CHANGE     /* 22 */ : str = "END_CONNECTION_CHANGE"; break;
		case DBG_IOG_SOURCE_SYSWILLPOWERON            /* 23 */ : str = "SYSWILLPOWERON"; break;
		case DBG_IOG_SOURCE_IOGETHWCLAMSHELL          /* 24 */ : str = "IOGETHWCLAMSHELL"; break;
		case DBG_IOG_SOURCE_CLAMSHELL_HANDLER         /* 25 */ : str = "CLAMSHELL_HANDLER"; break;
		case DBG_IOG_SOURCE_READ_CLAMSHELL            /* 26 */ : str = "READ_CLAMSHELL"; break;
		case DBG_IOG_SOURCE_RESET_CLAMSHELL           /* 27 */ : str = "RESET_CLAMSHELL"; break;
		case DBG_IOG_SOURCE_SYSWORK_READCLAMSHELL     /* 28 */ : str = "SYSWORK_READCLAMSHELL"; break;
		case DBG_IOG_SOURCE_SYSWORK_RESETCLAMSHELL    /* 29 */ : str = "SYSWORK_RESETCLAMSHELL"; break;
		case DBG_IOG_SOURCE_SYSWORK_ENABLECLAMSHELL   /* 30 */ : str = "SYSWORK_ENABLECLAMSHELL"; break;
		case DBG_IOG_SOURCE_SYSWORK_PROBECLAMSHELL    /* 31 */ : str = "SYSWORK_PROBECLAMSHELL"; break;
		case DBG_IOG_SOURCE_CLAMSHELL_OFFLINE_CHANGE  /* 32 */ : str = "CLAMSHELL_OFFLINE_CHANGE"; break;
		case DBG_IOG_SOURCE_UPDATE_ONLINE             /* 33 */ : str = "UPDATE_ONLINE"; break;
		case DBG_IOG_SOURCE_GLOBAL_CONNECTION_COUNT   /* 34 */ : str = "GLOBAL_CONNECTION_COUNT"; break;
		case DBG_IOG_SOURCE_SERVER_ACK_TIMEOUT        /* 35 */ : str = "SERVER_ACK_TIMEOUT"; break;
		case DBG_IOG_SOURCE_DIM_DISPLAY_TIMEOUT       /* 36 */ : str = "?DIM_DISPLAY_TIMEOUT"; break;
		case DBG_IOG_SOURCE_DIMENGINE_37              /* 37 */ : str = "?DIMENGINE_37"; break;
		case DBG_IOG_SOURCE_INIT_FB                   /* 38 */ : str = "INIT_FB"; break;

		case DBG_IOG_SOURCE_DO_SETUP                  /* 40 */ : str = "DO_SETUP"; break;
		case DBG_IOG_SOURCE_DO_SET_DISPLAY_MODE       /* 41 */ : str = "DO_SET_DISPLAY_MODE"; break;
			
		case DBG_IOG_SOURCE_SYSWORK_RESETCLAMSHELL_V2 /* 43 */ : str = "SYSWORK_RESETCLAMSHELL_V2"; break;
		case DBG_IOG_SOURCE_VENDOR                    /* 44 */ : str = "VENDOR"; break;
		case DBG_IOG_SOURCE_SET_TRANSFORM             /* 45 */ : str = "SET_TRANSFORM"; break;

		case DBG_IOG_SOURCE_SET_ATTR_FOR_CONN_EXT     /* 47 */ : str = "SET_ATTR_FOR_CONN_EXT"; break;
		case DBG_IOG_SOURCE_OVERSCAN                  /* 48 */ : str = "OVERSCAN"; break;
		
		default                                                : str = buf; snprintf(buf, bufsize, "?0x%llx", src);
	}
	return str;
}


#define DBG_IOG_SET_PROPERTIES              51
#define DBG_IOG_SET_SYSTEM_DIM_STATE_52     52
#define DBG_IOG_DIMENGINE_53                53
#define DBG_IOG_TIMELOCK                    54
#define DBG_IOG_CURSORLOCK                  55
#define DBG_IOG_LOG_SYNCH                   56

#define DBG_IOG_CP_INTERUPT_58              58
#define DBG_IOG_EXT_GET_ATTRIBUTE_59        59
#define DBG_IOG_FB_EXT_CLOSE                60
#define DBG_IOG_MSG_CONNECT_CHANGE          61
#define DBG_IOG_CAPTURED_RETRAIN            62
#define DBG_IOG_CONNECT_WORK_ASYNC          63
#define DBG_IOG_MUX_ACTIVITY_CHANGE         64
#define DBG_IOG_SET_ATTR_FOR_CONN_EXT       65
#define DBG_IOG_EXT_END_CONNECT_CHANGE      66
#define DBG_IOG_EXT_PROCESS_CONNECT_CHANGE  67
#define DBG_IOG_ASYNC_WORK                  68
#define DBG_IOG_SYSTEM_WORK                 69
#define DBG_IOG_BUILTIN_PANEL_POWER         70
#define DBG_IOG_CONNECT_CHANGE_INTERRUPT_V2 71

#define ABL_SET_BRIGHTNESS                 256
#define ABL_SET_DISPLAY_POWER              260
#define ABL_SET_BRIGHTNESS_PROBE           261
#define ABL_DO_UPDATE                      262
#define ABL_SET_DISPLAY                    263
#define ABL_COMMIT                         264

const char *GetGTraceTagStr(char *tagbuf, int tagsize, int tag) {
	const char *str;
	switch (tag) {
		case DBG_BUFFER_BRACKET                  /*   9 */ : str = "BUFFER_BRACKET"; break;
		case DBG_IOG_NOTIFY_SERVER               /*  10 */ : str = "IOG_NOTIFY_SERVER"; break;
		case DBG_IOG_SERVER_ACK                  /*  11 */ : str = "IOG_SERVER_ACK"; break;
		case DBG_IOG_VRAM_RESTORE                /*  12 */ : str = "IOG_VRAM_RESTORE"; break;
		case DBG_IOG_VRAM_BLACK                  /*  13 */ : str = "IOG_VRAM_BLACK"; break;
		case DBG_IOG_WSAA_DEFER_ENTER            /*  14 */ : str = "IOG_WSAA_DEFER_ENTER"; break;
		case DBG_IOG_WSAA_DEFER_EXIT             /*  15 */ : str = "IOG_WSAA_DEFER_EXIT"; break;
		case DBG_IOG_SET_POWER_STATE             /*  16 */ : str = "IOG_SET_POWER_STATE"; break;
		case DBG_IOG_SYSTEM_POWER_CHANGE         /*  17 */ : str = "IOG_SYSTEM_POWER_CHANGE"; break;
		case DBG_IOG_ACK_POWER_STATE             /*  18 */ : str = "IOG_ACK_POWER_STATE"; break;
		case DBG_IOG_SET_POWER_ATTRIBUTE         /*  19 */ : str = "IOG_SET_POWER_ATTRIBUTE"; break;
		case DBG_IOG_ALLOW_POWER_CHANGE          /*  20 */ : str = "IOG_ALLOW_POWER_CHANGE"; break;
		case DBG_IOG_MUX_ALLOW_POWER_CHANGE      /*  21 */ : str = "IOG_MUX_ALLOW_POWER_CHANGE"; break;
		case DBG_IOG_SERVER_TIMEOUT              /*  22 */ : str = "IOG_SERVER_TIMEOUT"; break;
		case DBG_IOG_NOTIFY_CALLOUT              /*  23 */ : str = "IOG_NOTIFY_CALLOUT"; break;
		case DBG_IOG_MUX_POWER_MESSAGE           /*  24 */ : str = "IOG_MUX_POWER_MESSAGE"; break;
		case DBG_IOG_FB_POWER_CHANGE             /*  25 */ : str = "IOG_FB_POWER_CHANGE"; break;
		case DBG_IOG_WAKE_FROM_DOZE              /*  26 */ : str = "IOG_WAKE_FROM_DOZE"; break;
		case DBG_IOG_RECEIVE_POWER_NOTIFICATION  /*  27 */ : str = "IOG_RECEIVE_POWER_NOTIFICATION"; break;
		case DBG_IOG_CHANGE_POWER_STATE_PRIV     /*  28 */ : str = "IOG_CHANGE_POWER_STATE_PRIV"; break;
		case DBG_IOG_CLAMP_POWER_ON              /*  29 */ : str = "IOG_CLAMP_POWER_ON"; break;
		case DBG_IOG_SET_TIMER_PERIOD            /*  30 */ : str = "IOG_SET_TIMER_PERIOD"; break;
		case DBG_IOG_HANDLE_EVENT                /*  31 */ : str = "IOG_HANDLE_EVENT"; break;
		case DBG_IOG_SET_ATTRIBUTE_EXT           /*  32 */ : str = "IOG_SET_ATTRIBUTE_EXT"; break;
		case DBG_IOG_CLAMSHELL                   /*  33 */ : str = "IOG_CLAMSHELL"; break;
		case DBG_IOG_HANDLE_VBL_INTERRUPT        /*  35 */ : str = "IOG_HANDLE_VBL_INTERRUPT"; break;
		case DBG_IOG_WAIT_QUIET                  /*  36 */ : str = "IOG_WAIT_QUIET"; break;
		case DBG_IOG_PLATFORM_CONSOLE            /*  37 */ : str = "IOG_PLATFORM_CONSOLE"; break;
		case DBG_IOG_CONSOLE_CONFIG              /*  38 */ : str = "IOG_CONSOLE_CONFIG"; break;
		case DBG_IOG_VRAM_CONFIG                 /*  39 */ : str = "IOG_VRAM_CONFIG"; break;
		case DBG_IOG_SET_GAMMA_TABLE             /*  40 */ : str = "IOG_SET_GAMMA_TABLE"; break;
		case DBG_IOG_NEW_USER_CLIENT             /*  41 */ : str = "IOG_NEW_USER_CLIENT"; break;
		case DBG_IOG_FB_CLOSE                    /*  42 */ : str = "IOG_FB_CLOSE"; break;
		case DBG_IOG_NOTIFY_CALLOUT_TIMEOUT      /*  43 */ : str = "IOG_NOTIFY_CALLOUT_TIMEOUT"; break;
		case DBG_IOG_CONNECTION_ENABLE           /*  44 */ : str = "IOG_CONNECTION_ENABLE"; break;
		case DBG_IOG_CONNECTION_ENABLE_CHECK     /*  45 */ : str = "IOG_CONNECTION_ENABLE_CHECK"; break;
		case DBG_IOG_PROCESS_CONNECT_CHANGE      /*  46 */ : str = "IOG_PROCESS_CONNECT_CHANGE"; break;
		case DBG_IOG_CONNECT_CHANGE_INTERRUPT    /*  47 */ : str = "IOG_CONNECT_CHANGE_INTERRUPT"; break;
		case DBG_IOG_DELIVER_NOTIFY              /*  48 */ : str = "IOG_DELIVER_NOTIFY"; break;
		case DBG_IOG_AGC_MSG                     /*  49 */ : str = "IOG_AGC_MSG"; break;
		case DBG_IOG_AGC_MUTE                    /*  50 */ : str = "IOG_AGC_MUTE"; break;

		case DBG_IOG_SET_PROPERTIES              /*  51 */ : str = "IOG_SET_PROPERTIES"; break; // string, string, string
		case DBG_IOG_SET_SYSTEM_DIM_STATE_52     /*  52 */ : str = "¿IOG_SET_SYSTEM_DIM_STATE_52"; break;
		case DBG_IOG_DIMENGINE_53                /*  53 */ : str = "¿IOG_DIMENGINE_53"; break;
		case DBG_IOG_TIMELOCK                    /*  54 */ : str = "IOG_TIMELOCK"; break; // string, string, string
		case DBG_IOG_CURSORLOCK                  /*  55 */ : str = "IOG_CURSORLOCK"; break; // string, string, string
		case DBG_IOG_LOG_SYNCH                   /*  56 */ : str = "IOG_LOG_SYNCH"; break;

		case DBG_IOG_CP_INTERUPT_58              /*  58 */ : str = "¿IOG_CP_INTERUPT_58"; break;
		case DBG_IOG_EXT_GET_ATTRIBUTE_59        /*  59 */ : str = "¿IOG_EXT_GET_ATTRIBUTE_59"; break;
		case DBG_IOG_FB_EXT_CLOSE                /*  60 */ : str = "IOG_FB_EXT_CLOSE"; break;
		case DBG_IOG_MSG_CONNECT_CHANGE          /*  61 */ : str = "IOG_MSG_CONNECT_CHANGE"; break;
		case DBG_IOG_CAPTURED_RETRAIN            /*  62 */ : str = "IOG_CAPTURED_RETRAIN"; break;
		case DBG_IOG_CONNECT_WORK_ASYNC          /*  63 */ : str = "IOG_CONNECT_WORK_ASYNC"; break;
		case DBG_IOG_MUX_ACTIVITY_CHANGE         /*  64 */ : str = "IOG_MUX_ACTIVITY_CHANGE"; break;
		case DBG_IOG_SET_ATTR_FOR_CONN_EXT       /*  65 */ : str = "IOG_SET_ATTR_FOR_CONN_EXT"; break;
		case DBG_IOG_EXT_END_CONNECT_CHANGE      /*  66 */ : str = "IOG_EXT_END_CONNECT_CHANGE"; break;
		case DBG_IOG_EXT_PROCESS_CONNECT_CHANGE  /*  67 */ : str = "IOG_EXT_PROCESS_CONNECT_CHANGE"; break;
		case DBG_IOG_ASYNC_WORK                  /*  68 */ : str = "IOG_ASYNC_WORK"; break;
		case DBG_IOG_SYSTEM_WORK                 /*  69 */ : str = "IOG_SYSTEM_WORK"; break;
		case DBG_IOG_BUILTIN_PANEL_POWER         /*  70 */ : str = "IOG_BUILTIN_PANEL_POWER"; break;
		case DBG_IOG_CONNECT_CHANGE_INTERRUPT_V2 /*  71 */ : str = "IOG_CONNECT_CHANGE_INTERRUPT_V2"; break;

		case DBG_IOG_SET_DISPLAY_MODE            /* 100 */ : str = "IOG_SET_DISPLAY_MODE"; break;
		case DBG_IOG_SET_DETAILED_TIMING         /* 101 */ : str = "IOG_SET_DETAILED_TIMING"; break;
		case DBG_IOG_GET_CURRENT_DISPLAY_MODE    /* 102 */ : str = "IOG_GET_CURRENT_DISPLAY_MODE"; break;
			
		case ABL_SET_BRIGHTNESS                  /* 256 */ : str = "ABL_SET_BRIGHTNESS"; break;
		case ABL_SET_DISPLAY_POWER               /* 260 */ : str = "ABL_SET_DISPLAY_POWER"; break;
		case ABL_SET_BRIGHTNESS_PROBE            /* 261 */ : str = "ABL_SET_BRIGHTNESS_PROBE"; break;
		case ABL_DO_UPDATE                       /* 262 */ : str = "ABL_DO_UPDATE"; break;
		case ABL_SET_DISPLAY                     /* 263 */ : str = "ABL_SET_DISPLAY"; break;
		case ABL_COMMIT                          /* 264 */ : str = "ABL_COMMIT"; break;
		default                                            : str = tagbuf; snprintf(tagbuf, tagsize, "?0x%x", tag);
	}
	return str;
}

const char *GetTagTypeStr(char *buf, int bufsize, uint16_t val) {
	const char *str;
	switch (val) {
		case kGTRACE_ARGUMENT_Reserved1 : str = "Reserved1"; break;
		case kGTRACE_ARGUMENT_STRING    : str = "STRING"; break;
		case kGTRACE_ARGUMENT_BESTRING  : str = "BESTRING"; break;
		case 0x8000                     : str = "POINTER"; break;
		default                         : str = buf; snprintf(buf, bufsize, "?%#x", val);
	}
	return str;
}

const char *GetPowerStr(char *buf, int bufsize, uint64_t val) {
	const char *str;
	switch (val) {
		case kIOFBNS_Sleep : str = "Sleep"; break;
		case kIOFBNS_Wake  : str = "Wake"; break;
		case kIOFBNS_Doze  : str = "Doze"; break;
		case kIOFBNS_Dim   : str = "Dim"; break;
		case kIOFBNS_UnDim : str = "UnDim"; break;
		default            : str = buf; snprintf(buf, bufsize, "?0x%llx", val);
	}
	return str;
}


const char *GetNotifyServerMessageStr(char *buf, int bufsize, uint64_t val) {
	const char *str = buf;
	char powerStr[20];
	
	if ((val & 0x0ffffffff) == kIOFBNS_Rendezvous)
		snprintf(buf, bufsize, "Rendezvous%s%s",
			val & 0xffffffff00000000ULL ? " " : "",
			val & 0xffffffff00000000ULL ? UNKNOWN_VALUE(val & 0xffffffff00000000ULL) : ""
		);
	else {
		snprintf(buf, bufsize, "Message:%s DisplayState:%s Generation:%lld%s%s",
			GetPowerStr(powerStr, sizeof(powerStr), val & kIOFBNS_MessageMask),
			(val & kIOFBNS_DisplayStateMask) >> kIOFBNS_DisplayStateShift == 0                                  ? "none" :
			(val & kIOFBNS_DisplayStateMask) >> kIOFBNS_DisplayStateShift == kIOFBDisplayState_AlreadyActive    ? "AlreadyActive" :
			(val & kIOFBNS_DisplayStateMask) >> kIOFBNS_DisplayStateShift == kIOFBDisplayState_RestoredProfile  ? "RestoredProfile" :
			(val & kIOFBNS_DisplayStateMask) >> kIOFBNS_DisplayStateShift == kIOFBDisplayState_PipelineBlack    ? "PipelineBlack" :
			UNKNOWN_VALUE((val & kIOFBNS_DisplayStateMask) >> kIOFBNS_DisplayStateShift),
		
			(val & kIOFBNS_GenerationMask) >> kIOFBNS_GenerationShift,
			
			val & 0xffffffff8000f0f0ULL ? " " : "",
			val & 0xffffffff8000f0f0ULL ? UNKNOWN_VALUE(val & 0xffffffff8000f0f0ULL) : ""
		);
	}
	return str;
}

const char *GetWSAAStateStr(char *buf, int bufsize, uint64_t val) {
	const char *str;
	switch (val) {
		case kIOWSAA_Unaccelerated    : str = "Unaccelerated"; break;
		case kIOWSAA_Accelerated      : str = "Accelerated"; break;
		case kIOWSAA_From_Accelerated : str = "From_Accelerated"; break;
		case kIOWSAA_To_Accelerated   : str = "To_Accelerated"; break;
		case kIOWSAA_Sleep            : str = "Sleep"; break;
		case kIOWSAA_DriverOpen       : str = "DriverOpen"; break;
		case kIOWSAA_StateMask        : str = "StateMask"; break;
		default            : str = buf; snprintf(buf, bufsize, "?0x%llx", val);
	}
	return str;
}

const char *GetWSAAStr(char *buf, int bufsize, uint64_t val) {
	const char *str = buf;
	char stateStr[20];
	int inc = snprintf(buf, bufsize, "wsaaState:%s wsaaAttributes:%s%s%s%s%s%s",
		GetWSAAStateStr(stateStr, sizeof(stateStr), val & kIOWSAA_StateMask),
		val & kIOWSAA_Transactional    ? "Transactional," : "",
		val & kIOWSAA_DeferStart       ? "DeferStart," : "",
		val & kIOWSAA_DeferEnd         ? "DeferEnd," : "",
		val & kIOWSAA_NonConsoleDevice ? "NonConsoleDevice," : "",
		val & 0xfffffffffffff8e0ULL ? UNKNOWN_VALUE(val & 0xfffffffffffff8e0ULL) : "",
		val & 0xfffffffffffff8e0ULL ? "," : ""
	);
	buf[inc-1] = '\0';
	return str;
}

const char *GetChangeFlagsStr(char *buf, int bufsize, uint64_t val) {
	const char *str;
	switch (val) {
		case kIOPMSystemCapabilityWillChange : str = "SystemCapabilityWillChange"; break;
		case kIOPMSystemCapabilityDidChange  : str = "SystemCapabilityDidChange"; break;
		default : str = buf; snprintf(buf, bufsize, "?0x%llx", val);
	}
	return str;
}

const char *GetSystemCapabilityStr(char *buf, int bufsize, uint64_t val) {
	const char *str;
	switch (val) {
		case kIOPMSystemCapabilityCPU      : str = "kIOPMSystemCapabilityCPU"; break;
		case kIOPMSystemCapabilityGraphics : str = "kIOPMSystemCapabilityGraphics"; break;
		case kIOPMSystemCapabilityAudio    : str = "kIOPMSystemCapabilityAudio"; break;
		case kIOPMSystemCapabilityNetwork  : str = "kIOPMSystemCapabilityNetwork"; break;
		default : str = buf; snprintf(buf, bufsize, "?0x%llx", val);
	}
	return str;
}

const char *DumpOneReturn32(char *buf, int bufsize, uint64_t val) {
	if ((val & 0xffffffff00000000ULL) == 0x8000000000000000ULL) {
		return DumpOneReturn(buf, bufsize, (int32_t)val);
	}
	snprintf(buf, bufsize, "?0x%llx", val);
	return buf;
}

const char *GetMessageStr(char *buf, int bufsize, uint64_t val) {
	const char *str;
	
	switch (val) {
		case kIOReturnSuccess: str = buf; snprintf(buf, bufsize, "?%#llx", val); break;
		default: str = GetIOReturnStr(buf, bufsize, val);
	}
	return str;
}

const char *GetExitTypeStr(char *buf, int bufsize, int64_t val) {
	// -1 (success early exit 1),-2 (success early exit 2), 0 (final exit)
	const char *str;
	switch (val) {
		case  0 : str = "FinalExit"; break;
		case -1 : str = "EarlyExit1"; break;
		case -2 : str = "EarlyExit2"; break;
		default : str = buf; snprintf(buf, bufsize, "?0x%llx", val);
	}
	return str;
}

#define DBG_IOG_PWR_EVENT_DISPLAYASSERTIONCREATED 5

const char *GetPowerEventStr(char *buf, int bufsize, uint64_t val) {
	// -1 (success early exit 1),-2 (success early exit 2), 0 (final exit)
	const char *str;
	switch (val) {
		case DBG_IOG_PWR_EVENT_DESKTOPMODE             : str = "DESKTOPMODE"; break;
		case DBG_IOG_PWR_EVENT_DISPLAYONLINE           : str = "DISPLAYONLINE"; break;
		case DBG_IOG_PWR_EVENT_SYSTEMPWRCHANGE         : str = "SYSTEMPWRCHANGE"; break;
		case DBG_IOG_PWR_EVENT_PROCCONNECTCHANGE       : str = "PROCCONNECTCHANGE"; break;
		case DBG_IOG_PWR_EVENT_DISPLAYASSERTIONCREATED : str = "DISPLAYASSERTIONCREATED"; break;
		default : str = buf; snprintf(buf, bufsize, "?0x%llx", val);
	}
	return str;
}

const char *GetClamshellStateStr(char *buf, int bufsize, uint64_t val) {
	// -1 (success early exit 1),-2 (success early exit 2), 0 (final exit)
	const char *str;
	switch (val) {
		case DBG_IOG_CLAMSHELL_STATE_NOT_PRESENT : str = "NOT_PRESENT"; break;
		case DBG_IOG_CLAMSHELL_STATE_CLOSED      : str = "CLOSED"; break;
		case DBG_IOG_CLAMSHELL_STATE_OPEN        : str = "OPEN"; break;
		default : str = buf; snprintf(buf, bufsize, "?0x%llx", val);
	}
	return str;
}

const char *GetPMStr(char *buf, int bufsize, uint64_t val) {
	const char *str = buf;
	int inc = snprintf(buf, bufsize, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		val & kIOPMSleepNow             ? "SleepNow," : "",
		val & kIOPMAllowSleep           ? "AllowSleep," : "",
		val & kIOPMPreventSleep         ? "PreventSleep," : "",
		val & kIOPMPowerButton          ? "PowerButton," : "",
		val & kIOPMClamshellClosed      ? "ClamshellClosed," : "",
		val & kIOPMPowerEmergency       ? "PowerEmergency," : "",
		val & kIOPMDisableClamshell     ? "DisableClamshell," : "",
		val & kIOPMEnableClamshell      ? "EnableClamshell," : "",
		val & kIOPMProcessorSpeedChange ? "ProcessorSpeedChange," : "",
		val & kIOPMOverTemp             ? "OverTemp," : "",
		val & kIOPMClamshellOpened      ? "ClamshellOpened," : "",
		val & kIOPMDWOverTemp           ? "DWOverTemp," : "",
		val & kIOPMPowerButtonUp        ? "PowerButtonUp," : "",
		val & kIOPMProModeEngaged       ? "ProModeEngaged," : "",
		val & kIOPMProModeDisengaged    ? "ProModeDisengaged," : "",
		val & 0xffffffffffffc000ULL ? UNKNOWN_VALUE(val & 0xffffffffffffc000ULL) : "",
		val & 0xffffffffffffc000ULL ? "," : ""
	);
	buf[inc-1] = '\0';
	return str;
}

const char *GetIOFBNotifyStr(char *buf, int bufsize, uint64_t val) {
	const char *str;
	switch (val) {
		case kIOFBNotifyDisplayModeWillChange : str = "DisplayModeWillChange"; break;
		case kIOFBNotifyDisplayModeDidChange  : str = "DisplayModeDidChange"; break;
		case kIOFBNotifyWillSleep             : str = "WillSleep"; break;
		case kIOFBNotifyDidWake               : str = "DidWake"; break;
//		case kIOFBNotifyDidPowerOff           : str = "DidPowerOff"; break;
//		case kIOFBNotifyWillPowerOn           : str = "WillPowerOn"; break;
		case kIOFBNotifyDidSleep              : str = "DidSleep"; break; // kIOFBNotifyDidPowerOff
		case kIOFBNotifyWillWake              : str = "WillWake"; break; //kIOFBNotifyWillPowerOn
		case kIOFBNotifyWillPowerOff          : str = "WillPowerOff"; break;
		case kIOFBNotifyDidPowerOn            : str = "DidPowerOn"; break;
		case kIOFBNotifyWillChangeSpeed       : str = "WillChangeSpeed"; break;
		case kIOFBNotifyDidChangeSpeed        : str = "DidChangeSpeed"; break;
		case kIOFBNotifyHDACodecWillPowerOn   : str = "HDACodecWillPowerOn"; break;
		case kIOFBNotifyHDACodecDidPowerOn    : str = "HDACodecDidPowerOn"; break;
		case kIOFBNotifyHDACodecWillPowerOff  : str = "HDACodecWillPowerOff"; break;
		case kIOFBNotifyHDACodecDidPowerOff   : str = "HDACodecDidPowerOff"; break;
		case kIOFBNotifyClamshellChange       : str = "ClamshellChange"; break;
		case kIOFBNotifyCaptureChange         : str = "CaptureChange"; break;
		case kIOFBNotifyOnlineChange          : str = "OnlineChange"; break;
		case kIOFBNotifyDisplayDimsChange     : str = "DisplayDimsChange"; break;
		case kIOFBNotifyProbed                : str = "Probed"; break;
		case kIOFBNotifyVRAMReady             : str = "VRAMReady"; break;
		case kIOFBNotifyWillNotify            : str = "WillNotify"; break;
		case kIOFBNotifyDidNotify             : str = "DidNotify"; break;
		case kIOFBNotifyWSAAWillEnterDefer    : str = "WSAAWillEnterDefer"; break;
		case kIOFBNotifyWSAAWillExitDefer     : str = "WSAAWillExitDefer"; break;
		case kIOFBNotifyWSAADidEnterDefer     : str = "WSAADidEnterDefer"; break;
		case kIOFBNotifyWSAADidExitDefer      : str = "WSAADidExitDefer"; break;
//		case kIOFBNotifyWSAAEnterDefer        : str = "WSAAEnterDefer"; break; // kIOFBNotifyWSAADidEnterDefer
//		case kIOFBNotifyWSAAExitDefer         : str = "WSAAExitDefer"; break; // kIOFBNotifyWSAADidExitDefer
		case kIOFBNotifyTerminated            : str = "Terminated"; break;
		default : str = buf; snprintf(buf, bufsize, "?0x%llx", val);
	}
	return str;
}

const char *GetBitsStr(char *buf, int bufsize, uint64_t val) {
	const char *str = buf;
	snprintf(buf, bufsize, "CurrentClamshellState:%s LastReadClamshellState:%s probeNow:%s LidOpenMode:%s hasExtDisplayChangedWhileClosed:%s hasLidChanged:%s%s%s",
		(val & (1ULL << 0)) ? "true" : "false",
		(val & (1ULL << 1)) ? "true" : "false",
		(val & (1ULL << 2)) ? "true" : "false",
		(val & (1ULL << 3)) ? "true" : "false",
		(val & (1ULL << 4)) ? "true" : "false",
		(val & (1ULL << 5)) ? "true" : "false",
		(val & 0xffffffffffffffc0ULL) ? " Unknown:" : "",
		(val & 0xffffffffffffffc0ULL) ? UNKNOWN_VALUE(val & 0xffffffffffffffc0ULL) : ""
	);
	return str;
}

const char *GetStateStr(char *buf, int bufsize, uint64_t val) {
	const char *str = buf;
	snprintf(buf, bufsize, "DbgNoClamshellOffline:%s BuiltInPanel:%s captured:%s isMuted:%s CurrentClamshellState:%s online:%s LidOpenMode:%s%s%s",
		(val & (1ULL << 0)) ? "true" : "false",
		(val & (1ULL << 1)) ? "true" : "false",
		(val & (1ULL << 2)) ? "true" : "false",
		(val & (1ULL << 3)) ? "true" : "false",
		(val & (1ULL << 4)) ? "true" : "false",
		(val & (1ULL << 5)) ? "true" : "false",
		(val & (1ULL << 6)) ? "true" : "false",
		(val & 0xffffffffffffff80ULL) ? " Unknown:" : "",
		(val & 0xffffffffffffff80ULL) ? UNKNOWN_VALUE(val & 0xffffffffffffff80ULL) : ""
	);
	return str;
}

const char *GetCountsStr(char *buf, int bufsize, uint64_t val) {
	const char *str = buf;
	snprintf(buf, bufsize, "BacklightDisplayCount:%d DisplayCount:%d FBLastDisplayCount:%d%s%s",
		GUNPACKUINT8T(2, val),
		GUNPACKUINT8T(1, val),
		GUNPACKUINT8T(0, val),
		(val & 0xffffffffff000000ULL) ? " Unknown:" : "",
		(val & 0xffffffffff000000ULL) ? UNKNOWN_VALUE(val & 0xffffffffff000000ULL) : ""
	);
	return str;
}

#ifndef kPERefreshBootGraphics
#define kPERefreshBootGraphics  9
#endif

const char *GetOpStr(char *buf, int bufsize, uint64_t val) {
	const char *str;
	switch (val) {
		case kPEGraphicsMode        : str = "kPEGraphicsMode"; break;
		case kPETextMode            : str = "kPETextMode"; break;
		case kPETextScreen          : str = "kPETextScreen"; break;
		case kPEAcquireScreen       : str = "kPEAcquireScreen"; break;
		case kPEReleaseScreen       : str = "kPEReleaseScreen"; break;
		case kPEEnableScreen        : str = "kPEEnableScreen"; break;
		case kPEDisableScreen       : str = "kPEDisableScreen"; break;
		case kPEBaseAddressChange   : str = "kPEBaseAddressChange"; break;
		case kPERefreshBootGraphics : str = "kPERefreshBootGraphics"; break;
		default : str = buf; snprintf(buf, bufsize, "?0x%llx", val);
	}
	return str;
}

const char *GetTypeStr(char *buf, int bufsize, uint64_t val) {
	const char *str;
	switch (val) {
		case kIOFBServerConnectType  : str = "kIOFBServerConnectType"; break;
		case kIOFBSharedConnectType  : str = "kIOFBSharedConnectType"; break;
		case kIOGDiagnoseGTraceType  : str = "kIOGDiagnoseGTraceType"; break;
		case kIOGDiagnoseConnectType : str = "kIOGDiagnoseConnectType"; break;
		default : str = buf; snprintf(buf, bufsize, "?0x%llx", val);
	}
	return str;
}

const char *GetLocationStr(char *buf, int bufsize, uint64_t val) {
	const char *str;
	switch (val) {
		case 0 : str = "normal"; break;
		case 1 : str = "diagnostic"; break;
		case 2 : str = "waitQuiet"; break;
		default : str = buf; snprintf(buf, bufsize, "?0x%llx", val);
	}
	return str;
}

const char *GetSwitchStateStr(char *buf, int bufsize, uint64_t val) {
	const char *str;
	switch (val) {
		case gMux_WillSwitch   : str = "WillSwitch"; break;
		case gMux_DidSwitch    : str = "DidSwitch"; break;
		case gMux_DidNotSwitch : str = "DidNotSwitch"; break;
		default : str = buf; snprintf(buf, bufsize, "?0x%llx", val);
	}
	return str;
}

const char *GetProcessConnectChangeModeStr(char *buf, int bufsize, uint64_t val) {
	const char *str;
	switch (val) {
		case 0     : str = "0"; break;
		case fg    : str = "fg"; break;
		case bg    : str = "bg"; break;
		case fgOff : str = "fgOff"; break;
		case bgOff : str = "bgOff"; break;
		default : str = buf; snprintf(buf, bufsize, "?0x%llx", val);
	}
	return str;
}

const char *GetWorkStr(char *buf, int bufsize, uint64_t val) {
	const char *str = buf;
	int inc = snprintf(buf, bufsize, "%s%s%s%s%s",
		val & kWorkStateChange ? "kWorkStateChange," : "",
		val & kWorkPower       ? "kWorkPower," : "",
		val & kWorkSuspend     ? "kWorkSuspend," : "",
		val & ~7LL ? UNKNOWN_VALUE(val & ~7LL) : "",
		val & ~7LL ? "," : ""
	);
	buf[inc-1] = '\0';
	return str;
}

const char *GetAllStateStr(char *buf, int bufsize, uint64_t val) {
	const char *str = buf;
	int inc = snprintf(buf, bufsize, "%s%s%s%s%s%s%s%s%s%s%s%s%s",
		val & kIOFBDidWork             ? "kIOFBDidWork," : "",
		val & kIOFBWorking             ? "kIOFBWorking," : "",
		val & kIOFBPaging              ? "kIOFBPaging," : "",
		val & kIOFBWsWait              ? "kIOFBWsWait," : "",
		val & kIOFBDimmed              ? "kIOFBDimmed," : "",
		val & kIOFBServerSentPowerOff  ? "kIOFBServerSentPowerOff," : "",
		val & kIOFBServerAckedPowerOn  ? "kIOFBServerAckedPowerOn," : "",
		val & kIOFBServerAckedPowerOff ? "kIOFBServerAckedPowerOff," : "",
		val & kIOFBCaptured            ? "kIOFBCaptured," : "",
		val & kIOFBDimDisable          ? "kIOFBDimDisable," : "",
		val & kIOFBDisplaysChanging    ? "kIOFBDisplaysChanging," : "",
		val & ~0x13ffLL ? UNKNOWN_VALUE(val & ~0x13ffLL) : "",
		val & ~0x13ffLL ? "," : ""
	);
	buf[inc-1] = '\0';
	return str;
}

const char *GetIOFBEventStr(char *buf, int bufsize, uint64_t val) {
	const char *str = buf;
	int inc = snprintf(buf, bufsize, "%s%s%s%s%s%s%s%s%s%s%s",
		val & kIOFBEventCaptureSetting     ? "kIOFBEventCaptureSetting," : "",
		val & kIOFBEventDisplayDimsSetting ? "kIOFBEventDisplayDimsSetting," : "",
		val & kIOFBEventReadClamshell      ? "kIOFBEventReadClamshell," : "",
		val & kIOFBEventResetClamshell     ? "kIOFBEventResetClamshell," : "",
		val & kIOFBEventEnableClamshell    ? "kIOFBEventEnableClamshell," : "",
		val & kIOFBEventProbeAll           ? "kIOFBEventProbeAll," : "",
		val & kIOFBEventDisplaysPowerState ? "kIOFBEventDisplaysPowerState," : "",
		val & kIOFBEventSystemPowerOn      ? "kIOFBEventSystemPowerOn," : "",
		val & kIOFBEventVBLMultiplier      ? "kIOFBEventVBLMultiplier," : "",
		val & ~0x1ffLL ? UNKNOWN_VALUE(val & ~0x1ffLL) : "",
		val & ~0x1ffLL ? "," : ""
	);
	buf[inc-1] = '\0';
	return str;
}

#define ABL_SOURCE_IOD_ADDPARAMETERHANDLER 1
#define ABL_SOURCE_IOD_SETPARAMETER        2
#define ABL_SOURCE_IOD_DOINTEGERSET        3
#define ABL_SOURCE_IOD_DOUPDATE            4

const char *GetABLSourceStr(char *buf, int bufsize, uint64_t val) {
	const char *str;
	switch (val) {
		case ABL_SOURCE_IOD_ADDPARAMETERHANDLER : str = "ABL_SOURCE_IOD_ADDPARAMETERHANDLER"; break;
		case ABL_SOURCE_IOD_SETPARAMETER        : str = "ABL_SOURCE_IOD_SETPARAMETER"; break;
		case ABL_SOURCE_IOD_DOINTEGERSET        : str = "ABL_SOURCE_IOD_DOINTEGERSET"; break;
		case ABL_SOURCE_IOD_DOUPDATE            : str = "ABL_SOURCE_IOD_DOUPDATE"; break;
		default : str = buf; snprintf(buf, bufsize, "?0x%llx", val);
	}
	return str;
}

#define tagstring(count) \
	do { \
		if (arg > 3) cprintf(" ?too many args"); \
		else if (arg < 1) cprintf(" ?first arg cannot be string"); \
		else for (i = arg; i < arg + count; i++) { \
			if (entry.fArgsTag.tag(i) != kGTRACE_ARGUMENT_STRING) \
				cprintf(" ?tag:%s:", GetTagTypeStr(tagtypebuf, sizeof(tagtypebuf), entry.fArgsTag.tag(i))); \
			else cprintf(" "); \
		} \
	} while (0)


// #define GTRACEARG_string(_name, _len)           do { tagstring((_len + 7) / 8); cprintf("%s:\"%.*s\""  , _name, _len, (char *)(&entry.arg64(arg))); arg += (_len + 7) / 8; } while (0)


const char *GetEntryString(char *buf, int bufsize, const GTraceEntry& entry, int arg, int maxsize) {
	const char *str;
	int count = maxsize / 8;
	/**/ if (!maxsize) str = "?string size cannot be zero";
	else if (maxsize & 7) str = "?string size is not multiple of 8";
	else if (arg < 1) str = "?first arg cannot be string";
	else if (arg * 8 + maxsize > 32) str = "?string is too long";
	else {
		bool iszerotag = true;
		bool iszerodata = true;
		bool isstringtag = true;
		for (int i = arg; i < arg + count; i++) {
			if (entry.fArgsTag.tag(i) != kGTRACE_ARGUMENT_STRING) isstringtag = false;
			if (entry.fArgsTag.tag(i) != 0) iszerotag = false;
			if (entry.arg64(arg)) iszerodata = false;
		}
		
		/**/ if (iszerotag && iszerodata) str = "?no string";
		else if (isstringtag) { str = buf; snprintf(buf, bufsize, "\"%.*s\"", maxsize, (const char *)&entry.arg64(arg)); }
		else {
			str = buf;
			int inc = 0;
			for (int i = arg; i < arg + count; i++) {
				for (int j = i; j < arg + count; j++) {
					if ((j > 0 && entry.fArgsTag.tag(j)) || entry.arg64(j)) {
						if (i > arg) {
							inc = scnprintf(buf + inc, bufsize - inc, " ");
						}
						if (i > 0 && entry.fArgsTag.tag(i)) {
							char tagtypebuf[20];
							inc = scnprintf(buf + inc, bufsize - inc, "%s:", GetTagTypeStr(tagtypebuf, sizeof(tagtypebuf), entry.fArgsTag.tag(i)));
						}
						inc = scnprintf(buf + inc, bufsize - inc, "%#llx", entry.arg64(i));
						break;
					} // if j
				} // for j
			} // for i
		} // else invalid string
	} // else valid arg and len
	return str;
}

#if 0
	// Clamshell States
	#define DBG_IOG_CLAMSHELL_STATE_NOT_PRESENT         0
	#define DBG_IOG_CLAMSHELL_STATE_CLOSED              1
	#define DBG_IOG_CLAMSHELL_STATE_OPEN                2

	__ZN12GTraceBuffer11recordTokenEtyyyyyyyyy == GTraceBuffer::recordToken(unsigned short, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long)
		__ZN21AppleBacklightDisplay8fadeWorkEP18IOTimerEventSource == AppleBacklightDisplay::fadeWork(IOTimerEventSource*) + 0x214
			(line 673) IOG_KTRACE(DBG_IOG_ACK_POWER_STATE (18), DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_IODISPLAY (6), 0, 0, 0, 0, 0, 0);
		__ZN21AppleBacklightDisplay9fadeAbortEv == AppleBacklightDisplay::fadeAbort() + 0xe4
			(line 586) IOG_KTRACE(DBG_IOG_ACK_POWER_STATE (18), DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_IODISPLAY (6), 0, 0, 0, 0, 0, 0);
		__ZN21AppleBacklightDisplay13setPowerStateEmP9IOService == AppleBacklightDisplay::setPowerState(unsigned long, IOService*) + 0x45
			(line 379) IOG_KTRACE(DBG_IOG_SET_POWER_STATE (16), DBG_FUNC_NONE, 0, powerState, 0, DBG_IOG_SOURCE_APPLEBACKLIGHTDISPLAY (10), 0, 0, 0, 0);
	__ZN12GTraceBuffer11formatTokenEtyyyyyyyyy == GTraceBuffer::formatToken(unsigned short, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long)
		same as next
	__ZN12GTraceBuffer11recordTokenERK11GTraceEntry == GTraceBuffer::recordToken(GTraceEntry const&)
		__ZN17IODisplayWrangler17setAggressivenessEmm == IODisplayWrangler::setAggressiveness(unsigned long, unsigned long) + 0x126
			(line 527) IOG_KTRACE_NT(DBG_IOG_SET_TIMER_PERIOD (30), DBG_FUNC_NONE, DBG_IOG_SOURCE_SET_AGGRESSIVENESS (9), newLevel * 30, 0, 0);
		__ZN17IODisplayWrangler13setPowerStateEmP9IOService == IODisplayWrangler::setPowerState(unsigned long, IOService*) + 0x5a
		__ZN17IODisplayWrangler13setPowerStateEmP9IOService == IODisplayWrangler::setPowerState(unsigned long, IOService*) + 0xc5
		__ZN17IODisplayWrangler13setPowerStateEmP9IOService == IODisplayWrangler::setPowerState(unsigned long, IOService*) + 0x193
			(line 556) IOG_KTRACE_LOG_SYNCH(DBG_IOG_LOG_SYNCH (56))
			(line 565) IOG_KTRACE(DBG_IOG_SET_POWER_STATE (16), DBG_FUNC_NONE, 0, powerStateOrdinal, 0, DBG_IOG_SOURCE_IODISPLAYWRANGLER (5), 0, 0, 0, 0);
			(line 586) IOG_KTRACE(DBG_IOG_CHANGE_POWER_STATE_PRIV (28), DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_IODISPLAYWRANGLER (5), 0, 0, 0, 0, 0, 0);
		__ZN17IODisplayWrangler15nextIdleTimeoutEyyj == IODisplayWrangler::nextIdleTimeout(unsigned long long, unsigned long long, unsigned int) + 0x118
			(line 695) IOG_KTRACE(DBG_IOG_CHANGE_POWER_STATE_PRIV (28), DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_IODISPLAYWRANGLER (5), 0, 2, 0, 0, 0, 0);
		__ZN17IODisplayWrangler14activityTickleEmm == IODisplayWrangler::activityTickle(unsigned long, unsigned long) + 0xd2
			(line 783) IOG_KTRACE(DBG_IOG_WAKE_FROM_DOZE (26), DBG_FUNC_NONE, 0, x, 0, y, 0, 0, 0, 0);
		__ZN17IODisplayWrangler13forceIdleImplEv == IODisplayWrangler::forceIdleImpl() + 0xb4
			(line 835) IOG_KTRACE(DBG_IOG_CHANGE_POWER_STATE_PRIV (28), DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_IODISPLAYWRANGLER (5), 0, 1, kGTRACE_ARGUMENT_STRING, procNameInt[0], kGTRACE_ARGUMENT_STRING, procNameInt[1]);
		__ZN17IODisplayWrangler13setPropertiesEP8OSObject == IODisplayWrangler::setProperties(OSObject*) + 0x161
			(line 881) GTraceBuffer::formatToken(&var_98, rbx, 881, DBG_IOG_SET_PROPERTIES, r14, 0x2000, var_50, 0x2000, var_48, 0x2000, var_40, rax);
		__ZN14IOFBController9asyncWorkEP22IOInterruptEventSourcei == IOFBController::asyncWork(IOInterruptEventSource*, int) + 0x91
		__ZN14IOFBController9asyncWorkEP22IOInterruptEventSourcei == IOFBController::asyncWork(IOInterruptEventSource*, int) + 0x1e7
			(line 1721, 1, 0x44) IOG_KTRACE_NT(DBG_IOG_ASYNC_WORK, DBG_FUNC_START, fFbs[0]->__private->regID, work, fDidWork, 0);
			(line 1775, 2, 0x44) IOG_KTRACE_NT(DBG_IOG_ASYNC_WORK, DBG_FUNC_END, fFbs[0]->__private->regID, fAsyncWork, fDidWork, 0);
		__ZN13IOFramebuffer30deliverFramebufferNotificationEiPv == IOFramebuffer::deliverFramebufferNotification(int, void*) + 0x238
			(line 13189, , ) IOG_KTRACE_NT(DBG_IOG_DELIVER_NOTIFY, DBG_FUNC_NONE, __private->regID, event, ret, 0);
			GTraceBuffer::formatToken(&var_78, rbx, 13189, 0x30, r15, 0x0, sign_extend_64(r14), 0x0, 0x0, 0x0, 0x0, rax);
		__ZN14IOFBController23messageConnectionChangeEv == IOFBController::messageConnectionChange() + 0xc9
		__ZN14IOFBController23messageConnectionChangeEv == IOFBController::messageConnectionChange() + 0x217
			(1934, 56) GTraceBuffer::formatToken(&var_70, r15, 1934, 56, r14, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE_LOG_SYNCH(DBG_IOG_LOG_SYNCH);
			(1975, 61) GTraceBuffer::formatToken(&var_70, r14, 1975, 61, r12, 0x0, r13, 0x0, var_78, 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_MSG_CONNECT_CHANGE, DBG_FUNC_NONE, fFbs[0]->__private->regID, c1, c2, 0);
		__ZN13IOFramebuffer20processConnectChangeEj == IOFramebuffer::processConnectChange(unsigned int) + 0x87
		__ZN13IOFramebuffer20processConnectChangeEj == IOFramebuffer::processConnectChange(unsigned int) + 0x385
		__ZN13IOFramebuffer20processConnectChangeEj == IOFramebuffer::processConnectChange(unsigned int) + 0x41a
		__ZN13IOFramebuffer20processConnectChangeEj == IOFramebuffer::processConnectChange(unsigned int) + 0x54a
			(7850,1,46) GTraceBuffer::formatToken(&var_E0, rbx, 7850, 0x42e, r15, 0x0, r14, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_PROCESS_CONNECT_CHANGE (46), DBG_FUNC_START (1), __private->regID, mode, 0, 0);
			(7862,2,46) GTraceBuffer::formatToken(rdi, rsi, rdx, rcx, r8, 0x0, stack[-360], stack[-352], stack[-344], stack[-336], stack[-328], stack[-320]);
			IOG_KTRACE_NT(DBG_IOG_PROCESS_CONNECT_CHANGE (46), DBG_FUNC_END (2), __private->regID, 1, __private->online, 0);
			(7869,2,46) GTraceBuffer::formatToken(rdi, rsi, rdx, rcx, r8, 0x0, stack[-360], stack[-352], stack[-344], stack[-336], stack[-328], stack[-320]);
			IOG_KTRACE_NT(DBG_IOG_PROCESS_CONNECT_CHANGE (46), DBG_FUNC_END (2), __private->regID, 2, __private->online, 0);
			(7869,1,100) GTraceBuffer::formatToken(&var_130, r15, 7942, 0x464, 0x2, 0x0, stack[-360], 0x0, rbx, 0x0, stack[-328], rax);
			IOG_KTRACE(DBG_IOG_SET_DISPLAY_MODE (100), DBG_FUNC_START (1), 0, DBG_IOG_SOURCE_PROCESS_CONNECTION_CHANGE (2), 0, __private->regID, 0, __private->offlineMode, 0, __private->currentDepth);
			(7869,2,100) GTraceBuffer::formatToken(&var_130, rbx, 7947, 0x864, 0x2, 0x0, stack[-360], 0x0, sign_extend_64(r15), 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SET_DISPLAY_MODE (100), DBG_FUNC_END   (2), 0, DBG_IOG_SOURCE_PROCESS_CONNECTION_CHANGE (2), 0, __private->regID, 0, err, 0, 0);
			(7963,2,46) GTraceBuffer::formatToken(rdi, rsi, rdx, rcx, r8, 0x0, stack[-360], stack[-352], stack[-344], stack[-336], stack[-328], stack[-320]);
			IOG_KTRACE_NT(DBG_IOG_PROCESS_CONNECT_CHANGE (46), DBG_FUNC_END (2), __private->regID, 0, __private->online, 0);
		__ZN13IOFramebuffer16matchFramebufferEv == IOFramebuffer::matchFramebuffer() + 0x1a0
		__ZN13IOFramebuffer16matchFramebufferEv == IOFramebuffer::matchFramebuffer() + 0x219
			GTraceBuffer::formatToken(&var_80, rbx, 8180, 0x464, 0x1, 0x0, r13, 0x0, 0xffffffffc0000000ULL, 0x0, sign_extend_64(r14), rax);
			IOG_KTRACE(DBG_IOG_SET_DISPLAY_MODE, DBG_FUNC_START, 0, DBG_IOG_SOURCE_MATCH_FRAMEBUFFER, 0, __private->regID, 0, mode, 0, depth);
			GTraceBuffer::formatToken(&var_80, r12, 8185, 0x864, 0x1, 0x0, var_38, 0x0, sign_extend_64(r13), 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SET_DISPLAY_MODE, DBG_FUNC_END, 0, DBG_IOG_SOURCE_MATCH_FRAMEBUFFER, 0, __private->regID, 0, err, 0, 0);
		__ZN13IOFramebuffer14checkPowerWorkEj == IOFramebuffer::checkPowerWork(unsigned int) + 0x254
		__ZN13IOFramebuffer14checkPowerWorkEj == IOFramebuffer::checkPowerWork(unsigned int) + 0x2bd
		__ZN13IOFramebuffer14checkPowerWorkEj == IOFramebuffer::checkPowerWork(unsigned int) + 0x3eb
			GTraceBuffer::formatToken(&var_88, r13, 7619, 0x413, r8, 0x0, r15, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SET_POWER_ATTRIBUTE, DBG_FUNC_START, 0, __private->regID, 0, newState, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_88, r13, 7623, 0x813, r14, 0x0, r15, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SET_POWER_ATTRIBUTE, DBG_FUNC_END, 0, __private->regID, 0, newState, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_88, r14, 7660, 0x12, 0x8, 0x0, r13, 0x0, var_30, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_ACK_POWER_STATE, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_CHECK_POWER_WORK, 0, __private->regID, 0, newState, 0, 0);
		__ZN14IOFBController19checkConnectionWorkEj == IOFBController::checkConnectionWork(unsigned int) + 0x251
		__ZN14IOFBController19checkConnectionWorkEj == IOFBController::checkConnectionWork(unsigned int) + 0x347
			GTraceBuffer::formatToken(&var_B0, r14, 1862, 0x3e, r15, 0x0, r13 << 0x10 | rcx, 0x0, rbx, 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_CAPTURED_RETRAIN, DBG_FUNC_NONE, fFbs[0]->__private->regID, c1, c2, 0);
			GTraceBuffer::formatToken(&var_B0, r14, 1899, 0x3f, r12, 0x0, stack[-248], 0x0, rbx << 0x3c | rsi << 0x3d | rdi, 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_CONNECT_WORK_ASYNC, DBG_FUNC_NONE, fFbs[0]->__private->regID, c1, c2, 0);
		__ZN18IOGraphicsWorkLoop14timedCloseGateEPKcS1_ == IOGraphicsWorkLoop::timedCloseGate(char const*, char const*) + 0xd3
			GTraceBuffer::formatToken(&var_80, rbx, 807, 54, r8, 0x2000, var_30, 0x2000, var_40, 0x2000, var_38, rax);
			IOG_KTRACE_IFSLOW_END(DBG_IOG_TIMELOCK, DBG_FUNC_NONE, kGTRACE_ARGUMENT_STRING, nameBufInt[0], kGTRACE_ARGUMENT_STRING, fnBufInt[0], kGTRACE_ARGUMENT_STRING, fnBufInt[1], kTIMELOCK_TIMEOUT_AT);
		__ZN13IOFramebuffer16updateGammaTableEjjjPKvibb == IOFramebuffer::updateGammaTable(unsigned int, unsigned int, unsigned int, void const*, int, bool, bool) + 0x4ee
		__ZN13IOFramebuffer16updateGammaTableEjjjPKvibb == IOFramebuffer::updateGammaTable(unsigned int, unsigned int, unsigned int, void const*, int, bool, bool) + 0x596
		__ZN13IOFramebuffer16updateGammaTableEjjjPKvibb == IOFramebuffer::updateGammaTable(unsigned int, unsigned int, unsigned int, void const*, int, bool, bool) + 0x601
		__ZN13IOFramebuffer16updateGammaTableEjjjPKvibb == IOFramebuffer::updateGammaTable(unsigned int, unsigned int, unsigned int, void const*, int, bool, bool) + 0x68d
			GTraceBuffer::formatToken(&var_D8, rbx, 3555, 0x428, r14, 0x0, 0x10, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SET_GAMMA_TABLE, DBG_FUNC_START, 0, __private->regID, 0, DBG_IOG_SOURCE_UPDATE_GAMMA_TABLE, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_D8, r14, 3564, 0x828, r12, 0x0, 0x10, 0x0, sign_extend_64(rbx), 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SET_GAMMA_TABLE, DBG_FUNC_END, 0, __private->regID, 0, DBG_IOG_SOURCE_UPDATE_GAMMA_TABLE, 0, err, 0, 0);
			GTraceBuffer::formatToken(&var_D8, rbx, 3579, 0x428, r14, 0x0, 0x10, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SET_GAMMA_TABLE, DBG_FUNC_START, 0, __private->regID, 0, DBG_IOG_SOURCE_UPDATE_GAMMA_TABLE, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_D8, r14, 3587, 0x828, r12, 0x0, 0x10, 0x0, sign_extend_64(rbx), 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SET_GAMMA_TABLE, DBG_FUNC_END, 0, __private->regID, 0, DBG_IOG_SOURCE_UPDATE_GAMMA_TABLE, 0, err, 0, 0);
		__ZN13IOFramebuffer20checkDeferredCLUTSetEv == IOFramebuffer::checkDeferredCLUTSet() + 0x9b
		__ZN13IOFramebuffer20checkDeferredCLUTSetEv == IOFramebuffer::checkDeferredCLUTSet() + 0x136
		__ZN13IOFramebuffer20checkDeferredCLUTSetEv == IOFramebuffer::checkDeferredCLUTSet() + 0x19c
		__ZN13IOFramebuffer20checkDeferredCLUTSetEv == IOFramebuffer::checkDeferredCLUTSet() + 0x220
			GTraceBuffer::formatToken(&var_70, r14, 3764, 0x428, r15, 0x0, 0x11, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SET_GAMMA_TABLE, DBG_FUNC_START, 0, __private->regID, 0, DBG_IOG_SOURCE_DEFERRED_CLUT, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_70, r15, 3772, 0x828, r12, 0x0, 0x11, 0x0, sign_extend_64(r14), 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SET_GAMMA_TABLE, DBG_FUNC_END, 0, __private->regID, 0, DBG_IOG_SOURCE_DEFERRED_CLUT, 0, ret, 0, 0);
			GTraceBuffer::formatToken(&var_70, r14, 3787, 0x428, r15, 0x0, 0x11, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SET_GAMMA_TABLE, DBG_FUNC_START, 0, __private->regID, 0, DBG_IOG_SOURCE_DEFERRED_CLUT, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_70, r14, 3794, 0x828, r15, 0x0, 0x11, 0x0, r12, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SET_GAMMA_TABLE, DBG_FUNC_END, 0, __private->regID, 0, DBG_IOG_SOURCE_DEFERRED_CLUT, 0, ret, 0, 0);
		__ZN13IOFramebuffer7doSetupEb == IOFramebuffer::doSetup(bool) + 0x10d
		__ZN13IOFramebuffer7doSetupEb == IOFramebuffer::doSetup(bool) + 0x4ab
			GTraceBuffer::formatToken(&var_B0, r14, 11221, 102, 0x28, 0x0, var_40, 0x0, var_30 | var_38 << 0x20, 0x0, sign_extend_64(r15), rax);
			IOG_KTRACE(DBG_IOG_GET_CURRENT_DISPLAY_MODE, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_DO_SETUP, 0, __private->regID, 0, GPACKUINT32T(1, depth) | GPACKUINT32T(0, mode), 0, err);
			GTraceBuffer::formatToken(&var_B0, r14, 11321, 39, var_50, 0x0, var_60, 0x0, var_58, 0x0, stack[-200], rax);
			IOG_KTRACE_NT(DBG_IOG_VRAM_CONFIG, DBG_FUNC_NONE, __private->regID, __private->pixelInfo.activeHeight, __private->pixelInfo.bytesPerRow, fVramMap->getLength());
		__ZN13IOFramebuffer19waitQuietControllerEv == IOFramebuffer::waitQuietController() + 0x155
			GTraceBuffer::formatToken(&var_230, rbx, 3986, 36, r13, 0x0, sign_extend_64(r14), 0x0, r15, 0x0, 0xf, rax);
			IOG_KTRACE(DBG_IOG_WAIT_QUIET, DBG_FUNC_NONE, 0, pciRegID, 0, status, 0, getRegistryEntryID(), 0, timeout);
		__ZN13IOFramebuffer18setPlatformConsoleEP8PE_Videojy == IOFramebuffer::setPlatformConsole(PE_Video*, unsigned int, unsigned long long) + 0x7d
		__ZN13IOFramebuffer18setPlatformConsoleEP8PE_Videojy == IOFramebuffer::setPlatformConsole(PE_Video*, unsigned int, unsigned long long) + 0x12e
			GTraceBuffer::formatToken(&var_80, r12, 4012, 37, rcx, 0x0, var_38, 0x0, r13 != 0x0 ? 0x1 : 0x0, 0x0, r15, rax);
			IOG_KTRACE(DBG_IOG_PLATFORM_CONSOLE, DBG_FUNC_NONE, 0, where, 0, __private->regID, 0, static_cast<bool>(consoleInfo), 0, op);
			GTraceBuffer::formatToken(&var_80, r12, 4019, 38, var_30, 0x0, r14, 0x0, var_38, 0x0, stack[-152], rax);
			IOG_KTRACE(DBG_IOG_CONSOLE_CONFIG, DBG_FUNC_NONE, 0, __private->regID, 0, consoleInfo->v_width << 32 | consoleInfo->v_height, 0, consoleInfo->v_rowBytes, 0, consoleInfo->v_depth << 32 | consoleInfo->v_scale);
		__ZN13IOFramebuffer13newUserClientEP4taskPvjPP12IOUserClient == IOFramebuffer::newUserClient(task*, void*, unsigned int, IOUserClient**) + 0x7c
		__ZN13IOFramebuffer13newUserClientEP4taskPvjPP12IOUserClient == IOFramebuffer::newUserClient(task*, void*, unsigned int, IOUserClient**) + 0x1a1
			GTraceBuffer::formatToken(&var_80, rbx, 4056, 41, r14, 0x0, rcx, 0x0, sign_extend_64(r15), 0x0, 0x2, rax);
			IOG_KTRACE(DBG_IOG_NEW_USER_CLIENT, DBG_FUNC_NONE, 0, __private->regID, 0, type, 0, err, 0, /* location */ 2);
			GTraceBuffer::formatToken(&var_80, rbx, 4127, 41, r13, 0x0, r14, 0x0, sign_extend_64(r15), 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_NEW_USER_CLIENT, DBG_FUNC_NONE, 0, __private->regID, 0, type, 0, err, 0, /* location */ 0);
		__ZN13IOFramebuffer4stopEP9IOService == IOFramebuffer::stop(IOService*) + 0x43e
			GTraceBuffer::formatToken(&var_78, r13, 4538, 29, 7, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_CLAMP_POWER_ON, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_STOP, 0, 0, 0, 0, 0, 0);
		__ZN13IOFramebuffer10agcMessageEPvS0_jP9IOServiceS0_m == IOFramebuffer::agcMessage(void*, void*, unsigned int, IOService*, void*, unsigned long) + 0x59
		__ZN13IOFramebuffer10agcMessageEPvS0_jP9IOServiceS0_m == IOFramebuffer::agcMessage(void*, void*, unsigned int, IOService*, void*, unsigned long) + 0x13f
			GTraceBuffer::formatToken(&var_68, rbx, 8661, 49, r13, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_AGC_MSG, DBG_FUNC_NONE, 0, switchState, 0, 0, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_68, rbx, 8687, 21, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_MUX_ALLOW_POWER_CHANGE, DBG_FUNC_NONE, 0, 0, 0, 0, 0, 0, 0, 0);
		__ZN13IOFramebuffer17systemPowerChangeEPvS0_jP9IOServiceS0_m == IOFramebuffer::systemPowerChange(void*, void*, unsigned int, IOService*, void*, unsigned long) + 0x60
		__ZN13IOFramebuffer17systemPowerChangeEPvS0_jP9IOServiceS0_m == IOFramebuffer::systemPowerChange(void*, void*, unsigned int, IOService*, void*, unsigned long) + 0xcb
		__ZN13IOFramebuffer17systemPowerChangeEPvS0_jP9IOServiceS0_m == IOFramebuffer::systemPowerChange(void*, void*, unsigned int, IOService*, void*, unsigned long) + 0x1e6
		__ZN13IOFramebuffer17systemPowerChangeEPvS0_jP9IOServiceS0_m == IOFramebuffer::systemPowerChange(void*, void*, unsigned int, IOService*, void*, unsigned long) + 0x2a9
		__ZN13IOFramebuffer17systemPowerChangeEPvS0_jP9IOServiceS0_m == IOFramebuffer::systemPowerChange(void*, void*, unsigned int, IOService*, void*, unsigned long) + 0x40f
		__ZN13IOFramebuffer17systemPowerChangeEPvS0_jP9IOServiceS0_m == IOFramebuffer::systemPowerChange(void*, void*, unsigned int, IOService*, void*, unsigned long) + 0x5cb
		__ZN13IOFramebuffer17systemPowerChangeEPvS0_jP9IOServiceS0_m == IOFramebuffer::systemPowerChange(void*, void*, unsigned int, IOService*, void*, unsigned long) + 0x6a4
		__ZN13IOFramebuffer17systemPowerChangeEPvS0_jP9IOServiceS0_m == IOFramebuffer::systemPowerChange(void*, void*, unsigned int, IOService*, void*, unsigned long) + 0x704
		__ZN13IOFramebuffer17systemPowerChangeEPvS0_jP9IOServiceS0_m == IOFramebuffer::systemPowerChange(void*, void*, unsigned int, IOService*, void*, unsigned long) + 0x7d1
			GTraceBuffer::formatToken(&var_88, r14, 8779, 56, r15, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE_LOG_SYNCH(DBG_IOG_LOG_SYNCH);
			GTraceBuffer::formatToken(&var_88, rbx, 8791, 0x411, r13, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SYSTEM_POWER_CHANGE, DBG_FUNC_START, 0, messageType, 0, 0, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_88, rbx, 8899, 0x811, 0xffffffffe0000340ULL, 0x0, r13, r14, r15, r14, r12, rax);
			IOG_KTRACE(DBG_IOG_SYSTEM_POWER_CHANGE, DBG_FUNC_END, 0, messageType, 0, params->fromCapabilities, 0, params->toCapabilities, 0, params->changeFlags);
			GTraceBuffer::formatToken(&var_88, rbx, 8921, 27, 3, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_RECEIVE_POWER_NOTIFICATION, DBG_FUNC_NONE, 0, DBG_IOG_PWR_EVENT_SYSTEMPWRCHANGE (3), 0, kIOPMDisableClamshell, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_88, rbx, 8943, 0x811, 0xffffffffe0000280ULL, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SYSTEM_POWER_CHANGE, DBG_FUNC_END, 0, messageType, 0, ret, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_88, rbx, 8977, 0x811, 0xffffffffe0000320ULL, 0x0, r14, r14, r14, r14, r14, rax);
			IOG_KTRACE(DBG_IOG_SYSTEM_POWER_CHANGE, DBG_FUNC_END, 0, messageType, 0, ret, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_88, rbx, 8995, 0x811, 0xffffffffe0000300ULL, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SYSTEM_POWER_CHANGE, DBG_FUNC_END, 0, messageType, 0, ret, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_88, r12, 9031, 0x811, r13, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SYSTEM_POWER_CHANGE, DBG_FUNC_END, 0, messageType, 0, ret, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_88, rbx, 9041, 0x811, r13, 0x0, 0xffffffffe00002c7ULL, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SYSTEM_POWER_CHANGE, DBG_FUNC_END, 0, messageType, 0, ret, 0, 0, 0, 0);
		__ZN13IOFramebuffer10systemWorkEP8OSObjectP22IOInterruptEventSourcei == IOFramebuffer::systemWork(OSObject*, IOInterruptEventSource*, int) + 0x7f
		__ZN13IOFramebuffer10systemWorkEP8OSObjectP22IOInterruptEventSourcei == IOFramebuffer::systemWork(OSObject*, IOInterruptEventSource*, int) + 0x1b0
		__ZN13IOFramebuffer10systemWorkEP8OSObjectP22IOInterruptEventSourcei == IOFramebuffer::systemWork(OSObject*, IOInterruptEventSource*, int) + 0x3b8
		__ZN13IOFramebuffer10systemWorkEP8OSObjectP22IOInterruptEventSourcei == IOFramebuffer::systemWork(OSObject*, IOInterruptEventSource*, int) + 0x8aa
		__ZN13IOFramebuffer10systemWorkEP8OSObjectP22IOInterruptEventSourcei == IOFramebuffer::systemWork(OSObject*, IOInterruptEventSource*, int) + 0xac0
		__ZN13IOFramebuffer10systemWorkEP8OSObjectP22IOInterruptEventSourcei == IOFramebuffer::systemWork(OSObject*, IOInterruptEventSource*, int) + 0xb8d
		__ZN13IOFramebuffer10systemWorkEP8OSObjectP22IOInterruptEventSourcei == IOFramebuffer::systemWork(OSObject*, IOInterruptEventSource*, int) + 0xbf9
		__ZN13IOFramebuffer10systemWorkEP8OSObjectP22IOInterruptEventSourcei == IOFramebuffer::systemWork(OSObject*, IOInterruptEventSource*, int) + 0xd89
		__ZN13IOFramebuffer10systemWorkEP8OSObjectP22IOInterruptEventSourcei == IOFramebuffer::systemWork(OSObject*, IOInterruptEventSource*, int) + 0xf52
			GTraceBuffer::formatToken(&var_98, rbx, 6957, 0x445, r12, 0x0, stack[-216], 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_SYSTEM_WORK, DBG_FUNC_START, allState, gIOFBGlobalEvents, 0, 0);
			GTraceBuffer::formatToken(&var_98, r13, 6983, 64, r8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_MUX_ACTIVITY_CHANGE, DBG_FUNC_NONE, controller->fFbs[0]->__private->regID, 0, 0, 0);
			GTraceBuffer::formatToken(&var_98, rbx, 7060, 20, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_ALLOW_POWER_CHANGE, DBG_FUNC_NONE, 0, 0, 0, 0, 0, 0, 0, 0);
			(7223,33) GTraceBuffer::formatToken(rdi, rsi, rdx, 33, 28, 0x0, stack[-216], stack[-208], stack[-200], stack[-192], stack[-184], stack[-176]);
			IOG_KTRACE(DBG_IOG_CLAMSHELL, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_SYSWORK_READCLAMSHELL, 0, kOSBooleanTrue == clamshellProperty, 0, gIOFBCurrentClamshellState, 0, desktopMode);
			(7228,33) GTraceBuffer::formatToken(rdi, rsi, rdx, 33, 28, 0x0, stack[-216], stack[-208], stack[-200], stack[-192], stack[-184], stack[-176]);
			IOG_KTRACE(DBG_IOG_CLAMSHELL, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_SYSWORK_READCLAMSHELL, 0, -1, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_98, r13, 7299, 33, 43, 0x0, ((var_4C != 0x0 ? 0x1 : 0x0) | (r12 != 0x0 ? 0x1 : 0x0) + (r12 != 0x0 ? 0x1 : 0x0) | (r15 != r14 ? 0x1 : 0x0) << 0x5 | (var_48 & 0xff) << 0x4 | r8) + (var_31 != 0x0 ? 0x1 : 0x0) * 0x8, 0x0, rbx, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_CLAMSHELL, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_SYSWORK_RESETCLAMSHELL_V2, 0, bits, 0, counts, 0, 0);
			GTraceBuffer::formatToken(&var_98, r14, 7325, 27, 0x1, 0x0, rbx, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_RECEIVE_POWER_NOTIFICATION, DBG_FUNC_NONE, 0, DBG_IOG_PWR_EVENT_DESKTOPMODE (1), 0, gIOFBClamshellState, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_98, r14, 7332, 33, 30, 0x0, r12, 0x0, rbx, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_CLAMSHELL, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_SYSWORK_ENABLECLAMSHELL, 0, desktopMode, 0, change, 0, 0);
			GTraceBuffer::formatToken(&var_98, rbx, 7371, 33, 31, 0x0, r15, 0x0, r12, 0x0, stack[-184], rax);
			IOG_KTRACE(DBG_IOG_CLAMSHELL, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_SYSWORK_PROBECLAMSHELL, 0, gIOFBLastClamshellState, 0, gIOFBLastReadClamshellState, 0, gIOFBCurrentClamshellState);
			GTraceBuffer::formatToken(&var_98, rbx, 7410, 0x845, r12, 0x0, r15, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_SYSTEM_WORK, DBG_FUNC_END, allState, gIOFBGlobalEvents, 0, 0);
		__ZN13IOFramebuffer18serverPowerTimeoutEP8OSObjectP18IOTimerEventSource == IOFramebuffer::serverPowerTimeout(OSObject*, IOTimerEventSource*) + 0xe3
			GTraceBuffer::formatToken(&var_80, rcx, 7509, 22, var_38, 0x0, 0x23, 0x0, r15, 0x0, var_40, rax);
			IOG_KTRACE(DBG_IOG_SERVER_TIMEOUT, DBG_FUNC_NONE, 0, fb->__private->regID, 0, DBG_IOG_SOURCE_SERVER_ACK_TIMEOUT, 0, powerState, 0, fb->__private->fServerMsgCount);
		__ZN13IOFramebuffer5startEP9IOService == IOFramebuffer::start(IOService*) + 0x64
			GTraceBuffer::formatToken(&var_70, r14, 4852, 56, r12, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE_LOG_SYNCH(DBG_IOG_LOG_SYNCH);
		__ZN13IOFramebuffer10moveCursorEP8IOGPointi == IOFramebuffer::moveCursor(IOGPoint*, int) + 0x104
			GTraceBuffer::formatToken(&var_90, rbx, 5248, 55, r8, 0x2000, var_38, 0x2000, var_50, 0x2000, var_48, rax);
			IOG_KTRACE_IFSLOW_END(DBG_IOG_CURSORLOCK, DBG_FUNC_NONE, kGTRACE_ARGUMENT_STRING, nameBufInt[0], kGTRACE_ARGUMENT_STRING, fnBufInt[0], kGTRACE_ARGUMENT_STRING, fnBufInt[1], kCursorThresholdAT);
		__ZN13IOFramebuffer20extSetCursorPositionEP8OSObjectPvP25IOExternalMethodArguments == IOFramebuffer::extSetCursorPosition(OSObject*, void*, IOExternalMethodArguments*) + 0x1d6
			GTraceBuffer::formatToken(&var_A8, rbx, 5603, 55, r8, 0x2000, var_38, 0x2000, var_50, 0x2000, var_48, rax);
			IOG_KTRACE_IFSLOW_END(DBG_IOG_CURSORLOCK, DBG_FUNC_NONE, kGTRACE_ARGUMENT_STRING, nameBufInt[0], kGTRACE_ARGUMENT_STRING, fnBufInt[0], kGTRACE_ARGUMENT_STRING, fnBufInt[1], kCursorThresholdAT);
		__ZN13IOFramebuffer18restoreFramebufferEi == IOFramebuffer::restoreFramebuffer(int) + 0x2a7
		__ZN13IOFramebuffer18restoreFramebufferEi == IOFramebuffer::restoreFramebuffer(int) + 0x429
		__ZN13IOFramebuffer18restoreFramebufferEi == IOFramebuffer::restoreFramebuffer(int) + 0x4b0
		__ZN13IOFramebuffer18restoreFramebufferEi == IOFramebuffer::restoreFramebuffer(int) + 0x520
		__ZN13IOFramebuffer18restoreFramebufferEi == IOFramebuffer::restoreFramebuffer(int) + 0x6cd
			GTraceBuffer::formatToken(&var_A8, rbx, 6554, 0x40d, r13, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_VRAM_BLACK, DBG_FUNC_START, 0, __private->regID, 0, 0, 0, 0, 0, 0);
			(6562, 2, 13) GTraceBuffer::formatToken(rdi, rsi, rdx, rcx, r13, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_VRAM_BLACK, DBG_FUNC_END, 0, __private->regID, 0, 0, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_A8, rbx, 6567, 0x40c, r13, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_VRAM_RESTORE, DBG_FUNC_START, 0, __private->regID, 0, 0, 0, 0, 0, 0);
			(6578, 2, 12) GTraceBuffer::formatToken(rdi, rsi, rdx, rcx, r13, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_VRAM_RESTORE, DBG_FUNC_END, 0, __private->regID, 0, 0, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_A8, r14, 6608, 0x428, r12, 0x0, 0x12, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SET_GAMMA_TABLE, DBG_FUNC_START, 0, __private->regID, 0, DBG_IOG_SOURCE_RESTORE_FRAMEBUFFER, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_A8, r14, 6614, 0x828, r12, 0x0, 0x12, 0x0, r13, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SET_GAMMA_TABLE, DBG_FUNC_END, 0, __private->regID, 0, DBG_IOG_SOURCE_RESTORE_FRAMEBUFFER, 0, kr, 0, 0);
		__ZN13IOFramebuffer11handleEventEiPv == IOFramebuffer::handleEvent(int, void*) + 0x5f
			GTraceBuffer::formatToken(&var_70, rbx, 6633, 31, r15, 0x0, sign_extend_64(r12), 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_HANDLE_EVENT, DBG_FUNC_NONE, 0, __private->regID, 0, event, 0, 0, 0, 0);
		__ZN13IOFramebuffer12notifyServerEh == IOFramebuffer::notifyServer(unsigned char) + 0x2cb
			GTraceBuffer::formatToken(&var_90, rbx, 6810, 10, r8, 0x0, var_48, 0x0, r15, 0x0, rdx, rax);
			IOG_KTRACE(DBG_IOG_NOTIFY_SERVER, DBG_FUNC_NONE, 0, __private->regID, 0, msgh->msgh_id, 0, sentAckedPower, 0, hidden);
		__ZN13IOFramebuffer8postWakeEv == IOFramebuffer::postWake() + 0x142
			GTraceBuffer::formatToken(&var_70, rbx, 6919, 33, 21, 0x0, r12, 0x0, r13, 0x0, stack[-136], rax);
			IOG_KTRACE(DBG_IOG_CLAMSHELL, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_POSTWAKE, 0, gIOFBLastClamshellState, 0, gIOFBLastReadClamshellState, 0, gIOFBCurrentClamshellState);
		__ZN13IOFramebuffer14resetClamshellEjj == IOFramebuffer::resetClamshell(unsigned int, unsigned int) + 0x89
			GTraceBuffer::formatToken(&var_68, rbx, 8565, 33, 27, 0x0, r15, 0x0, r14, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_CLAMSHELL, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_RESET_CLAMSHELL, 0, where, 0, delay, 0, 0);
		__ZN13IOFramebuffer15muxPowerMessageEj == IOFramebuffer::muxPowerMessage(unsigned int) + 0x50
		__ZN13IOFramebuffer15muxPowerMessageEj == IOFramebuffer::muxPowerMessage(unsigned int) + 0x104
		__ZN13IOFramebuffer15muxPowerMessageEj == IOFramebuffer::muxPowerMessage(unsigned int) + 0x1bd
			GTraceBuffer::formatToken(&var_68, rbx, 8715, 0x418, r14, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_MUX_POWER_MESSAGE, DBG_FUNC_START, 0, messageType, 0, 0, 0, 0, 0, 0);
			(8723, 2, 24) GTraceBuffer::formatToken(rdi, rsi, rdx, 0x818, 0x0, 0x0, stack[-168], stack[-160], stack[-152], stack[-144], stack[-136], stack[-128]);
			IOG_KTRACE(DBG_IOG_MUX_POWER_MESSAGE, DBG_FUNC_END, 0, kIOReturnSuccess, 0, -1, 0, 0, 0, 0);
			(8737, 2, 24) GTraceBuffer::formatToken(rdi, rsi, rdx, 0x818, 0x0, 0x0, stack[-168], stack[-160], stack[-152], stack[-144], stack[-136], stack[-128]);
			IOG_KTRACE(DBG_IOG_MUX_POWER_MESSAGE, DBG_FUNC_END, 0, kIOReturnSuccess, 0, -2, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_68, r14, 8766, 0x818, sign_extend_64(r15), 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_MUX_POWER_MESSAGE, DBG_FUNC_END, 0, ret, 0, 0, 0, 0, 0, 0);
		__ZN13IOFramebuffer9dimEngineEv == IOFramebuffer::dimEngine() + 0x6d
		__ZN13IOFramebuffer9dimEngineEv == IOFramebuffer::dimEngine() + 0x1ed
		__ZN13IOFramebuffer9dimEngineEv == IOFramebuffer::dimEngine() + 0x36f
		__ZN13IOFramebuffer9dimEngineEv == IOFramebuffer::dimEngine() + 0x413
			GTraceBuffer::formatToken(&var_80, rbx, 8389, 0x435 (1,DBG_IOG_DIMENGINE_53), r15 & 0xff, 0x0, 0xa, 0x0, 0x0, 0x0, 0x0, rax);
			GTraceBuffer::formatToken(&var_80, r13, 8410, 22 (DBG_IOG_SERVER_TIMEOUT), r15, 0x0, 0x25, 0x0, rbx << 0x20 | rcx, 0x0, var_30, rax);
			GTraceBuffer::formatToken(&var_80, rbx, 8490, 22 (DBG_IOG_SERVER_TIMEOUT), var_40, 0x0, 0x24, 0x0, r15, 0x0, var_30, rax);
			GTraceBuffer::formatToken(&var_80, rbx, 8508, 0x835 (2,DBG_IOG_DIMENGINE_53), r15 & 0xff, 0x0, 0xa, 0x0, 0x0, 0x0, 0x0, rax);
		__ZN13IOFramebuffer29serverAcknowledgeNotificationEi == IOFramebuffer::serverAcknowledgeNotification(int) + 0x136
			GTraceBuffer::formatToken(&var_78, rbx, 9187, 11, r14, 0x0, sign_extend_64(r13), 0x0, rcx | r12 << 0x20, 0x0, rdx | r12 << 0x20, rax);
			IOG_KTRACE_NT(DBG_IOG_SERVER_ACK, DBG_FUNC_NONE, __private->regID, msgh_id, sentAckedPower, hidden);
		__ZN13IOFramebuffer22extEndConnectionChangeEv == IOFramebuffer::extEndConnectionChange() + 0xff
			GTraceBuffer::formatToken(&var_68, r14, 7702, 66, r13, 0x0, r12 << 0x20 | rcx >> 0x3 & 0x1, 0x0, rbx << 0x20 | rdx << 0x18 | (rsi & 0xff) << 0x10 | rdi << 0x8 & 0xffff | r8, 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_EXT_END_CONNECT_CHANGE, DBG_FUNC_NONE, controller->fFbs[0]->__private->regID, c1, c2, 0);
		__ZN13IOFramebuffer26extProcessConnectionChangeEv == IOFramebuffer::extProcessConnectionChange() + 0xe5
		__ZN13IOFramebuffer26extProcessConnectionChangeEv == IOFramebuffer::extProcessConnectionChange() + 0x1fa
		__ZN13IOFramebuffer26extProcessConnectionChangeEv == IOFramebuffer::extProcessConnectionChange() + 0x291
		__ZN13IOFramebuffer26extProcessConnectionChangeEv == IOFramebuffer::extProcessConnectionChange() + 0x386
			GTraceBuffer::formatToken(&var_80, rbx, 7755, 0x443, r14, 0x0, (r13 & 0xff) + (rdx & 0x1) * 0x2, 0x0, stack[-168], 0x0, var_38, rax);
			IOG_KTRACE_NT(DBG_IOG_EXT_PROCESS_CONNECT_CHANGE, DBG_FUNC_START, __private->regID, a1, a2, a3);
			GTraceBuffer::formatToken(&var_80, rbx, 7804, 29, 2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_CLAMP_POWER_ON, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_PROCESS_CONNECTION_CHANGE, 0, 0, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_80, rbx, 7824, 27, 0x4, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_RECEIVE_POWER_NOTIFICATION, DBG_FUNC_NONE, 0, DBG_IOG_PWR_EVENT_PROCCONNECTCHANGE (4), 0, kIOPMDisableClamshell, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&var_80, rbx, 7833, 0x843, r12, 0x0, r14, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_EXT_PROCESS_CONNECT_CHANGE, DBG_FUNC_END, __private->regID, mode, err, 0);
		__ZN13IOFramebuffer12updateOnlineEv == IOFramebuffer::updateOnline() + 0xa8
			GTraceBuffer::formatToken(&var_78, rbx, 7986, 45, 33, 0x0, var_30, 0x0, var_38, 0x0, sign_extend_64(r14), rax);
			IOG_KTRACE(DBG_IOG_CONNECTION_ENABLE_CHECK, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_UPDATE_ONLINE, 0, __private->regID, 0, connectEnabled, 0, err);
		__ZN13IOFramebuffer20doSetDetailedTimingsEP7OSArrayyy == IOFramebuffer::doSetDetailedTimings(OSArray*, unsigned long long, unsigned long long) + 0x5c
		__ZN13IOFramebuffer20doSetDetailedTimingsEP7OSArrayyy == IOFramebuffer::doSetDetailedTimings(OSArray*, unsigned long long, unsigned long long) + 0xc9
			GTraceBuffer::formatToken(&var_68, rbx, 8099, 0x465, r14, 0x0, r13, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_SET_DETAILED_TIMING, DBG_FUNC_START, source, __private->regID, 0, 0);
			GTraceBuffer::formatToken(&var_68, rbx, 8102, 0x865, r14, 0x0, r13, 0x0, sign_extend_64(r15), 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_SET_DETAILED_TIMING, DBG_FUNC_END, source, __private->regID, err, 0);
		__ZN13IOFramebuffer13setPowerStateEmP9IOService == IOFramebuffer::setPowerState(unsigned long, IOService*) + 0x5b
			GTraceBuffer::formatToken(&var_68, rbx, 8230, 25, r12, 0x0, r14, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_FB_POWER_CHANGE, DBG_FUNC_NONE, 0, __private->regID, 0, powerStateOrdinal, 0, 0, 0, 0);
		__ZN13IOFramebuffer17setSystemDimStateEb == IOFramebuffer::setSystemDimState(bool) + 0x166
			GTraceBuffer::formatToken(&var_70, r15, 8372, 52 (DBG_IOG_SET_SYSTEM_DIM_STATE_52), sign_extend_64(r12), 0x0, r14 >> 0x1f | (r13 != 0x0 ? 0x1 : 0x0) << 0x8 | r13 << 0x30 | rbx | rcx << 0x20, 0x0, 0x0, 0x0, 0x0, rax);
		__ZN13IOFramebuffer18readClamshellStateEy == IOFramebuffer::readClamshellState(unsigned long long) + 0x119
			GTraceBuffer::formatToken(&var_70, rbx, 12588, 33, 26, 0x0, r14, 0x0, r12, 0x0, r13, rax);
			IOG_KTRACE(DBG_IOG_CLAMSHELL, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_READ_CLAMSHELL, 0, where, 0, gIOFBLastClamshellState, 0, oldState);
		__ZN13IOFramebuffer26connectChangeInterruptImplEt == IOFramebuffer::connectChangeInterruptImpl(unsigned short) + 0x8e
			GTraceBuffer::formatToken(&var_68, r15, 9359, 71, rbx, 0x0, r14 & 0xffff | rcx << 0x20, 0x0, r13, 0x0, r12, rax);
			IOG_KTRACE_NT(DBG_IOG_CONNECT_CHANGE_INTERRUPT_V2, DBG_FUNC_NONE, __private->regID, a1, a2, a3);
		__ZN13IOFramebuffer16clamshellHandlerEPvS0_P9IOServiceP10IONotifier == IOFramebuffer::clamshellHandler(void*, void*, IOService*, IONotifier*)
			GTraceBuffer::formatToken(&var_60, rbx, 12626, 33, 25, 0x0, r12, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_CLAMSHELL, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_CLAMSHELL_HANDLER, 0, clamshellState, 0, 0, 0, 0);
		__ZN13IOFramebuffer17cpInterruptActionEP8OSObjectP22IOInterruptEventSourcei == IOFramebuffer::cpInterruptAction(OSObject*, IOInterruptEventSource*, int) + 0x62
			GTraceBuffer::formatToken(&var_60, r14, 15829, 0x83a (2,DBG_IOG_CP_INTERUPT_58), r12, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, rax);
		__ZN13IOFramebuffer19cpInterruptOccurredEP8OSObjectPv == IOFramebuffer::cpInterruptOccurred(OSObject*, void*) + 0x82
			GTraceBuffer::formatToken(&var_68, r15, 15821, 0x43a (1,DBG_IOG_CP_INTERUPT_58), r13, 0x0, rbx != 0x0 ? 0x1 : 0x0, 0x0, 0x0, 0x0, 0x0, rax);
		__ZN13IOFramebuffer24globalExtConnectionCountEv == IOFramebuffer::globalExtConnectionCount() + 0x117
			GTraceBuffer::formatToken(&var_90, r13, 9964, 45, 34, 0x0, var_50, 0x0, 0x0, 0x0, sign_extend_64(r14), rax);
			IOG_KTRACE(DBG_IOG_CONNECTION_ENABLE_CHECK, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_GLOBAL_CONNECTION_COUNT, 0, __private->regID, 0, connectEnabled, 0, err);
		__ZN13IOFramebuffer28clamshellOfflineShouldChangeEb == IOFramebuffer::clamshellOfflineShouldChange(bool) + 0x12e
			GTraceBuffer::formatToken(&var_78, r15, 10058, 33, 32, 0x0, r13, 0x0, var_38, 0x0, rbx, rax);
			IOG_KTRACE(DBG_IOG_CLAMSHELL, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_CLAMSHELL_OFFLINE_CHANGE, 0, __private->regID, 0, reject, 0, state);
		__ZN13IOFramebuffer23displayAssertionCreatedEj == IOFramebuffer::displayAssertionCreated(unsigned int) + 0x7e
			GTraceBuffer::formatToken(&var_60, r15, 10181, 27 (DBG_IOG_RECEIVE_POWER_NOTIFICATION), 5 (DBG_IOG_PWR_EVENT_DISPLAYASSERTIONCREATED), 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, rax);
		__ZN13IOFramebuffer6initFBEv == IOFramebuffer::initFB() + 0xf1
			GTraceBuffer::formatToken(&var_170, r13, 10223, 102, 38, 0x0, stack[-552], 0x0, var_1B0 | var_1AC << 0x20, 0x0, sign_extend_64(r15), rax);
			IOG_KTRACE(DBG_IOG_GET_CURRENT_DISPLAY_MODE, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_INIT_FB, 0, __private->regID, 0, GPACKUINT32T(1, depth) | GPACKUINT32T(0, mode), 0, err);
		__ZN13IOFramebuffer8extCloseEv == IOFramebuffer::extClose() + 0x56
		__ZN13IOFramebuffer8extCloseEv == IOFramebuffer::extClose() + 0x138
			GTraceBuffer::formatToken(&stack[-104], r14, 10769, 0x43c, r15, 0x0, stack[0], stack[8], stack[16], stack[24], stack[32], stack[40]);
			IOG_KTRACE(DBG_IOG_FB_EXT_CLOSE, DBG_FUNC_START, 0, __private->regID, 0, 0, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&stack[-104], rbx, 10784, 0x83c, r14, 0x0, stack[0], stack[8], stack[16], stack[24], stack[32], stack[40]);
			IOG_KTRACE(DBG_IOG_FB_EXT_CLOSE, DBG_FUNC_END  , 0, __private->regID, 0, 0, 0, 0, 0, 0);
		__ZN13IOFramebuffer5closeEv == IOFramebuffer::close() + 0x52
		__ZN13IOFramebuffer5closeEv == IOFramebuffer::close() + 0x1ac
		__ZN13IOFramebuffer5closeEv == IOFramebuffer::close() + 0x23e
		__ZN13IOFramebuffer5closeEv == IOFramebuffer::close() + 0x3a7
			GTraceBuffer::formatToken(&stack[-128], r14, 10806, 0x42a, r15, 0x0, stack[0], stack[8], stack[16], stack[24], stack[32], stack[40]);
			IOG_KTRACE(DBG_IOG_FB_CLOSE, DBG_FUNC_START, 0, __private->regID, 0, 0, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&stack[-128], r12, 10844, 0x428, stack[-56], 0x0, stack[0], stack[8], stack[16], stack[24], stack[32], stack[40]);
			IOG_KTRACE(DBG_IOG_SET_GAMMA_TABLE, DBG_FUNC_START, 0, __private->regID, 0, DBG_IOG_SOURCE_CLOSE, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&stack[-128], rbx, 10852, 0x828, stack[-64], 0x0, stack[0], stack[8], stack[16], stack[24], stack[32], stack[40]);
			IOG_KTRACE(DBG_IOG_SET_GAMMA_TABLE, DBG_FUNC_END, 0, __private->regID, 0, DBG_IOG_SOURCE_CLOSE, 0, ret, 0, 0);
			GTraceBuffer::formatToken(&stack[-128], r14, 10911, 0x82a, r12, 0x0, stack[0], stack[8], stack[16], stack[24], stack[32], stack[40]);
			IOG_KTRACE(DBG_IOG_FB_CLOSE, DBG_FUNC_END, 0, __private->regID, 0, 0, 0, 0, 0, 0);
		__ZN13IOFramebuffer16setWSAAAttributeEjj == IOFramebuffer::setWSAAAttribute(unsigned int, unsigned int) + 0xde
		__ZN13IOFramebuffer16setWSAAAttributeEjj == IOFramebuffer::setWSAAAttribute(unsigned int, unsigned int) + 0x179
		__ZN13IOFramebuffer16setWSAAAttributeEjj == IOFramebuffer::setWSAAAttribute(unsigned int, unsigned int) + 0x24c
		__ZN13IOFramebuffer16setWSAAAttributeEjj == IOFramebuffer::setWSAAAttribute(unsigned int, unsigned int) + 0x2ee
			GTraceBuffer::formatToken(&stack[-144], rbx, 11664, 0x40e, r8, 0x0, stack[0], stack[8], stack[16], stack[24], stack[32], stack[40]);
			IOG_KTRACE(DBG_IOG_WSAA_DEFER_ENTER, DBG_FUNC_START, 0, __private->regID, 0, value, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&stack[-144], r15, 11670, 0x80e, stack[-80], 0x0, stack[0], stack[8], stack[16], stack[24], stack[32], stack[40]);
			IOG_KTRACE(DBG_IOG_WSAA_DEFER_ENTER, DBG_FUNC_END, 0, __private->regID, 0, value, 0, a3, 0, 0);
			GTraceBuffer::formatToken(&stack[-144], rbx, 11706, 0x40f, r8, 0x0, stack[0], stack[8], stack[16], stack[24], stack[32], stack[40]);
			IOG_KTRACE(DBG_IOG_WSAA_DEFER_EXIT, DBG_FUNC_START, 0, __private->regID, 0, value, 0, 0, 0, 0);
			GTraceBuffer::formatToken(&stack[-144], r15, 11712, 0x80f, stack[-80], 0x0, stack[0], stack[8], stack[16], stack[24], stack[32], stack[40]);
			IOG_KTRACE(DBG_IOG_WSAA_DEFER_EXIT, DBG_FUNC_END, 0, __private->regID, 0, value, 0, a3, 0, 0);
		__ZN13IOFramebuffer17extSetDisplayModeEP8OSObjectPvP25IOExternalMethodArguments == IOFramebuffer::extSetDisplayMode(OSObject*, void*, IOExternalMethodArguments*) + 0x74
		__ZN13IOFramebuffer17extSetDisplayModeEP8OSObjectPvP25IOExternalMethodArguments == IOFramebuffer::extSetDisplayMode(OSObject*, void*, IOExternalMethodArguments*) + 0xe5
			GTraceBuffer::formatToken(&var_78, rbx, 11432, 0x464, 3, 0x0, var_38, 0x0, sign_extend_64(r14), 0x0, sign_extend_64(r15), rax);
			IOG_KTRACE(DBG_IOG_SET_DISPLAY_MODE, DBG_FUNC_START, 0, DBG_IOG_SOURCE_EXT_SET_DISPLAY_MODE, 0, inst->__private->regID, 0, displayMode, 0, depth);
			GTraceBuffer::formatToken(&var_78, rbx, 11437, 0x864, 3, 0x0, r12, 0x0, sign_extend_64(r14), 0x0, 0x0, rax);
			IOG_KTRACE(DBG_IOG_SET_DISPLAY_MODE, DBG_FUNC_END, 0, DBG_IOG_SOURCE_EXT_SET_DISPLAY_MODE, 0, inst->__private->regID, 0, err, 0, 0);
		__ZN13IOFramebuffer16doSetDisplayModeEii == IOFramebuffer::doSetDisplayMode(int, int) + 0xc8
			GTraceBuffer::formatToken(&var_78, rbx, 11487, 102, 41, 0x0, var_38, 0x0, var_2C | var_30 << 0x20, 0x0, sign_extend_64(r14), rax);
			IOG_KTRACE(DBG_IOG_GET_CURRENT_DISPLAY_MODE, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_DO_SET_DISPLAY_MODE, 0, __private->regID, 0, GPACKUINT32T(1, depth) | GPACKUINT32T(0, displayMode), 0, err);
		__ZN13IOFramebuffer15extGetAttributeEP8OSObjectPvP25IOExternalMethodArguments == IOFramebuffer::extGetAttribute(OSObject*, void*, IOExternalMethodArguments*) + 0x31f
			GTraceBuffer::formatToken(&var_78, r13, 11982, 59 (DBG_IOG_EXT_GET_ATTRIBUTE_59), rbx, 0x0, var_30, 0x0, var_38, 0x0, r15, rax);
		sub_1f772_setAttribute + 0xcd
			GTraceBuffer::formatToken(&var_68, rbx, 12526, 70, r12, 0x0, (r14 & 0xff) << 0x10 | (COND ? 0x1 : 0x0) | (rcx & 0x1) << 0x8, 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_BUILTIN_PANEL_POWER, DBG_FUNC_NONE, __private->regID, a1, 0, 0);
		__ZN13IOFramebuffer15setAttributeExtEjm == IOFramebuffer::setAttributeExt(unsigned int, unsigned long) + 0x63
		__ZN13IOFramebuffer15setAttributeExtEjm == IOFramebuffer::setAttributeExt(unsigned int, unsigned long) + 0x1bb
			GTraceBuffer::formatToken(&var_70, rbx, 13673, 0x420, r12, 0x0, r13, 0x0, r14, 0x0, 0x0, rax);
			GTraceBuffer::formatToken(&var_70, rbx, 13715, 0x820, r15, 0x0, stack[-168], 0x0, 0x0, 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_SET_ATTRIBUTE_EXT, DBG_FUNC_START, __private->regID, attribute, value, 0);
			IOG_KTRACE(DBG_IOG_SET_ATTRIBUTE_EXT, DBG_FUNC_END, 0, __private->regID, 0, err, 0, 0, 0, 0);
		__ZN13IOFramebuffer12getAttributeEjPm == IOFramebuffer::getAttribute(unsigned int, unsigned long*) + 0xb0
			GTraceBuffer::formatToken(&var_68, r14, 12665, 33, 20, 0x0, r13, 0x0, sign_extend_64(rbx), 0x0, r12, rax);
			IOG_KTRACE(DBG_IOG_CLAMSHELL, DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_GET_ATTRIBUTE, 0, __private->regID, 0, err, 0, *resultP);
		__ZN13IOFramebuffer24deliverGroupNotificationEijbiPv == IOFramebuffer::deliverGroupNotification(int, unsigned int, bool, int, void*) + 0x239
		__ZN13IOFramebuffer24deliverGroupNotificationEijbiPv == IOFramebuffer::deliverGroupNotification(int, unsigned int, bool, int, void*) + 0x309
		__ZN13IOFramebuffer24deliverGroupNotificationEijbiPv == IOFramebuffer::deliverGroupNotification(int, unsigned int, bool, int, void*) + 0x3bd
		__ZN13IOFramebuffer24deliverGroupNotificationEijbiPv == IOFramebuffer::deliverGroupNotification(int, unsigned int, bool, int, void*) + 0x488
			GTraceBuffer::formatToken(&var_D8, rbx, 13468, 0x42b, var_68, 0x2000, var_50, 0x2000, var_48, 0x2000, var_40, rax);
			IOG_KTRACE_DEFER_START( DBG_IOG_NOTIFY_CALLOUT_TIMEOUT, DBG_FUNC_START, 0, __private->regID, kGTRACE_ARGUMENT_STRING, nameBufInt[0], kGTRACE_ARGUMENT_STRING, nameBufInt[1], kGTRACE_ARGUMENT_STRING, nameBufInt[2]);
			GTraceBuffer::formatToken(&var_130, rsi, 13473, 0x82b, var_E0, 0x0, sign_extend_64(r15), 0x0, 0x0, 0x0, 0x0, rbx);
			IOG_KTRACE_DEFER_END(DBG_IOG_NOTIFY_CALLOUT_TIMEOUT, DBG_FUNC_END, 0, event, 0, r, 0, 0, 0, 0, kNOTIFY_TIMEOUT_NS);
			GTraceBuffer::formatToken(&var_D8, rbx, 13493, 0x42b, var_68, 0x2000, var_50, 0x2000, var_48, 0x2000, var_40, rax);
			IOG_KTRACE_DEFER_START( DBG_IOG_NOTIFY_CALLOUT_TIMEOUT, DBG_FUNC_START, 0, __private->regID, kGTRACE_ARGUMENT_STRING, nameBufInt[0], kGTRACE_ARGUMENT_STRING, nameBufInt[1], kGTRACE_ARGUMENT_STRING, nameBufInt[2]);
			GTraceBuffer::formatToken(&var_130, rsi, 13498, 0x82b, var_E0, 0x0, sign_extend_64(r15), 0x0, 0x0, 0x0, 0x0, rbx);
			IOG_KTRACE_DEFER_END(DBG_IOG_NOTIFY_CALLOUT_TIMEOUT, DBG_FUNC_END, 0, event, 0, r, 0, 0, 0, 0, kNOTIFY_TIMEOUT_NS);
		__ZN13IOFramebuffer28setAttributeForConnectionExtEijm == IOFramebuffer::setAttributeForConnectionExt(int, unsigned int, unsigned long) + 0xe3
		__ZN13IOFramebuffer28setAttributeForConnectionExtEijm == IOFramebuffer::setAttributeForConnectionExt(int, unsigned int, unsigned long) + 0x16c
		__ZN13IOFramebuffer28setAttributeForConnectionExtEijm == IOFramebuffer::setAttributeForConnectionExt(int, unsigned int, unsigned long) + 0x25d
			GTraceBuffer::formatToken(&var_90, r14, 13769, 50, var_40, 0x0, var_48, 0x0, stack[-184], 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_AGC_MUTE, DBG_FUNC_NONE, GPACKUINT64T(__private->regID), GPACKUINT32T(0, value), GPACKBIT(0, __private->controller->isMuted()), 0);
			GTraceBuffer::formatToken(&var_90, r14, 13778, 0x441, r8, 0x0, 0x70726f62, 0x0, rbx, 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_SET_ATTR_FOR_CONN_EXT, DBG_FUNC_START, __private->regID, attribute, value, 0);
			GTraceBuffer::formatToken(&var_90, rbx, 13805, var_38 (441 or 841), r15, 0x0, r12, 0x0, var_30, 0x0, sign_extend_64(r14), rax);
			IOG_KTRACE_NT(DBG_IOG_SET_ATTR_FOR_CONN_EXT, endTag, __private->regID, attribute, value, err);
		__ZN13IOFramebuffer25getAttributeForConnectionEijPm == IOFramebuffer::getAttributeForConnection(int, unsigned int, unsigned long*) + 0x140
			GTraceBuffer::formatToken(&var_70, r12, 14067, 44, r14, 0x0, var_30, 0x0, sign_extend_64(r15), 0x0, 0x0, rax);
			IOG_KTRACE_NT(DBG_IOG_CONNECTION_ENABLE, DBG_FUNC_NONE, __private->regID, *value, err, 0);

	__ZN12GTraceBuffer11recordTokenEtyyyyyyyyy_7482 == GTraceBuffer::recordToken(unsigned short, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long) + 0x33
		__ZN9IODisplay20setDisplayPowerStateEm == IODisplay::setDisplayPowerState(unsigned long) + 0x7b
			(line 1748) IOG_KTRACE(DBG_IOG_CHANGE_POWER_STATE_PRIV (28), DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_IODISPLAY (6), 0, state, 0, 0, 0, 0);
		__ZN9IODisplay13setPowerStateEmP9IOService == IODisplay::setPowerState(unsigned long, IOService*) + 0x42
			(line 1781) IOG_KTRACE(DBG_IOG_SET_POWER_STATE (16), DBG_FUNC_NONE, 0, powerState, 0, DBG_IOG_SOURCE_IODISPLAY (6), 0, 0, 0, 0);
	__ZN16IODisplayConnect17recordGTraceTokenEttytytyty == IODisplayConnect::recordGTraceToken(unsigned short, unsigned short, unsigned long long, unsigned short, unsigned long long, unsigned short, unsigned long long, unsigned short, unsigned long long) + 0x5c
		__ZN9IODisplay12setParameterEP12OSDictionaryPK8OSSymboli == IODisplay::setParameter(OSDictionary*, OSSymbol const*, int) + 0x38d
			(line {1451,261}, {1455,261}, {1458,256}, {1461,256}, {1464,264}, {1467,260}) IODisplayConnect::recordGTraceToken(rdi, rsi, rdx, 0x0, r8, 0x2000, stack[-120], 0x2000, stack[-104]);
			(line 1451,261) if (gIODisplayBrightnessProbeKey       == paramName) staticABL_GT_SET_BRIGHTNESS_PROBE(params, ABL_SOURCE_IOD_SETPARAMETER, value, -1, /* isLin */ false);        // a1 = 0x____ffff00000200ULL | r13 << 48;
			(line 1455,261) if (gIODisplayLinearBrightnessProbeKey == paramName) staticABL_GT_SET_BRIGHTNESS_PROBE(params, ABL_SOURCE_IOD_SETPARAMETER, -1, value, /* isLin */ true);         // a1 = 0xffff____80000200ULL | r13 << 32;
			(line 1458,256) if (gIODisplayBrightnessKey            == paramName) staticABL_GT_SET_BRIGHTNESS      (params, ABL_SOURCE_IOD_SETPARAMETER, value, -1, /* isLin */ false);        // a1 = 0x____ffff00000200ULL | r13 << 48;
			(line 1461,256) if (gIODisplayLinearBrightnessKey      == paramName) staticABL_GT_SET_BRIGHTNESS      (params, ABL_SOURCE_IOD_SETPARAMETER, -1, value, /* isLin */ true);         // a1 = 0xffff____80000200ULL | r13 << 32;
			(line 1464,264) if (gIODisplayParametersCommitKey      == paramName) staticABL_GT_COMMITTED           (params, ABL_SOURCE_IOD_SETPARAMETER, -1, /*nvram*/false, /*setup*/ false); // a1 = 0xffff000000000200ULL
			(line 1467,260) if (gIODisplayPowerStateKey            == paramName) staticABL_GT_SET_DISPLAY_POWER   (params, ABL_SOURCE_IOD_SETPARAMETER, value, -1, -1);                       // a1 = 0xffff0000ff__0200ULL | r13 << 16;
	__ZN16IODisplayConnect17recordGTraceTokenEtthtytyty == IODisplayConnect::recordGTraceToken(unsigned short, unsigned short, unsigned char, unsigned short, unsigned long long, unsigned short, unsigned long long, unsigned short, unsigned long long) + 0x45
		__ZN9IODisplay19addParameterHandlerEP25IODisplayParameterHandler == IODisplay::addParameterHandler(IODisplayParameterHandler*) + 0x92
			(line 852,263) IODisplayConnect::recordGTraceToken(*(r15 + 0x88), 852, 263, 0x0, 0x0, r9, 0, 0, 0);
			ABL_GT_SET_DISPLAY(this, ABL_SOURCE_IOD_ADDPARAMETERHANDLER, -1, isBuiltin, false); // a1 = 0xffff000040000100ULL
                                                                                                // a1 = 0xffff000000000100ULL
		__ZN9IODisplay12doIntegerSetEP12OSDictionaryPK8OSSymbolj == IODisplay::doIntegerSet(OSDictionary*, OSSymbol const*, unsigned int) + 0x256
			(line {1571,261}, {1574,261}, {1577,256}, {1580,256}, {1583,264}, {1586,260}) IODisplayConnect::recordGTraceToken(rdi, rsi, rdx, 0x0, 0x0, r9, rax, stack[-112], stack[-104]);
			(line 1571,261) if (gIODisplayBrightnessProbeKey       == paramName)       ABL_GT_SET_BRIGHTNESS_PROBE(this, ABL_SOURCE_IOD_DOINTEGERSET, value, -1, /* isLin */ false);          // a1 = 0x____ffff00000300ULL | r14 << 48;
			(line 1574,261) if (gIODisplayLinearBrightnessProbeKey == paramName)       ABL_GT_SET_BRIGHTNESS_PROBE(this, ABL_SOURCE_IOD_DOINTEGERSET, -1, value, /* isLin */ true);           // a1 = 0xffff____80000300ULL | r14 << 32;
			(line 1577,256) if (gIODisplayBrightnessKey            == paramName)       ABL_GT_SET_BRIGHTNESS      (this, ABL_SOURCE_IOD_DOINTEGERSET, value, -1, /* isLin */ false);          // a1 = 0x____ffff00000300ULL | r14 << 48;
			(line 1580,256) if (gIODisplayLinearBrightnessKey      == paramName)       ABL_GT_SET_BRIGHTNESS      (this, ABL_SOURCE_IOD_DOINTEGERSET, -1, value, /* isLin */ true);           // a1 = 0xffff____80000300ULL | r14 << 32;
			(line 1583,264) if (gIODisplayParametersCommitKey      == paramName)       ABL_GT_COMMITTED           (this, ABL_SOURCE_IOD_DOINTEGERSET, -1, /*nvram*/false, /*setup*/ false);   // a1 = 0xffff000000000300ULL
			(line 1586,260) if (gIODisplayPowerStateKey            == paramName)       ABL_GT_SET_DISPLAY_POWER   (this, ABL_SOURCE_IOD_DOINTEGERSET, value, -1, -1);                         // a1 = 0xffff0000ff__0300ULL | r14 << 16;
		__ZN9IODisplay8doUpdateEv == IODisplay::doUpdate() + 0x6a
			(line 1656, 262) IODisplayConnect::recordGTraceToken(*(r14 + 0x88), 1656, 262, 0x0, 0x0, 0xffffffff00000400ULL | (r12 & 0xff) << 0x1f, 0x0, 0x0, 0x0);              // a1 = 0xffffffff00000400ULL
			ABL_GT_DO_UPDATE(this, ABL_SOURCE_IOD_DOUPDATE, ok, /* update count */ -1);
	__ZN9IODisplay17recordGTraceTokenEtthtytyty == IODisplay::recordGTraceToken(unsigned short, unsigned short, unsigned char, unsigned short, unsigned long long, unsigned short, unsigned long long, unsigned short, unsigned long long) + 0xc
		none
	__Z15agdcGTraceTokenPK13IOFramebuffertbthtytyty == agdcGTraceToken(IOFramebuffer const*, unsigned short, bool, unsigned short, unsigned char, unsigned short, unsigned long long, unsigned short, unsigned long long, unsigned short, unsigned long long) + 0x86
		GTraceBuffer::formatToken(&var_78, rbx, var_2C & 0xffff, (rcx & 0x3ff | (r8 & 0x3 & 0xff) << 0xa) & 0xffff, rsi, r9 & 0xffff, var_8, rdx & 0xffff, var_24, r14, var_40, rax);
		not called by anything in IOGraphicsFamily

#endif


#ifdef __cplusplus
extern "C" {
#endif
void DumpOneAttribute(UInt64 attribute, bool set, bool forConnection, void *valuePtr, size_t len, bool align = false);
#ifdef __cplusplus
}
#endif

void dumpTokenBuffer(FILE* outfile, const vector<GTraceBuffer>& gtraces)
{
#if 0
	unsigned currentLine = 0;
    long total_lines = 1; // Include magic
    for (const GTraceBuffer& buffer : gtraces)
        total_lines += buffer.vec().size();
    if (!total_lines) {
        fprintf(outfile, "-1 (No Token Data Recorded)\n\n");
        return;
    }

    // Marks beginning of a multi
    const GTraceEntry magic(gtraces.size(), GTRACE_REVISION);
    string line = formatEntry(currentLine++, magic);

    fputs("\n\n", outfile);
    fprintf(outfile, "Token Buffers Recorded: %d\n",
            static_cast<int>(gtraces.size()));
    fprintf(outfile, "Token Buffer Lines    : %ld\n", total_lines);
    fprintf(outfile, "Token Buffer Size     : %ld\n",
            total_lines * kGTraceEntrySize);
    fprintf(outfile, "Token Buffer Data     :\n");
    fputs(line.c_str(), outfile); // out magic marks as v2 or later

    for (const GTraceBuffer& buffer : gtraces) {
        for (const GTraceEntry& entry : buffer.vec()) {
            line = formatEntry(currentLine++, entry);
            fputs(line.c_str(), outfile);
        }
    }
    fputc('\n', outfile);
#endif
	cprintf("\n\n");
	iprintf("Token Buffers = {\n"); INDENT

	unsigned currentBuffer = 0;
	for (const GTraceBuffer& buffer : gtraces) {
		iprintf("[%d] = {\n", currentBuffer); INDENT

		unsigned currentLine = 0;
		GTraceHeader *header = NULL;
		for (const GTraceEntry& entry : buffer.vec()) {
			if (currentLine == 0) {
				header = (GTraceHeader*)&entry;
				iprintf(
					"DecoderName:\"%s\"  BufferName:\"%s\"\n",
					(char*)&header->fDecoderName,
					(char*)&header->fBufferName
				);
				
				char thenstr[100];
				strftimestamp(thenstr, sizeof(thenstr), "%F %T", 0, header->fCreationTime);

				iprintf("CreationTime:%lld=%s  BufferID:%d  BufferSize:%d  TokenLine:%d  Version:%d  BufferIndex:%d  TokenMask:0x%x  BreadcrumbTokens:%d  TokensCopied:%d\n",
					header->fCreationTime,
					thenstr,
					header->fBufferID,
					header->fBufferSize,
					header->fTokenLine,
					header->fVersion,
					header->fBufferIndex,
					header->fTokensMask,
					header->fBreadcrumbTokens,
					header->fTokensCopied
				);
			}
			else if (currentLine > 1) { // header takes two entries
				if (currentLine == 2) {
					qsort((void*)&entry, header->fTokensCopied, sizeof(entry), entrycompare);
				}
				
				char buf[100];
				char buf2[20];
				char thenstr[100];
				char typebuf[20];
				char tagbuf[20];
				char tagtypebuf[20];
				const char *typeStr;
				const char *tagStr;

				strftimestamp(thenstr, sizeof(thenstr), "%F %T", 0, entry.timestamp());

				tagStr = GetGTraceTagStr(tagbuf, sizeof(tagbuf), GUNPACKFUNCCODE(entry.tag()));

				switch (GUNPACKFUNCTYPE(entry.tag())) {
					case DBG_FUNC_START : typeStr = "[ "; break;
					case DBG_FUNC_END   : typeStr = "] "; break;
					case DBG_FUNC_NONE  : typeStr = "  "; break;
					default             : typeStr = typebuf; snprintf(typebuf, sizeof(typebuf), "?%d", GUNPACKFUNCTYPE(entry.tag()));
				}

				iprintf("timestamp:%20llu=%s line:%-5u component:%#llx cpu:%-2u thread:%#-8x regid:%#x %s%s ",
						entry.timestamp(), thenstr, entry.line(), entry.component(),
						entry.cpu(), entry.threadID(), entry.registryID(),
						typeStr, tagStr
				);

				int arg = 0;
				int64_t err = 0;
				uint64_t src = 0;
				uint64_t attribute = 0;
				uint64_t value = 0;
				int i, j;
				

#define tagzero \
	do { \
		if (arg > 3) cprintf(" ?too many args"); \
		else if (arg > 0 && entry.fArgsTag.tag(arg)) \
			cprintf(" ?tag:%s:", GetTagTypeStr(tagtypebuf, sizeof(tagtypebuf), entry.fArgsTag.tag(arg))); \
		else cprintf(" "); \
	} while (0)

#define GTRACEARG_srcnamed(_name)               do { tagzero; src = entry.arg64(arg); cprintf("%s:%s", _name, GetGTraceSourceStr(buf, sizeof(buf), src)); arg++; } while (0)
#define GTRACEARG_regIDnamed(_name)             do { tagzero; cprintf("%s:%#llx"                       , _name, entry.arg64(arg)); arg++; } while (0)
#define GTRACEARG_units(_name, _units)          do { tagzero; cprintf("%s:%lld%s"                      , _name, entry.arg64(arg), _units); arg++; } while (0)
#define GTRACEARG_bool(_name)                   do { tagzero; cprintf("%s:%s"                          , _name, entry.arg64(arg) == 1 ? "true" : entry.arg64(arg) == 0 ? "false" : UNKNOWN_VALUE(entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_int(_name)                    do { tagzero; cprintf("%s:%lld"                        , _name, entry.arg64(arg)); arg++; } while (0)
#define GTRACEARG_hex(_name)                    do { tagzero; cprintf("%s:%#llx"                       , _name, entry.arg64(arg)); arg++; } while (0)
#define GTRACEARG_minus1(_name)                 do { tagzero; cprintf("%s:%s%s"                        , _name, entry.arg64(arg) != -1 ? "?" : "", entry.arg64(arg) == -1 ? "-1" : UNKNOWN_VALUE(entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_string(_name, _len)           do { cprintf(" %s:%s"  , _name, GetEntryString(buf, sizeof(buf), entry, arg, _len)); arg += (_len + 7) / 8; } while (0)

#define GTRACEARG_src                           GTRACEARG_srcnamed("src")
#define GTRACEARG_regID                         GTRACEARG_regIDnamed("regID")
#define GTRACEARG_notifystate                   do { tagzero; cprintf("%s", GetNotifyServerMessageStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_sentAckedPower                do { tagzero; cprintf("sentPower:%s ackedPower:%s", GetPowerStr(buf, sizeof(buf), entry.arg32(arg*2+1)), GetPowerStr(buf2, sizeof(buf2), entry.arg32(arg*2))); arg++; } while (0)
#define GTRACEARG_wsaaValue                     do { tagzero; cprintf("%s", GetWSAAStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_error32                       do { tagzero; cprintf("%s", DumpOneReturn32(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_powerState                    do { tagzero; cprintf("powerState:%s%lld"              , entry.arg64(arg) > kIODisplayMaxPowerState ? "?" : "", entry.arg64(arg)); arg++; } while (0)
#define GTRACEARG_messageType                   do { tagzero; cprintf("messageType:%s", GetMessageStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_fromCapabilities              do { tagzero; cprintf("fromCapabilities:%s"            , GetSystemCapabilityStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_toCapabilities                do { tagzero; cprintf("toCapabilities:%s"              , GetSystemCapabilityStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_changeFlags                   do { tagzero; cprintf("changeFlags:%s"                 , GetChangeFlagsStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_pmValue                       do { tagzero; cprintf("pmValue:%s"                     , GetPMStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_event                         do { tagzero; cprintf("event:%s"                       , GetIOFBNotifyStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_attribute_value               do { tagzero; attribute = entry.arg64(arg); arg++; tagzero; value = entry.arg64(arg); arg++; DumpOneAttribute(attribute, true, false, &value, sizeof(value)); } while (0)
#define GTRACEARG_error                         do { tagzero; cprintf("%s"                             , DumpOneReturn(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_muxpowerExitType              do { tagzero; cprintf("exitType:%s"                    , GetExitTypeStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_powerevent                    do { tagzero; cprintf("powerEvent:%s"                  , GetPowerEventStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_bits                          do { tagzero; cprintf("%s"                             , GetBitsStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_state                         do { tagzero; cprintf("%s"                             , GetStateStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_counts                        do { tagzero; cprintf("%s"                             , GetCountsStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_clamshellState                do { tagzero; cprintf("clamshellState:%s"              , GetClamshellStateStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_op                            do { tagzero; cprintf("op:%s"                          , GetOpStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_widthHeight                   do { tagzero; cprintf("width:%d height:%d"             , entry.arg32(arg*2+1), entry.arg32(arg*2)); arg++; } while (0)
#define GTRACEARG_depthScale                    do { tagzero; cprintf("depth:%d scale:%d"              , entry.arg32(arg*2+1), entry.arg32(arg*2)); arg++; } while (0)
#define GTRACEARG_type                          do { tagzero; cprintf("type:%s"                        , GetTypeStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_location                      do { tagzero; cprintf("location:%s"                    , GetLocationStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_return_code                   do { tagzero; cprintf("%s"                             , DumpOneReturn(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_mode                          do { tagzero; cprintf("mode:%s"                        , GetProcessConnectChangeModeStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_switchState                   do { tagzero; cprintf("switchState:%s"                 , GetSwitchStateStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_last_processed                do { tagzero; cprintf("lastProcessedChange:%#llx"      , entry.arg64(arg)); arg++; } while (0) // ?
#define GTRACEARG_last_finished_and_messaged    do { tagzero; cprintf("LastFinishedChange & fLastMessagedChange:%#llx" , entry.arg64(arg)); arg++; } while (0) // ?
#define GTRACEARG_changed_and_last_forced       do { tagzero; cprintf("lastMessagedChange & last_forced:%#llx" , entry.arg64(arg)); arg++; } while (0) // ?
#define GTRACEARG_connectc1                     do { tagzero; value = entry.arg64(arg); cprintf("fLastFinishedChange:%#x fLastMessagedChange:%#x lastMessagedChange:%#x connectChange:%#x", GUNPACKUINT16T(0, value), GUNPACKUINT16T(1, value), GUNPACKUINT16T(2, value), GUNPACKUINT16T(3, value)); arg++; } while (0)
#define GTRACEARG_connectc2                     do { tagzero; value = entry.arg64(arg); cprintf("discard:%s gIOFBIsMuxSwitching:%s%s%s", GUNPACKBIT(0, value) ? "true" : "false", GUNPACKBIT(1, value) ? "true" : "false", (value & ~3LL) ? " " : "", (value & ~3LL) ? UNKNOWN_VALUE(value) : ""); arg++; } while (0)
#define GTRACEARG_retrainc1                     do { tagzero; value = entry.arg64(arg); cprintf("fConnectChange:%#x fLastForceRetrain:%#x%s%s", GUNPACKUINT16T(0, value), GUNPACKUINT16T(1, value),  (value & ~0x0ffffffffLL) ? " " : "", (value & ~0x0ffffffffLL) ? UNKNOWN_VALUE(value) : ""); arg++; } while (0)
#define GTRACEARG_retrainc2                     do { tagzero; value = entry.arg64(arg); cprintf("fOnlineMask:%#x isMuted:%s%s%s", GUNPACKUINT32T(0, value), GUNPACKBIT(63, value) ? "true" : "false", (value & ~0x80000000ffffffffLL) ? " " : "", (value & ~0x80000000ffffffffLL) ? UNKNOWN_VALUE(value) : ""); arg++; } while (0)
#define GTRACEARG_workasync1                    do { tagzero; value = entry.arg64(arg); cprintf("fConnectChange:%#x lastProcessedChange:%#x fLastFinishedChange:%#x fPostWakeChange:%#x", GUNPACKUINT16T(0, value), GUNPACKUINT16T(1, value), GUNPACKUINT16T(2, value), GUNPACKUINT16T(3, value)); arg++; } while (0)
#define GTRACEARG_workasync2                    do { tagzero; value = entry.arg64(arg); cprintf("isMuted:%s 1:%s fMuxNeedsBgOn:%s fMuxNeedsBgOff:%s%s%s", GUNPACKBIT(63, value) ? "true" : "false", GUNPACKBIT(62, value) ? "true" : "false", GUNPACKBIT(61, value) ? "true" : "false", GUNPACKBIT(60, value) ? "true" : "false", (value & ~0xf000000000000000ULL) ? " " : "", (value & ~0xf000000000000000ULL) ? UNKNOWN_VALUE(value) : ""); arg++; } while (0)
#define GTRACEARG_endconnectc1                  do { tagzero; value = entry.arg64(arg); cprintf("messaged:%s fOnlineMask%#x%s%s", GUNPACKBIT(0, value) ? "true" : "false", GUNPACKUINT32T(1, value), (value & ~0xffffffff00000001LL) ? " " : "", (value & ~0xffffffff00000001LL) ? UNKNOWN_VALUE(value) : ""); arg++; } while (0)
#define GTRACEARG_endconnectc2                  do { tagzero; value = entry.arg64(arg); cprintf("fConnectChange:%#x fLastMessagedChange:%#x fLastFinishedChange:%#x fPostWakeChange:%#x lastProcessedChange:%#x%s%s", GUNPACKUINT8T(0, value), GUNPACKUINT8T(1, value), GUNPACKUINT8T(2, value), GUNPACKUINT8T(3, value), GUNPACKUINT8T(4, value), (value & ~0x0ffffffffffLL) ? " " : "", (value & ~0x0ffffffffffLL) ? UNKNOWN_VALUE(value) : ""); arg++; } while (0)
#define GTRACEARG_processconnecta1              do { tagzero; value = entry.arg64(arg); cprintf("msgd:%s isMuted:%s%s%s", GUNPACKBIT(0, value) ? "true" : "false", GUNPACKBIT(1, value) ? "true" : "false", (value & ~0x03LL) ? " " : "", (value & ~0x03LL) ? UNKNOWN_VALUE(value) : ""); arg++; } while (0)
#define GTRACEARG_processconnecta2              do { tagzero; value = entry.arg64(arg); cprintf("lastProcessedChange:%#x fConnectChange:%#x", GUNPACKUINT32T(0, value), GUNPACKUINT32T(1, value)); arg++; } while (0)
#define GTRACEARG_processconnecta3              do { tagzero; value = entry.arg64(arg); cprintf("aliasMode:%#x%s%s", GUNPACKUINT32T(0, value), (value & ~0x0ffffffffLL) ? " " : "", (value & ~0x0ffffffffLL) ? UNKNOWN_VALUE(value) : ""); arg++; } while (0)
#define GTRACEARG_modeID                        do { tagzero; cprintf("modeID:%#llx"                   , entry.arg64(arg)); arg++; } while (0)
#define GTRACEARG_work(_name)                   do { tagzero; cprintf("%s:%s"                          , _name, GetWorkStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_allState                      do { tagzero; cprintf("allState:%s"                    , GetAllStateStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_gIOFBGlobalEvents             do { tagzero; cprintf("gIOFBGlobalEvents:%s"           , GetIOFBEventStr(buf, sizeof(buf), entry.arg64(arg))); arg++; } while (0)
#define GTRACEARG_builtinpanelpowera1           do { tagzero; value = entry.arg64(arg); cprintf("gIOGraphicsControl:%s isMuted:%s value:%s%s%s", GUNPACKUINT8T(0, value) ? "true" : "false", GUNPACKUINT8T(1, value) ? "true" : "false", GUNPACKUINT8T(2, value) ? "true" : "false", (value & ~0x0010101LL) ? " " : "", (value & ~0x0010101LL) ? UNKNOWN_VALUE(value) : ""); arg++; } while (0)
#define GTRACEARG_connectchangeinterrupta1      do { tagzero; value = entry.arg64(arg); cprintf("src:%s lastProcessedChange:%#x%s%s", GetGTraceSourceStr(buf, sizeof(buf), GUNPACKUINT16T(0, value)), GUNPACKUINT32T(1, value), (value & ~0xffffffff0000ffffLL) ? " " : "", (value & ~0xffffffff0000ffffLL) ? UNKNOWN_VALUE(value) : ""); arg++; } while (0)
#define GTRACEARG_connectchangeinterrupta2      do { tagzero; value = entry.arg64(arg); cprintf("fLastMessagedChange:%#x fLastFinishedChange:%#x", GUNPACKUINT32T(0, value), GUNPACKUINT32T(1, value)); arg++; } while (0)
#define GTRACEARG_connectchangeinterrupta3      do { tagzero; value = entry.arg64(arg); cprintf("fConnectChange:%#x fLastForceRetrain:%#x", GUNPACKUINT32T(0, value), GUNPACKUINT32T(1, value)); arg++; } while (0)
#define GTRACEARG_depth                         do { tagzero; cprintf("depth:%lld"                     , entry.arg64(arg)); arg++; } while (0)
#define GTRACEARG_modeAndDepth                  do { tagzero; cprintf("mode:%#x depth:%d", GUNPACKUINT32T(0, value), GUNPACKUINT32T(1, value)); arg++; } while (0)
#define GTRACEARG_ablsetbrightnessa1            do { tagzero; value = entry.arg64(arg); cprintf("verion:%#x where:%s isLinear:%s lb:%d b:%d%s%s", GUNPACKUINT8T(0, value), GetABLSourceStr(buf, sizeof(buf), GUNPACKUINT8T(1, value)), GUNPACKBIT(31, value) ? "true" : "false", (int16_t)GUNPACKUINT16T(2, value), (int16_t)GUNPACKUINT16T(3, value), (value & ~0xffffffff8000ff00LL) ? " " : "", (value & ~0xffffffff8000ff00LL) ? UNKNOWN_VALUE(value) : ""); arg++; } while (0)
#define GTRACEARG_ablsetdisplaypowera1          do { tagzero; value = entry.arg64(arg); cprintf("verion:%#x where:%s newVal:%d oldVal:%d ind:%d%s%s", GUNPACKUINT8T(0, value), GetABLSourceStr(buf, sizeof(buf), GUNPACKUINT8T(1, value)), (int8_t)GUNPACKUINT8T(2, value), (int8_t)GUNPACKUINT8T(3, value), (int16_t)GUNPACKUINT16T(3, value), (value & ~0xffff0000ffffff00LL) ? " " : "", (value & ~0xffff0000ffffff00LL) ? UNKNOWN_VALUE(value) : ""); arg++; } while (0)
#define GTRACEARG_abldoupdatea1                 do { tagzero; value = entry.arg64(arg); cprintf("verion:%#x where:%s ok:%s updateCount:%d%s%s", GUNPACKUINT8T(0, value), GetABLSourceStr(buf, sizeof(buf), GUNPACKUINT8T(1, value)), GUNPACKBIT(31, value) ? "true" : "false", (int32_t)GUNPACKUINT32T(1, value), (value & ~0xffffffff8000ff00LL) ? " " : "", (value & ~0xffffffff8000ff00LL) ? UNKNOWN_VALUE(value) : ""); arg++; } while (0)
#define GTRACEARG_ablsetdisplaya1               do { tagzero; value = entry.arg64(arg); cprintf("verion:%#x where:%s isBuiltIn:%s%s%s%s%s%s", GUNPACKUINT8T(0, value), GetABLSourceStr(buf, sizeof(buf), GUNPACKUINT8T(1, value)), GUNPACKBIT(30, value) ? "true" : "false", GUNPACKBIT(31, value) ? " ?unknown:true" : "", (int16_t)GUNPACKUINT16T(3, value) != -1 ? " ?unknown:" : "", (int16_t)GUNPACKUINT16T(3, value) != -1 ? UNKNOWN_VALUE(GUNPACKUINT16T(3, value)) : "", (value & ~0xffff0000c000ff00LL) ? " " : "", (value & ~0xffff0000c000ff00LL) ? UNKNOWN_VALUE(value) : ""); arg++; } while (0)
#define GTRACEARG_ablcommita1                   do { tagzero; value = entry.arg64(arg); cprintf("verion:%#x where:%s setup:%s nvram:%s lb:%d%s%s", GUNPACKUINT8T(0, value), GetABLSourceStr(buf, sizeof(buf), GUNPACKUINT8T(1, value)), GUNPACKBIT(30, value) ? "true" : "false", GUNPACKBIT(31, value) ? "true" : "false", (int16_t)GUNPACKUINT16T(3, value), (value & ~0xffff0000c000ff00LL) ? " " : "", (value & ~0xffff0000c000ff00LL) ? UNKNOWN_VALUE(value) : ""); arg++; } while (0)
				
#define GTRACEARG_entry(_x)                     do { if (GUNPACKFUNCTYPE(entry.tag()) == DBG_FUNC_START) { _x; } } while (0)
#define GTRACEARG_exit(_x)                      do { if (GUNPACKFUNCTYPE(entry.tag()) == DBG_FUNC_END) { _x; } } while (0)
#define GTRACEARG_ifnoterror(_x)                do { err = entry.arg64(arg); if (err < 0) { GTRACEARG_error; } else { _x; } } while (0)
#define GTRACEARG_ifsrc(_src,_x)                do { if (src == _src) { _x; } } while (0)
#define GTRACEARG_iftype(_type,_x)              do { if (entry.fArgsTag.tag(arg) == _type) { _x; } } while (0)
			

				switch (GUNPACKFUNCCODE(entry.tag())) {
					case DBG_BUFFER_BRACKET                  : break;
					case DBG_IOG_NOTIFY_SERVER               : GTRACEARG_regID; GTRACEARG_notifystate; GTRACEARG_sentAckedPower; break; // __private->regID, msgh_id, sentAckedPower, hidden
					case DBG_IOG_SERVER_ACK                  : GTRACEARG_regID; GTRACEARG_notifystate; GTRACEARG_sentAckedPower; break; // __private->regID, msgh_id, sentAckedPower, hidden
					case DBG_IOG_VRAM_RESTORE                : GTRACEARG_regID; break; // __private->regID, 0, 0, 0
					case DBG_IOG_VRAM_BLACK                  : GTRACEARG_regID; break; // __private->regID, 0, 0, 0
					case DBG_IOG_WSAA_DEFER_ENTER            : GTRACEARG_regID; GTRACEARG_wsaaValue; GTRACEARG_exit(GTRACEARG_error32); break;
						// DBG_FUNC_START, __private->regID, value, 0, 0
						// DBG_FUNC_END  , __private->regID, value, a3, 0
					case DBG_IOG_WSAA_DEFER_EXIT             : GTRACEARG_regID; GTRACEARG_wsaaValue; GTRACEARG_exit(GTRACEARG_error32); break;
						// DBG_FUNC_START, __private->regID, value, 0, 0
						// DBG_FUNC_END  , __private->regID, value, a3, 0
					case DBG_IOG_SET_POWER_STATE             : GTRACEARG_powerState; GTRACEARG_src; break; // powerState, src, 0, 0
					case DBG_IOG_SYSTEM_POWER_CHANGE         : GTRACEARG_messageType; GTRACEARG_exit(GTRACEARG_ifnoterror(GTRACEARG_fromCapabilities; GTRACEARG_toCapabilities; GTRACEARG_changeFlags)); break;
						// DBG_FUNC_START, messageType, 0, 0, 0
						// DBG_FUNC_END  , messageType, params->fromCapabilities, params->toCapabilities, params->changeFlags
						// DBG_FUNC_END  , messageType, ret, 0, 0
					case DBG_IOG_ACK_POWER_STATE             : GTRACEARG_src; GTRACEARG_ifsrc(DBG_IOG_SOURCE_CHECK_POWER_WORK, GTRACEARG_regID; GTRACEARG_powerState;); break;
						// 0, DBG_IOG_SOURCE_IODISPLAY (6), 0, 0, 0, 0, 0, 0
						// DBG_IOG_SOURCE_CHECK_POWER_WORK, 0, __private->regID, 0, newState, 0, 0
					case DBG_IOG_SET_POWER_ATTRIBUTE         : GTRACEARG_regID; GTRACEARG_powerState; break;
						// DBG_FUNC_START, __private->regID, newState, 0, 0
						// DBG_FUNC_END  , __private->regID, newState, 0, 0
					case DBG_IOG_ALLOW_POWER_CHANGE          : break; // 0, 0, 0, 0
					case DBG_IOG_MUX_ALLOW_POWER_CHANGE      : break; // 0, 0, 0, 0
					case DBG_IOG_SERVER_TIMEOUT              : GTRACEARG_regID; GTRACEARG_src; GTRACEARG_sentAckedPower; GTRACEARG_int("serverMsgCount"); break; // fb->__private->regID, DBG_IOG_SOURCE_SERVER_ACK_TIMEOUT, powerState, fb->__private->fServerMsgCount
					case DBG_IOG_NOTIFY_CALLOUT              : GTRACEARG_regID; GTRACEARG_string("className", 24); break; // no known occurrences
					case DBG_IOG_MUX_POWER_MESSAGE           : GTRACEARG_entry(GTRACEARG_messageType); GTRACEARG_exit(GTRACEARG_error; GTRACEARG_muxpowerExitType); break;
						// DBG_FUNC_START, messageType, 0, 0, 0
						// DBG_FUNC_END, kIOReturnSuccess, -1, 0, 0
						// DBG_FUNC_END, kIOReturnSuccess, -2, 0, 0
						// DBG_FUNC_END, ret, 0, 0, 0
					case DBG_IOG_FB_POWER_CHANGE             : GTRACEARG_regID; GTRACEARG_powerState; break; // __private->regID, powerStateOrdinal, 0, 0
					case DBG_IOG_WAKE_FROM_DOZE              : GTRACEARG_int("x"); GTRACEARG_int("y"); break;
					case DBG_IOG_RECEIVE_POWER_NOTIFICATION  : GTRACEARG_powerevent; GTRACEARG_pmValue; break;
						// DBG_IOG_PWR_EVENT_SYSTEMPWRCHANGE (3), kIOPMDisableClamshell, 0, 0
						// DBG_IOG_PWR_EVENT_DESKTOPMODE (1), gIOFBClamshellState, 0, 0
						// DBG_IOG_PWR_EVENT_PROCCONNECTCHANGE (4), kIOPMDisableClamshell, 0, 0
						// DBG_IOG_PWR_EVENT_DISPLAYASSERTIONCREATED (5), 0x40, 0, 0
					case DBG_IOG_CHANGE_POWER_STATE_PRIV     : GTRACEARG_src; GTRACEARG_powerState; GTRACEARG_iftype(kGTRACE_ARGUMENT_STRING, GTRACEARG_string("procName", 16)); break;
						// DBG_IOG_CHANGE_POWER_STATE_PRIV (28), DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_IODISPLAYWRANGLER (5), 0, 0, 0, 0, 0, 0);
						// DBG_IOG_CHANGE_POWER_STATE_PRIV (28), DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_IODISPLAYWRANGLER (5), 0, 2, 0, 0, 0, 0);
						// DBG_IOG_CHANGE_POWER_STATE_PRIV (28), DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_IODISPLAYWRANGLER (5), 0, 1, kGTRACE_ARGUMENT_STRING, procNameInt[0], kGTRACE_ARGUMENT_STRING, procNameInt[1]);
						// DBG_IOG_CHANGE_POWER_STATE_PRIV (28), DBG_FUNC_NONE, 0, DBG_IOG_SOURCE_IODISPLAY         (6), 0, state, 0, 0, 0, 0);
					case DBG_IOG_CLAMP_POWER_ON              : GTRACEARG_src; break; // DBG_IOG_SOURCE_STOP, 0, 0, 0
					case DBG_IOG_SET_TIMER_PERIOD            : GTRACEARG_src; GTRACEARG_units("idleTime", "s"); break; // DBG_IOG_SOURCE_SET_AGGRESSIVENESS (9), newLevel * 30, 0, 0
					case DBG_IOG_HANDLE_EVENT                : GTRACEARG_regID; GTRACEARG_event; break; // __private->regID, event, 0, 0
					case DBG_IOG_SET_ATTRIBUTE_EXT           : GTRACEARG_regID; GTRACEARG_entry(GTRACEARG_attribute_value); GTRACEARG_exit(GTRACEARG_error); break;
						// DBG_FUNC_START, __private->regID, attribute, value, 0);
						// DBG_FUNC_END  , __private->regID, err, 0, 0);
					case DBG_IOG_CLAMSHELL                   :
						GTRACEARG_src;
						GTRACEARG_iftype(DBG_IOG_SOURCE_SYSWORK_READCLAMSHELL    , if (entry.arg64(arg) == 1) {GTRACEARG_bool("clamshellProperty"); GTRACEARG_bool("CurrentClamshellState"); GTRACEARG_bool("desktopMode"); } else if ((int64_t)entry.arg64(arg) == -1) { GTRACEARG_minus1("clamshellProperty"); } );
						GTRACEARG_iftype(DBG_IOG_SOURCE_SYSWORK_RESETCLAMSHELL_V2, GTRACEARG_bits; GTRACEARG_counts);
						GTRACEARG_iftype(DBG_IOG_SOURCE_SYSWORK_ENABLECLAMSHELL  , GTRACEARG_bool("desktopMode"); GTRACEARG_bool("change"));
						GTRACEARG_iftype(DBG_IOG_SOURCE_SYSWORK_PROBECLAMSHELL   , GTRACEARG_bool("LastClamshellState"); GTRACEARG_bool("LastReadClamshellState"); GTRACEARG_bool("CurrentClamshellState"));
						GTRACEARG_iftype(DBG_IOG_SOURCE_POSTWAKE                 , GTRACEARG_bool("LastClamshellState"); GTRACEARG_bool("LastReadClamshellState"); GTRACEARG_bool("CurrentClamshellState"));
						GTRACEARG_iftype(DBG_IOG_SOURCE_RESET_CLAMSHELL          , GTRACEARG_srcnamed("where"); GTRACEARG_units("delay", "ms"));
						GTRACEARG_iftype(DBG_IOG_SOURCE_READ_CLAMSHELL           , GTRACEARG_srcnamed("where"); GTRACEARG_bool("LastClamshellState"); GTRACEARG_bool("oldState"));
						GTRACEARG_iftype(DBG_IOG_SOURCE_CLAMSHELL_HANDLER        , GTRACEARG_clamshellState);
						GTRACEARG_iftype(DBG_IOG_SOURCE_CLAMSHELL_OFFLINE_CHANGE , GTRACEARG_regID; GTRACEARG_int("rejectLine"); GTRACEARG_state);
						GTRACEARG_iftype(DBG_IOG_SOURCE_GET_ATTRIBUTE            , GTRACEARG_regID; GTRACEARG_error; GTRACEARG_bool("resultP"));
						break;
					case DBG_IOG_HANDLE_VBL_INTERRUPT        : GTRACEARG_int("VBL_delta_real"); GTRACEARG_int("VBL_delta_calculated"); GTRACEARG_int("VBL_time"); GTRACEARG_int("VBL_count"); break;
					case DBG_IOG_WAIT_QUIET                  : GTRACEARG_regIDnamed("GPU_regID"); GTRACEARG_error; GTRACEARG_regID; GTRACEARG_units("timeout", "s"); break; // pciRegID, status, getRegistryEntryID(), timeout
					case DBG_IOG_PLATFORM_CONSOLE            : GTRACEARG_srcnamed("where"); GTRACEARG_regID; GTRACEARG_bool("hasVInfo"); GTRACEARG_op; ; break; // where, __private->regID, static_cast<bool>(consoleInfo), op
					case DBG_IOG_CONSOLE_CONFIG              : GTRACEARG_regID; GTRACEARG_widthHeight; GTRACEARG_hex("rowBytes"); GTRACEARG_depthScale; break; // __private->regID, consoleInfo->v_width << 32 | consoleInfo->v_height, consoleInfo->v_rowBytes, consoleInfo->v_depth << 32 | consoleInfo->v_scale
					case DBG_IOG_VRAM_CONFIG                 : GTRACEARG_regID; GTRACEARG_int("height"); GTRACEARG_hex("rowBytes"); GTRACEARG_hex("len"); break; // __private->regID, __private->pixelInfo.activeHeight, __private->pixelInfo.bytesPerRow, fVramMap->getLength()
					case DBG_IOG_SET_GAMMA_TABLE             : GTRACEARG_regID; GTRACEARG_src; GTRACEARG_exit(GTRACEARG_error); break;
						// DBG_IOG_SET_GAMMA_TABLE, DBG_FUNC_START, __private->regID, SOURCE, 0
						// DBG_IOG_SET_GAMMA_TABLE, DBG_FUNC_END  , __private->regID, SOURCE, err
					case DBG_IOG_NEW_USER_CLIENT             : GTRACEARG_regID; GTRACEARG_type; GTRACEARG_error; GTRACEARG_location; break; // __private->regID, type, err, /* location */ 0->normal, 1->diagnostic, 2->waitQuiet
					case DBG_IOG_FB_CLOSE                    : GTRACEARG_regID; break;
						// DBG_FUNC_START, __private->regID, 0, 0, 0
						// DBG_FUNC_END  , __private->regID, 0, 0, 0
					case DBG_IOG_NOTIFY_CALLOUT_TIMEOUT      : GTRACEARG_entry(GTRACEARG_regID; GTRACEARG_string("className", 24)); GTRACEARG_exit(GTRACEARG_event; GTRACEARG_return_code); break;
						// DBG_FUNC_START, __private->regID, kGTRACE_ARGUMENT_STRING, nameBufInt[0], kGTRACE_ARGUMENT_STRING, nameBufInt[1], kGTRACE_ARGUMENT_STRING, nameBufInt[2]
						// DBG_FUNC_END  , event, r, 0, 0
					case DBG_IOG_CONNECTION_ENABLE           : GTRACEARG_regID; GTRACEARG_bool("enable"); GTRACEARG_return_code; break; // __private->regID, *value, err, 0
					case DBG_IOG_CONNECTION_ENABLE_CHECK     : GTRACEARG_src; GTRACEARG_regID; GTRACEARG_bool("enable"); GTRACEARG_return_code; break; // DBG_IOG_SOURCE, 0, __private->regID, 0, connectEnabled, 0, err
					case DBG_IOG_PROCESS_CONNECT_CHANGE      : GTRACEARG_regID; GTRACEARG_entry(GTRACEARG_mode); GTRACEARG_exit(GTRACEARG_int("exit_point"); GTRACEARG_bool("online")); break;
						// DBG_FUNC_START (1), __private->regID, mode, 0, 0
						// DBG_FUNC_END   (2), __private->regID, 0, __private->online, 0
						// DBG_FUNC_END   (2), __private->regID, 1, __private->online, 0
						// DBG_FUNC_END   (2), __private->regID, 2, __private->online, 0
					case DBG_IOG_CONNECT_CHANGE_INTERRUPT    : GTRACEARG_regID; GTRACEARG_last_processed; GTRACEARG_last_finished_and_messaged; GTRACEARG_changed_and_last_forced; break; // these are all related: fConnectChange lastProcessedChange fLastFinishedChange fLastMessagedChange fLastForceRetrain connectChange fPostWakeChange lastMessagedChange
					case DBG_IOG_DELIVER_NOTIFY              : GTRACEARG_regID; GTRACEARG_event; GTRACEARG_return_code; break; // __private->regID, event, ret, 0
					case DBG_IOG_AGC_MSG                     : GTRACEARG_switchState; break; // switchState, 0, 0, 0
					case DBG_IOG_AGC_MUTE                    : GTRACEARG_regID; value = entry.arg64(arg); cprintf(" {"); DumpOneAttribute(kConnectionIgnore, true, true, &value, sizeof(value)); cprintf(" }"); GTRACEARG_bool("isMuted"); break; // GPACKUINT64T(__private->regID), GPACKUINT32T(0, value), GPACKBIT(0, __private->controller->isMuted()), 0
					case DBG_IOG_SET_PROPERTIES              : GTRACEARG_regID; GTRACEARG_string("?", 24); break; // DBG_IOG_SET_PROPERTIES, r14, 0x2000, var_50, 0x2000, var_48, 0x2000, var_40
					case DBG_IOG_SET_SYSTEM_DIM_STATE_52     : break; // sign_extend_64(r12), r14 >> 0x1f | (r13 != 0x0 ? 0x1 : 0x0) << 0x8 | r13 << 0x30 | rbx | rcx << 0x20, 0x0, 0x0
					case DBG_IOG_DIMENGINE_53                : break;
						// DBG_FUNC_START, r15 & 0xff, 0xa, 0x0, 0x0
						// DBG_FUNC_END  , r15 & 0xff, 0xa, 0x0, 0x0
					case DBG_IOG_TIMELOCK                    : GTRACEARG_units("gtrace_delta", "ns"); GTRACEARG_string("name", 8); GTRACEARG_string("fn", 16); break; // kGTRACE_ARGUMENT_STRING, nameBufInt[0], kGTRACE_ARGUMENT_STRING, fnBufInt[0], kGTRACE_ARGUMENT_STRING, fnBufInt[1]
					case DBG_IOG_CURSORLOCK                  : GTRACEARG_units("gtrace_delta", "ns"); GTRACEARG_string("name", 8); GTRACEARG_string("fn", 16); break; // kGTRACE_ARGUMENT_STRING, nameBufInt[0], kGTRACE_ARGUMENT_STRING, fnBufInt[0], kGTRACE_ARGUMENT_STRING, fnBufInt[1]
					case DBG_IOG_LOG_SYNCH                   : GTRACEARG_int("synchIndex"); break;
					case DBG_IOG_CP_INTERUPT_58              : break;
						// DBG_FUNC_START, r13, rbx != 0x0 ? 0x1 : 0x0, 0x0, 0x0
						// DBG_FUNC_END,   r12, 0x0, 0x0, 0x0
					case DBG_IOG_EXT_GET_ATTRIBUTE_59        : GTRACEARG_regID; GTRACEARG_attribute_value; break; // rbx, var_30, var_38, r15
					case DBG_IOG_FB_EXT_CLOSE                : GTRACEARG_regID; break;
						// DBG_FUNC_START, __private->regID, 0, 0, 0
						// DBG_FUNC_END  , __private->regID, 0, 0, 0
					case DBG_IOG_MSG_CONNECT_CHANGE          : GTRACEARG_regID; GTRACEARG_connectc1; GTRACEARG_connectc2; break; // fFbs[0]->__private->regID, c1, c2, 0
					case DBG_IOG_CAPTURED_RETRAIN            : GTRACEARG_regID; GTRACEARG_retrainc1; GTRACEARG_retrainc2; break; // fFbs[0]->__private->regID, c1, c2, 0
					case DBG_IOG_CONNECT_WORK_ASYNC          : GTRACEARG_regID; GTRACEARG_workasync1; GTRACEARG_workasync2; break; // fFbs[0]->__private->regID, c1, c2, 0
					case DBG_IOG_MUX_ACTIVITY_CHANGE         : GTRACEARG_regID; break; // controller->fFbs[0]->__private->regID, 0, 0, 0
					case DBG_IOG_SET_ATTR_FOR_CONN_EXT       : GTRACEARG_regID; GTRACEARG_attribute_value; GTRACEARG_exit(GTRACEARG_error); break; // __private->regID, attribute, value, err
						// DBG_FUNC_START, __private->regID, attribute, value, 0
						// endTag, __private->regID, attribute, value, err
					case DBG_IOG_EXT_END_CONNECT_CHANGE      : GTRACEARG_regID; GTRACEARG_endconnectc1; GTRACEARG_endconnectc2; break; // controller->fFbs[0]->__private->regID, c1, c2, 0
					case DBG_IOG_EXT_PROCESS_CONNECT_CHANGE  : GTRACEARG_regID; GTRACEARG_entry(GTRACEARG_processconnecta1; GTRACEARG_processconnecta2; GTRACEARG_processconnecta3); GTRACEARG_exit(GTRACEARG_mode; GTRACEARG_error); break;
						// DBG_FUNC_START, __private->regID, a1, a2, a3
						// DBG_FUNC_END  , __private->regID, mode, err, 0
					case DBG_IOG_ASYNC_WORK                  : GTRACEARG_regID; GTRACEARG_entry(GTRACEARG_work("work")); GTRACEARG_exit(GTRACEARG_work("fAsyncWork")); GTRACEARG_work("fDidWork"); break;
						// DBG_FUNC_START, fFbs[0]->__private->regID, work,       fDidWork, 0);
						// DBG_FUNC_END  , fFbs[0]->__private->regID, fAsyncWork, fDidWork, 0);
						
					case DBG_IOG_SYSTEM_WORK                 : GTRACEARG_allState; GTRACEARG_gIOFBGlobalEvents; break; // allState, gIOFBGlobalEvents, 0, 0
						// DBG_FUNC_START, allState, gIOFBGlobalEvents, 0, 0);
						// DBG_FUNC_END  , allState, gIOFBGlobalEvents, 0, 0);
					case DBG_IOG_BUILTIN_PANEL_POWER         : GTRACEARG_regID; GTRACEARG_builtinpanelpowera1; break; // __private->regID, a1, 0, 0
					case DBG_IOG_CONNECT_CHANGE_INTERRUPT_V2 : GTRACEARG_regID; GTRACEARG_connectchangeinterrupta1; GTRACEARG_connectchangeinterrupta2; GTRACEARG_connectchangeinterrupta3; break; // __private->regID, a1, a2, a3 // these are all related: fConnectChange lastProcessedChange fLastFinishedChange fLastMessagedChange fLastForceRetrain connectChange fPostWakeChange lastMessagedChange
					case DBG_IOG_SET_DISPLAY_MODE            : GTRACEARG_src; GTRACEARG_regID; GTRACEARG_entry(GTRACEARG_modeID); GTRACEARG_exit(GTRACEARG_error); GTRACEARG_entry(GTRACEARG_depth); break;
						// DBG_FUNC_START, DBG_IOG_SOURCE_* , __private->regID, mode, depth
						// DBG_FUNC_END  , DBG_IOG_SOURCE_* , __private->regID, err , 0
					case DBG_IOG_SET_DETAILED_TIMING         : GTRACEARG_src; GTRACEARG_regID; GTRACEARG_exit(GTRACEARG_error); break;
						// DBG_FUNC_START, source, __private->regID,   0, 0
						// DBG_FUNC_END  , source, __private->regID, err, 0
					case DBG_IOG_GET_CURRENT_DISPLAY_MODE    : GTRACEARG_src; GTRACEARG_regID; GTRACEARG_modeAndDepth; GTRACEARG_error; break; // DBG_IOG_SOURCE_DO_SET_DISPLAY_MODE, __private->regID, GPACKUINT32T(1, depth) | GPACKUINT32T(0, displayMode), err

					case ABL_SET_BRIGHTNESS                  : GTRACEARG_regID; GTRACEARG_ablsetbrightnessa1; GTRACEARG_string("procName", 16); break;
					case ABL_SET_DISPLAY_POWER               : GTRACEARG_regID; GTRACEARG_ablsetdisplaypowera1; GTRACEARG_string("procName", 16); break;
					case ABL_SET_BRIGHTNESS_PROBE            : GTRACEARG_regID; GTRACEARG_ablsetbrightnessa1; GTRACEARG_string("procName", 16); break;
					case ABL_DO_UPDATE                       : GTRACEARG_regID; GTRACEARG_abldoupdatea1; break;
					case ABL_SET_DISPLAY                     : GTRACEARG_regID; GTRACEARG_ablsetdisplaya1; break;
					case ABL_COMMIT                          : GTRACEARG_regID; GTRACEARG_ablcommita1; GTRACEARG_string("procName", 16); break;

					default                                  : break;
				}

				
				for (i = arg; i < 4; i++) {
					for (j = i; j < 4; j++) {
						if ((j > 0 && entry.fArgsTag.tag(j)) || entry.arg64(j)) {
							cprintf(" ");
							if (i > 0 && entry.fArgsTag.tag(i)) {
								cprintf("%s:", GetTagTypeStr(tagtypebuf, sizeof(tagtypebuf), entry.fArgsTag.tag(i)));
							}
							for (j = i; j > 0 && j < 4 && entry.fArgsTag.tag(j) == kGTRACE_ARGUMENT_STRING; j++);
							if (j > i) {
								cprintf("%s", GetEntryString(buf, sizeof(buf), entry, i, (j-i)*8));
								i = j - 1;
							}
							else
								cprintf("%#llx", entry.arg64(i));
							break;
						}
					}
				}
				cprintf("\n");
				
				
			} // if not a header line
			currentLine++;
		} // for entry

		OUTDENT iprintf("}\n");
		currentBuffer++;
	} // for buffer

	OUTDENT iprintf("} // Token Buffers");
}

void writeGTraceBinary(const vector<GTraceBuffer>& gtraces,
                       const char* filename)
{
    if (gtraces.empty()) {
        fprintf(stdout, "iogdiagnose: No GTrace data\n");
        return;
    }

    FILE* fp = fopen(filename, "w");
    if (!static_cast<bool>(fp)) {
        fprintf(stdout,
                "iogdiagnose: Failed to open %s for write access\n", filename);
        return;
    }

    // Marks beginning of a multi
    const GTraceEntry magic(gtraces.size(), GTRACE_REVISION);

    fwrite(&magic, sizeof(magic), 1, fp);  // Indicates v2 gtrace binary
    for (const GTraceBuffer& buf : gtraces)
        // TODO(gvdl): how about some error checking?
        fwrite(buf.data(), sizeof(*buf.data()), buf.size(), fp);
    fclose(fp);
}

// Simple helpers for report dumper
inline int bitIsSet(const uint32_t value, const uint32_t bit)
    { return static_cast<bool>(value & bit); }
inline int isStateSet(const IOGReport& fbState, uint32_t bit)
    { return bitIsSet(fbState.stateBits, bit); }
inline mach_timebase_info_data_t getMachTimebaseInfo()
{
    mach_timebase_info_data_t ret = { 0 };
    mach_timebase_info(&ret);
    return ret;
}
inline tm getTime()
{
    tm ret;
    time_t rawtime = 0; time(&rawtime);
    localtime_r(&rawtime, &ret);
    return ret;
}

void dumpGTraceReport(const IOGDiagnose& diag,
                      const vector<GTraceBuffer>& gtraces,
                      const bool bDumpToFile)
{
    if (diag.version < 7) {
        fprintf(stderr, "iogdiagnose: Can't read pre v7 IOGDiagnose reports\n");
        return;
    }

    const mach_timebase_info_data_t info = getMachTimebaseInfo();
    const tm timeinfo = getTime();
    char filename[kFILENAME_LENGTH] = {0};
    strftime(filename, sizeof(filename),
             "/tmp/com.apple.iokit.IOGraphics_%F_%H-%M-%S.txt", &timeinfo);

    FILE* const fp = (bDumpToFile) ? fopen(filename, "w+") : nullptr;
    FILE* const outfile = (static_cast<bool>(fp)) ? fp : stdout;
    foutsep(outfile, "IOGRAPHICS REPORT");
    strftime(filename, sizeof(filename), "Report date: %F %H:%M:%S", &timeinfo);
    fprintf(outfile, "%s\n", filename);

    const auto fbCount = std::min<uint64_t>(
        diag.framebufferCount, IOGRAPHICS_MAXIMUM_FBS);

    fprintf(outfile, "Report version: %#llx\n", diag.version);

	char thenstr[100];
	int64_t thenns = diag.systemBootEpochTime;
	time_t thentime = (time_t)(thenns / 1000000000);
	struct tm thentm;
	localtime_r(&thentime, &thentm);
	size_t len = strftime(thenstr, sizeof(thenstr), "%F %T", &thentm);
	snprintf(thenstr + len, sizeof(thenstr) - len, ".%09lld", thenns % 1000000000);

    fprintf(outfile, "Boot Epoch Time: %llu=%s\n", diag.systemBootEpochTime, thenstr);
    fprintf(outfile, "Number of framebuffers: %llu%s\n\n",
        diag.framebufferCount,
        ((fbCount != diag.framebufferCount)
            ? " (warning: some fbs not reported)" : ""));

    for (uint32_t i = 0; i < fbCount; i++) {
        const IOGReport& fbState = diag.fbState[i];

        fprintf(outfile, "\t%s::",
                (*fbState.objectName) ? fbState.objectName : "UNKNOWN::");
        fputs((*fbState.framebufferName) ? fbState.framebufferName : "IOFB?",
                outfile);
        fprintf(outfile,
                " (%u %#llx)\n", fbState.dependentIndex, fbState.regID);
        if (fbState.aliasID)
            fprintf(outfile, "\t\tAlias     : 0x%08x\n", fbState.aliasID);

        fprintf(outfile, "\t\tI-State   : 0x%08x (b", fbState.stateBits);
        for (uint32_t mask = 0x80000000; 0 != mask; mask >>= 1)
            fprintf(outfile, "%d", isStateSet(fbState, mask));
        fprintf(outfile, ")\n");

        fprintf(outfile, "\t\tStates    : OP:ON:US:PS:CS:CC:CL:CO:DS:MR:SG:WL:NA:SN:SS:SA:PP:SP:MS:MP:MU:PPS: NOTIFIED :   WSAA   \n");
        fprintf(outfile, "\t\tState Bits: %02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:0x%01x:0x%08x:0x%08x\n",
                isStateSet(fbState, kIOGReportState_Opened),
                isStateSet(fbState, kIOGReportState_Online),
                isStateSet(fbState, kIOGReportState_Usable),
                isStateSet(fbState, kIOGReportState_Paging),
                isStateSet(fbState, kIOGReportState_Clamshell),
                isStateSet(fbState, kIOGReportState_ClamshellCurrent),
                isStateSet(fbState, kIOGReportState_ClamshellLast),
                isStateSet(fbState, kIOGReportState_ClamshellOffline),
                isStateSet(fbState, kIOGReportState_SystemDark),
                isStateSet(fbState, kIOGReportState_Mirrored),
                isStateSet(fbState, kIOGReportState_SystemGated),
                isStateSet(fbState, kIOGReportState_WorkloopGated),
                isStateSet(fbState, kIOGReportState_NotificationActive),
                isStateSet(fbState, kIOGReportState_ServerNotified),
                isStateSet(fbState, kIOGReportState_ServerState),
                isStateSet(fbState, kIOGReportState_ServerPendingAck),
                isStateSet(fbState, kIOGReportState_PowerPendingChange),
                isStateSet(fbState, kIOGReportState_SystemPowerAckTo),
                isStateSet(fbState, kIOGReportState_IsMuxSwitching),
                isStateSet(fbState, kIOGReportState_PowerPendingMuxChange),
                isStateSet(fbState, kIOGReportState_Muted),
                fbState.pendingPowerState,
                fbState.notificationGroup,
                fbState.wsaaState);

        if ((diag.version >= 9) && (kIOReturnSuccess != fbState.lastWSAAStatus)) {
            fprintf(outfile,
                "\t\t**** WARNING: setAttribute(WSAA) failed: %#x\n",
                fbState.lastWSAAStatus);
        }

        fprintf(outfile, "\t\tA-State   : 0x%08x (b", fbState.externalAPIState);
        for (uint32_t mask = 0x80000000; 0 != mask; mask >>= 1)
            fprintf(outfile, "%d", bitIsSet(fbState.externalAPIState, mask));
        fprintf(outfile, ")\n");

        fprintf(outfile, "\t\tMode ID   : %#x\n", fbState.lastSuccessfulMode);
        fprintf(outfile, "\t\tSystem    : %llu (%#llx) (%u)\n",
                fbState.systemOwner, fbState.systemOwner,
                fbState.systemGatedCount);
        fprintf(outfile, "\t\tController: %llu (%#llx) (%u)\n",
                fbState.workloopOwner, fbState.workloopOwner,
                fbState.workloopGatedCount);


        for (int gi = 0; gi < IOGRAPHICS_MAXIMUM_REPORTS; ++gi) {
            const auto& group = fbState.notifications[gi];
            if (!group.groupID)
                continue;

            fprintf(outfile, "\n\t\tGroup ID: %#llx\n", group.groupID - 1);
            for (int si = 0; si < IOGRAPHICS_MAXIMUM_REPORTS; ++si) {
                const auto& stamp = group.stamp[si];
                if (stamp.lastEvent || stamp.start || stamp.end || *stamp.name)
                {
                    fprintf(outfile, "\t\t\tComponent   : %s\n", stamp.name);
                    fprintf(outfile, "\t\t\tLast Event  : %u (%#x)\n",
                            stamp.lastEvent, stamp.lastEvent);
                    fprintf(outfile, "\t\t\tStamp Start : %#llx\n", stamp.start);
                    fprintf(outfile, "\t\t\tStamp End   : %#llx\n", stamp.end);
                    fprintf(outfile, "\t\t\tStamp Delta : ");
                    if (stamp.start <= stamp.end) {
                        fprintf(outfile, "%llu ns\n",
                                ((stamp.end - stamp.start)
                                     * static_cast<uint64_t>(info.numer))
                                    / static_cast<uint64_t>(info.denom));
                    } else
                        fprintf(outfile, "Notifier Active\n");
                }
            }
        }
    }

    // Tokenized logging data
    dumpTokenBuffer(outfile, gtraces);
    fflush(outfile);

    foutsep(outfile);

	iprintf("AGDCDiagnose = {\n"); INDENT

	iprintf("AGDCDiagnose process = {\n"); INDENT
	agdcdiagnose(outfile);
	OUTDENT iprintf("} // AGDCDiagnose process\n");

	
	iprintf("AGDCDiagnose displaypolicyd parsed = {\n"); INDENT
	DoDisplayPolicyState();
	OUTDENT iprintf("} // AGDCDiagnose displaypolicyd parsed\n");

	OUTDENT iprintf("} // AGDCDiagnose\n");

	fflush(outfile);

    if (static_cast<bool>(fp))
        fclose(fp);
}

IOReturn fetchGTraceBuffers(const IOConnect& gtrace,
                            vector<GTraceBuffer>* traceBuffersP)
{
    uint64_t scalarParams[] = { GTRACE_REVISION, 0 };
    const uint32_t scalarParamsCount = COUNT_OF(scalarParams);
    size_t   bufSize;

    // TODO(gvdl): Need to multithread the fetches the problem is that the
    // fetch code makes a subroutine call into the driver.
    // Single thread for the time being.

    // Default vector fo GTraceEntries allocates a MiB of entries
    const size_t kBufSizeBytes = 1024 * 1024;
    const vector<GTraceEntry>::size_type
        kBufSize = kBufSizeBytes / sizeof(GTraceEntry);

    IOReturn err = kIOReturnSuccess;
    for (int i = 0; i < kGTraceMaximumBufferCount; i++) {
        GTraceBuffer::vector_type buf(kBufSize);  // allocate a buffer
        scalarParams[1] = i;  // GTrace buffer index
        bufSize = kBufSizeBytes;
        const IOReturn bufErr = gtrace.callMethod(
                0, // kIOGDGTCInterface_fetch
                scalarParams, scalarParamsCount, NULL, 0,  // Inputs
                NULL, NULL, buf.data(), &bufSize);         // Outputs
        if (bufErr) {
            if (bufErr != kIOReturnNotFound) {
                fprintf(stderr, "Error retrieving gtrace buffer %d - %s[%x]\n",
                        i, mach_error_string(bufErr), bufErr);
                if (!err)
                    err = bufErr;  // Record first error
            }
            continue;  // Problem with buffer
        }
        if (bufSize < kGTraceHeaderSize) {
            fprintf(stderr, "malformed gtrace buffer %d no header %ld\n",
                    i, bufSize);
            if (!err)
                err = kIOReturnUnformattedMedia;  // Record first error
            continue;
        }
        const auto* header = reinterpret_cast<GTraceHeader*>(buf.data());
        if (bufSize < header->fBufferSize) {
            fprintf(stderr,
                    "malformed gtrace buffer %d bad copyout %ld exp %d\n",
                    i, bufSize, header->fBufferSize);
            if (!err)
                err = kIOReturnInternalError;  // Record first error
            continue;
        }
        auto numEntries = header->fBufferSize / sizeof(GTraceEntry);
        const auto expectTokens = kGTraceHeaderEntries + header->fTokensCopied;
        if (numEntries < expectTokens) {
            fprintf(stderr, "malformed gtrace buffer %d too small %ld\n",
                    i, numEntries);
            if (!err)
                err = kIOReturnUnformattedMedia;  // Record first error
            continue;
        }
        const auto alwaysCopy
            = kGTraceHeaderEntries + header->fBreadcrumbTokens;
        auto citer = buf.cbegin();
        auto cstopat = buf.cbegin() + numEntries;
        for (citer += alwaysCopy; citer < cstopat; ++citer)
            if (!citer->timestamp())
                break;
        numEntries = citer - buf.cbegin();

        // Copy the non-zero timestamp entries and update header
        traceBuffersP->emplace_back(buf.cbegin(), citer);
        auto& bufHeader = traceBuffersP->back().header();
        bufHeader.fBufferSize
            = static_cast<uint32_t>(numEntries * sizeof(*citer));
        bufHeader.fTokensCopied = numEntries - kGTraceHeaderEntries;
    }
    return err;
}

// If errmsgP is set then the caller is required to free returned string
kern_return_t iogDiagnose(
        const IOConnect& diag, IOGDiagnose* reportP, size_t reportLength,
        const char** errmsgP)
{
    const char*   errmsg;
    kern_return_t err = kIOReturnInternalError;

    do {
        errmsg = "NULL IOGDiagnose pointer argument passed";
        err = kIOReturnBadArgument;
        if (!reportP)
            continue;
        errmsg = "report too small for an IOGDiagnose header";
        err = kIOReturnBadArgument;
        if (sizeof(*reportP) > reportLength)
            continue;

        // Grab the main IOFB reports first
        errmsg = "Problem getting framebuffer diagnostic data";
        uint64_t       scalarParams[]    = {reportLength};
        const uint32_t scalarParamsCount = COUNT_OF(scalarParams);
        memset(reportP, 0, reportLength);
        err = diag.callMethod(
                0, // kIOGDUCInterface_diagnose,
                scalarParams, scalarParamsCount, NULL, 0,  // input
                NULL, NULL, reportP, &reportLength);       // output
    } while(false);

    if (err && errmsgP) {
        char *tmpMsg;
        asprintf(&tmpMsg, "%s - %s(%x)", errmsg, mach_error_string(err), err);
        *errmsgP = tmpMsg;
    }
    return err;
}
}; // namespace

#ifdef __cplusplus
extern "C" {
#endif
	int iogdiagnose(int dumpToFile, char *optarg);
#ifdef __cplusplus
}
#endif


int iogdiagnose(int dumpToFile, char *optarg)
{
	bool                 bDumpToFile = false;
    bool                 bBinaryToFile = false;
	char                 inputFilename[256] = { 0 };

#if 0
	testtime();
#endif
	
	bDumpToFile = dumpToFile;
	if (optarg != nullptr) {
		const size_t len = strlen(optarg);
		if (len <= sizeof(inputFilename)) {
			strncpy(inputFilename, optarg, len);
			bBinaryToFile = true;
		}
	}

    const char *error = nullptr;
    kern_return_t err = kIOReturnSuccess;

    vector<GTraceBuffer> gtraces;
    {
        IOConnect gtrace; // GTrace connection
        err = openGTrace(&gtrace, &error);
        if (!err) {
            error = "A problem occured fetching gTraces, see kernel logs";
            err = fetchGTraceBuffers(gtrace, &gtraces);
        }
		if (err) {
			fprintf(stderr, "%s %s (%#x)\n", error, mach_error_string(err), err);
			return(EXIT_FAILURE);
		}
    }
    if (bBinaryToFile) {
        writeGTraceBinary(gtraces, inputFilename);
        return(EXIT_SUCCESS);
    }

    IOGDiagnose report = { 0 };
    {
        IOConnect diag; // Diagnostic connection
        err = openDiagnostics(&diag, &error);
        if (!err)
            err = iogDiagnose(diag, &report, sizeof(report), &error);
		if (err) {
			fprintf(stderr, "%s %s (%#x)\n", error, mach_error_string(err), err);
			return(EXIT_FAILURE);
		}
    }

    dumpGTraceReport(report, gtraces, bDumpToFile);
    return EXIT_SUCCESS;
}

//#endif //TARGET_CPU_X86_64
