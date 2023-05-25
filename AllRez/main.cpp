//
//  main.m
//  AllRez
//
//  Created by joevt on 2022-01-03.
//
//=================================================================================================================================
// Apple includes

#include "MacOSMacros.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysctl.h>

//#include <Foundation/Foundation.h>

#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFNumber.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFByteOrder.h>

#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_8
#include <CoreGraphics/CGDirectDisplay.h>
#include <CoreGraphics/CGDisplayConfiguration.h> // must include before IOGraphicsTypes
#endif

//#include <CoreGraphics/CoreGraphicsPrivate.h>

#include <IOKit/graphics/IOGraphicsLib.h>
#include <IOKit/graphics/IOGraphicsTypes.h>
#include <IOKit/graphics/IOGraphicsInterfaceTypes.h>
//#include <IOKit/graphics/IOGraphicsTypesPrivate.h>
//#include <IOKit/ndrvsupport/IONDRVLibraries.h>

#include <IOKit/ndrvsupport/IOMacOSVideo.h>

//#include <ApplicationServices/ApplicationServices.h>
//#include <Kernel/IOKit/ndrvsupport/IONDRVFramebuffer.h>
//#include <IOKitUser/graphics/IOGraphicsLibInternal.h>

#include "AppleMisc.h"


#ifdef __cplusplus
extern "C" {
#endif

#include <IOKit/IOTypes.h>
#include <IOKit/i2c/IOI2CInterface.h>

extern int iogdiagnose6(int dumpToFile, const char *optarg);
extern int iogdiagnose9(int dumpToFile, const char *optarg);
extern int iogdiagnose_10_15_7(int dumpToFile, const char *optarg);
extern int iogdiagnose (int dumpToFile, const char *optarg);

#ifdef __cplusplus
}
#endif

//=================================================================================================================================
// Debugging and testing options

int notTestDisplayIndex = 0;
int testDisplayIndex = 1;

const int doSetupIOFB          = 0; // 0
const int doAttributeTest      = 0; // 0
const int doEdidOverrideTest   = 0;
const int doParseTest          = 0;
const int doDisplayPortTest    = 0;
const int doDumpAll            = 1; // 1
const int doIogDiagnose        = 1; // 1
      int doSetDisplayModeTest = 0; // 0


//=================================================================================================================================
// Linux includes

#include <drm/display/drm_dp_helper.h>
#include <drm/display/drm_hdcp.h>
#include "dpcd_defs.h"

//=================================================================================================================================
// Includes

#include "vcp.h"
#include "dpcd.h"
#include "printf.h"
#include "utilities.h"
#include "iofbdebuguser.h"
#include "displayport.h"

//=================================================================================================================================
// Defines

#define TRYSUBADDRESS 0 // I don't know when sub address is useful or not - better not try it


//================================================================================================================================


//=================================================================================================================================
// Utilities

void KeyArrayCallback(const void *key, const void *value, void *context)
{
	CFArrayAppendValue((CFMutableArrayRef)context, key);
}


//=================================================================================================================================

static void DumpOneID(CFNumberRef ID, int modeAlias) {
	uint32_t val;
	if (CFNumberGetValue(ID, kCFNumberSInt32Type, &val)) {
		char hexDigits[] = "0123456789abcdef";
		hexDigits[modeAlias] = '.';
		cprintf("0x%04x%c%03x",
			val >> 16, // mode
			hexDigits[(val >> 12) & 15],
			val & 0x0fff
		);
	}
	else {
		cprintf("?");
	}
}

const char *GetOneAppleTimingID(IOAppleTimingID appleTimingID) {
	return
	appleTimingID == kIOTimingIDInvalid               ? "Invalid (not a standard timing)" :
	appleTimingID == kIOTimingIDApple_FixedRateLCD    ?    "Apple_FixedRateLCD" :
	appleTimingID == kIOTimingIDApple_512x384_60hz    ?    "Apple_512x384_60hz" :
	appleTimingID == kIOTimingIDApple_560x384_60hz    ?    "Apple_560x384_60hz" :
	appleTimingID == kIOTimingIDApple_640x480_67hz    ?    "Apple_640x480_67hz" :
	appleTimingID == kIOTimingIDApple_640x400_67hz    ?    "Apple_640x400_67hz" :
	appleTimingID == kIOTimingIDVESA_640x480_60hz     ?     "VESA_640x480_60hz" :
	appleTimingID == kIOTimingIDVESA_640x480_72hz     ?     "VESA_640x480_72hz" :
	appleTimingID == kIOTimingIDVESA_640x480_75hz     ?     "VESA_640x480_75hz" :
	appleTimingID == kIOTimingIDVESA_640x480_85hz     ?     "VESA_640x480_85hz" :
	appleTimingID == kIOTimingIDGTF_640x480_120hz     ?     "GTF_640x480_120hz" :
	appleTimingID == kIOTimingIDApple_640x870_75hz    ?    "Apple_640x870_75hz" :
	appleTimingID == kIOTimingIDApple_640x818_75hz    ?    "Apple_640x818_75hz" :
	appleTimingID == kIOTimingIDApple_832x624_75hz    ?    "Apple_832x624_75hz" :
	appleTimingID == kIOTimingIDVESA_800x600_56hz     ?     "VESA_800x600_56hz" :
	appleTimingID == kIOTimingIDVESA_800x600_60hz     ?     "VESA_800x600_60hz" :
	appleTimingID == kIOTimingIDVESA_800x600_72hz     ?     "VESA_800x600_72hz" :
	appleTimingID == kIOTimingIDVESA_800x600_75hz     ?     "VESA_800x600_75hz" :
	appleTimingID == kIOTimingIDVESA_800x600_85hz     ?     "VESA_800x600_85hz" :
	appleTimingID == kIOTimingIDVESA_1024x768_60hz    ?    "VESA_1024x768_60hz" :
	appleTimingID == kIOTimingIDVESA_1024x768_70hz    ?    "VESA_1024x768_70hz" :
	appleTimingID == kIOTimingIDVESA_1024x768_75hz    ?    "VESA_1024x768_75hz" :
	appleTimingID == kIOTimingIDVESA_1024x768_85hz    ?    "VESA_1024x768_85hz" :
	appleTimingID == kIOTimingIDApple_1024x768_75hz   ?   "Apple_1024x768_75hz" :
	appleTimingID == kIOTimingIDVESA_1152x864_75hz    ?    "VESA_1152x864_75hz" :
	appleTimingID == kIOTimingIDApple_1152x870_75hz   ?   "Apple_1152x870_75hz" :
	appleTimingID == kIOTimingIDAppleNTSC_ST          ?          "AppleNTSC_ST" :
	appleTimingID == kIOTimingIDAppleNTSC_FF          ?          "AppleNTSC_FF" :
	appleTimingID == kIOTimingIDAppleNTSC_STconv      ?      "AppleNTSC_STconv" :
	appleTimingID == kIOTimingIDAppleNTSC_FFconv      ?      "AppleNTSC_FFconv" :
	appleTimingID == kIOTimingIDApplePAL_ST           ?           "ApplePAL_ST" :
	appleTimingID == kIOTimingIDApplePAL_FF           ?           "ApplePAL_FF" :
	appleTimingID == kIOTimingIDApplePAL_STconv       ?       "ApplePAL_STconv" :
	appleTimingID == kIOTimingIDApplePAL_FFconv       ?       "ApplePAL_FFconv" :
	appleTimingID == kIOTimingIDVESA_1280x960_75hz    ?    "VESA_1280x960_75hz" :
	appleTimingID == kIOTimingIDVESA_1280x960_60hz    ?    "VESA_1280x960_60hz" :
	appleTimingID == kIOTimingIDVESA_1280x960_85hz    ?    "VESA_1280x960_85hz" :
	appleTimingID == kIOTimingIDVESA_1280x1024_60hz   ?   "VESA_1280x1024_60hz" :
	appleTimingID == kIOTimingIDVESA_1280x1024_75hz   ?   "VESA_1280x1024_75hz" :
	appleTimingID == kIOTimingIDVESA_1280x1024_85hz   ?   "VESA_1280x1024_85hz" :
	appleTimingID == kIOTimingIDVESA_1600x1200_60hz   ?   "VESA_1600x1200_60hz" :
	appleTimingID == kIOTimingIDVESA_1600x1200_65hz   ?   "VESA_1600x1200_65hz" :
	appleTimingID == kIOTimingIDVESA_1600x1200_70hz   ?   "VESA_1600x1200_70hz" :
	appleTimingID == kIOTimingIDVESA_1600x1200_75hz   ?   "VESA_1600x1200_75hz" :
	appleTimingID == kIOTimingIDVESA_1600x1200_80hz   ?   "VESA_1600x1200_80hz" :
	appleTimingID == kIOTimingIDVESA_1600x1200_85hz   ?   "VESA_1600x1200_85hz" :
	appleTimingID == kIOTimingIDVESA_1792x1344_60hz   ?   "VESA_1792x1344_60hz" :
	appleTimingID == kIOTimingIDVESA_1792x1344_75hz   ?   "VESA_1792x1344_75hz" :
	appleTimingID == kIOTimingIDVESA_1856x1392_60hz   ?   "VESA_1856x1392_60hz" :
	appleTimingID == kIOTimingIDVESA_1856x1392_75hz   ?   "VESA_1856x1392_75hz" :
	appleTimingID == kIOTimingIDVESA_1920x1440_60hz   ?   "VESA_1920x1440_60hz" :
	appleTimingID == kIOTimingIDVESA_1920x1440_75hz   ?   "VESA_1920x1440_75hz" :
	appleTimingID == kIOTimingIDSMPTE240M_60hz        ?        "SMPTE240M_60hz" :
	appleTimingID == kIOTimingIDFilmRate_48hz         ?         "FilmRate_48hz" :
	appleTimingID == kIOTimingIDSony_1600x1024_76hz   ?   "Sony_1600x1024_76hz" :
	appleTimingID == kIOTimingIDSony_1920x1080_60hz   ?   "Sony_1920x1080_60hz" :
	appleTimingID == kIOTimingIDSony_1920x1080_72hz   ?   "Sony_1920x1080_72hz" :
	appleTimingID == kIOTimingIDSony_1920x1200_76hz   ?   "Sony_1920x1200_76hz" :
	appleTimingID == kIOTimingIDApple_0x0_0hz_Offline ? "Apple_0x0_0hz_Offline" :
	appleTimingID == kIOTimingIDVESA_848x480_60hz     ?     "VESA_848x480_60hz" :
	appleTimingID == kIOTimingIDVESA_1360x768_60hz    ?    "VESA_1360x768_60hz" :
	UNKNOWN_VALUE(appleTimingID);
}

void RemoveTrailingComma(char *flagsstr)
{
	size_t len = strlen(flagsstr);
	if (len && flagsstr[len-1] == ',') {
		flagsstr[len-1] = '\0';
	}
}

static char * GetOneFlagsStr(UInt64 flags) {
	char * flagsstr = (char *)malloc(1000);
	if (flagsstr) {
		snprintf(flagsstr, 1000,
			"%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",                                        // csTimingFlags values in VDTimingInfoRec
			flags & kDisplayModeValidFlag              ?                  "Valid," : "", // 0x00000001 // kModeValid
			flags & kDisplayModeSafeFlag               ?                   "Safe," : "", // 0x00000002 // kModeSafe
			flags & kDisplayModeDefaultFlag            ?                "Default," : "", // 0x00000004 // kModeDefault
			flags & kDisplayModeAlwaysShowFlag         ?             "AlwaysShow," : "", // 0x00000008 // kModeShowNow
			flags & kDisplayModeNotResizeFlag          ?              "NotResize," : "", // 0x00000010 // kModeNotResize
			flags & kDisplayModeRequiresPanFlag        ?            "RequiresPan," : "", // 0x00000020 // kModeRequiresPan
			flags & kDisplayModeInterlacedFlag         ?             "Interlaced," : "", // 0x00000040 // kModeInterlaced
			flags & kDisplayModeNeverShowFlag          ?              "NeverShow," : "", // 0x00000080 // kModeShowNever
			flags & kDisplayModeSimulscanFlag          ?              "Simulscan," : "", // 0x00000100 // kModeSimulscan
			flags & kDisplayModeNotPresetFlag          ?              "NotPreset," : "", // 0x00000200 // kModeNotPreset
			flags & kDisplayModeBuiltInFlag            ?                "BuiltIn," : "", // 0x00000400 // kModeBuiltIn
			flags & kDisplayModeStretchedFlag          ?              "Stretched," : "", // 0x00000800 // kModeStretched
			flags & kDisplayModeNotGraphicsQualityFlag ?     "NotGraphicsQuality," : "", // 0x00001000 // kModeNotGraphicsQuality
			flags & kDisplayModeValidateAgainstDisplay ? "ValidateAgainstDisplay," : "", // 0x00002000 // kModeValidateAgainstDisplay

			flags & kDisplayModeTelevisionFlag         ?             "Television," : "", // 0x00100000
			flags & kDisplayModeValidForMirroringFlag  ?      "ValidForMirroring," : "", // 0x00200000
			flags & kDisplayModeAcceleratorBackedFlag  ?      "AcceleratorBacked," : "", // 0x00400000
			flags & kDisplayModeValidForHiResFlag      ?          "ValidForHiRes," : "", // 0x00800000
			flags & kDisplayModeValidForAirPlayFlag    ?        "ValidForAirPlay," : "", // 0x01000000
			flags & kDisplayModeNativeFlag             ?                 "Native," : "", // 0x02000000

			(flags & 0x0000c000) == 0x00004000         ?                 "¿4<<12," :
			(flags & 0x0000c000) == 0x00008000         ?                 "¿8<<12," :
			(flags & 0x0000c000) == 0x0000c000         ?                 "¿C<<12," : "",

			(flags & 0x000f0000) == 0x00010000         ?                 "¿1<<16," :
			(flags & 0x000f0000) == 0x00020000         ?                 "¿2<<16," :
			(flags & 0x000f0000) == 0x00030000         ?                 "¿3<<16," :
			(flags & 0x000f0000) == 0x00040000         ?                 "¿4<<16," :
			(flags & 0x000f0000) == 0x00050000         ?                 "¿5<<16," :
			(flags & 0x000f0000) == 0x00060000         ?                 "¿6<<16," :
			(flags & 0x000f0000) == 0x00070000         ?                 "¿7<<16," :
			(flags & 0x000f0000) == 0x00080000         ?                 "¿8<<16," :
			(flags & 0x000f0000) == 0x00090000         ?                 "¿9<<16," :
			(flags & 0x000f0000) == 0x000a0000         ?                 "¿A<<16," :
			(flags & 0x000f0000) == 0x000b0000         ?                 "¿B<<16," :
			(flags & 0x000f0000) == 0x000c0000         ?                 "¿C<<16," :
			(flags & 0x000f0000) == 0x000d0000         ?                 "¿D<<16," :
			(flags & 0x000f0000) == 0x000e0000         ?                 "¿E<<16," :
			(flags & 0x000f0000) == 0x000f0000         ?                 "¿F<<16," : "",

			(flags & 0x0c000000) == 0x04000000         ?                 "¿4<<24," :
			(flags & 0x0c000000) == 0x08000000         ?                 "¿8<<24," :
			(flags & 0x0c000000) == 0x0c000000         ?                 "¿C<<24," : "",

			(flags & 0xf0000000) == 0x10000000         ?                 "¿1<<28," :
			(flags & 0xf0000000) == 0x20000000         ?                 "¿2<<28," :
			(flags & 0xf0000000) == 0x30000000         ?                 "¿3<<28," :
			(flags & 0xf0000000) == 0x40000000         ?                 "¿4<<28," :
			(flags & 0xf0000000) == 0x50000000         ?                 "¿5<<28," :
			(flags & 0xf0000000) == 0x60000000         ?                 "¿6<<28," : // used only by CGSDisplayModeDescription.flags for the 10bpc modes
			(flags & 0xf0000000) == 0x70000000         ?                 "¿7<<28," :
			(flags & 0xf0000000) == 0x80000000         ?                 "¿8<<28," :
			(flags & 0xf0000000) == 0x90000000         ?                 "¿9<<28," :
			(flags & 0xf0000000) == 0xa0000000         ?                 "¿A<<28," :
			(flags & 0xf0000000) == 0xb0000000         ?                 "¿B<<28," :
			(flags & 0xf0000000) == 0xc0000000         ?                 "¿C<<28," :
			(flags & 0xf0000000) == 0xd0000000         ?                 "¿D<<28," :
			(flags & 0xf0000000) == 0xe0000000         ?                 "¿E<<28," :
			(flags & 0xf0000000) == 0xf0000000         ?                 "¿F<<28," : ""
		);
	}
	RemoveTrailingComma(flagsstr);
	return flagsstr;
} // GetOneFlagsStr

static bool DumpOneCursorInfo(CFDataRef IOFBOneCursorInfo, int compareNdx) {
	bool good = true;
	CFIndex size = CFDataGetLength(IOFBOneCursorInfo);
	
	IOHardwareCursorDescriptor_64bit infoCopy;
	IOHardwareCursorDescriptor_64bit *info = NULL;
	IOHardwareCursorDescriptor_32bit *info32;

	if (size != sizeof(IOHardwareCursorDescriptor_64bit) && size != sizeof(IOHardwareCursorDescriptor_32bit)) {
		good = false;
		cprintf("Unexpected size:%ld", (long)size);
		if (size >= sizeof(IOHardwareCursorDescriptor_32bit)) {
			cprintf(" ");
		}
	}

	if (size >= sizeof(IOHardwareCursorDescriptor_64bit)) {
		cprintf("(64 bit) ");
		info = (IOHardwareCursorDescriptor_64bit *)CFDataGetBytePtr(IOFBOneCursorInfo);
	}
	else if (size >= sizeof(IOHardwareCursorDescriptor_32bit)) {
		cprintf("(32 bit) ");
		info = &infoCopy;
		info32 = (IOHardwareCursorDescriptor_32bit *)CFDataGetBytePtr(IOFBOneCursorInfo);
		info->majorVersion              = info32->majorVersion              ;
		info->minorVersion              = info32->minorVersion              ;
		info->height                    = info32->height                    ;
		info->width                     = info32->width                     ;
		info->bitDepth                  = info32->bitDepth                  ;
		info->maskBitDepth              = info32->maskBitDepth              ;
		info->numColors                 = info32->numColors                 ;
		info->colorEncodings            = info32->colorEncodings            ;
		info->flags                     = info32->flags                     ;
		info->supportedSpecialEncodings = info32->supportedSpecialEncodings ;
		memcpy(info->specialEncodings, info32->specialEncodings, sizeof(info->specialEncodings));
	}

	if (info) {
		cprintf("{ version:%u.%u size:%ux%u depth:%s maskBitDepth:%x colors:%u colorEncodings:%llx flags:%x specialEncodings:(",
			info->majorVersion,
			info->minorVersion,
			(uint32_t)info->height,
			(uint32_t)info->width,
			// bits per pixel, or a QD/QT pixel type
			info->bitDepth == kIO1MonochromePixelFormat  ?  "1Monochrome" :
			info->bitDepth == kIO2IndexedPixelFormat     ?     "2Indexed" :
			info->bitDepth == kIO4IndexedPixelFormat     ?     "4Indexed" :
			info->bitDepth == kIO8IndexedPixelFormat     ?     "8Indexed" :
			info->bitDepth == kIO16BE555PixelFormat      ?      "16BE555" :
			info->bitDepth == kIO24RGBPixelFormat        ?        "24RGB" :
			info->bitDepth == kIO32ARGBPixelFormat       ?       "32ARGB" :
			info->bitDepth == kIO1IndexedGrayPixelFormat ? "1IndexedGray" :
			info->bitDepth == kIO2IndexedGrayPixelFormat ? "2IndexedGray" :
			info->bitDepth == kIO4IndexedGrayPixelFormat ? "4IndexedGray" :
			info->bitDepth == kIO8IndexedGrayPixelFormat ? "8IndexedGray" :
			info->bitDepth == kIO16LE555PixelFormat      ?      "16LE555" :
			info->bitDepth == kIO16LE5551PixelFormat     ?     "16LE5551" :
			info->bitDepth == kIO16BE565PixelFormat      ?      "16BE565" :
			info->bitDepth == kIO16LE565PixelFormat      ?      "16LE565" :
			info->bitDepth == kIO24BGRPixelFormat        ?        "24BGR" :
			info->bitDepth == kIO32BGRAPixelFormat       ?       "32BGRA" :
			info->bitDepth == kIO32ABGRPixelFormat       ?       "32ABGR" :
			info->bitDepth == kIO32RGBAPixelFormat       ?       "32RGBA" :
			info->bitDepth == kIOYUVSPixelFormat         ?         "YUVS" :
			info->bitDepth == kIOYUVUPixelFormat         ?         "YUVU" :
			info->bitDepth == kIOYVU9PixelFormat         ?         "YVU9" :
			info->bitDepth == kIOYUV411PixelFormat       ?       "YUV411" :
			info->bitDepth == kIOYVYU422PixelFormat      ?      "YVYU422" :
			info->bitDepth == kIOUYVY422PixelFormat      ?      "UYVY422" :
			info->bitDepth == kIOYUV211PixelFormat       ?       "YUV211" :
			info->bitDepth == kIO2vuyPixelFormat         ?         "2vuy" :
			info->bitDepth == kIO16LE4444PixelFormat     ?     "16LE4444" :
			info->bitDepth == kIO16BE4444PixelFormat     ?     "16BE4444" :
			info->bitDepth == kIO64BGRAPixelFormat       ?       "64BGRA" :
			info->bitDepth == kIO64RGBAFloatPixelFormat  ?  "64RGBAFloat" :
			info->bitDepth == kIO128RGBAFloatPixelFormat ? "128RGBAFloat" :
			UNKNOWN_VALUE(info->bitDepth),
				
			(uint32_t)info->maskBitDepth,                   // unused
			(uint32_t)info->numColors,                      // number of colors in the colorMap. ie.
			(UInt64)info->colorEncodings,         // UInt32 *
			(uint32_t)info->flags
		);
		
		bool gotone = false;
		for (int i = 0; i < 16; i++) {
			int supported = (info->supportedSpecialEncodings >> (i << 1)) & 3;
			if (info->specialEncodings[i] || supported) {
				if (gotone) cprintf(",");
				switch (i) {
					case kTransparentEncoding: cprintf("transparent"); break;
					case kInvertingEncoding  : cprintf("inverting"  ); break;
					default                  : cprintf("%d?", i     ); break;
				}
				switch (supported) {
					case 0  : cprintf(" (not supported?)"); break;
					case 1  : cprintf(""                 ); break;
					default : cprintf(" (%d?)", supported); break;
				}
				cprintf(":%06x", (uint32_t)info->specialEncodings[i]);
				gotone = true;
			}
		}
		cprintf(") },");
	}

	return good;
} // DumpOneCursorInfo

static char * DumpOneDetailedTimingInformationPtr(char * buf, size_t bufSize, void *IOFBDetailedTiming, CFIndex size, int modeAlias) {
	int inc = 0;
	switch (size) {
		case sizeof(IODetailedTimingInformationV1): inc += scnprintf(buf+inc, bufSize-inc, "V1"); break;
		case sizeof(IODetailedTimingInformationV2): inc += scnprintf(buf+inc, bufSize-inc, "V2"); break;
		default                                   : inc += scnprintf(buf+inc, bufSize-inc, "Unexpected size:%ld", (long)size); break;
	}

	if (size >= sizeof(IODetailedTimingInformationV2)) {
		char hexDigits[] = "0123456789abcdef";
		hexDigits[modeAlias] = '.';
		
		IODetailedTimingInformationV2_12 *timing = (IODetailedTimingInformationV2_12 *)IOFBDetailedTiming;

#if 0
		if (((SInt32)timing->verticalSyncOffset) < 0) {
			printf("oops");
		}
#endif

		inc += scnprintf(buf+inc, bufSize-inc,
			" id:0x%04x%c%03x %ux%u%s@%.3fHz",

			 //__reservedA[0]
			(uint32_t)timing->detailedTimingModeID_10_1 >> 16, // mode
			(uint32_t)(hexDigits[(timing->detailedTimingModeID_10_1 >> 12) & 15]),
			(uint32_t)(timing->detailedTimingModeID_10_1 & 0x0fff),

			(uint32_t)(timing->horizontalScaled ? timing->horizontalScaled : timing->horizontalActive),
			(uint32_t)(timing->verticalScaled ? timing->verticalScaled : timing->verticalActive),
			
			(timing->signalConfig & kIOInterlacedCEATiming) ? "i" : "",

			timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * (timing->verticalActive + timing->verticalBlanking)) // Hz
		);

		if (DarwinMajorVersion() >= 17 && timing->verticalBlankingExtension) { // 10.13
			inc += scnprintf(buf+inc, bufSize-inc,
				" (min:%.3fHz)",
				timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * (timing->verticalActive + timing->verticalBlanking + timing->verticalBlankingExtension))
			);
		}

		inc += scnprintf(buf+inc, bufSize-inc,
			" %.3fkHz %.3fMHz",
			timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * 1000.0), // kHz
			timing->pixelClock / 1000000.0
		);

		if (timing->minPixelClock == 0 && timing->maxPixelClock == 0) {
			inc += scnprintf(buf+inc, bufSize-inc, " (errMHz ø,ø)");
		}
		else {
			double minPixelClockErr  = ((SInt64)timing->minPixelClock - (SInt64)timing->pixelClock) / 1000000.0;
			double minPixelClockErr2 = ((SInt32)timing->minPixelClock - (SInt64)timing->pixelClock) / 1000000.0;
			double maxPixelClockErr  = ((SInt64)timing->maxPixelClock - (SInt64)timing->pixelClock) / 1000000.0;
			
			inc += scnprintf(buf+inc, bufSize-inc, " (errMHz %g,%g)",
				(timing->pixelClock == 0 && (timing->minPixelClock >> 32) == 0) ? minPixelClockErr2 : // initialized to -100000 & 0xffffffff for unconnected display of 7800 GT
				minPixelClockErr, // Hz - With error what is slowest actual clock
				maxPixelClockErr // Hz - With error what is fasted actual clock
			);
		}

		inc += scnprintf(buf+inc, bufSize-inc,
			"  h(%d %u %u %s%s)  v(%d %u %u %s%s)  border(h%d:%u v%d:%u)  active:%ux%u %s",
			(int)(SInt32)timing->horizontalSyncOffset,           // pixels // initialized to -1 for unconnected display of 7800 GT
			(uint32_t)timing->horizontalSyncPulseWidth,       // pixels
			(uint32_t)(timing->horizontalBlanking - timing->horizontalSyncOffset - timing->horizontalSyncPulseWidth), // pixels

			timing->horizontalSyncConfig == kIOSyncPositivePolarity ? "+" :
			timing->horizontalSyncConfig == 0 ? "-" :
			UNKNOWN_VALUE(timing->horizontalSyncConfig),

			timing->horizontalSyncLevel == 0 ? "" :
			UNKNOWN_VALUE(timing->horizontalSyncLevel), // Future use (init to 0)

			(int)(SInt32)timing->verticalSyncOffset,             // lines // initialized to -1 for unconnected display of 7800 GT
			(uint32_t)timing->verticalSyncPulseWidth,         // lines
			(uint32_t)(timing->verticalBlanking - timing->verticalSyncOffset - timing->verticalSyncPulseWidth), // lines

			timing->verticalSyncConfig == kIOSyncPositivePolarity ? "+" :
			timing->verticalSyncConfig == 0 ? "-" :
			UNKNOWN_VALUE(timing->verticalSyncConfig),

			timing->verticalSyncLevel == 0 ? "" :
			UNKNOWN_VALUE(timing->verticalSyncLevel), // Future use (init to 0)

			(int)(SInt32)timing->horizontalBorderLeft,           // pixels // initialized to -1 for unconnected display of 7800 GT
			(uint32_t)timing->horizontalBorderRight,          // pixels
			(int)(SInt32)timing->verticalBorderTop,              // lines // initialized to -1 for unconnected display of 7800 GT
			(uint32_t)timing->verticalBorderBottom,           // lines

			(uint32_t)timing->horizontalActive,               // pixels
			(uint32_t)timing->verticalActive,                 // lines

			(timing->horizontalScaled && timing->verticalScaled) ? "(scaled)" :
			((!timing->horizontalScaled) != (!timing->verticalScaled)) ? "(scaled?)" : "(not scaled)"
		);

		if (DarwinMajorVersion() >= 8) { // 10.4
			inc += scnprintf(buf+inc, bufSize-inc,
				" inset:%ux%u",
				(uint32_t)timing->horizontalScaledInset,          // pixels
				(uint32_t)timing->verticalScaledInset             // lines
			);
		}

		inc += scnprintf(buf+inc, bufSize-inc,
			" flags(%s%s%s%s%s%s%s%s%s%s%s%s%s%s) signal(%s%s%s%s%s%s%s%s) levels:%s",
			(timing->scalerFlags & kIOScaleStretchToFit) ? "fit," : "",
			(timing->scalerFlags & 2) ? "2?," : "",
			(timing->scalerFlags & 4) ? "4?," : "",
			(timing->scalerFlags & 8) ? "8?," : "",
			((timing->scalerFlags & 0x70) == kIOScaleSwapAxes ) ?    "swap," : "", // 1
			((timing->scalerFlags & 0x70) == kIOScaleInvertX  ) ? "invertx," : "", // 2
			((timing->scalerFlags & 0x70) == kIOScaleInvertY  ) ? "inverty," : "", // 4
			((timing->scalerFlags & 0x70) == kIOScaleRotate0  ) ?      "0°," : "", // 0
			((timing->scalerFlags & 0x70) == kIOScaleRotate90 ) ?     "90°," : "", // 3
			((timing->scalerFlags & 0x70) == kIOScaleRotate180) ?    "180°," : "", // 6
			((timing->scalerFlags & 0x70) == kIOScaleRotate270) ?    "270°," : "", // 5
			((timing->scalerFlags & 0x70) == 0x70             ) ? "swap,invertx,inverty," : "",
			((timing->scalerFlags & 0x80)                     ) ?   "0x80?," : "",
			UNKNOWN_FLAG(timing->scalerFlags & 0xffffff00),

			(timing->signalConfig & kIODigitalSignal      ) ?               "digital," : "",
			(timing->signalConfig & kIOAnalogSetupExpected) ? "analog setup expected," : "",
			(timing->signalConfig & kIOInterlacedCEATiming) ?        "interlaced CEA," : "",
			(timing->signalConfig & kIONTSCTiming         ) ?                  "NTSC," : "",
			(timing->signalConfig & kIOPALTiming          ) ?                   "PAL," : "",
			(timing->signalConfig & kIODSCBlockPredEnable ) ? "DSC block pred enable," : "",
			(timing->signalConfig & kIOMultiAlignedTiming ) ?         "multi aligned," : "",
			UNKNOWN_FLAG(timing->signalConfig & 0xffffff80),

			(timing->signalLevels == kIOAnalogSignalLevel_0700_0300) ? "0700_0300" :
			(timing->signalLevels == kIOAnalogSignalLevel_0714_0286) ? "0714_0286" :
			(timing->signalLevels == kIOAnalogSignalLevel_1000_0400) ? "1000_0400" :
			(timing->signalLevels == kIOAnalogSignalLevel_0700_0000) ? "0700_0000" :
			UNKNOWN_VALUE(timing->signalLevels)
		);

		if (DarwinMajorVersion() >= 7) { // 10.3 (maybe 10.3.9 but we'll do this for all revisions anyway
			inc += scnprintf(buf+inc, bufSize-inc,
				" links:%u",
				(uint32_t)timing->numLinks
			);
		}

		if (DarwinMajorVersion() >= 17) { // 10.13
			inc += scnprintf(buf+inc, bufSize-inc,
				" vbext:%u",
				(uint32_t)timing->verticalBlankingExtension      // lines (AdaptiveSync: 0 for non-AdaptiveSync support)
			);
		}

		if (DarwinMajorVersion() >= 21) { // 12
			inc += scnprintf(buf+inc, bufSize-inc,
				" vbstretch:%u vbshrink:%u",
				timing->verticalBlankingMaxStretchPerFrame,
				timing->verticalBlankingMaxShrinkPerFrame
			);
		}

		if (DarwinMajorVersion() >= 18) { // 10.14
			inc += scnprintf(buf+inc, bufSize-inc,
				" encodings(%s%s%s%s%s) bpc(%s%s%s%s%s%s) colorimetry(%s%s%s%s%s%s%s%s%s%s%s) dynamicrange(%s%s%s%s%s%s%s)",
				(timing->pixelEncoding & kIOPixelEncodingRGB444  ) ? "RGB," : "",
				(timing->pixelEncoding & kIOPixelEncodingYCbCr444) ? "444," : "",
				(timing->pixelEncoding & kIOPixelEncodingYCbCr422) ? "422," : "",
				(timing->pixelEncoding & kIOPixelEncodingYCbCr420) ? "420," : "",
				UNKNOWN_FLAG(timing->pixelEncoding & 0xfff0),
				
				(timing->bitsPerColorComponent & kIOBitsPerColorComponent6 ) ?  "6," : "",
				(timing->bitsPerColorComponent & kIOBitsPerColorComponent8 ) ?  "8," : "",
				(timing->bitsPerColorComponent & kIOBitsPerColorComponent10) ? "10," : "",
				(timing->bitsPerColorComponent & kIOBitsPerColorComponent12) ? "12," : "",
				(timing->bitsPerColorComponent & kIOBitsPerColorComponent16) ? "16," : "",
				UNKNOWN_FLAG(timing->bitsPerColorComponent & 0xffe0),
				
				(timing->colorimetry & kIOColorimetryNativeRGB) ? "NativeRGB," : "",
				(timing->colorimetry & kIOColorimetrysRGB     ) ?      "sRGB," : "",
				(timing->colorimetry & kIOColorimetryDCIP3    ) ?     "DCIP3," : "",
				(timing->colorimetry & kIOColorimetryAdobeRGB ) ?  "AdobeRGB," : "",
				(timing->colorimetry & kIOColorimetryxvYCC    ) ?     "xvYCC," : "",
				(timing->colorimetry & kIOColorimetryWGRGB    ) ?     "WGRGB," : "",
				(timing->colorimetry & kIOColorimetryBT601    ) ?     "BT601," : "",
				(timing->colorimetry & kIOColorimetryBT709    ) ?     "BT709," : "",
				(timing->colorimetry & kIOColorimetryBT2020   ) ?    "BT2020," : "",
				(timing->colorimetry & kIOColorimetryBT2100   ) ?    "BT2100," : "",
				UNKNOWN_FLAG(timing->colorimetry & 0xfc00),
				
				(timing->dynamicRange & kIODynamicRangeSDR                ) ?                 "SDR," : "",
				(timing->dynamicRange & kIODynamicRangeHDR10              ) ?               "HDR10," : "",
				(timing->dynamicRange & kIODynamicRangeDolbyNormalMode    ) ?     "DolbyNormalMode," : "",
				(timing->dynamicRange & kIODynamicRangeDolbyTunnelMode    ) ?     "DolbyTunnelMode," : "",
				(timing->dynamicRange & kIODynamicRangeTraditionalGammaHDR) ? "TraditionalGammaHDR," : "",
				(timing->dynamicRange & kIODynamicRangeTraditionalGammaSDR) ? "TraditionalGammaSDR," : "",
				UNKNOWN_FLAG(timing->dynamicRange & 0xffc0)
			);
		}

		if (DarwinMajorVersion() >= 19) { // 10.15
			inc += scnprintf(buf+inc, bufSize-inc,
				" dsc(%ux%u %gbpp)",
				timing->dscSliceWidth,
				timing->dscSliceHeight,
				timing->dscCompressedBitsPerPixel / 16.0
			);
		}

#define DUMP_RESERVED(start, reserved, name) \
		do { \
			for (int i = start; i < sizeof(reserved) / sizeof(reserved[0]); i++) { \
				if (reserved[i]) { \
					inc += scnprintf(buf+inc, bufSize-inc, " %s%d:%s", name, i, UNKNOWN_VALUE(reserved[i])); \
				} \
			} \
		} while(0)

		if (DarwinMajorVersion() >= 8) { // 10.4
			DUMP_RESERVED(1, timing->__reservedA_10_4, "reservedA");
		}
		else {
			DUMP_RESERVED(1, timing->__reservedA_10_1, "reservedA");
		}

		if (DarwinMajorVersion() >= 21) { // 12
			DUMP_RESERVED(0, timing->__reservedB_12_3, "reservedB");
		}
		else if (DarwinMajorVersion() >= 19) { // 10.15
			DUMP_RESERVED(0, timing->__reservedB_10_15, "reservedB");
		}
		else if (DarwinMajorVersion() >= 18) { // 10.14
			DUMP_RESERVED(0, timing->__reservedB_10_14, "reservedB");
		}
		else if (DarwinMajorVersion() >= 17) { // 10.13
			DUMP_RESERVED(0, timing->__reservedB_10_13, "reservedB");
		}
		else if (DarwinMajorVersion() >= 7) { // 10.3
			DUMP_RESERVED(0, timing->__reservedB_10_3_9, "reservedB");
		}
		else {
			DUMP_RESERVED(0, timing->__reservedB_10_1, "reservedB");
		}
	}
	else if (size >= sizeof(IODetailedTimingInformationV1)) {
		IODetailedTimingInformationV1 *timing = (IODetailedTimingInformationV1 *)IOFBDetailedTiming;
		inc += scnprintf(buf+inc, bufSize-inc, " %ux%u@%.3fHz %.3fkHz %.3fMHz  h(%u %u %u)  v(%u %u %u)  border(h%u v%u)",
			(uint32_t)timing->horizontalActive,               // pixels
			(uint32_t)timing->verticalActive,                 // lines
			timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * (timing->verticalActive + timing->verticalBlanking)), // Hz
			timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * 1000.0), // kHz
			timing->pixelClock / 1000000.0,

			(uint32_t)timing->horizontalSyncOffset,           // pixels
			(uint32_t)timing->horizontalSyncWidth,            // pixels
			(uint32_t)(timing->horizontalBlanking - timing->horizontalSyncOffset - timing->horizontalSyncWidth), // pixels

			(uint32_t)timing->verticalSyncOffset,             // lines
			(uint32_t)timing->verticalSyncWidth,              // lines
			(uint32_t)(timing->verticalBlanking - timing->verticalSyncOffset - timing->verticalSyncWidth), // lines

			(uint32_t)timing->horizontalBorder,       // pixels
			(uint32_t)timing->verticalBorder          // lines
		);
	}
	return buf;
} // DumpOneDetailedTimingInformationPtr

// "timing"
static char * DumpOneTimingInformationPtr(char * buf, size_t bufSize, IOTimingInformation * info, size_t detailedSize, int modeAlias) {
	int inc = 0;
	char * result = buf;
	
	char timinginfo[1000];
	
	inc += scnprintf(buf+inc, bufSize-inc, "appleTimingID = %s; flags = %s%s%s;",
		GetOneAppleTimingID(info->appleTimingID),
		info->flags & kIODetailedTimingValid ? "DetailedTimingValid," : "",
		info->flags & kIOScalingInfoValid    ?    "ScalingInfoValid," : "",
		UNKNOWN_FLAG(info->flags & 0x3fffffff)
	);
	
	if (info->flags & kIODetailedTimingValid) {
		inc += scnprintf(buf+inc, bufSize-inc, " DetailedTimingInformation = { %s }",
			DumpOneDetailedTimingInformationPtr(timinginfo, sizeof(timinginfo), &info->detailedInfo, detailedSize, modeAlias)
		);
	}

	return result;
} // DumpOneTimingInformationPtr

static char * DumpOneDetailedTimingInformationPtrFromEdid(char * buf, size_t bufSize, IODetailedTimingInformationV2_12 *timing) {
	int inc = 0;

	inc += scnprintf(buf+inc, bufSize-inc,
		"%ux%u%s@%.3fHz",

		(uint32_t)(timing->horizontalActive),
		(uint32_t)(timing->verticalActive),
		
		(timing->signalConfig & kIOInterlacedCEATiming) ? "i" : "",

		timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * (timing->verticalActive + timing->verticalBlanking)) // Hz
	);

	inc += scnprintf(buf+inc, bufSize-inc,
		" %.3fkHz %.3fMHz",
		timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * 1000.0), // kHz
		timing->pixelClock / 1000000.0
	);

	inc += scnprintf(buf+inc, bufSize-inc,
		"  h(%d %u %u %s)  v(%d %u %u %s)  border(h%d:%u v%d:%u)",
		(int)(SInt32)timing->horizontalSyncOffset,           // pixels // initialized to -1 for unconnected display of 7800 GT
		(uint32_t)timing->horizontalSyncPulseWidth,       // pixels
		(uint32_t)(timing->horizontalBlanking - timing->horizontalSyncOffset - timing->horizontalSyncPulseWidth), // pixels

		timing->horizontalSyncConfig == kIOSyncPositivePolarity ? "+" :
		timing->horizontalSyncConfig == 0 ? "-" :
		UNKNOWN_VALUE(timing->horizontalSyncConfig),

		(int)(SInt32)timing->verticalSyncOffset,             // lines // initialized to -1 for unconnected display of 7800 GT
		(uint32_t)timing->verticalSyncPulseWidth,         // lines
		(uint32_t)(timing->verticalBlanking - timing->verticalSyncOffset - timing->verticalSyncPulseWidth), // lines

		timing->verticalSyncConfig == kIOSyncPositivePolarity ? "+" :
		timing->verticalSyncConfig == 0 ? "-" :
		UNKNOWN_VALUE(timing->verticalSyncConfig),

		(int)(SInt32)timing->horizontalBorderLeft,           // pixels // initialized to -1 for unconnected display of 7800 GT
		(uint32_t)timing->horizontalBorderRight,          // pixels
		(int)(SInt32)timing->verticalBorderTop,              // lines // initialized to -1 for unconnected display of 7800 GT
		(uint32_t)timing->verticalBorderBottom           // lines
	);

	inc += scnprintf(buf+inc, bufSize-inc,
		"  signal(%s%s%s%s%s%s%s%s) levels:%s",
		(timing->signalConfig & kIODigitalSignal      ) ?               "digital," : "",
		(timing->signalConfig & kIOAnalogSetupExpected) ? "analog setup expected," : "",
		(timing->signalConfig & kIOInterlacedCEATiming) ?        "interlaced CEA," : "",
		(timing->signalConfig & kIONTSCTiming         ) ?                  "NTSC," : "",
		(timing->signalConfig & kIOPALTiming          ) ?                   "PAL," : "",
		(timing->signalConfig & kIODSCBlockPredEnable ) ? "DSC block pred enable," : "",
		(timing->signalConfig & kIOMultiAlignedTiming ) ?         "multi aligned," : "",
		UNKNOWN_FLAG(timing->signalConfig & 0xffffff80),

		(timing->signalLevels == kIOAnalogSignalLevel_0700_0300) ? "0700_0300" :
		(timing->signalLevels == kIOAnalogSignalLevel_0714_0286) ? "0714_0286" :
		(timing->signalLevels == kIOAnalogSignalLevel_1000_0400) ? "1000_0400" :
		(timing->signalLevels == kIOAnalogSignalLevel_0700_0000) ? "0700_0000" :
		UNKNOWN_VALUE(timing->signalLevels)
	);

	return buf;
} // DumpOneDetailedTimingInformationPtrFromEdid

static void DumpOneTimingInformation(CFDataRef IOTimingInformationData, int modeAlias) {
	char timinginfo[1000];
	CFIndex size = CFDataGetLength(IOTimingInformationData);
	if (size < offsetof(IOTimingInformation, detailedInfo)) {
		cprintf("Unexpected size:%ld", (long)size);
	}
	else {
		IOTimingInformation *info = (IOTimingInformation *)CFDataGetBytePtr(IOTimingInformationData);
		cprintf("%s", DumpOneTimingInformationPtr(timinginfo, sizeof(timinginfo), info, size - offsetof(IOTimingInformation, detailedInfo), modeAlias));
	}
} // DumpOneTimingInformation

// "info"
static char * DumpOneDisplayModeInformationPtr(char * buf, size_t bufSize, IODisplayModeInformation_10_8 * info) {
	int inc = 0;
	char * result = buf;
	char * flagsstr = GetOneFlagsStr(info->flags);
	inc += scnprintf(buf+inc, bufSize-inc,
		"%ux%u@%.3fHz maxdepth:%u flags:%s"
		,
		(uint32_t)info->nominalWidth,
		(uint32_t)info->nominalHeight,
		info->refreshRate / 65536.0,
		(uint32_t)info->maxDepthIndex,
		flagsstr
	);
	if (DarwinMajorVersion() >= 12) { // 10.8
		inc += scnprintf(buf+inc, bufSize-inc,
			" imagesize:%ux%umm",
			info->imageWidth,
			info->imageHeight
		);
		DUMP_RESERVED(0, info->reserved_10_8, "reserved");
	}
	else {
		DUMP_RESERVED(0, info->reserved_10_1, "reserved");
	}
	free(flagsstr);
	return result;
} // DumpOneDisplayModeInformationPtr


// "desc"
static char * DumpOneFBDisplayModeDescriptionPtr(char * buf, size_t bufSize, IOFBDisplayModeDescription * desc, int modeAlias) {
	int inc = 0;
	char * result = buf;
	char temp[1000];
	
	inc += scnprintf(buf+inc, bufSize-inc, "DisplayModeInformation = { %s },",
		DumpOneDisplayModeInformationPtr(temp, sizeof(temp), (IODisplayModeInformation_10_8*)&desc->info)
	);
	inc += scnprintf(buf+inc, bufSize-inc, " TimingInformation = { %s }",
		DumpOneTimingInformationPtr(temp, sizeof(temp), &desc->timingInfo, sizeof(desc->timingInfo.detailedInfo.v2), modeAlias)
	);
	return result;
} // DumpOneFBDisplayModeDescriptionPtr


static IODetailedTimingInformationV2 *detailedTimingsArr = NULL;
static CFIndex detailedTimingsCount = 0;

static bool DumpOneDetailedTimingInformation(CFDataRef IOFBDetailedTiming, int compareNdx, int modeAlias) {
	CFIndex size = CFDataGetLength(IOFBDetailedTiming);
	IODetailedTimingInformationV2 timing;
	bzero(&timing, sizeof(timing));
	memcpy(&timing, CFDataGetBytePtr(IOFBDetailedTiming), MIN(sizeof(timing), CFDataGetLength(IOFBDetailedTiming)));
	bool omitted = false;
	if (compareNdx < 0 ||
		memcmp(&timing, &detailedTimingsArr[compareNdx], sizeof(timing) )
	) {
		char timinginfo[1000];
		if (compareNdx >= 0) {
			iprintf("[%d] = { ", compareNdx);
		}
		DumpOneDetailedTimingInformationPtr(timinginfo, sizeof(timinginfo), &timing, size, modeAlias);
		cprintf("%s", timinginfo);
		if (compareNdx >= 0) {
			cprintf(" };\n");
		}
	}
	else {
		omitted = true;
	}
	return omitted;
} // DumpOneDetailedTimingInformation

static void DumpOneAppleID(CFNumberRef appleTimingIDRef) {
	IOAppleTimingID appleTimingID;
	CFNumberGetValue(appleTimingIDRef, kCFNumberSInt32Type, &appleTimingID);
	cprintf("%d:%s", (int)appleTimingID, GetOneAppleTimingID(appleTimingID));
} // DumpOneAppleID

static void DumpOneFlags(CFNumberRef flagsRef) {
	UInt32 flags;
	CFNumberGetValue(flagsRef, kCFNumberSInt32Type, &flags);
	char * flagsstr = GetOneFlagsStr(flags);
	cprintf("%s", flagsstr);
	free(flagsstr);
} // DumpOneFlags

static void DumpOneDisplayTimingRange(CFDataRef IOFBTimingRange) {
	int inc = 0;
	const int bufSize = 1000;
	char tempresult[bufSize];
	char * buf = tempresult;

	CFIndex size = CFDataGetLength(IOFBTimingRange);
	switch (size) {
		case sizeof(IODisplayTimingRangeV1_12): inc += scnprintf(buf+inc, bufSize-inc, "V1"); break;
		case sizeof(IODisplayTimingRangeV2_12): inc += scnprintf(buf+inc, bufSize-inc, "V2"); break;
		default                            : inc += scnprintf(buf+inc, bufSize-inc, "Unexpected size:%ld", (long)size); break;
	}
	
	if (size >= sizeof(IODisplayTimingRangeV1_12)) {
		IODisplayTimingRangeV1_12 *range = (IODisplayTimingRangeV1_12 *)CFDataGetBytePtr(IOFBTimingRange);
		inc += scnprintf(buf+inc, bufSize-inc,
			" version:%u %u…%uHz %.3f…%.3fkHz %.3f…%.3f±%.3fMHz sync(%s%s%s%s%s%s) levels(%s%s%s%s%s)",
			(uint32_t)range->version,                       // Init to 0

			(uint32_t)range->minFrameRate,                  // Hz
			(uint32_t)range->maxFrameRate,                  // Hz
			(uint32_t)range->minLineRate / 1000.0,          // Hz -> kHz
			(uint32_t)range->maxLineRate / 1000.0,          // Hz -> kHz
			range->minPixelClock / 1000000.0,               // Min dot clock in Hz -> MHz
			range->maxPixelClock / 1000000.0,               // Max dot clock in Hz -> MHz
			range->maxPixelError / 1000000.0,               // Max dot clock error

			(range->supportedSyncFlags & kIORangeSupportsSeparateSyncs ) ?  "seperate," : "",
			(range->supportedSyncFlags & kIORangeSupportsSyncOnGreen   ) ?     "green," : "",
			(range->supportedSyncFlags & kIORangeSupportsCompositeSync ) ? "composite," : "",
			(range->supportedSyncFlags & kIORangeSupportsVSyncSerration) ? "serration," : "",
			(range->supportedSyncFlags & kIORangeSupportsVRR           ) ?       "VRR," : "",
			UNKNOWN_FLAG(range->supportedSyncFlags & 0xffffffe0),

			(range->supportedSignalLevels & kIORangeSupportsSignal_0700_0300) ? "0700_0300," : "",
			(range->supportedSignalLevels & kIORangeSupportsSignal_0714_0286) ? "0714_0286," : "",
			(range->supportedSignalLevels & kIORangeSupportsSignal_1000_0400) ? "1000_0400," : "",
			(range->supportedSignalLevels & kIORangeSupportsSignal_0700_0000) ? "0700_0000," : "",
			UNKNOWN_FLAG(range->supportedSignalLevels & 0xfffffff0)
		);

		if (DarwinMajorVersion() >= 7) { // 10.3
			inc += scnprintf(buf+inc, bufSize-inc,
				" signal(%s%s%s%s)",
				(range->supportedSignalConfigs & kIORangeSupportsInterlacedCEATiming           ) ?            "interlaced CEA," : "",
				(range->supportedSignalConfigs & kIORangeSupportsInterlacedCEATimingWithConfirm) ? "interlaced CEA w/ confirm," : "",
				(range->supportedSignalConfigs & kIORangeSupportsMultiAlignedTiming            ) ?             "multi aligned," : "",
				UNKNOWN_FLAG(range->supportedSignalConfigs & 0xffffffb3)
			 );
		}
		else {
			DUMP_RESERVED(0, range->__reservedC_10_3_0, "reservedC");
		}

		inc += scnprintf(buf+inc, bufSize-inc,
			" (total,active,blank,frontp,syncw,border1,border2)(charsize(h(%u,%u,%u,%u,%u,%u,%u) v(%u,%u,%u,%u,%u,%u,%u)) pixels(h(%u,%u…%u,%u…%u,%u…%u,%u…%u,%u…%u,%u…%u) v(%u,%u…%u,%u…%u,%u…%u,%u…%u,%u…%u,%u…%u)))"
			" links(#:%u 0:%.3f…%.3f 1:%.3f…%.3f MHz)"
			" encodings(%s%s%s%s%s) bpc(%s%s%s%s%s%s) colorimetry(%s%s%s%s%s%s%s%s%s%s%s) dynamicrange(%s%s%s%s%s%s%s)",
			range->charSizeHorizontalTotal,                // Character size for active + blanking
			range->charSizeHorizontalActive,
			range->charSizeHorizontalBlanking,
			range->charSizeHorizontalSyncOffset,
			range->charSizeHorizontalSyncPulse,
			range->charSizeHorizontalBorderLeft,
			range->charSizeHorizontalBorderRight,

			range->charSizeVerticalTotal,                  // Character size for active + blanking
			range->charSizeVerticalActive,
			range->charSizeVerticalBlanking,
			range->charSizeVerticalSyncOffset,
			range->charSizeVerticalSyncPulse,
			range->charSizeVerticalBorderTop,
			range->charSizeVerticalBorderBottom,

			(uint32_t)range->maxHorizontalTotal,             // Clocks - Maximum total (active + blanking)
			(uint32_t)range->minHorizontalActiveClocks,
			(uint32_t)range->maxHorizontalActiveClocks,
			(uint32_t)range->minHorizontalBlankingClocks,
			(uint32_t)range->maxHorizontalBlankingClocks,
			(uint32_t)range->minHorizontalSyncOffsetClocks,
			(uint32_t)range->maxHorizontalSyncOffsetClocks,
			(uint32_t)range->minHorizontalPulseWidthClocks,
			(uint32_t)range->maxHorizontalPulseWidthClocks,
			(uint32_t)range->minHorizontalBorderLeft,
			(uint32_t)range->maxHorizontalBorderLeft,
			(uint32_t)range->minHorizontalBorderRight,
			(uint32_t)range->maxHorizontalBorderRight,

			(uint32_t)range->maxVerticalTotal,               // Clocks - Maximum total (active + blanking)
			(uint32_t)range->minVerticalActiveClocks,
			(uint32_t)range->maxVerticalActiveClocks,
			(uint32_t)range->minVerticalBlankingClocks,
			(uint32_t)range->maxVerticalBlankingClocks,
			(uint32_t)range->minVerticalSyncOffsetClocks,
			(uint32_t)range->maxVerticalSyncOffsetClocks,
			(uint32_t)range->minVerticalPulseWidthClocks,
			(uint32_t)range->maxVerticalPulseWidthClocks,
			(uint32_t)range->minVerticalBorderTop,
			(uint32_t)range->maxVerticalBorderTop,
			(uint32_t)range->minVerticalBorderBottom,
			(uint32_t)range->maxVerticalBorderBottom,

			(uint32_t)range->maxNumLinks,                       // number of links, if zero, assume link 1
			range->minLink0PixelClock / 1000.0,       // min pixel clock for link 0 (kHz)
			range->maxLink0PixelClock / 1000.0,       // max pixel clock for link 0 (kHz)
			range->minLink1PixelClock / 1000.0,       // min pixel clock for link 1 (kHz)
			range->maxLink1PixelClock / 1000.0,       // max pixel clock for link 1 (kHz)

			(range->supportedPixelEncoding & kIORangePixelEncodingRGB444  ) ? "RGB," : "",
			(range->supportedPixelEncoding & kIORangePixelEncodingYCbCr444) ? "444," : "",
			(range->supportedPixelEncoding & kIORangePixelEncodingYCbCr422) ? "422," : "",
			(range->supportedPixelEncoding & kIORangePixelEncodingYCbCr420) ? "420," : "",
			UNKNOWN_FLAG(range->supportedPixelEncoding & 0xfff0),

			(range->supportedBitsPerColorComponent & kIORangeBitsPerColorComponent6 ) ?  "6," : "",
			(range->supportedBitsPerColorComponent & kIORangeBitsPerColorComponent8 ) ?  "8," : "",
			(range->supportedBitsPerColorComponent & kIORangeBitsPerColorComponent10) ? "10," : "",
			(range->supportedBitsPerColorComponent & kIORangeBitsPerColorComponent12) ? "12," : "",
			(range->supportedBitsPerColorComponent & kIORangeBitsPerColorComponent16) ? "16," : "",
			UNKNOWN_FLAG(range->supportedBitsPerColorComponent & 0xffe0),

			(range->supportedColorimetryModes & kIORangeColorimetryNativeRGB) ? "NativeRGB," : "",
			(range->supportedColorimetryModes & kIORangeColorimetrysRGB     ) ?      "sRGB," : "",
			(range->supportedColorimetryModes & kIORangeColorimetryDCIP3    ) ?     "DCIP3," : "",
			(range->supportedColorimetryModes & kIORangeColorimetryAdobeRGB ) ?  "AdobeRGB," : "",
			(range->supportedColorimetryModes & kIORangeColorimetryxvYCC    ) ?     "xvYCC," : "",
			(range->supportedColorimetryModes & kIORangeColorimetryWGRGB    ) ?     "WGRGB," : "",
			(range->supportedColorimetryModes & kIORangeColorimetryBT601    ) ?     "BT601," : "",
			(range->supportedColorimetryModes & kIORangeColorimetryBT709    ) ?     "BT709," : "",
			(range->supportedColorimetryModes & kIORangeColorimetryBT2020   ) ?    "BT2020," : "",
			(range->supportedColorimetryModes & kIORangeColorimetryBT2100   ) ?    "BT2100," : "",
			UNKNOWN_FLAG(range->supportedColorimetryModes & 0xfc00),

			(range->supportedDynamicRangeModes & kIORangeDynamicRangeSDR                ) ?                 "SDR," : "",
			(range->supportedDynamicRangeModes & kIORangeDynamicRangeHDR10              ) ?               "HDR10," : "",
			(range->supportedDynamicRangeModes & kIORangeDynamicRangeDolbyNormalMode    ) ?     "DolbyNormalMode," : "",
			(range->supportedDynamicRangeModes & kIORangeDynamicRangeDolbyTunnelMode    ) ?     "DolbyTunnelMode," : "",
			(range->supportedDynamicRangeModes & kIORangeDynamicRangeTraditionalGammaHDR) ? "TraditionalGammaHDR," : "",
			(range->supportedDynamicRangeModes & kIORangeDynamicRangeTraditionalGammaSDR) ? "TraditionalGammaSDR," : "",
			UNKNOWN_FLAG(range->supportedDynamicRangeModes & 0xffc0)
		);

		DUMP_RESERVED(0, range->__reservedA, "reservedA");
		DUMP_RESERVED(0, range->__reservedB, "reservedB");
		DUMP_RESERVED(0, range->__reservedD, "reservedD");
		DUMP_RESERVED(0, range->__reservedE, "reservedE");

		 if (DarwinMajorVersion() >= 18) { // 10.14
			 DUMP_RESERVED(0, range->__reservedF_10_14, "reservedF");
		 }
		 else if (DarwinMajorVersion() >= 7) { // 10.3
			 DUMP_RESERVED(0, range->__reservedF_10_3_9, "reservedF");
		 }
		 else {
			 DUMP_RESERVED(0, range->__reservedF_10_3_0, "reservedF");
		 }
	}

	if (size >= sizeof(IODisplayTimingRangeV2_12)) {
		IODisplayTimingRangeV2_12 *range = (IODisplayTimingRangeV2_12 *)CFDataGetBytePtr(IOFBTimingRange);
		inc += scnprintf(buf+inc, bufSize-inc, " dsc(%.3fGbps%s slice:%dx%d…%dx%d slice/line:%d…%d %d…%dbpc %d…%dbpp VBR:%s BlockPred:%s)",

			range->maxBandwidth / 1000000000.0,
			((range->maxBandwidth >> 32) == 0xffffffff) ? "?(32-bit sign extension error)" : "",
			
			(uint32_t)range->dscMinSliceWidth,
			(uint32_t)range->dscMinSliceHeight,
			(uint32_t)range->dscMaxSliceWidth,
			(uint32_t)range->dscMaxSliceHeight,
			(uint32_t)range->dscMinSlicePerLine,
			(uint32_t)range->dscMaxSlicePerLine,

			range->dscMinBPC,
			range->dscMaxBPC,
			range->dscMinBPP,
			range->dscMaxBPP,

			range->dscVBR == 0 ? "disabled" :
			range->dscVBR == 1 ? "enabled" :
			UNKNOWN_VALUE(range->dscVBR),

			range->dscBlockPredEnable == 0 ? "unused" :
			range->dscBlockPredEnable == 1 ?
				range->dscVBR == 1 ? "used" : "?used but VBR is disabled"
			:
				"?not zero or one"
		);
		
		DUMP_RESERVED(0, range->__reservedC_10_15, "reservedC");
	}
	
	cprintf("%s", buf);
} // DumpOneDisplayTimingRange

static char * DumpOneDisplayTimingRangeFromEdid(char *buf, size_t bufSize, IODisplayTimingRangeV1_12 *range) {
	int inc = 0;
	inc += scnprintf(buf+inc, bufSize-inc,
		"%u…%uHz %.3f…%.3fkHz %.3fMHz maxActivePixels:%ux%u",

		(uint32_t)range->minFrameRate,                  // Hz
		(uint32_t)range->maxFrameRate,                  // Hz
		(uint32_t)range->minLineRate / 1000.0,          // Hz -> kHz
		(uint32_t)range->maxLineRate / 1000.0,          // Hz -> kHz

		range->maxPixelClock / 1000000.0,               // Max dot clock in Hz -> MHz

		(uint32_t)range->maxHorizontalActiveClocks,
		(uint32_t)range->maxVerticalActiveClocks
	);
	return buf;
} // DumpOneDisplayTimingRangeFromEdid

static void DumpOneDisplayScalerInfo(CFDataRef IOFBScalerInfo) {
	CFIndex size = CFDataGetLength(IOFBScalerInfo);

	if (size != sizeof(IODisplayScalerInformation)) {
		cprintf("Unexpected size:%ld", (long)size);
		if (size >= sizeof(IODisplayScalerInformation)) {
			cprintf(" ");
		}
	}
	
	if (size >= sizeof(IODisplayScalerInformation)) {
		IODisplayScalerInformation *info = (IODisplayScalerInformation *)CFDataGetBytePtr(IOFBScalerInfo);
		cprintf(
			"version:%u maxPixels:%ux%u options(%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s) %x.%x.%x.%x.%x.%x.%x.%x",

			(unsigned int)info->version,                       // Init to 0
			(unsigned int)info->maxHorizontalPixels,
			(unsigned int)info->maxVerticalPixels,

			(info->scalerFeatures & kIOScaleStretchOnly        ) ?      "StretchOnly," : "", // 0x00000001
			(info->scalerFeatures & kIOScaleCanUpSamplePixels  ) ?   "UpSamplePixels," : "", // 0x00000002
			(info->scalerFeatures & kIOScaleCanDownSamplePixels) ? "DownSamplePixels," : "", // 0x00000004
			(info->scalerFeatures & kIOScaleCanScaleInterlaced ) ?  "ScaleInterlaced," : "", // 0x00000008
			(info->scalerFeatures & kIOScaleCanSupportInset    ) ?     "SupportInset," : "", // 0x00000010
			(info->scalerFeatures & kIOScaleCanRotate          ) ?           "Rotate," : "", // 0x00000020
			(info->scalerFeatures & kIOScaleCanBorderInsetOnly ) ?  "BorderInsetOnly," : "", // 0x00000040
			(info->scalerFeatures & 0x00000080                 ) ?               "7?," : "",
			(info->scalerFeatures & 0x00000100                 ) ?               "8?," : "",
			(info->scalerFeatures & 0x00000200                 ) ?               "9?," : "",
			(info->scalerFeatures & 0x00000400                 ) ?              "10?," : "",
			(info->scalerFeatures & 0x00000800                 ) ?              "11?," : "",
			(info->scalerFeatures & 0x00001000                 ) ?              "12?," : "",
			(info->scalerFeatures & 0x00002000                 ) ?              "13?," : "",
			(info->scalerFeatures & 0x00004000                 ) ?              "14?," : "",
			(info->scalerFeatures & 0x00008000                 ) ?              "15?," : "",
			(info->scalerFeatures & 0x00010000                 ) ?              "16?," : "",
			(info->scalerFeatures & 0x00020000                 ) ?              "17?," : "",
			(info->scalerFeatures & 0x00040000                 ) ?              "18?," : "",
			(info->scalerFeatures & 0x00080000                 ) ?              "19?," : "",
			(info->scalerFeatures & 0x00100000                 ) ?              "20?," : "",
			(info->scalerFeatures & 0x00200000                 ) ?              "21?," : "",
			(info->scalerFeatures & 0x00400000                 ) ?              "22?," : "",
			(info->scalerFeatures & 0x00800000                 ) ?              "23?," : "",
			(info->scalerFeatures & 0x01000000                 ) ?              "24?," : "",
			(info->scalerFeatures & 0x02000000                 ) ?              "25?," : "",
			(info->scalerFeatures & 0x04000000                 ) ?              "26?," : "",
			(info->scalerFeatures & 0x08000000                 ) ?              "27?," : "",
			(info->scalerFeatures & 0x10000000                 ) ?              "28?," : "",
			(info->scalerFeatures & 0x20000000                 ) ?              "29?," : "",
			(info->scalerFeatures & 0x40000000                 ) ?              "30?," : "",
			(info->scalerFeatures & 0x80000000                 ) ?              "31?," : "",

			(unsigned int)info->__reservedA[0], // Init to 0

			(unsigned int)info->__reservedB[0], // Init to 0
			(unsigned int)info->__reservedB[1], // Init to 0

			(unsigned int)info->__reservedC[0], // Init to 0
			(unsigned int)info->__reservedC[1], // Init to 0
			(unsigned int)info->__reservedC[2], // Init to 0
			(unsigned int)info->__reservedC[3], // Init to 0
			(unsigned int)info->__reservedC[4]  // Init to 0
		);
	}

} // DumpOneDisplayScalerInfo

static void DumpOneTransform(SInt32 numValue) {
	cprintf("%s%s%s%s%s%s%s%s%s%s",
		(numValue & 7) == kIOFBSwapAxes  ?    "swap," : "", // 1
		(numValue & 7) == kIOFBInvertX   ? "invertx," : "", // 2
		(numValue & 7) == kIOFBInvertY   ? "inverty," : "", // 4
		(numValue & 7) == kIOFBRotate0   ?      "0°," : "", // 0
		(numValue & 7) == kIOFBRotate90  ?     "90°," : "", // 3
		(numValue & 7) == kIOFBRotate180 ?    "180°," : "", // 6
		(numValue & 7) == kIOFBRotate270 ?    "270°," : "", // 5
		(numValue & 7) == 7              ? "swap,invertx,inverty," : "",
		(numValue & 8                  ) ?      "8?," : "",
		UNKNOWN_FLAG(numValue & 0xfffffff0)
	);
} // DumpOneTransform

static void DumpOneDisplayModeInformation(CFDataRef displayModeInformation) {
	CFIndex size = CFDataGetLength(displayModeInformation);

	if (size != sizeof(IODisplayModeInformation)) {
		cprintf("Unexpected size:%ld", (long)size);
		if (size >= sizeof(IODisplayModeInformation)) {
			cprintf(" ");
		}
	}

	if (size < sizeof(IODisplayModeInformation)) {
		return;
	}
	IODisplayModeInformation_10_8 *info = (IODisplayModeInformation_10_8 *)CFDataGetBytePtr(displayModeInformation);
	char temp[1000];
	cprintf(
		"%s", DumpOneDisplayModeInformationPtr(temp, sizeof(temp), info)
	);
} // DumpOneDisplayModeInformation

static void DumpOneDisplayModeDescription(CGSDisplayModeDescription *mode, int modeAlias) {
	char *flagsstr1 = GetOneFlagsStr(mode->IOFlags);
	char *flagsstr2 = GetOneFlagsStr(mode->flags);

	char hexDigits[] = "0123456789abcdef";
	hexDigits[modeAlias] = '.';

	cprintf("%d: id:0x%04x%c%03x %dx%d@%.3fHz %dHz",
		mode->mode,
		(unsigned int)((UInt32)mode->DisplayModeID >> 16),
		hexDigits[(mode->DisplayModeID >> 12) & 15],
		(unsigned int)(mode->DisplayModeID & 0x0fff),
		mode->width, mode->height, mode->refreshRate / 65536.0, mode->intRefreshRate
	);

	if (mode->size >= 212) {
		cprintf(" (dens=%.1f)", mode->resolution);
	}
	else {
		cprintf(" (dens=ø)");
	}

	if (mode->size >= 208) {
		cprintf(" pixels:%dx%d", mode->PixelsWide, mode->PixelsHigh);
	}
	else {
		cprintf(" pixels:øxø");
	}

	cprintf(" resolution:%dx%d %dbpp %dbpc %dcpp rowbytes:%d IOFlags:(%s) flags:(%s) depthFormat:%d encoding:%s refreshRate.unk0.unk1:%08x.%04x.%08x%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		mode->horizontalResolution, mode->verticalResolution,
		mode->bitsPerPixel, mode->bitsPerSample, mode->samplesPerPixel, mode->bytesPerRow,
		flagsstr1, flagsstr2, mode->depthFormat, mode->pixelEncoding,
		mode->refreshRate, mode->unknown0, mode->unknown1,
		mode->unknown2[0] ? " unknown20:" : "",
		mode->unknown2[0] ? UNKNOWN_VALUE(mode->unknown2[0]) : "",
		mode->unknown2[1] ? " unknown21:" : "",
		mode->unknown2[1] ? UNKNOWN_VALUE(mode->unknown2[1]) : "",
		mode->unknown2[2] ? " unknown22:" : "",
		mode->unknown2[2] ? UNKNOWN_VALUE(mode->unknown2[2]) : "",
		mode->unknown2[3] ? " unknown23:" : "",
		mode->unknown2[3] ? UNKNOWN_VALUE(mode->unknown2[3]) : "",
		mode->unknown2[4] ? " unknown24:" : "",
		mode->unknown2[4] ? UNKNOWN_VALUE(mode->unknown2[4]) : "",
		mode->unknown2[5] ? " unknown25:" : "",
		mode->unknown2[5] ? UNKNOWN_VALUE(mode->unknown2[5]) : "",
		mode->unknown2[6] ? " unknown26:" : "",
		mode->unknown2[6] ? UNKNOWN_VALUE(mode->unknown2[6]) : "",
		mode->unknown2[7] ? " unknown27:" : "",
		mode->unknown2[7] ? UNKNOWN_VALUE(mode->unknown2[7]) : ""
	);

	free(flagsstr1);
	free(flagsstr2);
	
	if (mode->size != 212 && mode->size != 200) {
		cprintf(" (?%d bytes)", mode->size);
	}

	if (mode->size > 212) {
		cprintf(" extra:");
		for (int i = 212; i < mode->size; i++) {
			cprintf("%02X", mode->bytes[i]);
		}
	}
} // DumpOneDisplayModeDescription

#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_6
static CFDictionaryRef CGDisplayModeToDict(CGDisplayModeRef mode) {
	typedef struct CGDisplayMode {
#ifdef __LP64__
		UInt8 bytes[16];
#else
		UInt8 bytes[8];
#endif
		CFDictionaryRef dict;
	} CGDisplayMode;


	CGDisplayMode *themode = (CGDisplayMode *)mode;
	CFDictionaryRef dict = NULL;
	if (mode) {
		dict = themode->dict;
	}
	return dict;
}
#endif

#define onefloat(_field, _format) \
	float val_ ## _field = 0.0f; \
	char str_ ## _field[20]; \
	CFTypeRef cf_ ## _field = CFDictionaryGetValue(dict, CFSTR(#_field)); \
	if (cf_ ## _field) { \
		if (CFGetTypeID(cf_ ## _field) == CFNumberGetTypeID()) { \
			if (copy) CFDictionaryRemoveValue(copy, CFSTR(#_field)); \
			CFNumberGetValue((CFNumberRef)cf_ ## _field, kCFNumberFloat32Type, &val_ ## _field); \
			snprintf(str_ ## _field, sizeof(str_ ## _field), _format, val_ ## _field); \
		} \
		else \
			snprintf(str_ ## _field, sizeof(str_ ## _field), "?"); \
	} \
	else \
		snprintf(str_ ## _field, sizeof(str_ ## _field), "ø");

#define onenum(_size, _field, _format) \
	UInt ## _size val_ ## _field = 0; \
	char str_ ## _field[20]; \
	CFTypeRef cf_ ## _field = CFDictionaryGetValue(dict, CFSTR(#_field)); \
	if (cf_ ## _field) { \
		if (CFGetTypeID(cf_ ## _field) == CFNumberGetTypeID() || CFGetTypeID(cf_ ## _field) == CFBooleanGetTypeID()) { \
			if (copy) CFDictionaryRemoveValue(copy, CFSTR(#_field)); \
			if (CFNumberGetValue((CFNumberRef)cf_ ## _field, kCFNumberSInt ## _size ## Type, &val_ ## _field)) \
				snprintf(str_ ## _field, sizeof(str_ ## _field), _format, (uint## _size ## _t)val_ ## _field); \
			else \
				snprintf(str_ ## _field, sizeof(str_ ## _field), "¿"); \
		} \
		else \
			snprintf(str_ ## _field, sizeof(str_ ## _field), "?"); \
	} \
	else \
		snprintf(str_ ## _field, sizeof(str_ ## _field), "ø");

#define onestr(_field) \
	char str_ ## _field[100] = {0}; \
	CFTypeRef cf_ ## _field = CFDictionaryGetValue(dict, CFSTR(#_field)); \
	if (cf_ ## _field) { \
		if (CFGetTypeID(cf_ ## _field) == CFStringGetTypeID()) { \
			if (copy) CFDictionaryRemoveValue(copy, CFSTR(#_field)); \
			CFStringGetCString((CFStringRef)cf_ ## _field, str_ ## _field, sizeof(str_ ## _field), kCFStringEncodingUTF8); \
		} \
		else \
			snprintf(str_ ## _field, sizeof(str_ ## _field), "?"); \
	} \
	else \
		snprintf(str_ ## _field, sizeof(str_ ## _field), "ø");

#define oneflag(_field,_what) (strcmp(str_ ## _field, "1") ? strcmp(str_ ## _field, "0") ? str_ ## _field : "" : _what), (strcmp(str_ ## _field, "1") ? strcmp(str_ ## _field, "0") ? "," : "" : ",")

static void DumpOneCGDisplayMode(CGDisplayModeRef mode, int modeAlias) {
	
	if (!mode) {
		cprintf("NULL");
		return;
	}

	CFDictionaryRef dict = (CFDictionaryRef)mode;

	static CFTypeID dictType = CFDictionaryGetTypeID();

#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_6
	API_OR_SDK_AVAILABLE_BEGIN(10.6, CGDisplayModeGetTypeID) {
		static CFTypeID displayModeType = CGDisplayModeGetTypeID();

		if (CFGetTypeID(mode) == displayModeType) {
			dict = CGDisplayModeToDict(mode);
	#if 0
			//CFStringRef theinfo = CFCopyDescription(mode); // can't really parse this if the format can change for every macOS
			
			CFStringRef encoding = CGDisplayModeCopyPixelEncoding(mode);
			char str_encoding[100];
			if (encoding) {
				if (!CFStringGetCString(encoding, str_encoding, sizeof(str_encoding), kCFStringEncodingUTF8)) {
					snprintf(str_encoding, sizeof(str_encoding), "¿");
				}
				CFRelease(encoding);
			}
			else
				snprintf(str_encoding, sizeof(str_encoding), "ø");
			
			char hexDigits[] = "0123456789abcdef";
			hexDigits[modeAlias] = '.';
			int DisplayModeID = CGDisplayModeGetIODisplayModeID(mode);
					
			cprintf("id:0x%04x%c%03x %ldx%ld@%.3fHz pixels:%ldx%ld IOFlags:%x flags:(%s) encoding:%s\n",
				DisplayModeID >> 16,
				hexDigits[(DisplayModeID >> 12) & 15],
				DisplayModeID & 0x0fff,
				CGDisplayModeGetWidth(mode), CGDisplayModeGetHeight(mode), CGDisplayModeGetRefreshRate(mode), CGDisplayModeGetPixelWidth(mode), CGDisplayModeGetPixelHeight(mode),
				CGDisplayModeGetIOFlags(mode),
				CGDisplayModeIsUsableForDesktopGUI(mode) ? "gui usable," : "",
				str_encoding
			);

			//CFRelease(theinfo);
			return;
	#endif
		}
	} API_OR_SDK_AVAILABLE_END
#endif
	
	static CFTypeID arrType = CFArrayGetTypeID();
	if (CFGetTypeID(dict) == arrType) {
		CFArrayRef arr = (CFArrayRef)dict;
		if (CFArrayGetCount(arr) != 1) {
			cprintf("not a single display mode\n");
			return;
		}
		dict = (CFDictionaryRef)CFArrayGetValueAtIndex(arr, 0);
	}

	if (CFGetTypeID(dict) != dictType) {
		cprintf("not a CGDisplayModeRef\n");
		return;
	}

	CFMutableDictionaryRef copy = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, dict);

	onenum(32, BitsPerPixel, "%u")
	onenum(32, BitsPerSample, "%u")
	onenum(32, Height, "%u")
	onenum(32, IODisplayModeID, "0x%08x")
	onenum(32, IOFlags, "%x")
	onenum(32, Mode, "%u")
	onefloat(RefreshRate, "%.3f")
	onenum(32, SamplesPerPixel, "%u")
	onenum(32, Width, "%u")
	onenum(32, kCGDisplayBytesPerRow, "%u")
	onenum(32, kCGDisplayHorizontalResolution, "%u")
	onenum(32, kCGDisplayPixelsHigh, "%u")
	onenum(32, kCGDisplayPixelsWide, "%u")
	onefloat(kCGDisplayResolution, "%.1f")
	onenum(32, kCGDisplayVerticalResolution, "%u")
	onenum(32, UsableForDesktopGUI, "%u")
	onenum(32, kCGDisplayModeIsSafeForHardware, "%u")
	onenum(32, kCGDisplayModeIsTelevisionOutput, "%u")

	onenum(32, kCGDisplayModeIsInterlaced, "%u")
	onenum(32, kCGDisplayModeIsStretched, "%u")
	onenum(32, kCGDisplayModeIsUnavailable, "%u")
	onenum(32, kCGDisplayModeSuitableForUI, "%u")
	onenum(32, DepthFormat, "%u")
	onestr(PixelEncoding)

	
	char hexDigits[] = "0123456789abcdef";
	hexDigits[modeAlias] = '.';
	
	if (str_IODisplayModeID[0] == '0') {
		snprintf(str_IODisplayModeID, sizeof(str_IODisplayModeID), "0x%04x%c%03x",
			(uint32_t)(val_IODisplayModeID >> 16),
			hexDigits[(val_IODisplayModeID >> 12) & 15],
			(uint32_t)(val_IODisplayModeID & 0x0fff)
		);
	}

	char * flagsstr = GetOneFlagsStr(val_IOFlags);
	char flagsstr2[500];
	snprintf(flagsstr2, sizeof(flagsstr2), "%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		oneflag(UsableForDesktopGUI             , "gui usable" ),
		oneflag(kCGDisplayModeIsSafeForHardware , "hw safe"    ),
		oneflag(kCGDisplayModeIsTelevisionOutput, "tv out"     ),

		oneflag(kCGDisplayModeIsInterlaced      , "interlaced" ),
		oneflag(kCGDisplayModeIsStretched       , "stretched"  ),
		oneflag(kCGDisplayModeIsUnavailable     , "unavailable"),
		oneflag(kCGDisplayModeSuitableForUI     , "ui suitable")
	);
	RemoveTrailingComma(flagsstr2);

	cprintf("%s: id:%s %sx%s@%sHz %.0fHz (dens=%s) pixels:%sx%s resolution:%sx%s %sbpp %sbpc %scpp rowbytes:%s IOFlags:(%s) flags:(%s) depthFormat:%s encoding:%s",
		str_Mode, str_IODisplayModeID,
		str_Width, str_Height, str_RefreshRate, val_RefreshRate, str_kCGDisplayResolution, str_kCGDisplayPixelsWide, str_kCGDisplayPixelsHigh, str_kCGDisplayHorizontalResolution, str_kCGDisplayVerticalResolution,
		str_BitsPerPixel, str_BitsPerSample, str_SamplesPerPixel, str_kCGDisplayBytesPerRow,
		flagsstr,
		flagsstr2,
		str_DepthFormat, str_PixelEncoding
	);
	free(flagsstr);
	if (CFDictionaryGetCount(copy) > 0) {
		lf;
		iprintf("unknown fields = ");
		CFOutput(copy);
	}
	CFRelease(copy);
} // DumpOneCGDisplayMode

static void printBooleanKeys(const void *key, const void *value, void *context) {
	if (CFGetTypeID(value) == CFBooleanGetTypeID()) {
		char keyStr[256];
		CFStringGetCString((CFStringRef)key, keyStr, sizeof(keyStr), CFStringGetSystemEncoding());

		SInt32 numValue;
		CFNumberGetValue((CFNumberRef)value, kCFNumberSInt32Type, &numValue);
		iprintf("%s = %s;\n", keyStr,
			numValue == 1 ?  "true" :
			numValue == 0 ? "false" :
			UNKNOWN_VALUE(numValue)
		);
		CFDictionaryRemoveValue((CFMutableDictionaryRef)context, key);
	}
} // printBooleanKeys

static void DoAllBooleans(CFMutableDictionaryRef dict) {
	CFDictionaryRef loopdict = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, dict);
	CFDictionaryApplyFunction(loopdict, printBooleanKeys, dict);
	CFRelease(loopdict);
}


static void printEDIDKeys(const void *key, const void *value, void *context) {
	if (CFGetTypeID(value) == CFDataGetTypeID()) {
		char keyStr[256];
		CFStringGetCString((CFStringRef)key, keyStr, sizeof(keyStr), CFStringGetSystemEncoding());
		if (!strncmp(keyStr, "IOFBEDID", 8)) {
			CFDataRef data = (CFDataRef)value;
			iprintf("%s = ", keyStr);
			const UInt8 *p = CFDataGetBytePtr(data);
			for (int i = 0; i < CFDataGetLength(data); i++) {
				cprintf("%02x", p[i]);
			}
			lf;
			CFDictionaryRemoveValue((CFMutableDictionaryRef)context, key);
		}
	}
} // printEDIDKeys

static void DoAllEDIDs(CFMutableDictionaryRef dict) {
	CFDictionaryRef loopdict = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, dict);
	CFDictionaryApplyFunction(dict, printEDIDKeys, dict);
	CFRelease(loopdict);
} // DoAllEDIDs


static void DoOneDisplayPort(io_service_t ioFramebufferService, IOI2CConnectRef i2cconnect, UInt8 *inpath, int pathLength, bool shorttest) {
	// If pathLength is 0, then do normal DisplayPort (not mst sideband messages - there is no port or RAD).
	// If pathLength is 1, The first number in the path is the port number. The RAD is empty.
	// If pathLength is 2 or more, then the RAD lists 1 or more ports.
	
	IOReturn result = kIOReturnSuccess;
	UInt8 path[16];
	memcpy(path, inpath, pathLength);
	
	UInt8 *dpcd = (UInt8 *)malloc(0x100000);
	if (!dpcd) return;
	bzero(dpcd, 0x100000);
	DpError dperr = dpNoError;
	char resultStr[40];
	bool isSynaptics = false;
	int dpcdAddr;
	const int dpcdIncrement = 16;
	CFMutableStringRef synapticsLog = NULL;

	int dpcdRangeNdx;
	for (dpcdRangeNdx = 0; (dpcdAddr = dpcdranges[dpcdRangeNdx]) >= 0; dpcdRangeNdx += 2) {
		bool hasError = false;

		for ( ; dpcdAddr < dpcdranges[dpcdRangeNdx + 1]; dpcdAddr += dpcdIncrement) {

			isSynaptics = (dpcdAddr == 0x5e0 && dpcd[DP_BRANCH_OUI+0] == 0x90 && dpcd[DP_BRANCH_OUI+1] == 0xcc && dpcd[DP_BRANCH_OUI+2] == 0x24);

			do {
				int maxattempts = 1;
				int attempt;
				for (attempt = 0; attempt < maxattempts; attempt++) {
					gDumpSidebandMessage = (dpcdAddr == 0) * (kReq | kRep);
					result = mst_req_dpcd_read(ioFramebufferService, i2cconnect, path, pathLength, dpcdAddr, dpcdIncrement, &dpcd[dpcdAddr], &dperr);
					gDumpSidebandMessage = 0;

					if (result || dperr) {
						iprintf("(%05xh:%s%s)\n", dpcdAddr, DpErrorStr(dperr), DumpOneReturn(resultStr, sizeof(resultStr), result));
					}
					else if (attempt > 0) {
						iprintf("(%05xh: success)\n", dpcdAddr);
					}
					if (!result) {
						break;
					}
					if (dperr == dpErrSequenceMismatch) {
						usleep(4 * 1000000); // wait four seconds to timeout the messages before retrying
						maxattempts = 3;
						continue;
					}
					if (dperr == dpErrCrc) {
						maxattempts = 3;
						continue;
					}
					if (dperr == dpErrNak) {
						dpcdAddr = 0; // pretend that we NAK'ed on the first address which cause all other addresses to be skipped
						break;
					}
				} // for attempt

				hasError = false;
				if (result) {
					bzero(&dpcd[dpcdAddr], dpcdIncrement);
					if (attempt > 1) {
						//iprintf("(%05xh:%s%s after %d attempts)\n", dpcdAddr, DpErrorStr(dperr), DumpOneReturn(resultStr, sizeof(resultStr), result), attempt);
					}
					hasError = true;
					break;
				}
				else if (isSynaptics && dpcd[dpcdAddr] > 0 && dpcd[dpcdAddr] < 16) {
					if (!synapticsLog) {
						synapticsLog = CFStringCreateMutable(kCFAllocatorDefault, 0);
					}
					CFStringAppendPascalString(synapticsLog, (ConstStr255Param)&dpcd[dpcdAddr], kCFStringEncodingUTF8);
				}
				else
					break;
			} while (1);

			if (hasError && dpcdAddr == 0) {
				break; // if we can't read dpcd 00000h then it's probably not a DisplayPort device
			}
		} // for dpcdAddr
		if (hasError && dpcdAddr == 0) {
			break;
		}
	} // for dpcdRangeNdx
	if (dpcdRangeNdx > 0) {

		if (synapticsLog) {
			iprintf("Synaptics Log = {\n"); INDENT
			iprintf("");

			size_t maxsize = CFStringGetMaximumSizeForEncoding(CFStringGetLength(synapticsLog), kCFStringEncodingUTF8);
			char *strinfo = (char *)malloc(maxsize);
			if (strinfo) {
				if (CFStringGetCString(synapticsLog, strinfo, maxsize, kCFStringEncodingUTF8)) {
					char *s = strinfo;
					char *c;
					for (c = s; ; c++) {
						if (*c == 0x00 || *c == 0x0d) {
							cprintf("%.*s\n", (int)(c - s), s);
							if (*c == 0x00 || (c[1] == 0x0a && c[2] == 0x00)) break;
							iprintf("");
							c++;
							if (*c == 0x0a) c++;
							s = c;
						}
					}
				}
				else {
					iprintf("Error: Could not convert Synaptics Log.\n");
				}
				free(strinfo);
			}
			else {
				iprintf("Error: Could not allocate Synaptics Log.\n");
			}
			
			OUTDENT; iprintf("} // Synaptics Log\n");
		}

		parsedpcd(dpcd);
		
		int numPorts = dpcd[DP_DOWN_STREAM_PORT_COUNT] & DP_PORT_COUNT_MASK;
		if (numPorts) {
			Sideband_MSG_Body *link_address_body;
			int link_address_body_Length;
			
			if (pathLength > 0) {
				path[pathLength] = path[0];
			}
			path[0] = 0;
			pathLength++;
			
			gDumpSidebandMessage = kReq | kRep;
			bool diddump = gDumpSidebandMessage & kRep;
			IOReturn link_address_result = mst_req_link_address(ioFramebufferService, i2cconnect, path, pathLength, &link_address_body, &link_address_body_Length, &dperr);
			gDumpSidebandMessage = 0;
			if (link_address_result) {
				iprintf("LINK_ADDRESS%s%s\n", DpErrorStr(dperr), DumpOneReturn(resultStr, sizeof(resultStr), link_address_result));
			}
			else {
				if (!diddump) {
					DumpOneDisplayPortMessageBody(link_address_body, link_address_body_Length, true);
					cprintf("\n");
				}
			}

			if (!link_address_result) {
				int portIndex;
				Link_Address_Port *port;

				
				for (
					portIndex = 0, port = link_address_body->reply.ACK.Link_Address.Ports;
					portIndex < link_address_body->reply.ACK.Link_Address.Number_Of_Ports;
					portIndex++, port = port->Input_Port ? (Link_Address_Port *)&port->Input.end : (Link_Address_Port *)&port->Output.end
				) {
				} // for port

				
				for (
					portIndex = 0, port = link_address_body->reply.ACK.Link_Address.Ports;
					portIndex < link_address_body->reply.ACK.Link_Address.Number_Of_Ports;
					portIndex++, port = port->Input_Port ? (Link_Address_Port *)&port->Input.end : (Link_Address_Port *)&port->Output.end
				) {
					if (!port->Input_Port) {
						path[0] = port->Port_Number;
						iprintf("Port %d = {\n", port->Port_Number); INDENT

						{
							Sideband_MSG_Body *enum_path_resources_body;
							int enum_path_resources_body_Length;
							path[0] = port->Port_Number;

							gDumpSidebandMessage = kReq | kRep;
							bool diddump = gDumpSidebandMessage & kRep;
							IOReturn enum_path_resoruces_result = mst_req_enum_path_resources(ioFramebufferService, i2cconnect, path, pathLength, &enum_path_resources_body, &enum_path_resources_body_Length, &dperr);
							gDumpSidebandMessage = 0;
							if (enum_path_resoruces_result) {
								iprintf("ENUM_PATH_RESOURCES%s%s\n", DpErrorStr(dperr), DumpOneReturn(resultStr, sizeof(resultStr), enum_path_resoruces_result));
							}
							else {
								if (!diddump) {
									DumpOneDisplayPortMessageBody(enum_path_resources_body, enum_path_resources_body_Length, true);
									cprintf("\n");
								}
							}
						}

						if (port->Peer_Device_Type) {
							DoOneDisplayPort(ioFramebufferService, i2cconnect, path, pathLength, shorttest);
						}
						OUTDENT iprintf("}; // Port %d\n", port->Port_Number);
					}
				} // for port
				
				
				free(link_address_body);
			} // if !link_address_result
		} // if numPorts
	} // if dpcdRangeNdx

	free(dpcd);
} // DoOneDisplayPort

const char *GetParameterName(const char *key, bool &isFixedPoint)
{
	const char *name;
	if (0) { }
	else if(!strcmp(key, kIODisplayBrightnessProbeKey        )) { name = "kIODisplayBrightnessProbeKey"; }
	else if(!strcmp(key, kIODisplayLinearBrightnessProbeKey  )) { name = "kIODisplayLinearBrightnessProbeKey"; }
	else if(!strcmp(key, kIODisplayBrightnessKey             )) { name = "kIODisplayBrightnessKey"; }
	else if(!strcmp(key, kIODisplayLinearBrightnessKey       )) { name = "kIODisplayLinearBrightnessKey"; }
	else if(!strcmp(key, kIODisplayUsableLinearBrightnessKey )) { name = "kIODisplayUsableLinearBrightnessKey"; }
	else if(!strcmp(key, kIODisplayBrightnessFadeKey         )) { name = "kIODisplayBrightnessFadeKey"; }
	else if(!strcmp(key, kIODisplayContrastKey               )) { name = "kIODisplayContrastKey"; }
	else if(!strcmp(key, kIODisplayHorizontalPositionKey     )) { name = "kIODisplayHorizontalPositionKey"; }
	else if(!strcmp(key, kIODisplayHorizontalSizeKey         )) { name = "kIODisplayHorizontalSizeKey"; }
	else if(!strcmp(key, kIODisplayVerticalPositionKey       )) { name = "kIODisplayVerticalPositionKey"; }
	else if(!strcmp(key, kIODisplayVerticalSizeKey           )) { name = "kIODisplayVerticalSizeKey"; }
	else if(!strcmp(key, kIODisplayTrapezoidKey              )) { name = "kIODisplayTrapezoidKey"; }
	else if(!strcmp(key, kIODisplayPincushionKey             )) { name = "kIODisplayPincushionKey"; }
	else if(!strcmp(key, kIODisplayParallelogramKey          )) { name = "kIODisplayParallelogramKey"; }
	else if(!strcmp(key, kIODisplayRotationKey               )) { name = "kIODisplayRotationKey"; }
	else if(!strcmp(key, kIODisplayTheatreModeKey            )) { name = "kIODisplayTheatreModeKey"; }
	else if(!strcmp(key, kIODisplayTheatreModeWindowKey      )) { name = "kIODisplayTheatreModeWindowKey"; }
	else if(!strcmp(key, kIODisplayOverscanKey               )) { name = "kIODisplayOverscanKey"; }
	else if(!strcmp(key, kIODisplayVideoBestKey              )) { name = "kIODisplayVideoBestKey"; }

	else if(!strcmp(key, kIODisplaySpeakerVolumeKey          )) { name = "kIODisplaySpeakerVolumeKey"; }
	else if(!strcmp(key, kIODisplaySpeakerSelectKey          )) { name = "kIODisplaySpeakerSelectKey"; }
	else if(!strcmp(key, kIODisplayMicrophoneVolumeKey       )) { name = "kIODisplayMicrophoneVolumeKey"; }
	else if(!strcmp(key, kIODisplayAmbientLightSensorKey     )) { name = "kIODisplayAmbientLightSensorKey"; }
	else if(!strcmp(key, kIODisplayAudioMuteAndScreenBlankKey)) { name = "kIODisplayAudioMuteAndScreenBlankKey"; }
	else if(!strcmp(key, kIODisplayAudioTrebleKey            )) { name = "kIODisplayAudioTrebleKey"; }
	else if(!strcmp(key, kIODisplayAudioBassKey              )) { name = "kIODisplayAudioBassKey"; }
	else if(!strcmp(key, kIODisplayAudioBalanceLRKey         )) { name = "kIODisplayAudioBalanceLRKey"; }
	else if(!strcmp(key, kIODisplayAudioProcessorModeKey     )) { name = "kIODisplayAudioProcessorModeKey"; }
	else if(!strcmp(key, kIODisplayPowerModeKey              )) { name = "kIODisplayPowerModeKey"; }
	else if(!strcmp(key, kIODisplayManufacturerSpecificKey   )) { name = "kIODisplayManufacturerSpecificKey"; }

	else if(!strcmp(key, kIODisplayPowerStateKey             )) { name = "kIODisplayPowerStateKey"; }

	else if(!strcmp(key, kIODisplayControllerIDKey           )) { name = "kIODisplayControllerIDKey"; }
	else if(!strcmp(key, kIODisplayCapabilityStringKey       )) { name = "kIODisplayCapabilityStringKey"; }

	else if(!strcmp(key, kIODisplayRedGammaScaleKey          )) { name = "kIODisplayRedGammaScaleKey"; isFixedPoint = true; }
	else if(!strcmp(key, kIODisplayGreenGammaScaleKey        )) { name = "kIODisplayGreenGammaScaleKey"; isFixedPoint = true; }
	else if(!strcmp(key, kIODisplayBlueGammaScaleKey         )) { name = "kIODisplayBlueGammaScaleKey"; isFixedPoint = true; }

	else if(!strcmp(key, kIODisplayGammaScaleKey             )) { name = "kIODisplayGammaScaleKey"; isFixedPoint = true; }

	else if(!strcmp(key, kIODisplayParametersCommitKey       )) { name = "kIODisplayParametersCommitKey"; }
	else if(!strcmp(key, kIODisplayParametersDefaultKey      )) { name = "kIODisplayParametersDefaultKey"; }
	else if(!strcmp(key, kIODisplayParametersFlushKey        )) { name = "kIODisplayParametersFlushKey"; }

	else if(!strcmp(key, "vblm"                              )) { name = "kConnectionVBLMultiplier"; isFixedPoint = true; }
	else if(!strcmp(key, "pscn"                              )) { name = "kConnectionUnderscan"; isFixedPoint = true; }

	else name = key;
	return name;
}


CFMutableDictionaryRef DumpDisplayParameters(const char *parametersName, CFDictionaryRef dictDisplayParameters0, bool isFloat = false) {
	iprintf("%s = {\n", parametersName); INDENT

	CFMutableDictionaryRef dictDisplayParameters = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, dictDisplayParameters0);
	if (dictDisplayParameters) {
		CFIndex itemCount = CFDictionaryGetCount(dictDisplayParameters);

		const void **keys   = (const void **)malloc(itemCount * sizeof(void*));
		const void **values = (const void **)malloc(itemCount * sizeof(void*));
		CFDictionaryGetKeysAndValues(dictDisplayParameters, keys, values);

		for (int i = 0; i < itemCount; i++) {
			char key[50];
			if (CFStringGetCString((CFStringRef)keys[i], key , sizeof(key), kCFStringEncodingUTF8)) {
				if (CFGetTypeID(values[i]) == CFDictionaryGetTypeID()) {
					CFMutableDictionaryRef parameterDict = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, (CFDictionaryRef)values[i]);
					if (parameterDict) {
						CFNumberRef min = (CFNumberRef)CFDictionaryGetValue(parameterDict, CFSTR(kIODisplayMinValueKey));
						CFNumberRef max = (CFNumberRef)CFDictionaryGetValue(parameterDict, CFSTR(kIODisplayMaxValueKey));
						CFNumberRef val = (CFNumberRef)CFDictionaryGetValue(parameterDict, CFSTR(kIODisplayValueKey   ));
						if (min && max && val && CFGetTypeID(min) == CFNumberGetTypeID() && CFGetTypeID(max) == CFNumberGetTypeID() && CFGetTypeID(val) == CFNumberGetTypeID()) {
							SInt32 val_min = 0;
							SInt32 val_max = 0;
							SInt32 val_val = 0;
							CFNumberGetValue(min, kCFNumberSInt32Type, &val_min);
							CFNumberGetValue(max, kCFNumberSInt32Type, &val_max);
							CFNumberGetValue(val, kCFNumberSInt32Type, &val_val);
							
							bool isFixedPoint = 0;
							const char *name = GetParameterName(key, isFixedPoint);
							
							if (isFixedPoint) {
								iprintf("%s = %.5f (%g…%g);\n", name, val_val / 65536.0, val_min / 65536.0, val_max / 65536.0);
							}
							else {
								iprintf("%s = %d (%d…%d);\n", name, (int)val_val, (int)val_min, (int)val_max);
							}

							CFDictionaryRemoveValue(parameterDict, CFSTR(kIODisplayMinValueKey));
							CFDictionaryRemoveValue(parameterDict, CFSTR(kIODisplayMaxValueKey));
							CFDictionaryRemoveValue(parameterDict, CFSTR(kIODisplayValueKey   ));
							if (CFDictionaryGetCount(parameterDict)) {
								CFDictionarySetValue(dictDisplayParameters, keys[i], parameterDict);
							}
							else {
								CFDictionaryRemoveValue(dictDisplayParameters, keys[i]);
							}

						} // if min ...
						
					} // if parameterDict

				} // if is dict

			} // if key string
			
		} // for key

		free(keys);
		free(values);
		
		if (CFDictionaryGetCount(dictDisplayParameters)) {
			iprintf("%s properties (unparsed) = ", parametersName);
			CFOutput(dictDisplayParameters);
			cprintf("; // %s properties (unparsed)\n", parametersName);
		}
	} // if dictDisplayParameters
	else {
		CFOutput(dictDisplayParameters0);
	}
	OUTDENT iprintf("}; // %s\n", parametersName);
	return dictDisplayParameters;
} // DumpDisplayParameters


#define STRTOORD4(_s) ((_s[0]<<24) | (_s[1]<<16) | (_s[2]<<8) | (_s[3]))

UInt32 attributes[] = {
	kIOPowerStateAttribute           ,
	kIOPowerAttribute                ,
	kIODriverPowerAttribute          ,
	kIOHardwareCursorAttribute       ,
	kIOMirrorAttribute               ,
	kIOMirrorDefaultAttribute        ,
	kIOCapturedAttribute             ,
	kIOCursorControlAttribute        ,
	kIOSystemPowerAttribute          ,
	kIOWindowServerActiveAttribute   ,
	kIOVRAMSaveAttribute             ,
	kIODeferCLUTSetAttribute         ,
	kIOClamshellStateAttribute       ,
	kIOFBDisplayPortTrainingAttribute,
	kIOFBDisplayState                ,
	kIOFBVariableRefreshRate         ,
	kIOFBLimitHDCPAttribute          ,
	kIOFBLimitHDCPStateAttribute     ,
	kIOFBStop                        ,
	kIOFBRedGammaScaleAttribute      ,
	kIOFBGreenGammaScaleAttribute    ,
	kIOFBBlueGammaScaleAttribute     ,
	kIOFBHDRMetaDataAttribute        ,
	kIOBuiltinPanelPowerAttribute    ,

	kIOFBSpeedAttribute                 ,
	kIOFBWSStartAttribute               ,
	kIOFBProcessConnectChangeAttribute  ,
	kIOFBEndConnectChangeAttribute      ,
	kIOFBMatchedConnectChangeAttribute  ,

	kConnectionInTVMode                 ,
	kConnectionWSSB                     ,
	kConnectionRawBacklight             ,
	kConnectionBacklightSave            ,
	kConnectionVendorTag                ,
	
	kConnectionFlags                    ,
	
	kConnectionSyncEnable               ,
	kConnectionSyncFlags                ,
	
	kConnectionSupportsAppleSense       ,
	kConnectionSupportsLLDDCSense       ,
	kConnectionSupportsHLDDCSense       ,
	kConnectionEnable                   ,
	kConnectionCheckEnable              ,
	kConnectionProbe                    ,
	kConnectionIgnore                   ,
	kConnectionChanged                  ,
//		kConnectionPower                    , // kIOPowerAttribute
	kConnectionPostWake                 ,
	kConnectionDisplayParameterCount    ,
	kConnectionDisplayParameters        ,
	kConnectionOverscan                 ,
	kConnectionVideoBest                ,
	
	kConnectionRedGammaScale            ,
	kConnectionGreenGammaScale          ,
	kConnectionBlueGammaScale           ,
	kConnectionGammaScale               ,
	kConnectionFlushParameters          ,
	
	kConnectionVBLMultiplier            ,
	
	kConnectionHandleDisplayPortEvent   ,
	
	kConnectionPanelTimingDisable       ,
	
	kConnectionColorMode                ,
	kConnectionColorModesSupported      ,
	kConnectionColorDepthsSupported     ,
	
	kConnectionControllerDepthsSupported,
	kConnectionControllerColorDepth     ,
	kConnectionControllerDitherControl  ,
	
	kConnectionDisplayFlags             ,
	
	kConnectionEnableAudio              ,
	kConnectionAudioStreaming           ,
	
	kConnectionStartOfFrameTime         ,
	
	kConnectionUnderscan                ,
	kTempAttribute                      ,
	kIODisplaySelectedColorModeKey4cc   ,
	'dith',
//	'ownr', // this will cause a crash if passed to IODisplayGetFloatParameter
	0
};


static char * GetAttributeCodeStr(char *buf, size_t bufSize, UInt64 attribute, const char * nullStr) {
	char* atrc = (char*)&attribute;
	int x = 0;
	if (attribute & 0xffffffff00000000ULL)
		x += scnprintf(buf + x, bufSize - x, "?0x%llx", attribute);
	else {
		if (atrc[3] || !nullStr) x += scnprintf(buf + x, bufSize - x, "%c", atrc[3]); else x += scnprintf(buf + x, bufSize - x, "%s", nullStr);
		if (atrc[2] || !nullStr) x += scnprintf(buf + x, bufSize - x, "%c", atrc[2]); else x += scnprintf(buf + x, bufSize - x, "%s", nullStr);
		if (atrc[1] || !nullStr) x += scnprintf(buf + x, bufSize - x, "%c", atrc[1]); else x += scnprintf(buf + x, bufSize - x, "%s", nullStr);
		if (atrc[0] || !nullStr) x += scnprintf(buf + x, bufSize - x, "%c", atrc[0]); else x += scnprintf(buf + x, bufSize - x, "%s", nullStr);
	}
	return buf;
} // GetAttributeCodeStr


void DumpAllParameters(const char *parametersName, io_service_t service) {
	bool gotOneParameter = false;

	for (UInt32 *attribute = attributes; *attribute; attribute++) {
		float    fval;
		SInt32   value;
		SInt32   min;
		SInt32   max;
		IOReturn result1;
		IOReturn result2;
		IOReturn result3;

		char	attributename[20];
		for (int i = 0; i < 2; i++) {
			// I don't think any attributes/parameters that contain a null character will ever get a result but we'll try:
			GetAttributeCodeStr(attributename, sizeof(attributename), *attribute, i ? "" : NULL);
			CFStringRef attributeStr = i ?
				CFStringCreateWithCString(kCFAllocatorDefault, attributename, kCFStringEncodingUTF8) :
				CFStringCreateWithBytes(kCFAllocatorDefault, (UInt8*)attributename, 4, kCFStringEncodingUTF8, false);

			fval = 0.0f;
			value = 0;
			min = 0;
			max = 0;

			result1 = IODisplayGetFloatParameter(service, kNilOptions, attributeStr, &fval);
			result2 = IODisplayGetIntegerRangeParameter(service, kNilOptions, attributeStr, &value, &min, &max);
			result3 = IODisplaySetIntegerParameter(service, kNilOptions, attributeStr, value);
			CFRelease(attributeStr);
			if (result1 == kIOReturnSuccess || result2 == kIOReturnSuccess || result3 == kIOReturnSuccess) {
				if (!gotOneParameter) {
					iprintf("%s = {\n", parametersName); INDENT
					gotOneParameter = true;
				}
				
				iprintf("'%.4s' %f %d (%d..%d)%s\n",
					attributename,
					fval,
					(int)value,
					(int)min,
					(int)max,
					result3 == kIOReturnSuccess ? " (can set)" : ""
				);
				break;
			}
		}
	}
	if (gotOneParameter) {
		OUTDENT iprintf("}; // %s\n\n", parametersName);
	}
} // DumpAllParameters


CFMutableDictionaryRef DumpI2CProperties(CFDictionaryRef dict)
{
	CFMutableDictionaryRef copy = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, dict);

	onenum(64, IOI2CInterfaceID , "0x%llx")
	onenum(32, IOI2CBusType , "%d")
	onenum(32, IOI2CTransactionTypes , "0x%x")
	onenum(32, IOI2CSupportedCommFlags , "0x%x")

	cprintf("{ id:%s busType:%s transactionTypes:(%s%s%s%s%s%s) commFlags:(%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s) }",
		str_IOI2CInterfaceID,

		val_IOI2CBusType == kIOI2CBusTypeI2C         ?         "I2C" :
		val_IOI2CBusType == kIOI2CBusTypeDisplayPort ? "DisplayPort" :
		UNKNOWN_VALUE(val_IOI2CBusType),

		(val_IOI2CTransactionTypes & (1 << kIOI2CNoTransactionType               )) ?          "No," : "",
		(val_IOI2CTransactionTypes & (1 << kIOI2CSimpleTransactionType           )) ?      "Simple," : "",
		(val_IOI2CTransactionTypes & (1 << kIOI2CDDCciReplyTransactionType       )) ?       "DDCci," : "",
		(val_IOI2CTransactionTypes & (1 << kIOI2CCombinedTransactionType         )) ?    "Combined," : "",
		(val_IOI2CTransactionTypes & (1 << kIOI2CDisplayPortNativeTransactionType)) ? "DisplayPort," : "",
		UNKNOWN_FLAG(val_IOI2CTransactionTypes & 0xffffffe0),
		
		(val_IOI2CSupportedCommFlags & 0x00000001                 ) ?            "0?," : "",
		(val_IOI2CSupportedCommFlags & kIOI2CUseSubAddressCommFlag) ? "UseSubAddress," : "",
		(val_IOI2CSupportedCommFlags & 0x00000004                 ) ?            "2?," : "",
		(val_IOI2CSupportedCommFlags & 0x00000008                 ) ?            "3?," : "",
		(val_IOI2CSupportedCommFlags & 0x00000010                 ) ?            "4?," : "",
		(val_IOI2CSupportedCommFlags & 0x00000020                 ) ?            "5?," : "",
		(val_IOI2CSupportedCommFlags & 0x00000040                 ) ?            "6?," : "",
		(val_IOI2CSupportedCommFlags & 0x00000080                 ) ?            "7?," : "",
		(val_IOI2CSupportedCommFlags & 0x00000100                 ) ?            "8?," : "",
		(val_IOI2CSupportedCommFlags & 0x00000200                 ) ?            "9?," : "",
		(val_IOI2CSupportedCommFlags & 0x00000400                 ) ?           "10?," : "",
		(val_IOI2CSupportedCommFlags & 0x00000800                 ) ?           "11?," : "",
		(val_IOI2CSupportedCommFlags & 0x00001000                 ) ?           "12?," : "",
		(val_IOI2CSupportedCommFlags & 0x00002000                 ) ?           "13?," : "",
		(val_IOI2CSupportedCommFlags & 0x00004000                 ) ?           "14?," : "",
		(val_IOI2CSupportedCommFlags & 0x00008000                 ) ?           "15?," : "",
		(val_IOI2CSupportedCommFlags & 0x00010000                 ) ?           "16?," : "",
		(val_IOI2CSupportedCommFlags & 0x00020000                 ) ?           "17?," : "",
		(val_IOI2CSupportedCommFlags & 0x00040000                 ) ?           "18?," : "",
		(val_IOI2CSupportedCommFlags & 0x00080000                 ) ?           "19?," : "",
		(val_IOI2CSupportedCommFlags & 0x00100000                 ) ?           "20?," : "",
		(val_IOI2CSupportedCommFlags & 0x00200000                 ) ?           "21?," : "",
		(val_IOI2CSupportedCommFlags & 0x00400000                 ) ?           "22?," : "",
		(val_IOI2CSupportedCommFlags & 0x00800000                 ) ?           "23?," : "",
		(val_IOI2CSupportedCommFlags & 0x01000000                 ) ?           "24?," : "",
		(val_IOI2CSupportedCommFlags & 0x02000000                 ) ?           "25?," : "",
		(val_IOI2CSupportedCommFlags & 0x04000000                 ) ?           "26?," : "",
		(val_IOI2CSupportedCommFlags & 0x08000000                 ) ?           "27?," : "",
		(val_IOI2CSupportedCommFlags & 0x10000000                 ) ?           "28?," : "",
		(val_IOI2CSupportedCommFlags & 0x20000000                 ) ?           "29?," : "",
		(val_IOI2CSupportedCommFlags & 0x40000000                 ) ?           "30?," : "",
		(val_IOI2CSupportedCommFlags & 0x80000000                 ) ?           "31?," : ""
	);

	return copy;
}

static const char * GetAttributeName(UInt64 attribute, bool forConnection)
{
	switch (attribute) {
		case kIOPowerStateAttribute           : return "kIOPowerStateAttribute";
		case kIOPowerAttribute                : return forConnection ? "kConnectionPower" : "kIOPowerAttribute";
		case kIODriverPowerAttribute          : return "kIODriverPowerAttribute";
		case kIOHardwareCursorAttribute       : return "kIOHardwareCursorAttribute";
		case kIOMirrorAttribute               : return "kIOMirrorAttribute";
		case kIOMirrorDefaultAttribute        : return "kIOMirrorDefaultAttribute";
		case kIOCapturedAttribute             : return "kIOCapturedAttribute";
		case kIOCursorControlAttribute        : return "kIOCursorControlAttribute";
		case kIOSystemPowerAttribute          : return "kIOSystemPowerAttribute";
		case kIOWindowServerActiveAttribute   : return "kIOWindowServerActiveAttribute";
		case kIOVRAMSaveAttribute             : return "kIOVRAMSaveAttribute";
		case kIODeferCLUTSetAttribute         : return "kIODeferCLUTSetAttribute";
		case kIOClamshellStateAttribute       : return "kIOClamshellStateAttribute";
		case kIOFBDisplayPortTrainingAttribute: return "kIOFBDisplayPortTrainingAttribute";
		case kIOFBDisplayState                : return "kIOFBDisplayState";
		case kIOFBVariableRefreshRate         : return "kIOFBVariableRefreshRate";
		case kIOFBLimitHDCPAttribute          : return "kIOFBLimitHDCPAttribute";
		case kIOFBLimitHDCPStateAttribute     : return "kIOFBLimitHDCPStateAttribute";
		case kIOFBStop                        : return "kIOFBStop";
		case kIOFBRedGammaScaleAttribute      : return "kIOFBRedGammaScaleAttribute";
		case kIOFBGreenGammaScaleAttribute    : return "kIOFBGreenGammaScaleAttribute";
		case kIOFBBlueGammaScaleAttribute     : return "kIOFBBlueGammaScaleAttribute";
		case kIOFBHDRMetaDataAttribute        : return "kIOFBHDRMetaDataAttribute";
		case kIOBuiltinPanelPowerAttribute    : return "kIOBuiltinPanelPowerAttribute";

		case kIOFBSpeedAttribute                 : return "kIOFBSpeedAttribute";
		case kIOFBWSStartAttribute               : return "kIOFBWSStartAttribute";
		case kIOFBProcessConnectChangeAttribute  : return "kIOFBProcessConnectChangeAttribute";
		case kIOFBEndConnectChangeAttribute      : return "kIOFBEndConnectChangeAttribute";
		case kIOFBMatchedConnectChangeAttribute  : return "kIOFBMatchedConnectChangeAttribute";
		// Connection attributes
		case kConnectionInTVMode                 : return "kConnectionInTVMode";
		case kConnectionWSSB                     : return "kConnectionWSSB";
		case kConnectionRawBacklight             : return "kConnectionRawBacklight";
		case kConnectionBacklightSave            : return "kConnectionBacklightSave";
		case kConnectionVendorTag                : return "kConnectionVendorTag";
			
		case kConnectionFlags                    : return "kConnectionFlags";
		
		case kConnectionSyncEnable               : return "kConnectionSyncEnable";
		case kConnectionSyncFlags                : return "kConnectionSyncFlags";
		
		case kConnectionSupportsAppleSense       : return "kConnectionSupportsAppleSense";
		case kConnectionSupportsLLDDCSense       : return "kConnectionSupportsLLDDCSense";
		case kConnectionSupportsHLDDCSense       : return "kConnectionSupportsHLDDCSense";
		case kConnectionEnable                   : return "kConnectionEnable";
		case kConnectionCheckEnable              : return "kConnectionCheckEnable";
		case kConnectionProbe                    : return "kConnectionProbe";
		case kConnectionIgnore                   : return "kConnectionIgnore";
		case kConnectionChanged                  : return "kConnectionChanged";
//		case kConnectionPower                    : return "kConnectionPower"; // kIOPowerAttribute
		case kConnectionPostWake                 : return "kConnectionPostWake";
		case kConnectionDisplayParameterCount    : return "kConnectionDisplayParameterCount";
		case kConnectionDisplayParameters        : return "kConnectionDisplayParameters";
		case kConnectionOverscan                 : return "kConnectionOverscan";
		case kConnectionVideoBest                : return "kConnectionVideoBest";
			
		case kConnectionRedGammaScale            : return "kConnectionRedGammaScale";
		case kConnectionGreenGammaScale          : return "kConnectionGreenGammaScale";
		case kConnectionBlueGammaScale           : return "kConnectionBlueGammaScale";
		case kConnectionGammaScale               : return "kConnectionGammaScale";
		case kConnectionFlushParameters          : return "kConnectionFlushParameters";
			
		case kConnectionVBLMultiplier            : return "kConnectionVBLMultiplier";
			
		case kConnectionHandleDisplayPortEvent   : return "kConnectionHandleDisplayPortEvent";
			
		case kConnectionPanelTimingDisable       : return "kConnectionPanelTimingDisable";
			
		case kConnectionColorMode                : return "kConnectionColorMode";
		case kConnectionColorModesSupported      : return "kConnectionColorModesSupported";
		case kConnectionColorDepthsSupported     : return "kConnectionColorDepthsSupported";
			
		case kConnectionControllerDepthsSupported: return "kConnectionControllerDepthsSupported";
		case kConnectionControllerColorDepth     : return "kConnectionControllerColorDepth";
		case kConnectionControllerDitherControl  : return "kConnectionControllerDitherControl";
			
		case kConnectionDisplayFlags             : return "kConnectionDisplayFlags";
			

		case kConnectionEnableAudio              : return "kConnectionEnableAudio";
		case kConnectionAudioStreaming           : return "kConnectionAudioStreaming";
			
		case kConnectionStartOfFrameTime         : return "kConnectionStartOfFrameTime";
			
		case kConnectionUnderscan                : return "kConnectionUnderscan";
		case kTempAttribute                      : return "kTempAttribute";
		case kIODisplaySelectedColorModeKey4cc   : return "kIODisplaySelectedColorModeKey";

		default                               : return NULL;
	}
} // GetAttributeName

#ifdef __cplusplus
extern "C" {
#endif
void DumpOneAttribute(UInt64 attribute, bool set, bool forConnection, void *valuePtr, size_t len, bool align = true);
#ifdef __cplusplus
}
#endif

void DumpOneAttribute(UInt64 attribute, bool set, bool forConnection, void *valuePtr, size_t len, bool align)
{
	char valueStr[500];
	char attributename[20];

	char* buf = valueStr;
	size_t bufSize = sizeof(valueStr);
	
	const char* name = GetAttributeName(attribute, forConnection);

	size_t inc = 0;
	if (!valuePtr) {
		switch (attribute) {
			case kConnectionChanged:
			case kConnectionSupportsAppleSense:
			case kConnectionSupportsHLDDCSense:
			case kConnectionSupportsLLDDCSense:
				inc += scnprintf(buf+inc, bufSize-inc, "n/a");
				break;
			default:
				inc += scnprintf(buf+inc, bufSize-inc, "NULL");
				break;
		}
	}
	else {
		UInt64 value = 0;
		switch (len) {
			case 1: value = *(UInt8 *)valuePtr; inc += scnprintf(buf+inc, bufSize-inc, "0x%02llx : ", value); break;
			case 2: value = *(UInt16*)valuePtr; inc += scnprintf(buf+inc, bufSize-inc, "0x%04llx : ", value); break;
			case 4: value = *(UInt32*)valuePtr; inc += scnprintf(buf+inc, bufSize-inc, "0x%08llx : ", value); break;
			case 8:
				if (attribute != kConnectionDisplayParameters) {
					value = *(UInt64*)valuePtr;
					int hexdigits =
						(value >> 32) ?
							(value >> 32) == 0x0ffffffff ?
								8
							:
								16
						:
							8;
					if (hexdigits == 8) value &= 0x0ffffffff;
					inc += scnprintf(buf+inc, bufSize-inc, "0x%0*llx : ", hexdigits, value);
				}
				break;
		}

		switch (attribute) {
			case kConnectionFlags:
				inc += scnprintf(buf+inc, bufSize-inc, "%s%s%s",
					value & kIOConnectionBuiltIn    ?    "BuiltIn," : "",
					value & kIOConnectionStereoSync ? "StereoSync," : "",
					UNKNOWN_FLAG(value & 0xffff77ff)
				); break;

			case kConnectionSyncFlags:
				inc += scnprintf(buf+inc, bufSize-inc, "%s%s%s%s%s%s%s%s%s",
					value & kIOHSyncDisable          ?          "HSyncDisable," : "",
					value & kIOVSyncDisable          ?          "VSyncDisable," : "",
					value & kIOCSyncDisable          ?          "CSyncDisable," : "",
					value & kIONoSeparateSyncControl ? "NoSeparateSyncControl," : "",
					value & kIOTriStateSyncs         ?         "TriStateSyncs," : "",
					value & kIOSyncOnBlue            ?            "SyncOnBlue," : "",
					value & kIOSyncOnGreen           ?           "SyncOnGreen," : "",
					value & kIOSyncOnRed             ?             "SyncOnRed," : "",
					UNKNOWN_FLAG(value & 0xffffff00)
				); break;

			case kConnectionHandleDisplayPortEvent:
				if (set) {
					inc += scnprintf(buf+inc, bufSize-inc, "%s",
						value == kIODPEventStart                       ?                       "Start" :
						value == kIODPEventIdle                        ?                        "Idle" :
						value == kIODPEventForceRetrain                ?                "ForceRetrain" :
						value == kIODPEventRemoteControlCommandPending ? "RemoteControlCommandPending" :
						value == kIODPEventAutomatedTestRequest        ?        "AutomatedTestRequest" :
						value == kIODPEventContentProtection           ?           "ContentProtection" :
						value == kIODPEventMCCS                        ?                        "MCCS" :
						value == kIODPEventSinkSpecific                ?                "SinkSpecific" :
						UNKNOWN_VALUE(value)
					);
				}
				else {
					for (int i=0; i < len; i++) {
						inc += scnprintf(buf+inc, bufSize-inc, "%02x", ((UInt8*)valuePtr)[i]);
					}
				}
				break;

			case kConnectionColorMode:
			case kConnectionColorModesSupported:
				inc += scnprintf(buf+inc, bufSize-inc, "%s%s%s%s%s%s%s",
					value == kIODisplayColorModeReserved  ?    "Reserved" : "",
					value & kIODisplayColorModeRGB        ?        "RGB," : "",
					value & kIODisplayColorModeYCbCr422   ?        "422," : "",
					value & kIODisplayColorModeYCbCr444   ?        "444," : "",
					value & kIODisplayColorModeRGBLimited ? "RGBLimited," : "",
					value & kIODisplayColorModeAuto       ?       "Auto," : "",
					UNKNOWN_FLAG(value & 0xefffeeee)
				); break;

			case kConnectionColorDepthsSupported:
			case kConnectionControllerDepthsSupported:
			case kConnectionControllerColorDepth:
				inc += scnprintf(buf+inc, bufSize-inc, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
					value == kIODisplayRGBColorComponentBitsUnknown ? "Unknown" : "",
					value & kIODisplayRGBColorComponentBits6        ?  "RGB 6," : "",
					value & kIODisplayRGBColorComponentBits8        ?  "RGB 8," : "",
					value & kIODisplayRGBColorComponentBits10       ? "RGB 10," : "",
					value & kIODisplayRGBColorComponentBits12       ? "RGB 12," : "",
					value & kIODisplayRGBColorComponentBits14       ? "RGB 14," : "",
					value & kIODisplayRGBColorComponentBits16       ? "RGB 16," : "",
					value & kIODisplayYCbCr444ColorComponentBits6   ?  "444 6," : "",
					value & kIODisplayYCbCr444ColorComponentBits8   ?  "444 8," : "",
					value & kIODisplayYCbCr444ColorComponentBits10  ? "444 10," : "",
					value & kIODisplayYCbCr444ColorComponentBits12  ? "444 12," : "",
					value & kIODisplayYCbCr444ColorComponentBits14  ? "444 14," : "",
					value & kIODisplayYCbCr444ColorComponentBits16  ? "444 16," : "",
					value & kIODisplayYCbCr422ColorComponentBits6   ?  "422 6," : "",
					value & kIODisplayYCbCr422ColorComponentBits8   ?  "422 8," : "",
					value & kIODisplayYCbCr422ColorComponentBits10  ? "422 10," : "",
					value & kIODisplayYCbCr422ColorComponentBits12  ? "422 12," : "",
					value & kIODisplayYCbCr422ColorComponentBits14  ? "422 14," : "",
					value & kIODisplayYCbCr422ColorComponentBits16  ? "422 16," : "",
					UNKNOWN_FLAG(value & 0xffc00000)
				); break;

			case kConnectionControllerDitherControl:
				inc += scnprintf(buf+inc, bufSize-inc, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
					((value >> kIODisplayDitherRGBShift     ) & 0xff) == kIODisplayDitherDisable         ?          "RGB Disabled," : "",
					((value >> kIODisplayDitherRGBShift     ) & 0xff) == kIODisplayDitherAll             ?               "RGB All," : "",
					((value >> kIODisplayDitherRGBShift     ) & 0xff) & kIODisplayDitherSpatial          ?           "RGB Spatial," : "",
					((value >> kIODisplayDitherRGBShift     ) & 0xff) & kIODisplayDitherTemporal         ?          "RGB Temporal," : "",
					((value >> kIODisplayDitherRGBShift     ) & 0xff) & kIODisplayDitherFrameRateControl ?  "RGB FrameRateControl," : "",
					((value >> kIODisplayDitherRGBShift     ) & 0xff) & kIODisplayDitherDefault          ?           "RGB Default," : "",
					((value >> kIODisplayDitherRGBShift     ) & 0xff) & 0x70                             ?                 "RGB ?," : "",
					((value >> kIODisplayDitherYCbCr444Shift) & 0xff) == kIODisplayDitherDisable         ?          "444 Disabled," : "",
					((value >> kIODisplayDitherYCbCr444Shift) & 0xff) == kIODisplayDitherAll             ?               "444 All," : "",
					((value >> kIODisplayDitherYCbCr444Shift) & 0xff) & kIODisplayDitherSpatial          ?           "444 Spatial," : "",
					((value >> kIODisplayDitherYCbCr444Shift) & 0xff) & kIODisplayDitherTemporal         ?          "444 Temporal," : "",
					((value >> kIODisplayDitherYCbCr444Shift) & 0xff) & kIODisplayDitherFrameRateControl ?  "444 FrameRateControl," : "",
					((value >> kIODisplayDitherYCbCr444Shift) & 0xff) & kIODisplayDitherDefault          ?           "444 Default," : "",
					((value >> kIODisplayDitherYCbCr444Shift) & 0xff) & 0x70                             ?                 "444 ?," : "",
					((value >> kIODisplayDitherYCbCr422Shift) & 0xff) == kIODisplayDitherDisable         ?          "422 Disabled," : "",
					((value >> kIODisplayDitherYCbCr422Shift) & 0xff) == kIODisplayDitherAll             ?               "422 All," : "",
					((value >> kIODisplayDitherYCbCr422Shift) & 0xff) & kIODisplayDitherSpatial          ?           "422 Spatial," : "",
					((value >> kIODisplayDitherYCbCr422Shift) & 0xff) & kIODisplayDitherTemporal         ?          "422 Temporal," : "",
					((value >> kIODisplayDitherYCbCr422Shift) & 0xff) & kIODisplayDitherFrameRateControl ?  "422 FrameRateControl," : "",
					((value >> kIODisplayDitherYCbCr422Shift) & 0xff) & kIODisplayDitherDefault          ?           "422 Default," : "",
					((value >> kIODisplayDitherYCbCr422Shift) & 0xff) & 0x70                             ?                 "422 ?," : "",
					UNKNOWN_FLAG(value & 0xff000000)
				); break;
				
			case kConnectionDisplayFlags:
				inc += scnprintf(buf+inc, bufSize-inc, "%s%s",
					value == kIODisplayNeedsCEAUnderscan ? "NeedsCEAUnderscan," : "",
						 UNKNOWN_FLAG(value & 0xfffffffe)
				); break;

			case kIOMirrorAttribute:
				inc += scnprintf(buf+inc, bufSize-inc,
					"%s%s%s%s",
					value & kIOMirrorIsPrimary  ?  "kIOMirrorIsPrimary," : "",
					value & kIOMirrorHWClipped  ?  "kIOMirrorHWClipped," : "",
					value & kIOMirrorIsMirrored ? "kIOMirrorIsMirrored," : "",
					UNKNOWN_FLAG(value & 0x1fffffff)
				); break;

			case kIOMirrorDefaultAttribute:
				inc += scnprintf(buf+inc, bufSize-inc, "%s%s%s%s%s%s%s",
					value & kIOMirrorDefault        ?        "kIOMirrorDefault," : "",
					value & kIOMirrorForced         ?         "kIOMirrorForced," : "",
					value & kIOGPlatformYCbCr       ?       "kIOGPlatformYCbCr," : "",
					value & kIOFBDesktopModeAllowed ? "kIOFBDesktopModeAllowed," : "",
					value & kIOMirrorNoAutoHDMI     ?     "kIOMirrorNoAutoHDMI," : "",
					value & kIOMirrorHint           ?           "kIOMirrorHint," : "",
					UNKNOWN_FLAG(value & 0xfffeffe0)
				); break;
				
			case kConnectionDisplayParameterCount:
			case kTempAttribute:
			case kIOFBSpeedAttribute:
			case kIOPowerStateAttribute:
			case kConnectionOverscan:
			case kConnectionUnderscan:
			case kIODisplaySelectedColorModeKey4cc:
				inc += scnprintf(buf+inc, bufSize-inc, "%ld", (unsigned long)value);
				break;

			case 'dith':
			case kIOHardwareCursorAttribute:
			case kIOClamshellStateAttribute:
			case kConnectionEnable:
			case kConnectionCheckEnable:
			case kIOVRAMSaveAttribute:
			case kIOCapturedAttribute:
			case kIOFBLimitHDCPAttribute:
			case kIOFBLimitHDCPStateAttribute:
			case kConnectionFlushParameters:
				inc += scnprintf(buf+inc, bufSize-inc, "%s", value == 1 ? "true" : value == 0 ? "false" : UNKNOWN_VALUE(value));
				break;
			
			case kConnectionProbe:
				inc += scnprintf(buf+inc, bufSize-inc, "%s", value == kIOFBUserRequestProbe ? "kIOFBUserRequestProbe" : UNKNOWN_VALUE(value));
				break;

			case kConnectionIgnore:
				inc += scnprintf(buf+inc, bufSize-inc, "isMuted:%s%s%s", ((value & (1LL << 31)) == 1) ? "true" : "false", (value & ~(1LL << 31)) ? " " : "", (value & ~(1 << 31)) ? UNKNOWN_VALUE(value & ~(1 << 31)) : "");
				break;
			
			case kConnectionChanged:
			case kConnectionSupportsAppleSense:
			case kConnectionSupportsHLDDCSense:
			case kConnectionSupportsLLDDCSense:
				inc += scnprintf(buf+inc, bufSize-inc, "n/a");
				break;

			case kConnectionDisplayParameters:
				for (int i = 0; i < len / sizeof(UInt32); i++) { // actually an array of UInt64 but we'll double check to make sure the high part is zero
					UInt32 parameter = ((UInt32*)(valuePtr))[i];
					if (parameter || (i & 1) == 0) {
						const char *parameterName = GetAttributeName(parameter, forConnection);
						inc += scnprintf(buf+inc, bufSize-inc, "%s'%s' %s", i ? ", " : "",
							GetAttributeCodeStr(attributename, sizeof(attributename), parameter, "ø"),
							parameterName ? parameterName : ""
						);
					}
				}
				
				break;

			default:
				inc += scnprintf(buf+inc, bufSize-inc, "0x%llx", value);
		}
	}

	if (align)
		cprintf("%-36s '%s' = %s", name ? name : "", GetAttributeCodeStr(attributename, sizeof(attributename), attribute, "ø"), valueStr);
	else
		cprintf("%s%s'%s' = %s", name ? name : "", name ? " " : "", GetAttributeCodeStr(attributename, sizeof(attributename), attribute, "ø"), valueStr);
} // DumpOneAttribute


void DumpOneIODisplayAttributes(CFMutableDictionaryRef dictDisplayInfo)
{
	typedef struct IODisplayAttributesRec {
		UInt32 attribute;
		UInt32 value;
	} IODisplayAttributesRec;

	CFDataRef IODisplayAttributesData = (CFDataRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kIODisplayAttributesKey));
	
	if (IODisplayAttributesData && CFGetTypeID(IODisplayAttributesData) == CFDataGetTypeID()) {
		iprintf("IODisplayAttributes = {\n"); INDENT
		
		if (CFDataGetLength(IODisplayAttributesData) % sizeof(IODisplayAttributesRec) != 0) {
			iprintf("DisplayPixelDimensions = unexpected size %ld\n", CFDataGetLength(IODisplayAttributesData));
		}
		else {
			IODisplayAttributesRec *p = (IODisplayAttributesRec *)CFDataGetBytePtr(IODisplayAttributesData);
			for (int i = 0; i < CFDataGetLength(IODisplayAttributesData) / sizeof(IODisplayAttributesRec); i++) {
				iprintf("[%d] = { ", i);
				DumpOneAttribute(p[i].attribute, false, false, &p[i].value, sizeof(p[i].value));
				cprintf(" },\n");
			}
			CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kIODisplayAttributesKey));
		}
		OUTDENT iprintf("}; // IODisplayAttributes\n");
	} // if IODisplayAttributes

} // DumpOneIODisplayAttributes


static void DumpOneIOFBAttribute(CFDataRef data, int index)
{
	typedef struct Attribute {
		bool      set;
		bool      notnull;
		IOIndex   connectIndex;
		IOSelect  attribute;
		IOReturn  result;
		UInt64    value;
	} Attribute;

	if (!data) {
		iprintf("NULL\n");
		return;
	}
	
	CFIndex len = CFDataGetLength(data);
	Attribute* atr = (Attribute*) CFDataGetBytePtr(data);

	if (!atr) {
		iprintf("NULL2\n");
		return;
	}
	iprintf("%sAttribute", atr->set ? "set" : "get");
	if (atr->connectIndex >= 0) {
		cprintf("ForConnection(%u) ", (unsigned int)atr->connectIndex);
	}
	else {
		cprintf("                 ");
	}
	
	char resultStr[40];
	cprintf("{ ");
	DumpOneAttribute(atr->attribute, atr->set, atr->connectIndex >= 0, atr->notnull ? &atr->value : NULL, len - offsetof(Attribute, value) );
	cprintf (" }%s\n", DumpOneReturn(resultStr, sizeof(resultStr), atr->result));
} // DumpOneIOFBAttribute


void DumpDisplayInfo(CFMutableDictionaryRef dictDisplayInfo)
{
	SInt32 numValue;
	CFNumberRef num;

	{ // DisplayVendorID
		num = (CFNumberRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplayVendorID));
		if (num) {
			CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
			if (numValue == 'unkn') {
				iprintf("DisplayVendorID = %d (0x%04x) 'unkn';\n", (int)numValue, (int)numValue);
			}
			else {
				iprintf("DisplayVendorID = %d (0x%04x) %s\"%c%c%c\";\n", (int)numValue, (int)numValue,
					numValue & 0xffff8000 ? UNKNOWN_VALUE(numValue & 0xffff8000) : "",
					(int)(((numValue >> 10) & 31) + '@'),
					(int)(((numValue >>  5) & 31) + '@'),
					(int)(((numValue >>  0) & 31) + '@')
				);
			}
			CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kDisplayVendorID));
		}
	} // DisplayVendorID
	
	{ // DisplayProductID
		num = (CFNumberRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplayProductID));
		if (num) {
			CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
			iprintf("DisplayProductID = %d (0x%04x);\n", (int)numValue, (int)numValue);
			CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kDisplayProductID));
		}
	} // DisplayProductID

	{ // DisplayProductName

		// Show the preferred DisplayProductName
		{
			CFDictionaryRef names = (CFDictionaryRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplayProductName));
			if (names) {
				CFArrayRef langKeys = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
				CFDictionaryApplyFunction(names, KeyArrayCallback, (void *) langKeys);
				CFArrayRef orderLangKeys = CFBundleCopyPreferredLocalizationsFromArray(langKeys);
				CFRelease(langKeys);
				if (orderLangKeys && CFArrayGetCount(orderLangKeys)) {
					char            cName[256];
					CFStringRef     langKey;
					CFStringRef     localName;

					langKey = (CFStringRef)CFArrayGetValueAtIndex(orderLangKeys, 0);
					localName = (CFStringRef)CFDictionaryGetValue(names, langKey);
					CFStringGetCString(localName, cName, sizeof(cName),
										CFStringGetSystemEncoding());
					iprintf("DisplayProductName (preferred) = \"%s\"\n", cName);
				}
				CFRelease(orderLangKeys);
			}
		}

		// Show all languages used for each DisplayProductName
		{
			CFMutableDictionaryRef DisplayProductName = NULL;
			CFDictionaryRef DisplayProductName0 = (CFDictionaryRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplayProductName));
			if (DisplayProductName0) {
				DisplayProductName = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, DisplayProductName0);

				CFIndex itemCount = CFDictionaryGetCount(DisplayProductName);

				const void **keys   = (const void **)malloc(itemCount * sizeof(void*));
				const void **values = (const void **)malloc(itemCount * sizeof(void*));
				CFDictionaryGetKeysAndValues(DisplayProductName, keys, values);
				
				iprintf("DisplayProductName = { ");
				
				CFIndex nextname = 0;
				char currentName[200];
				char name[200];
				char key[50];
				CFIndex langCount = 0;
				CFIndex nameCount = 0;

				while (nextname < itemCount) {
					langCount = 0;
					for (CFIndex i = nextname; i < itemCount; i++) {
						if (
							CFStringGetCString((CFStringRef)values[i], name, sizeof(name), kCFStringEncodingUTF8) &&
							CFStringGetCString((CFStringRef)keys  [i], key , sizeof(key ), kCFStringEncodingUTF8))
						{
							if (i == nextname) {
								nameCount++;
								langCount++;
								strncpy(currentName, name, sizeof(currentName));
								cprintf("%s\"%s\" = { %s", nextname ? ", " : "", name, key);
								nextname = itemCount;
								CFDictionaryRemoveValue(DisplayProductName, keys[i]);
							}
							else if (!strcmp(name, currentName)) {
								cprintf(", %s", key);
								CFDictionaryRemoveValue(DisplayProductName, keys[i]);
							}
							else if (nextname == itemCount) {
								nextname = i;
							}
						}
						else {
							if (i == nextname) {
								nextname++;
							}
						}
					}
					if (langCount) {
						cprintf(" }");
					}
				}

				free(keys);
				free(values);

				cprintf("%s};\n", nameCount ? " " : "");

				if (CFDictionaryGetCount(DisplayProductName)) {
					CFDictionarySetValue(dictDisplayInfo, CFSTR(kDisplayProductName), DisplayProductName);
				}
				else {
					CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kDisplayProductName));
				}
			}
		}
	} // DisplayProductName

	{ // DisplaySerialNumber
		num = (CFNumberRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplaySerialNumber));
		if (num) {
			UInt32 numValue;
			CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
			iprintf("DisplaySerialNumber = %u;\n", (uint32_t)numValue);
			CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kDisplaySerialNumber));
		}
	} // DisplaySerialNumber

	{ // IODisplayConnectFlags
		CFDataRef IODisplayConnectFlagsData = (CFDataRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kIODisplayConnectFlagsKey));
		typedef struct IODisplayConnectFlagsRec {
			UInt32 flags;
		} IODisplayConnectFlagsRec;
		
		if (IODisplayConnectFlagsData) {
			if (CFDataGetLength(IODisplayConnectFlagsData) != sizeof(IODisplayConnectFlagsRec)) {
				iprintf("IODisplayConnectFlags = unexpected size %ld;\n", CFDataGetLength(IODisplayConnectFlagsData));
			}
			else {
				IODisplayConnectFlagsRec *p = (IODisplayConnectFlagsRec *)CFDataGetBytePtr(IODisplayConnectFlagsData);
				iprintf("IODisplayConnectFlags = %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s;\n",
					p->flags & (1 << kAllModesValid         ) ?          "AllModesValid," : "",
					p->flags & (1 << kAllModesSafe          ) ?           "AllModesSafe," : "",
					p->flags & (1 << kReportsTagging        ) ?         "ReportsTagging," : "",
					p->flags & (1 << kHasDirectConnection   ) ?    "HasDirectConnection," : "",
					p->flags & (1 << kIsMonoDev             ) ?              "IsMonoDev," : "",
					p->flags & (1 << kUncertainConnection   ) ?    "UncertainConnection," : "",
					p->flags & (1 << kTaggingInfoNonStandard) ? "TaggingInfoNonStandard," : "",
					p->flags & (1 << kReportsDDCConnection  ) ?   "ReportsDDCConnection," : "",
					p->flags & (1 << kHasDDCConnection      ) ?       "HasDDCConnection," : "",
					p->flags & (1 << kConnectionInactive    ) ?     "ConnectionInactive," : "",
					p->flags & (1 << kDependentConnection   ) ?    "DependentConnection," : "",
					p->flags & (1 << kBuiltInConnection     ) ?      "BuiltInConnection," : "",
					p->flags & (1 << kOverrideConnection    ) ?     "OverrideConnection," : "",
					p->flags & (1 << kFastCheckForDDC       ) ?        "FastCheckForDDC," : "",
					p->flags & (1 << kReportsHotPlugging    ) ?     "ReportsHotPlugging," : "",
					p->flags & (1 << kStereoSyncConnection  ) ?   "StereoSyncConnection," : "",
					UNKNOWN_FLAG(p->flags & 0xffff0000)
				);
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kIODisplayConnectFlagsKey));
			}
		}
	} // IODisplayConnectFlags

	{ // IODisplayAttributes
		CFDictionaryRef IODisplayAttributesDict0 = (CFDictionaryRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kIODisplayAttributesKey));
		
		if (IODisplayAttributesDict0 && CFGetTypeID(IODisplayAttributesDict0) == CFDictionaryGetTypeID()) {
			iprintf("IODisplayAttributes = {\n"); INDENT
			CFMutableDictionaryRef IODisplayAttributesDict = NULL;
			IODisplayAttributesDict = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, IODisplayAttributesDict0);
			if (IODisplayAttributesDict) {
				DumpOneIODisplayAttributes(IODisplayAttributesDict);
				if (CFDictionaryGetCount(IODisplayAttributesDict)) {
					CFDictionarySetValue(dictDisplayInfo, CFSTR(kIODisplayAttributesKey), IODisplayAttributesDict);
				}
				else {
					CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kIODisplayAttributesKey));
				}
			}
			OUTDENT iprintf("}; // IODisplayAttributes\n");
		} else {
			DumpOneIODisplayAttributes(dictDisplayInfo);
		}
	} // IODisplayAttributes

	{ // IODisplayParameters
		CFDictionaryRef dictDisplayParameters0 = (CFDictionaryRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kIODisplayParametersKey));
		if (dictDisplayParameters0 && CFGetTypeID(dictDisplayParameters0) == CFDictionaryGetTypeID()) {
			CFMutableDictionaryRef dictDisplayParameters = DumpDisplayParameters("IODisplayParameters", dictDisplayParameters0);
			if (dictDisplayParameters) {
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kIODisplayParametersKey));
			}
		}
	} // IODisplayParameters

	{ // IODisplayEDID
		CFDataRef IODisplayEDIDData = (CFDataRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kIODisplayEDIDKey));
		if (IODisplayEDIDData) {
			if (CFDataGetLength(IODisplayEDIDData)) {
				iprintf("IODisplayEDID%s = ", CFDictionaryGetCountOfKey(dictDisplayInfo, CFSTR("IODisplayEDIDOriginal")) ? "        " : ""); // kIODisplayEDIDOriginalKey
				const UInt8 *p = CFDataGetBytePtr(IODisplayEDIDData);
				for (int i = 0; i < CFDataGetLength(IODisplayEDIDData); i++) {
					cprintf("%02x", p[i]);
				}
				lf;
			}
			CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kIODisplayEDIDKey));
		}
	} // IODisplayEDID

	{ // IODisplayEDIDOriginal
		CFDataRef IODisplayEDIDOriginalData = (CFDataRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR("IODisplayEDIDOriginal")); // kIODisplayEDIDOriginalKey
		if (IODisplayEDIDOriginalData) {
			if (CFDataGetLength(IODisplayEDIDOriginalData)) {
				iprintf("IODisplayEDIDOriginal = ");
				const UInt8 *p = CFDataGetBytePtr(IODisplayEDIDOriginalData);
				for (int i = 0; i < CFDataGetLength(IODisplayEDIDOriginalData); i++) {
					cprintf("%02x", p[i]);
				}
				lf;
			}
			CFDictionaryRemoveValue(dictDisplayInfo, CFSTR("IODisplayEDIDOriginal")); // kIODisplayEDIDOriginalKey
		}
	} // IODisplayEDIDOriginal

	// DisplayFixedPixelFormat
	// IODisplayHasBacklight
	// IODisplayHasBacklight
	// IODisplayIsDigital
	// IODisplayIsHDMISink
	// DisplayBrightnessAffectsGamma
	// DisplayViewAngleAffectsGamma
	DoAllBooleans(dictDisplayInfo);

} // DumpDisplayInfo

int dumpedCount = 0;
io_service_t dumpedServices[100];

void DumpOneIODisplay(io_service_t ioDisplayService)
{
	CFNumberRef num;
	SInt32 numValue;

	dumpedServices[dumpedCount++] = ioDisplayService;
	IOObjectRetain(ioDisplayService);
	
	CFMutableDictionaryRef IODProperties = NULL;
	if (!IORegistryEntryCreateCFProperties(ioDisplayService, &IODProperties, kCFAllocatorDefault, kNilOptions)) {
		if (IODProperties) {

			DumpDisplayInfo(IODProperties);

			{ // AppleDisplayType
				num = (CFNumberRef)CFDictionaryGetValue(IODProperties, CFSTR(kAppleDisplayTypeKey));
				if (num) {
					CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
					iprintf("AppleDisplayType = %s;\n",
						numValue == 0                     ?            "0?" :
						numValue == kUnknownConnect       ?       "Unknown" :
						numValue == kPanelConnect         ?         "Panel" :
						numValue == kPanelTFTConnect      ?      "PanelTFT" :
						numValue == kFixedModeCRTConnect  ?  "FixedModeCRT" :
						numValue == kMultiModeCRT1Connect ? "MultiModeCRT1" :
						numValue == kMultiModeCRT2Connect ? "MultiModeCRT2" :
						numValue == kMultiModeCRT3Connect ? "MultiModeCRT3" :
						numValue == kMultiModeCRT4Connect ? "MultiModeCRT4" :
						numValue == kModelessConnect      ?      "Modeless" :
						numValue == kFullPageConnect      ?      "FullPage" :
						numValue == kVGAConnect           ?           "VGA" :
						numValue == kNTSCConnect          ?          "NTSC" :
						numValue == kPALConnect           ?           "PAL" :
						numValue == kHRConnect            ?            "HR" :
						numValue == kPanelFSTNConnect     ?     "PanelFSTN" :
						numValue == kMonoTwoPageConnect   ?   "MonoTwoPage" :
						numValue == kColorTwoPageConnect  ?  "ColorTwoPage" :
						numValue == kColor16Connect       ?       "Color16" :
						numValue == kColor19Connect       ?       "Color19" :
						numValue == kGenericCRT           ?    "GenericCRT" :
						numValue == kGenericLCD           ?    "GenericLCD" :
						numValue == kDDCConnect           ?           "DDC" :
						numValue == kNoConnect            ?     "NoConnect" :
						UNKNOWN_VALUE(numValue)
					);
					CFDictionaryRemoveValue(IODProperties, CFSTR(kAppleDisplayTypeKey));
				}
			} // AppleDisplayType

			{ // AppleSense
				// https://developer.apple.com/library/archive/technotes/hw/hw_30.html
				num = (CFNumberRef)CFDictionaryGetValue(IODProperties, CFSTR(kAppleSenseKey));
				if (num) {
					CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
					iprintf("AppleSense = %s%s%s,%s%s%d%d/%d%d/%d%d:%s;\n",
						numValue & 0xfffff800 ? UNKNOWN_VALUE(numValue & 0xfffff800) : "",
						numValue & 0xfffff800 ? " " : "",

						// sense 2, sense1, sense 0
						((numValue >> 8) & 7) == kRSCZero  ?  "RSCZero" :
						((numValue >> 8) & 7) == kRSCOne   ?   "RSCOne" :
						((numValue >> 8) & 7) == kRSCTwo   ?   "RSCTwo" :
						((numValue >> 8) & 7) == kRSCThree ? "RSCThree" :
						((numValue >> 8) & 7) == kRSCFour  ?  "RSCFour" :
						((numValue >> 8) & 7) == kRSCFive  ?  "RSCFive" :
						((numValue >> 8) & 7) == kRSCSix   ?   "RSCSix" :
						((numValue >> 8) & 7) == kRSCSeven ? "RSCSeven" : "?",

						numValue & 0xc0                    ?        UNKNOWN_VALUE(numValue & 0xc0) : "",
						numValue & 0xc0                    ?        " " : "",
						numValue & 0x20                    ?          1 : 0, // sense 2 -> sense 1
						numValue & 0x10                    ?          1 : 0, //         -> sense 0
						numValue & 0x08                    ?          1 : 0, // sense 1 -> sense 2
						numValue & 0x04                    ?          1 : 0, //         -> sense 0
						numValue & 0x02                    ?          1 : 0, // sense 0 -> sense 2
						numValue & 0x01                    ?          1 : 0, //         -> sense 1

						(numValue & 0x3f) == kESCZero21Inch            ?            "ESCZero21Inch" :
						(numValue & 0x3f) == kESCOnePortraitMono       ?       "ESCOnePortraitMono" :
						(numValue & 0x3f) == kESCTwo12Inch             ?             "ESCTwo12Inch" :
						(numValue & 0x3f) == kESCThree21InchRadius     ?     "ESCThree21InchRadius" :
						(numValue & 0x3f) == kESCThree21InchMonoRadius ? "ESCThree21InchMonoRadius" :
						(numValue & 0x3f) == kESCThree21InchMono       ?       "ESCThree21InchMono" :
						(numValue & 0x3f) == kESCFourNTSC              ?              "ESCFourNTSC" :
						(numValue & 0x3f) == kESCFivePortrait          ?          "ESCFivePortrait" :
						(numValue & 0x3f) == kESCSixMSB1               ?               "ESCSixMSB1" :
						(numValue & 0x3f) == kESCSixMSB2               ?               "ESCSixMSB2" :
						(numValue & 0x3f) == kESCSixMSB3               ?               "ESCSixMSB3" :
						(numValue & 0x3f) == kESCSixStandard           ?           "ESCSixStandard" :
						(numValue & 0x3f) == kESCSevenPAL              ?              "ESCSevenPAL" :
						(numValue & 0x3f) == kESCSevenNTSC             ?             "ESCSevenNTSC" :
						(numValue & 0x3f) == kESCSevenVGA              ?              "ESCSevenVGA" :
						(numValue & 0x3f) == kESCSeven16Inch           ?           "ESCSeven16Inch" :
						(numValue & 0x3f) == kESCSevenPALAlternate     ?     "ESCSevenPALAlternate" :
						(numValue & 0x3f) == kESCSeven19Inch           ?           "ESCSeven19Inch" :
						(numValue & 0x3f) == kESCSevenDDC              ?              "ESCSevenDDC" :
						(numValue & 0x3f) == kESCSevenNoDisplay        ?        "ESCSevenNoDisplay" :
						UNKNOWN_VALUE(numValue & 0x3f)
					);
					CFDictionaryRemoveValue(IODProperties, CFSTR(kAppleSenseKey));
				}
			} // AppleSense

			iprintf("IODisplay properties (unparsed) = ");
			CFOutput(IODProperties);
			cprintf("; // IODisplay properties (unparsed)\n");

			CFRelease(IODProperties);
		} // if IODProperties
	} // if !IORegistryEntryCreateCFProperties
} // DumpOneIODisplay


void GetIODisplays(io_service_t parent, int &ioDisplayCount, io_service_t *allIODisplayServices)
{
	#define printchildren 0
	char resultStr[40];
	kern_return_t result;
	
	io_iterator_t iterator = MACH_PORT_NULL;
	
	result = IORegistryEntryGetChildIterator(parent, kIOServicePlane, &iterator);
	if (result != KERN_SUCCESS) {
		iprintf("(IORegistryEntryGetChildIterator %s)\n", DumpOneReturn(resultStr, sizeof(resultStr), result));
		return;
	}
	
	io_service_t service;
	for (; IOIteratorIsValid(iterator) && (service = IOIteratorNext(iterator)); IOObjectRelease(service)) {
		bool conforms = IOObjectConformsTo(service, "IODisplay");
		if (conforms) {
			allIODisplayServices[ioDisplayCount++] = service;
			IOObjectRetain(service);
		}

#if printchildren
		io_name_t name;
		name[0] = '\0';
		IORegistryEntryGetName(service, name);
		iprintf("+ %s%s\n", name, conforms ? " (IODisplay)" : ""); INDENT
#endif
		GetIODisplays(service, ioDisplayCount, allIODisplayServices);
#if printchildren
		OUTDENT
#endif
	}
	IOObjectRelease(iterator);
} // GetIODisplays


char * GetServicePath(io_service_t service)
{
	// for an ioregistry service, return an ioregistry path string
	char *ioregPath = (char *)malloc(4096);
	ioregPath[0] = 0;

	char temp[4096];
	kern_return_t kr;
	io_service_t leaf_service = service;
	
	while (service) {
		io_iterator_t parentIterator = 0;
		if (IOObjectConformsTo(service, "IOPlatformExpertDevice"))
		{
			snprintf(temp, sizeof(temp), "%s%s", ioregPath[0] ? "/" : "", ioregPath);
			strcpy(ioregPath, temp);
			kr = KERN_ABORTED; // no more parents
		}
		else
		{
			io_name_t name;
			io_name_t locationInPlane;
			//const char *deviceLocation = NULL, *functionLocation = NULL;
			//unsigned int deviceInt = 0, functionInt = 0;
			int len;
			name[0] = '\0';
			
			IORegistryEntryGetName(service, name);
			if (IORegistryEntryGetLocationInPlane(service, kIOServicePlane, locationInPlane) == KERN_SUCCESS) {
				len = snprintf(temp, sizeof(temp), "%s@%s", name, locationInPlane);
				//deviceLocation = strtok(locationInPlane, ",");
				//functionLocation = strtok(NULL, ",");
				//if (deviceLocation != NULL) deviceInt = (unsigned int)strtol(deviceLocation, NULL, 16);
				//if (functionLocation != NULL) functionInt = (unsigned int)strtol(functionLocation, NULL, 16);
			}
			else {
				len = snprintf(temp, sizeof(temp), "%s", name);
			}
			snprintf(temp + len, sizeof(temp) - len, "%s%s", ioregPath[0] ? "/" : "", ioregPath);
			strcpy(ioregPath, temp);

			kr = IORegistryEntryGetParentIterator(service, kIOServicePlane, &parentIterator);
		} // !IOPlatformExpertDevice
	
		if (service != leaf_service) IOObjectRelease(service);
		if (kr != KERN_SUCCESS)
			break;
		service = IOIteratorNext(parentIterator);
	} // while service
	return ioregPath;
} // GetServicePath


// https://github.com/apple-oss-distributions/IOKitUser/blob/main/graphics.subproj/IODisplayTest.c
void DumpDisplayService(io_service_t displayService, int modeAlias, const char *serviceType)
{
	CFNumberRef         num;
	kern_return_t       kerr;
	SInt32              numValue;
	char                resultStr[40];
	char                path[1000];
	char                *servicePath;

	dumpedServices[dumpedCount++] = displayService;
	IOObjectRetain(displayService);

	CFDictionaryRef dictDisplayInfo0 = IODisplayCreateInfoDictionary(displayService, kNilOptions);
	CFMutableDictionaryRef dictDisplayInfo = NULL;
	CFStringRef location;
	
	if (
		dictDisplayInfo0 && CFDictionaryGetCount(dictDisplayInfo0) == 1
		&& (location = (CFStringRef)CFDictionaryGetValue(dictDisplayInfo0, CFSTR(kIODisplayLocationKey)))
		&& kCFCompareEqualTo == CFStringCompare(location, CFSTR("unknown"), kNilOptions)
	) {
	
	}
	else
	{
		dictDisplayInfo = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, dictDisplayInfo0);
	}
	CFRelease(dictDisplayInfo0);

	if (dictDisplayInfo) { // DisplayInfo
		iprintf("DisplayInfo = {\n"); INDENT

		DumpDisplayInfo(dictDisplayInfo);

		{ // DisplayHorizontalImageSize && DisplayVerticalImageSize
			CFNumberRef h = (CFNumberRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplayHorizontalImageSize));
			CFNumberRef v = (CFNumberRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplayVerticalImageSize));
			if (h || v) {
				SInt32 hval = -1;
				SInt32 vval = -1;
				CFNumberGetValue(h, kCFNumberSInt32Type, &hval);
				CFNumberGetValue(v, kCFNumberSInt32Type, &vval);
				iprintf("DisplayHorizontalImageSize, DisplayVerticalImageSize = %dx%dmm;\n", (int)hval, (int)vval);
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kDisplayHorizontalImageSize));
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kDisplayVerticalImageSize));
			}
		} // DisplayHorizontalImageSize

		{ // DisplaySubPixelLayout
			num = (CFNumberRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplaySubPixelLayout));
			if (num) {
				CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
				iprintf("DisplaySubPixelLayout = %s;\n",
					numValue == kDisplaySubPixelLayoutUndefined ? "Undefined" :
					numValue == kDisplaySubPixelLayoutRGB       ?       "RGB" :
					numValue == kDisplaySubPixelLayoutBGR       ?       "BGR" :
					numValue == kDisplaySubPixelLayoutQuadGBL   ?   "QuadGBL" :
					numValue == kDisplaySubPixelLayoutQuadGBR   ?   "QuadGBR" :
					UNKNOWN_VALUE(numValue)
				);
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kDisplaySubPixelLayout));
			}
		} // DisplaySubPixelLayout
		
		{ // DisplaySubPixelConfiguration
			num = (CFNumberRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplaySubPixelConfiguration));
			if (num) {
				CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
				iprintf("DisplaySubPixelConfiguration = %s;\n",
					numValue == kDisplaySubPixelConfigurationUndefined    ?    "Undefined" :
					numValue == kDisplaySubPixelConfigurationDelta        ?        "Delta" :
					numValue == kDisplaySubPixelConfigurationStripe       ?       "Stripe" :
					numValue == kDisplaySubPixelConfigurationStripeOffset ? "StripeOffset" :
					numValue == kDisplaySubPixelConfigurationQuad         ?         "Quad" :
					UNKNOWN_VALUE(numValue)
				);
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kDisplaySubPixelConfiguration));
			}
		} // DisplaySubPixelConfiguration
		
		{ // DisplaySubPixelShape
			num = (CFNumberRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplaySubPixelShape));
			if (num) {
				CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
				iprintf("DisplaySubPixelShape = %s;\n",
					numValue == kDisplaySubPixelShapeUndefined   ?   "Undefined" :
					numValue == kDisplaySubPixelShapeRound       ?       "Round" :
					numValue == kDisplaySubPixelShapeSquare      ?      "Square" :
					numValue == kDisplaySubPixelShapeRectangular ? "Rectangular" :
					numValue == kDisplaySubPixelShapeOval        ?        "Oval" :
					numValue == kDisplaySubPixelShapeElliptical  ?  "Elliptical" :
					UNKNOWN_VALUE(numValue)
				);
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kDisplaySubPixelShape));
			}
		} // DisplaySubPixelShape
		
		{ // DisplayPixelDimensions
			CFDataRef displayPixelDimensionsData = (CFDataRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR("DisplayPixelDimensions"));
			typedef struct DisplayPixelDimensionsRec {
				UInt32 width;
				UInt32 height;
			} DisplayPixelDimensionsRec;
			
			if (displayPixelDimensionsData) {
				if (CFDataGetLength(displayPixelDimensionsData) != sizeof(DisplayPixelDimensionsRec)) {
					iprintf("DisplayPixelDimensions = unexpected size %ld\n", CFDataGetLength(displayPixelDimensionsData));
				}
				else {
					DisplayPixelDimensionsRec *p = (DisplayPixelDimensionsRec *)CFDataGetBytePtr(displayPixelDimensionsData);
					iprintf("DisplayPixelDimensions = %dx%d;\n", CFSwapInt32BigToHost(p->width), CFSwapInt32BigToHost(p->height));
					CFDictionaryRemoveValue(dictDisplayInfo, CFSTR("DisplayPixelDimensions"));
				}
			}
		} // DisplayPixelDimensions

		{ // IOFBTransform
			num = (CFNumberRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kIOFBTransformKey));
			if (num) {
				CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
				iprintf("IOFBTransform = ");
				DumpOneTransform(numValue);
				cprintf(";\n");
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kIOFBTransformKey));
			}
		} // IOFBTransform
		
		{ // graphic-options
			num = (CFNumberRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR("graphic-options"));
			if (num) {
				CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
				iprintf("graphic-options = %s%s%s%s%s%s%s;\n",
						numValue == kIOMirrorDefault        ?           "MirrorDefault," : "",
						numValue == kIOMirrorForced         ?            "MirrorForced," : "",
						numValue == kIOGPlatformYCbCr       ?          "GPlatformYCbCr," : "",
						numValue == kIOFBDesktopModeAllowed ?    "FBDesktopModeAllowed," : "",
						numValue == kIOMirrorNoAutoHDMI     ?        "MirrorNoAutoHDMI," : "",
						numValue == kIOMirrorHint           ?              "MirrorHint," : "",
						UNKNOWN_FLAG(numValue & 0xfffeffe0)
				);
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR("graphic-options"));
			}
		} // graphic-options

		{ // Display RGBWhite Point
			#define POINT1(C,X) \
				float val_ ## C ## Point ## X = 0.0f; \
				CFNumberRef num_ ## C ## Point ## X = (CFNumberRef)CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplay ## C ## Point ## X)); \
				char str_ ## C ## Point ## X[20] = {}; \
				if (num_ ## C ## Point ## X) { \
					CFNumberGetValue(num_ ## C ## Point ## X, kCFNumberFloatType, &val_ ## C ## Point ## X); \
					CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kDisplay ## C ## Point ## X)); \
					snprintf(str_ ## C ## Point ## X, sizeof(str_ ## C ## Point ## X), "%.7g", val_ ## C ## Point ## X); \
				}
			#define POINT(C) POINT1(C,X); POINT1(C,Y);

			POINT(Red);
			POINT(Green);
			POINT(Blue);
			POINT(White);

			if (num_RedPointX || num_RedPointY || num_GreenPointX || num_GreenPointY || num_BluePointX || num_BluePointY || num_WhitePointX || num_WhitePointY) {
				iprintf("Display Red, Green, Blue, White Point (X,Y) = R(%s,%s), G(%s,%s), B(%s,%s), W(%s,%s);\n",
					str_RedPointX, str_RedPointY, str_GreenPointX, str_GreenPointY, str_BluePointX, str_BluePointY, str_WhitePointX, str_WhitePointY);
			}
		} // Display RGBWhite Point
		
		{ // scale-resolutions
			// https://github.com/apple-oss-distributions/IOKitUser/blob/IOKitUser-1445.71.1/graphics.subproj/IOGraphicsLib.c#L3696
			
			const char *scaleResTypes[] = { "scale-resolutions", "scale-resolutions-4k" };
			CFStringRef scaleResTypesCF[] = { CFSTR("scale-resolutions"), CFSTR("scale-resolutions-4k") };
			for (int j = 0; j < 2; j++) {
				CFArrayRef scaleResolutions = (CFArrayRef)CFDictionaryGetValue(dictDisplayInfo, scaleResTypesCF[j]);
				if (scaleResolutions && CFArrayGetTypeID() == CFGetTypeID(scaleResolutions) && CFArrayGetCount(scaleResolutions)) {
					iprintf("%s = {\n", scaleResTypes[j]); INDENT
					CFIndex count = CFArrayGetCount(scaleResolutions);
					for (int i = 0; i < count; i++) {
						CFDataRef scaleResolution = (CFDataRef)CFArrayGetValueAtIndex(scaleResolutions, i);
						if (scaleResolution) {
							bool isUnknownStruct = true;
							if (CFDataGetTypeID() == CFGetTypeID(scaleResolution)) {
								CFIndex len = CFDataGetLength(scaleResolution);
								if (len >= 4) {
									isUnknownStruct = false;
									const UInt32 *value = (UInt32 *)CFDataGetBytePtr(scaleResolution);
									if (len < 8 || value[1] == 0) {
										iprintf("h:%u", CFSwapInt32BigToHost(value[0]));
									}
									else {
										iprintf("%ux%u", CFSwapInt32BigToHost(value[0]), CFSwapInt32BigToHost(value[1]));
									}
									if (len >= 12) {
										numValue = CFSwapInt32BigToHost(value[2]);
										cprintf(" install:%s%s%s%s%s",
											numValue & kScaleInstallAlways         ?         "always," : "",
											numValue & kScaleInstallNoStretch      ?      "noStretch," : "",
											numValue & kScaleInstallNoResTransform ? "noResTransform," : "",
											numValue & kScaleInstallMirrorDeps     ?     "mirrorDeps," : "",
											UNKNOWN_FLAG(numValue & 0xfffffff0)
										);
									}
									if (len >= 16) {
										numValue = CFSwapInt32BigToHost(value[3]);
										char * flagsstr = GetOneFlagsStr(numValue);
										cprintf(" setModeFlags:%s", flagsstr);
										free(flagsstr);
									}
									if (len >= 20) {
										numValue = CFSwapInt32BigToHost(value[4]);
										char * flagsstr = GetOneFlagsStr(numValue);
										cprintf(" clrModeFlags:%s", flagsstr);
										free(flagsstr);
									}
									CFIndex bytestart = (len / 4) * 4;
									if (len > bytestart) {
										cprintf(" unknown:");
										uint8_t *bytep = (uint8_t *)value;
										bytep += bytestart;
										for (; len > bytestart; len--) {
											cprintf("%02x", *bytep++);
										}
									}
									cprintf("\n");
								}
							}
							else if (CFNumberGetTypeID() == CFGetTypeID(scaleResolution)) {
								isUnknownStruct = false;
								num = (CFNumberRef)scaleResolution;
								CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
								if ((uint32_t)numValue >> 16 == 0) {
									iprintf("h:%u", (uint32_t)(UInt16)numValue);
								}
								else {
									iprintf("%ux%u", (uint32_t)(UInt16)numValue, (uint32_t)numValue >> 16);
								}
							}
							if (isUnknownStruct) {
								iprintf("");
								CFOutput(scaleResolution);
								cprintf("\n");
							}
						}
					}
					OUTDENT iprintf("}; // %s\n", scaleResTypes[j]);
					CFDictionaryRemoveValue(dictDisplayInfo, scaleResTypesCF[j]);
				}
			} // for
		} // scale-resolutions
		
		{ // dspc
			// https://github.com/apple-oss-distributions/IOKitUser/blob/IOKitUser-1445.71.1/graphics.subproj/IOGraphicsLib.c#L3284
			CFArrayRef dspc = (CFArrayRef)CFDictionaryGetValue( dictDisplayInfo, CFSTR("dspc") );
			if (dspc && CFArrayGetTypeID() == CFGetTypeID(dspc) && CFArrayGetCount(dspc)) {
				iprintf("dspc = { // override timing recs (18-byte)\n"); INDENT
				
				EDID edid;
				bzero(&edid, sizeof(edid));
				
				CFIndex itemCount = CFArrayGetCount(dspc);
				for( int i = 0; i < itemCount; i++ ) {
					EDIDDetailedTimingDesc * dspcRec = NULL;
					CFDataRef modedspc = (CFDataRef)CFArrayGetValueAtIndex(dspc, i);
					if (modedspc && CFGetTypeID(modedspc) == CFDataGetTypeID() && CFDataGetLength(modedspc) == sizeof(EDIDDetailedTimingDesc)) {
						dspcRec = (EDIDDetailedTimingDesc *) CFDataGetBytePtr(modedspc);
						IODetailedTimingInformationV2_12 timing;
						if (kIOReturnSuccess == EDIDDescToDetailedTiming( &edid, dspcRec, &timing )) {
							char timinginfo[1000];
							iprintf("{ %s }", DumpOneDetailedTimingInformationPtrFromEdid(timinginfo, sizeof(timinginfo), &timing));
						}
						else {
							dspcRec = NULL;
						}
					}
					if (!dspcRec) {
						iprintf("");
						CFOutput(modedspc);
					}
					cprintf("\n");
				} // for
				OUTDENT iprintf("}; // dspc\n");
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR("dspc"));
			} // if
		} // dspc

		{ // drng
			// https://github.com/apple-oss-distributions/IOKitUser/blob/IOKitUser-1445.71.1/graphics.subproj/IODisplayLib.c#L3787
			CFDataRef drng = (CFDataRef)CFDictionaryGetValue( dictDisplayInfo, CFSTR("drng") );
			if (drng && CFDataGetTypeID() == CFGetTypeID(drng) && CFDataGetLength(drng) == sizeof(EDIDGeneralDesc)) {
				EDID edid;
				bzero(&edid, sizeof(edid));
				edid.version = 1;
				edid.revision = 1;

				EDIDGeneralDesc * drngRec = (EDIDGeneralDesc *) CFDataGetBytePtr(drng);
				IODisplayTimingRangeV1_12 displayRange;
			
				if( EDIDDescToDisplayTimingRangeRec( &edid, drngRec, &displayRange )) {
					char timinginfo[1000];
					iprintf("drng = { %s };\n", DumpOneDisplayTimingRangeFromEdid(timinginfo, sizeof(timinginfo), &displayRange));
					CFDictionaryRemoveValue(dictDisplayInfo, CFSTR("drng"));
				}
			} // if
		} // drng

		{ // trng
			// https://github.com/apple-oss-distributions/IOKitUser/blob/IOKitUser-1445.71.1/graphics.subproj/IOGraphicsLib.c#L3284
			CFDataRef trng = (CFDataRef)CFDictionaryGetValue( dictDisplayInfo, CFSTR("trng") );
			if (trng && CFDataGetTypeID() == CFGetTypeID(trng)) {
				iprintf("trng = { ");
				DumpOneDisplayTimingRange(trng);
				cprintf(" };\n");
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR("trng"));
			} // if
		} // trng

		{ // tinf
			// https://github.com/apple-oss-distributions/IOKitUser/blob/IOKitUser-1445.71.1/graphics.subproj/IOGraphicsLib.c#L1126
			CFDictionaryRef tinf = (CFDictionaryRef)CFDictionaryGetValue( dictDisplayInfo, CFSTR("tinf") );
			if (tinf && CFDictionaryGetTypeID() == CFGetTypeID(tinf) && CFDictionaryGetCount(tinf)) {
				iprintf("tinf = { // timing info\n"); INDENT
				
				CFIndex itemCount = CFDictionaryGetCount(tinf);
				
				const void **keys   = (const void **)malloc(itemCount * sizeof(void*));
				const void **values = (const void **)malloc(itemCount * sizeof(void*));
				CFDictionaryGetKeysAndValues(tinf, keys, values);
				
				for (int i = 0; i < itemCount; i++) {
					char key[50];
					SInt32 appleTimingID = 0;
					bool gotIt = false;
					if (CFGetTypeID(keys[i]) == CFNumberGetTypeID()) {
						gotIt = CFNumberGetValue((CFNumberRef)keys[i], kCFNumberSInt32Type, &appleTimingID);
					}
					else if (CFGetTypeID(keys[i]) == CFStringGetTypeID()) {
						gotIt = (CFStringGetCString((CFStringRef)keys[i], key, sizeof(key), kCFStringEncodingUTF8));
						if (gotIt) {
							int tempnum;
							if (sscanf(key, "%d", &tempnum)) {
								appleTimingID = tempnum;
							} else {
								gotIt = false;
							}
						}
					}
					if (gotIt) {
						iprintf("%3d:%-21s = ", (int)appleTimingID, GetOneAppleTimingID(appleTimingID));
					} else {
						iprintf("");
						CFOutput(keys[i]);
						cprintf(" = ");
					}
					
					CFDataRef modetinf = (CFDataRef)values[i];
					DMDisplayTimingInfoRec * tinfRec = NULL;
					if (modetinf && CFGetTypeID(modetinf) == CFDataGetTypeID()) {
						tinfRec = (DMDisplayTimingInfoRec *) CFDataGetBytePtr(modetinf);
					}
					if (tinfRec) {
						cprintf("{");
						CFIndex len = CFDataGetLength(modetinf);
						if (len >= 4) {
							numValue = CFSwapInt32BigToHost(tinfRec->timingInfoVersion);
							if (numValue) cprintf(" version:%d", (int)numValue);
						}
						if (len >= 8) {
							numValue = CFSwapInt32BigToHost(tinfRec->timingInfoAttributes);
							if (numValue) cprintf(" attributes:0x%x", (int)numValue);
						}
						if (len >= 12) {
							numValue = CFSwapInt32BigToHost(tinfRec->timingInfoRelativeQuality);
							if (numValue) cprintf(" relativeQuality:%d", (int)numValue);
						}
						if (len >= 16) {
							numValue = CFSwapInt32BigToHost(tinfRec->timingInfoRelativeDefault);
							if (numValue) cprintf(" relativeDefault:%d", (int)numValue);
						}
						CFIndex bytestart = (len / 4) * 4;
						if (len > bytestart) {
							cprintf(" unknown:");
							uint8_t *bytep = (uint8_t *)tinfRec;
							bytep += bytestart;
							for (; len > bytestart; len--) {
								cprintf("%02x", *bytep++);
							}
						}
						cprintf(" }");
					}
					else {
						CFOutput(modetinf);
					}
					cprintf("\n");
				} // for
				OUTDENT iprintf("}; // tinf\n");
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR("tinf"));
			} // if
		} // tinf

		{ // tovr
			// https://github.com/apple-oss-distributions/IOKitUser/blob/IOKitUser-1445.71.1/graphics.subproj/IOGraphicsLib.c#L1126
			CFDictionaryRef tovr = (CFDictionaryRef)CFDictionaryGetValue( dictDisplayInfo, CFSTR("tovr") );
			if (tovr && CFDictionaryGetTypeID() == CFGetTypeID(tovr) && CFDictionaryGetCount(tovr)) {
				iprintf("tovr = { // timing overrides\n"); INDENT
				
				CFIndex itemCount = CFDictionaryGetCount(tovr);
				
				const void **keys   = (const void **)malloc(itemCount * sizeof(void*));
				const void **values = (const void **)malloc(itemCount * sizeof(void*));
				CFDictionaryGetKeysAndValues(tovr, keys, values);
				
				for (int i = 0; i < itemCount; i++) {
					char key[50];
					SInt32 appleTimingID = 0;
					bool gotIt = false;
					if (CFGetTypeID(keys[i]) == CFNumberGetTypeID()) {
						gotIt = CFNumberGetValue((CFNumberRef)keys[i], kCFNumberSInt32Type, &appleTimingID);
					}
					else if (CFGetTypeID(keys[i]) == CFStringGetTypeID()) {
						gotIt = (CFStringGetCString((CFStringRef)keys[i], key, sizeof(key), kCFStringEncodingUTF8));
						if (gotIt) {
							int tempnum;
							if (sscanf(key, "%d", &tempnum)) {
								appleTimingID = tempnum;
							} else {
								gotIt = false;
							}
						}
					}
					if (gotIt) {
						iprintf("%3d:%-21s = ", (int)appleTimingID, GetOneAppleTimingID(appleTimingID));
					} else {
						iprintf("");
						CFOutput(keys[i]);
						cprintf(" = ");
					}
					
					CFDataRef modetovr = (CFDataRef)values[i];
					DMTimingOverrideRec * tovrRec = NULL;
					if (modetovr && CFGetTypeID(modetovr) == CFDataGetTypeID()) {
						tovrRec = (DMTimingOverrideRec *) CFDataGetBytePtr(modetovr);
					}
					if (tovrRec) {
						cprintf("{");
						CFIndex len = CFDataGetLength(modetovr);
						if (len >= 4) {
							numValue = CFSwapInt32BigToHost(tovrRec->timingOverrideVersion);
							if (numValue) cprintf(" version:%d", (int)numValue);
						}
						if (len >= 8) {
							numValue = CFSwapInt32BigToHost(tovrRec->timingOverrideAttributes);
							if (numValue) cprintf(" attributes:0x%x", (int)numValue);
						}
						if (len >= 12) {
							numValue = CFSwapInt32BigToHost(tovrRec->timingOverrideSetFlags);
							if (numValue) {
								char * flagsstr = GetOneFlagsStr(numValue);
								cprintf(" setFlags:%s", flagsstr);
								free(flagsstr);
							}
						}
						if (len >= 16) {
							numValue = CFSwapInt32BigToHost(tovrRec->timingOverrideClearFlags);
							if (numValue) {
								char * flagsstr = GetOneFlagsStr(numValue);
								cprintf(" clearFlags:%s", flagsstr);
								free(flagsstr);
							}
						}
						CFIndex bytestart = (len / 4) * 4;
						if (len > bytestart) {
							cprintf(" unknown:");
							uint8_t *bytep = (uint8_t *)tovrRec;
							bytep += bytestart;
							for (; len > bytestart; len--) {
								cprintf("%02x", *bytep++);
							}
						}
						cprintf(" }");
					}
					else {
						CFOutput(modetovr);
					}
					cprintf("\n");
				} // for
				OUTDENT iprintf("}; // tovr\n");
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR("tovr"));
			} // if
		} // tovr

		iprintf("DisplayInfo properties (unparsed) = ");
		CFOutput(dictDisplayInfo);
		cprintf("; // DisplayInfo properties (unparsed)\n");

		OUTDENT iprintf("}; // DisplayInfo\n\n");
		CFRelease(dictDisplayInfo);
	} // DisplayInfo

	io_service_t ioDisplayService = 0;
	if (GetIODisplayForFramebufferPtr()) {
		// IODisplayForFramebuffer is not named in 10.5 SDK but does exist in Mac OS X 10.5.8 but it's not external.
		ioDisplayService = (GetIODisplayForFramebufferPtr())( displayService, kNilOptions );
	}
	
	int ioDisplayCount = 0;
	io_service_t allIODisplayServices[20];
	
	GetIODisplays(displayService, ioDisplayCount, allIODisplayServices);

	{
		CFDictionaryRef dictDisplayParameters = NULL;
		if (KERN_SUCCESS == IODisplayCopyParameters(displayService, kNilOptions, &dictDisplayParameters)) {
			// same as IODisplay/IODisplayParameters
			DumpDisplayParameters("IODisplayCopyParameters for IOFramebuffer", dictDisplayParameters);
			cprintf("\n");
			CFRelease(dictDisplayParameters);
		}
		if (ioDisplayService) {
			if (KERN_SUCCESS == IODisplayCopyParameters(ioDisplayService, 0xffffffff, &dictDisplayParameters)) {
				// same as IODisplay/IODisplayParameters
				DumpDisplayParameters("IODisplayCopyParameters for IODisplay", dictDisplayParameters);
				cprintf("\n");
				CFRelease(dictDisplayParameters);
			}
		}
		for (int i = 0; i < ioDisplayCount; i++) {
			if (allIODisplayServices[i] != ioDisplayService && KERN_SUCCESS == IODisplayCopyParameters(allIODisplayServices[i], 0xffffffff, &dictDisplayParameters)) {
				servicePath = GetServicePath(allIODisplayServices[i]);
				snprintf(path, sizeof(path), "IODisplayCopyParameters for IODisplay: %s", servicePath ? servicePath : "");
				DumpDisplayParameters(path, dictDisplayParameters);
				cprintf("\n");
				CFRelease(dictDisplayParameters);
				if (servicePath) free (servicePath);
			}
		}
	}

	
	{
		CFDictionaryRef dictDisplayFloatParameters = NULL;
		if (KERN_SUCCESS == IODisplayCopyFloatParameters(displayService, 0xffffffff, &dictDisplayFloatParameters)) {
			// same as IODisplay/IODisplayParameters
			DumpDisplayParameters("IODisplayCopyFloatParameters for IOFramebuffer", dictDisplayFloatParameters, true);
			cprintf("\n");
			CFRelease(dictDisplayFloatParameters);
		}
		if (ioDisplayService) {
			if (KERN_SUCCESS == IODisplayCopyFloatParameters(ioDisplayService, 0xffffffff, &dictDisplayFloatParameters)) {
				// same as IODisplay/IODisplayParameters
				DumpDisplayParameters("IODisplayCopyFloatParameters for IODisplay", dictDisplayFloatParameters, true);
				cprintf("\n");
				CFRelease(dictDisplayFloatParameters);
			}
		}
		for (int i = 0; i < ioDisplayCount; i++) {
			if (allIODisplayServices[i] != ioDisplayService && KERN_SUCCESS == IODisplayCopyFloatParameters(allIODisplayServices[i], 0xffffffff, &dictDisplayFloatParameters)) {
				servicePath = GetServicePath(allIODisplayServices[i]);
				snprintf(path, sizeof(path), "IODisplayCopyFloatParameters for IODisplay: %s", servicePath ? servicePath : "");
				DumpDisplayParameters(path, dictDisplayFloatParameters);
				cprintf("\n");
				CFRelease(dictDisplayFloatParameters);
				if (servicePath) free (servicePath);
			}
		}
	}

	{
		DumpAllParameters("IODisplayGet*Parameter for IOFramebuffer", displayService);

		if (ioDisplayService) {
			DumpAllParameters("IODisplayGet*Parameter for IODisplay", ioDisplayService);
		}

		for (int i = 0; i < ioDisplayCount; i++) {
			if (allIODisplayServices[i] != ioDisplayService) {
				servicePath = GetServicePath(allIODisplayServices[i]);
				snprintf(path, sizeof(path), "IODisplayGet*Parameter for IODisplay: %s", servicePath ? servicePath : "");
				DumpAllParameters(path, allIODisplayServices[i]);
				if (servicePath) free (servicePath);
			}
		}
	}

#if 0
√	IODisplayForFramebuffer

√	IODisplayCopyParameters
√	IODisplayCopyFloatParameters

√	IODisplayGetFloatParameter
√	IODisplayGetIntegerRangeParameter

	IODisplaySetFloatParameter
	IODisplaySetIntegerParameter

	IODisplaySetParameters
	IODisplayCommitParameters

#endif


	iprintf("%s 0x%0x = {\n", serviceType, displayService); INDENT
	{
		CFMutableDictionaryRef IOFBProperties = NULL;
		if (!IORegistryEntryCreateCFProperties(displayService, &IOFBProperties, kCFAllocatorDefault, kNilOptions)) {
			if (IOFBProperties) {
				
				{ // IOFBConfig
					CFMutableDictionaryRef IOFBConfig = NULL;
					CFDictionaryRef IOFBConfig0 = (CFDictionaryRef)CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBConfigKey));
					if (IOFBConfig0) IOFBConfig = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, IOFBConfig0);
					if (IOFBConfig) {
						iprintf("IOFBConfig = {\n"); INDENT
						
						{ // IOFBModes
							CFMutableArrayRef IOFBModes = NULL;
							CFArrayRef IOFBModes0 = (CFArrayRef)CFDictionaryGetValue(IOFBConfig, CFSTR(kIOFBModesKey));
							if (IOFBModes0) IOFBModes = CFArrayCreateMutableCopy(kCFAllocatorDefault, 0, IOFBModes0);
							if (IOFBModes && CFArrayGetCount(IOFBModes)) {
								iprintf("IOFBModes = {\n"); INDENT
								detailedTimingsCount = CFArrayGetCount(IOFBModes);
								detailedTimingsArr = (IODetailedTimingInformationV2 *)malloc(detailedTimingsCount * sizeof(*detailedTimingsArr));
								if (detailedTimingsArr) { bzero(detailedTimingsArr, detailedTimingsCount * sizeof(*detailedTimingsArr)); }
								for (int i = 0, j = 0; i < detailedTimingsCount; i++) {
									CFMutableDictionaryRef IOFBMode = NULL;
									CFDictionaryRef IOFBMode0 = (CFDictionaryRef)CFArrayGetValueAtIndex(IOFBModes, j);
									if (IOFBMode0) IOFBMode = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, IOFBMode0);
									iprintf("{");
									if (IOFBMode) {
										CFTypeRef val;
										{ val = CFDictionaryGetValue(IOFBMode, CFSTR(kIOFBModeTMKey )); if (val) { cprintf(" DetailedTimingInformation = { "); DumpOneDetailedTimingInformation((CFDataRef)val, -1, modeAlias); cprintf(" };");
											memcpy(&detailedTimingsArr[i], CFDataGetBytePtr((CFDataRef)val), MIN(sizeof(*detailedTimingsArr), CFDataGetLength((CFDataRef)val)));                                              CFDictionaryRemoveValue(IOFBMode, CFSTR(kIOFBModeTMKey )); } }
										{ val = CFDictionaryGetValue(IOFBMode, CFSTR(kIOFBModeDMKey )); if (val) { cprintf(   " DisplayModeInformation = { "); DumpOneDisplayModeInformation((CFDataRef)val); cprintf(" };"); CFDictionaryRemoveValue(IOFBMode, CFSTR(kIOFBModeDMKey )); } }
										{ val = CFDictionaryGetValue(IOFBMode, CFSTR(kIOFBModeAIDKey)); if (val) { cprintf(              " AppleTimingID = "); DumpOneAppleID               ((CFNumberRef)val); cprintf(";"); CFDictionaryRemoveValue(IOFBMode, CFSTR(kIOFBModeAIDKey)); } }
										{ val = CFDictionaryGetValue(IOFBMode, CFSTR(kIOFBModeDFKey )); if (val) { cprintf(                " DriverFlags = "); DumpOneFlags                 ((CFNumberRef)val); cprintf(";"); CFDictionaryRemoveValue(IOFBMode, CFSTR(kIOFBModeDFKey )); } }
	//									{ val = CFDictionaryGetValue(IOFBMode, CFSTR(kIOFBModePIKey));  if (val) { dumpproc((void*)val); } }
	//									{ val = CFDictionaryGetValue(IOFBMode, CFSTR(kIOFBModeDMKey));  if (val) { dumpproc((void*)val); } }
										{ val = CFDictionaryGetValue(IOFBMode, CFSTR(kIOFBModeIDKey));  if (val) { cprintf(                         " ID = "); DumpOneID         ((CFNumberRef)val, modeAlias); cprintf(";"); if (CFDictionaryGetCount(IOFBMode) == 1) { CFDictionaryRemoveValue(IOFBMode, CFSTR(kIOFBModeIDKey)); } } }
										if (CFDictionaryGetCount(IOFBMode) > 0) {
											CFArraySetValueAtIndex(IOFBModes, j++, IOFBMode);
										}
										else {
											CFArrayRemoveValueAtIndex(IOFBModes, j);
										}
									}
									cprintf(" },\n");
								} // for
								OUTDENT iprintf("}; // IOFBModes\n");
								if (CFArrayGetCount(IOFBModes)) {
									CFDictionarySetValue(IOFBConfig, CFSTR(kIOFBModesKey), IOFBModes);
								}
								else {
									CFDictionaryRemoveValue(IOFBConfig, CFSTR(kIOFBModesKey));
								}
							} // if IOFBModes
							
						} // IOFBModes
						
						{ // IOFBDetailedTimings
							CFMutableArrayRef IOFBDetailedTimings = NULL;
							CFArrayRef IOFBDetailedTimings0 = (CFArrayRef)CFDictionaryGetValue(IOFBConfig, CFSTR(kIOFBDetailedTimingsKey));
							if (IOFBDetailedTimings0) IOFBDetailedTimings = CFArrayCreateMutableCopy(kCFAllocatorDefault, 0, IOFBDetailedTimings0);
							if (IOFBDetailedTimings && CFArrayGetCount(IOFBDetailedTimings)) {
								iprintf("IOFBDetailedTimings = {\n"); INDENT
								int dups = 0;
								CFIndex count = CFArrayGetCount(IOFBDetailedTimings);
								for (int i = 0, j = 0; i < count; i++) {
									CFDataRef IOFBDetailedTiming = (CFDataRef)CFArrayGetValueAtIndex(IOFBDetailedTimings, j);
									if (IOFBDetailedTiming) {
										dups += DumpOneDetailedTimingInformation(IOFBDetailedTiming, i, modeAlias) ? 1 : 0;
									}
									CFArrayRemoveValueAtIndex(IOFBDetailedTimings, j);
								}
								if (dups) {
									iprintf("%d duplicates\n", dups);
								}
								OUTDENT iprintf("}; // IOFBDetailedTimings\n");
								if (CFArrayGetCount(IOFBDetailedTimings)) {
									CFDictionarySetValue(IOFBConfig, CFSTR(kIOFBDetailedTimingsKey), IOFBDetailedTimings);
								}
								else {
									CFDictionaryRemoveValue(IOFBConfig, CFSTR(kIOFBDetailedTimingsKey));
								}
							}
						} // IOFBDetailedTimings

						{ // IOFB0Hz (suppressRefresh)
							num = (CFNumberRef)CFDictionaryGetValue(IOFBConfig, CFSTR("IOFB0Hz"));
							if (num) {
								CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
								iprintf("IOFB0Hz (suppressRefresh) = %s;\n", numValue == 1 ? "true" : numValue == 0 ? "false" : UNKNOWN_VALUE(numValue));
							}
							CFDictionaryRemoveValue(IOFBConfig, CFSTR("IOFB0Hz"));
						} // IOFB0Hz (suppressRefresh)

						{ // IOFBmHz (detailedRefresh)
							num = (CFNumberRef)CFDictionaryGetValue(IOFBConfig, CFSTR("IOFBmHz"));
							if (num) {
								CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
								iprintf("IOFBmHz (detailedRefresh) = %s;\n", numValue == 1 ? "true" : numValue == 0 ? "false" : UNKNOWN_VALUE(numValue));
								CFDictionaryRemoveValue(IOFBConfig, CFSTR("IOFBmHz"));
							}
						} // IOFBmHz (detailedRefresh)

						{ // IOFBmir (displayMirror)
							num = (CFNumberRef)CFDictionaryGetValue(IOFBConfig, CFSTR("IOFBmir"));
							if (num) {
								CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
								iprintf("IOFBmir (displayMirror) = %s\n", numValue == 1 ? "true" : numValue == 0 ? "false" : UNKNOWN_VALUE(numValue));
								CFDictionaryRemoveValue(IOFBConfig, CFSTR("IOFBmir"));
							}
						} // IOFBmir (displayMirror)

						{ // IOFBScalerUnderscan (useScalerUnderscan)
							num = (CFNumberRef)CFDictionaryGetValue(IOFBConfig, CFSTR("IOFBScalerUnderscan"));
							if (num) {
								CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
								iprintf("IOFBScalerUnderscan (useScalerUnderscan) = %s;\n", numValue == 1 ? "true" : numValue == 0 ? "false" : UNKNOWN_VALUE(numValue));
								CFDictionaryRemoveValue(IOFBConfig, CFSTR("IOFBScalerUnderscan"));
							}
						} // IOFBScalerUnderscan (useScalerUnderscan)

						{ // IOFBtv (addTVFlag)
							num = (CFNumberRef)CFDictionaryGetValue(IOFBConfig, CFSTR("IOFBtv"));
							if (num) {
								CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
								iprintf("IOFBtv (addTVFlag) = %s;\n", numValue == 1 ? "true" : numValue == 0 ? "false" : UNKNOWN_VALUE(numValue));
								CFDictionaryRemoveValue(IOFBConfig, CFSTR("IOFBtv"));
							}
						} // IOFBtv (addTVFlag)

						{ // dims (IOFBOvrDimensions)
							CFDataRef IOFBOvrDimensionsData = (CFDataRef)CFDictionaryGetValue(IOFBConfig, CFSTR("dims"));
							if (IOFBOvrDimensionsData) {
								if (CFDataGetLength(IOFBOvrDimensionsData) != sizeof(IOFBOvrDimensions)) {
									iprintf("dims (IOFBOvrDimensions) = unexpected size %ld\n", CFDataGetLength(IOFBOvrDimensionsData));
								}
								else {
									IOFBOvrDimensions *p = (IOFBOvrDimensions *)CFDataGetBytePtr(IOFBOvrDimensionsData);
									char *flagsstr1 = GetOneFlagsStr(p->setFlags);
									char *flagsstr2 = GetOneFlagsStr(p->clearFlags);
									iprintf("dims (IOFBOvrDimensions) = %dx%d setFlags:%s clearFlags:%s;\n",
										(int)(SInt32)p->width, (int)(SInt32)p->height, flagsstr1, flagsstr2
									);
									free(flagsstr1);
									free(flagsstr2);
									CFDictionaryRemoveValue(IOFBConfig, CFSTR("dims"));
								}
							}
						} // dims (IOFBOvrDimensions)


						{ // DM (IODisplayModeInformation)
							CFDataRef IOFBModeDMData = (CFDataRef)CFDictionaryGetValue(IOFBConfig, CFSTR(kIOFBModeDMKey));
							if (IOFBModeDMData) {
								iprintf("DM (IODisplayModeInformation) = { ");
								DumpOneDisplayModeInformation(IOFBModeDMData);
								cprintf(" };\n");
								CFDictionaryRemoveValue(IOFBConfig, CFSTR(kIOFBModeDMKey));
							}
						} // dims (IOFBModeDM)

						if (CFDictionaryGetCount(IOFBConfig)) {
							CFDictionarySetValue(IOFBProperties, CFSTR(kIOFBConfigKey), IOFBConfig);
						}
						else {
							CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBConfigKey));
						}
						OUTDENT iprintf("}; // IOFBConfig\n");

					} // if IOFBConfig
				} // IOFBConfig

				{ // IOFBCursorInfo
					CFArrayRef IOFBCursorInfo = (CFArrayRef)CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBCursorInfoKey));
					if (IOFBCursorInfo) {
						CFMutableDictionaryRef IOFBCursorInfoNew = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL, NULL);
						CFIndex itemCount = CFArrayGetCount(IOFBCursorInfo);
						if (itemCount) {
							iprintf("IOFBCursorInfo = {\n"); INDENT
							for (int i = 0; i < itemCount; i++) {
								bool good = false;
								CFDataRef IOFBOneCursorInfo = (CFDataRef)CFArrayGetValueAtIndex(IOFBCursorInfo, i);
								if (IOFBOneCursorInfo && CFGetTypeID(IOFBOneCursorInfo) == CFDataGetTypeID()) {
									iprintf("[%d] = ", i);
									good = DumpOneCursorInfo(IOFBOneCursorInfo, i);
									cprintf("\n");
								}
								if (!good) {
									if (IOFBOneCursorInfo) CFRetain(IOFBOneCursorInfo);
									//CFStringRef arrayIndex = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("[%d]"), i);
									CFNumberRef arrayIndex = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &i);
									CFDictionaryAddValue(IOFBCursorInfoNew, arrayIndex, IOFBOneCursorInfo);
								}
							}
							OUTDENT iprintf("}; // IOFBCursorInfo\n");
							
							if (CFDictionaryGetCount(IOFBCursorInfoNew)) {
								CFDictionarySetValue(IOFBProperties, CFSTR(kIOFBCursorInfoKey), IOFBCursorInfoNew);
							}
							else {
								CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBCursorInfoKey));
							}
						} // if itemCount
					} // if IOFBCursorInfo
				} // IOFBCursorInfo

				{ // IOFBDetailedTimings
					CFMutableArrayRef IOFBDetailedTimings = NULL;
					CFArrayRef IOFBDetailedTimings0 = (CFArrayRef)CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBDetailedTimingsKey));
					if (IOFBDetailedTimings0) IOFBDetailedTimings = CFArrayCreateMutableCopy(kCFAllocatorDefault, 0, IOFBDetailedTimings0);
					if (IOFBDetailedTimings && CFArrayGetCount(IOFBDetailedTimings)) {
						iprintf("IOFBDetailedTimings = {\n"); INDENT
						int dups = 0;
						CFIndex count = CFArrayGetCount(IOFBDetailedTimings);
						for (int i = 0, j = 0; i < count; i++) {
							CFDataRef IOFBDetailedTiming = (CFDataRef)CFArrayGetValueAtIndex(IOFBDetailedTimings, j);
							if (IOFBDetailedTiming) {
								dups += DumpOneDetailedTimingInformation(IOFBDetailedTiming, i, modeAlias) ? 1 : 0;
							}
							CFArrayRemoveValueAtIndex(IOFBDetailedTimings, j);
						}
						if (dups) {
							iprintf("%d duplicates\n", dups);
						}
						OUTDENT iprintf("}; // IOFBDetailedTimings\n");
						if (CFArrayGetCount(IOFBDetailedTimings)) {
							CFDictionarySetValue(IOFBProperties, CFSTR(kIOFBDetailedTimingsKey), IOFBDetailedTimings);
						}
						else {
							CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBDetailedTimingsKey));
						}
					}
				} // IOFBDetailedTimings

				{ // IOFBInvalidModes (feature of Lilu/WhateverGreen -iofbon patch)
					char timinginfo[1000];
					CFArrayRef IOFBInvalidModes = (CFArrayRef)CFDictionaryGetValue(IOFBProperties, CFSTR("IOFBInvalidModes"));
					if (IOFBInvalidModes && CFArrayGetCount(IOFBInvalidModes)) {
						iprintf("IOFBInvalidModes = {\n"); INDENT
						CFIndex count = CFArrayGetCount(IOFBInvalidModes);
						for (int i = 0; i < count; i++) {
							CFDataRef IOFBInvalidMode = (CFDataRef)CFArrayGetValueAtIndex(IOFBInvalidModes, i);
							if (IOFBInvalidMode) {
								const UInt8 *description = CFDataGetBytePtr(IOFBInvalidMode);
								IOReturn result = *(IOReturn*)description;
								description += sizeof(result);
								CFIndex descripSize = CFDataGetLength(IOFBInvalidMode) - sizeof(IOReturn);
								if (descripSize == sizeof(IOFBDisplayModeDescription))
								{
									IOFBDisplayModeDescription* desc = (IOFBDisplayModeDescription*)description;
									DumpOneFBDisplayModeDescriptionPtr(timinginfo, sizeof(timinginfo), desc, modeAlias);
									iprintf("DisplayModeDescription = { %s }%s,\n", timinginfo, DumpOneReturn(resultStr, sizeof(resultStr), result));
								}
								else {
									DumpOneDetailedTimingInformationPtr(timinginfo, sizeof(timinginfo), (void *)description, descripSize, modeAlias);
									iprintf("DetailedTimingInformation = { %s }%s,\n", timinginfo, DumpOneReturn(resultStr, sizeof(resultStr), result));
								}
							}
						}
						OUTDENT iprintf("}; // IOFBInvalidModes\n");
						CFDictionaryRemoveValue(IOFBProperties, CFSTR("IOFBInvalidModes"));
					}
				} // IOFBInvalidModes

				DoAllEDIDs(IOFBProperties);

				{ // IOFBAttributes (feature of Lilu/WhateverGreen -iofbon patch)
					#define kIOFBAttributesKey "IOFBAttributes"

					CFMutableArrayRef IOFBAttributes = NULL;
					CFArrayRef IOFBAttributes0 = (CFArrayRef)CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBAttributesKey));
					if (IOFBAttributes0) IOFBAttributes = CFArrayCreateMutableCopy(kCFAllocatorDefault, 0, IOFBAttributes0);
					if (IOFBAttributes && CFArrayGetCount(IOFBAttributes)) {
						iprintf("IOFBAttributes = {\n"); INDENT
						CFIndex count = CFArrayGetCount(IOFBAttributes);
						for (int i = 0, j = 0; i < count; i++) {
							CFDataRef IOFBAttribute = (CFDataRef)CFArrayGetValueAtIndex(IOFBAttributes, j);
							if (IOFBAttribute) {
								DumpOneIOFBAttribute(IOFBAttribute, i);
							}
							CFArrayRemoveValueAtIndex(IOFBAttributes, j);
						}
						OUTDENT iprintf("}; // IOFBAttributes\n");
						if (CFArrayGetCount(IOFBAttributes)) {
							CFDictionarySetValue(IOFBProperties, CFSTR(kIOFBAttributesKey), IOFBAttributes);
						}
						else {
							CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBAttributesKey));
						}
					}
				} // IOFBAttributes

				{ // IOFBProbeOptions
					num = (CFNumberRef)CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBProbeOptionsKey));
					if (num) {
						CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
						iprintf("IOFBProbeOptions = %s%s%s%s%s%s Transform:",
							numValue & kIOFBUserRequestProbe ? "UserRequestProbe," : "", // 0x00000001
							numValue & kIOFBForceReadEDID    ?    "ForceReadEDID," : "", // 0x00000100
							numValue & kIOFBAVProbe          ?          "AVProbe," : "", // 0x00000200
							numValue & kIOFBSetTransform     ?     "SetTransform," : "", // 0x00000400
							numValue & kIOFBScalerUnderscan  ?  "ScalerUnderscan," : "", // 0x01000000
							UNKNOWN_FLAG(numValue & 0xfef0f8fe)
						);
						DumpOneTransform((numValue >> kIOFBTransformShift) & 15);
						cprintf(";\n");
						CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBProbeOptionsKey));
					}
				} // IOFBProbeOptions

				{ // IOFBScalerInfo
					CFDataRef IOFBScalerInfo = (CFDataRef)CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBScalerInfoKey));
					if (IOFBScalerInfo) {
						iprintf("IOFBScalerInfo = { ");
						DumpOneDisplayScalerInfo(IOFBScalerInfo);
						cprintf(" };\n");
						CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBScalerInfoKey));
					}
				} // IOFBScalerInfo

				{ // IOFBTimingRange
					CFDataRef IOFBTimingRange = (CFDataRef)CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBTimingRangeKey));
					if (IOFBTimingRange) {
						iprintf("IOFBTimingRange = { ");
						DumpOneDisplayTimingRange(IOFBTimingRange);
						cprintf(" };\n");
						CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBTimingRangeKey));
					}
				} // IOFBTimingRange

				{ // IOFBTransform
					num = (CFNumberRef)CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBTransformKey));
					if (num) {
						CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
						iprintf("IOFBTransform = ");
						DumpOneTransform(numValue);
						cprintf(";\n");
						CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBTransformKey));
					}
				} // IOFBTransform

				{ // startup-timing
					CFDataRef IOTimingInformationData = (CFDataRef)CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBStartupTimingPrefsKey));
					if (IOTimingInformationData) {
						iprintf("startup-timing = { ");
						DumpOneTimingInformation(IOTimingInformationData, modeAlias);
						cprintf(" };\n");
						CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBStartupTimingPrefsKey));
					}
				} // startup-timing

				{ // IOFBI2CInterfaceIDs
					CFArrayRef IOFBI2CInterfaceIDs = (CFArrayRef)CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBI2CInterfaceIDsKey));
					if (IOFBI2CInterfaceIDs && CFGetTypeID(IOFBI2CInterfaceIDs) == CFArrayGetTypeID()) {
						iprintf("IOFBI2CInterfaceIDs = (\n"); INDENT
						CFIndex itemCount = CFArrayGetCount(IOFBI2CInterfaceIDs);
						for (int i = 0; i < itemCount; i++) {
							num = (CFNumberRef)CFArrayGetValueAtIndex(IOFBI2CInterfaceIDs, i);
							if (num && CFGetTypeID(num) == CFNumberGetTypeID()) {
								SInt64 numValue;
								CFNumberGetValue(num, kCFNumberSInt64Type, &numValue);
								iprintf("[%d] = 0x%llx", i, numValue);
							}
							else {
								iprintf("[%d] = ", i);
								CFOutput(NULL);
							}
							cprintf("%s\n", i < itemCount - 1 ? "," : "");
						}
						OUTDENT iprintf("); // IOFBI2CInterfaceIDs\n");
						CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBI2CInterfaceIDsKey));
					}
				} // IOFBI2CInterfaceIDs

				{ // IOFBI2CInterfaceInfo
					CFArrayRef IOFBI2CInterfaceInfo = (CFArrayRef)CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBI2CInterfaceInfoKey));
					if (IOFBI2CInterfaceInfo && CFGetTypeID(IOFBI2CInterfaceInfo) == CFArrayGetTypeID()) {
						iprintf("IOFBI2CInterfaceInfo = (\n"); INDENT
						CFIndex itemCount = CFArrayGetCount(IOFBI2CInterfaceInfo);
						for (int i = 0; i < itemCount; i++) {
							CFDictionaryRef dict = (CFDictionaryRef)CFArrayGetValueAtIndex(IOFBI2CInterfaceInfo, i);
							if (dict && CFGetTypeID(dict) == CFDictionaryGetTypeID()) {
								iprintf("[%d] = ", i);
								CFMutableDictionaryRef copy = DumpI2CProperties(dict);
								if (CFDictionaryGetCount(copy)) {
									cprintf(",\n");
									INDENT iprintf("[%d] properties (unparsed) = ", i);
									CFOutput(copy);
									cprintf("; // [%d] properties (unparsed)", i);
								}
							}
							else {
								iprintf("[%d] = ", i);
								CFOutput(NULL);
							}
							cprintf("%s\n", i < itemCount - 1 ? "," : "");
						}
						OUTDENT iprintf("); // IOFBI2CInterfaceInfo\n");
						CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBI2CInterfaceInfoKey));
					}
				} // IOFBI2CInterfaceInfo

				
				/*
				IOFBI2CInterfaceIDs =     (
					4294967296
				);
				} // IOFBI2CInterfaceIDs

				CFMutableDictionaryRef i2cproperties = DumpI2CProperties(dict);

				IOFBI2CInterfaceInfo =     (
							{
						IOI2CBusType = 1;
						IOI2CInterfaceID = 0;
						IOI2CSupportedCommFlags = 3;
						IOI2CTransactionTypes = 31;
					}
				);
*/
				
#define OneNumber0(_name, _format, ...) \
				do { \
					num = (CFNumberRef)CFDictionaryGetValue(IOFBProperties, CFSTR(_name)); \
					if (num && CFGetTypeID(num) == CFNumberGetTypeID()) { \
						CFNumberGetValue(num, kCFNumberSInt32Type, &numValue); \
						iprintf("%s = " _format ";\n", _name, ##__VA_ARGS__); \
						CFDictionaryRemoveValue(IOFBProperties, CFSTR(_name)); \
					} \
				} while (0)
#define OneNumber(_name, _format) OneNumber0(_name, _format, (int)numValue)

#define OneDataNum0(_name, _bits, _format, ...) \
				do { \
					CFDataRef data = (CFDataRef)CFDictionaryGetValue(IOFBProperties, CFSTR(_name)); \
					if (data && CFGetTypeID(data) == CFDataGetTypeID()) { \
						const UInt8* thebytes = CFDataGetBytePtr(data); \
						CFIndex len = CFDataGetLength(data); \
						if (thebytes && len * 8 == _bits) { \
							SInt ## _bits numValue = *(SInt ## _bits *)thebytes; \
							iprintf("%s = " _format ";\n", _name, ##__VA_ARGS__); \
							CFDictionaryRemoveValue(IOFBProperties, CFSTR(_name)); \
						} \
					} \
				} while (0)

#define OneDataString(_name) \
				do { \
					CFDataRef data = (CFDataRef)CFDictionaryGetValue(IOFBProperties, CFSTR(_name)); \
					if (data && CFGetTypeID(data) == CFDataGetTypeID()) { \
						const UInt8* thebytes = CFDataGetBytePtr(data); \
						CFIndex len = CFDataGetLength(data); \
						iprintf("%s = \"%.*s\";\n", _name, (int)len, thebytes); \
						CFDictionaryRemoveValue(IOFBProperties, CFSTR(_name)); \
					} \
				} while (0)


				
#define OneDataNum(_name, _bits, _format) OneDataNum0(_name, _bits, _format, (int ## _bits ## _t)numValue)
			
#define OneDataOrNum0(_name, _format, ...) do { OneDataNum0(_name, 32, _format, ##__VA_ARGS__); OneNumber0(_name, _format, ##__VA_ARGS__); } while (0)


				OneNumber0("IOFBCurrentPixelClock", "%g MHz", numValue / 1000000.0); // kIOFBCurrentPixelClockKey
				OneDataNum("IOFBUIScale", 32, "%d"); // kIOFBUIScaleKey
				OneDataNum0("IOScreenRestoreState", 32, "%s",
					numValue == kIOScreenRestoreStateNone   ? "kIOScreenRestoreStateNone" :
					numValue == kIOScreenRestoreStateNormal ? "kIOScreenRestoreStateNormal" :
					numValue == kIOScreenRestoreStateDark   ? "kIOScreenRestoreStateDark" :
					UNKNOWN_VALUE(numValue)
				); // kIOScreenRestoreStateKey

				OneNumber("IOFBDependentID", "0x%08x"); // kIOFBDependentIDKey

				OneNumber("IOFBWaitCursorFrames", "%d"); // kIOFBWaitCursorFramesKey
				OneNumber0("IOFBWaitCursorPeriod", "%d ns (%g Hz)", (int)numValue, 1000000000.0 / numValue); // kIOFBWaitCursorPeriodKey
				OneDataOrNum0("av-signal-type", "%s",
					numValue == kIOFBAVSignalTypeUnknown ? "kIOFBAVSignalTypeUnknown" :
					numValue == kIOFBAVSignalTypeVGA     ? "kIOFBAVSignalTypeVGA" :
					numValue == kIOFBAVSignalTypeDVI     ? "kIOFBAVSignalTypeDVI" :
					numValue == kIOFBAVSignalTypeHDMI    ? "kIOFBAVSignalTypeHDMI" :
					numValue == kIOFBAVSignalTypeDP      ? "kIOFBAVSignalTypeDP" :
					UNKNOWN_VALUE(numValue)
				); // kIOFBAVSignalTypeKey

				OneDataString("name"); // kPropertyName
				OneDataString("compatible"); // kPropertyCompatible
				OneDataString("display-type");

				OneDataNum("DPLanes", 32, "%d");
				OneDataNum("DPLinkBit", 32, "%d");
				OneDataNum0("DPLinkRate", 32, "%g Gbps", numValue / 10000.0);

				DoAllBooleans(IOFBProperties);
				OneDataNum("ATY,fb_linebytes", 32, "%d");
				OneDataNum("ATY,fb_offset", 64, "0x%llx");
				OneDataNum("ATY,fb_size", 64, "%lld");
				OneDataNum("ATY,fb_maxshrink", 16, "%hd");
				OneDataNum("ATY,fb_maxstretch", 16, "%hd");
				OneNumber("ATY,fb_minVTotal", "%d");
				OneNumber("ATY,fb_maxVTotal", "%d");
				OneDataString("ATY,EFIDisplay");
				
				iprintf("IOFramebuffer properties (unparsed) = ");
				CFOutput(IOFBProperties);
				cprintf("; // IOFramebuffer properties (unparsed)\n");

				CFRelease(IOFBProperties);
			}
		}
	} // IOFramebuffer
	OUTDENT iprintf("}; // %s\n", serviceType);

	{ // IODisplay
		if(ioDisplayService)
		{
			cprintf("\n");
			iprintf("IODisplay 0x%0x = {\n", ioDisplayService); INDENT
				DumpOneIODisplay(ioDisplayService);
			OUTDENT iprintf("}; // IODisplay\n");
			IOObjectRelease(ioDisplayService);
		}

		for (int i = 0; i < ioDisplayCount; i++) {
			if (allIODisplayServices[i] != ioDisplayService) {
				cprintf("\n");
				servicePath = GetServicePath(allIODisplayServices[i]);
				iprintf("IODisplay 0x%0x: %s = {\n", allIODisplayServices[i], servicePath); INDENT
					DumpOneIODisplay(allIODisplayServices[i]);
				OUTDENT iprintf("}; // IODisplay\n");
				if (servicePath) free (servicePath);
			}
			IOObjectRelease(allIODisplayServices[i]);
		}
	} // IODisplay

	{ // IOFramebufferOpen
		task_port_t selfTask = mach_task_self();
		io_connect_t ioFramebufferConnect;
		IOFramebufferOpen(displayService, selfTask, kIOFBSharedConnectType, &ioFramebufferConnect); //
		if (ioFramebufferConnect) {
			cprintf("\n");
			iprintf("IOFramebufferOpen = {\n"); INDENT

#if 0
			IOFBConnectRef connectRef = NULL;
			if (&IOFBConnectToRef) {
				connectRef = IOFBConnectToRef( ioFramebufferConnect );
				if (connectRef) {
					iprintf("Got IOFBConnectRef\n");
				}
				else {
					// We can't get IOFBConnectRef unless we are WindowServer which does kIOFBServerConnectType to spawn IOFramebufferUserClient instead of kIOFBSharedConnectType to IOFramebufferUserClient
					iprintf("Can't get IOFBConnectRef\n");
				}
			}
			else {
				iprintf("IOFBConnectToRef is not available.\n");
			}
#endif

			{ // StdFBShmem_t
				StdFBShmem_t *fbshmem;
#if __LP64__
				mach_vm_size_t size;
#else
				vm_size_t size;
#endif

				kerr = IOConnectMapMemory(
					ioFramebufferConnect,
					kIOFBCursorMemory,
					selfTask,
#if __LP64__
					(mach_vm_address_t*)&fbshmem,
#else
					(vm_address_t*)&fbshmem,
#endif
					&size,
					kIOMapAnywhere | kIOMapDefaultCache | kIOMapReadOnly
				);
				if (KERN_SUCCESS == kerr) {
					iprintf("kIOFBCursorMemory = { size:%lld version:%d location:%dx%d };\n", (uint64_t)size, fbshmem->version, fbshmem->cursorLoc.x, fbshmem->cursorLoc.y);
					IOConnectUnmapMemory(ioFramebufferConnect, kIOFBCursorMemory, selfTask,
#if __LP64__
						(mach_vm_address_t)fbshmem
#else
						(vm_address_t)fbshmem
#endif
					);
				} // if IOConnectMapMemory

				kerr = IOConnectMapMemory(
					ioFramebufferConnect,
					kIOFBVRAMMemory,
					selfTask,
#if __LP64__
					(mach_vm_address_t*)&fbshmem,
#else
					(vm_address_t*)&fbshmem,
#endif
					&size,
					kIOMapAnywhere | kIOMapDefaultCache | kIOMapReadOnly
				);
				if (KERN_SUCCESS == kerr) {
					iprintf("kIOFBVRAMMemory = { size:%gMB };\n", size / (1024 * 1024.0));
					IOConnectUnmapMemory(ioFramebufferConnect, kIOFBCursorMemory, selfTask,
#if __LP64__
						(mach_vm_address_t)fbshmem
#else
						(vm_address_t)fbshmem
#endif
					);
				} // if IOConnectMapMemory
			} // StdFBShmem_t
			
			IOServiceClose(ioFramebufferConnect);
			OUTDENT iprintf("}; // IOFramebufferOpen\n");
		}
	} // IOFramebufferOpen
	
	{ // I2C
		IOItemCount i2cInterfaceCount;
		IOReturn result;
		result = IOFBGetI2CInterfaceCount(displayService, &i2cInterfaceCount);
		if (KERN_SUCCESS == result && i2cInterfaceCount) {
			cprintf("\n");
			iprintf("I2C Interfaces = {\n"); INDENT
			for (int interfaceBus = 0; interfaceBus < i2cInterfaceCount; interfaceBus++) {
				io_service_t i2cservice;
				iprintf("[%d] = {\n", interfaceBus); INDENT
				result = IOFBCopyI2CInterfaceForBus(displayService, interfaceBus, &i2cservice);
				if (KERN_SUCCESS == result) {
					CFMutableDictionaryRef dict = NULL;
					result = IORegistryEntryCreateCFProperties(i2cservice, &dict, kCFAllocatorDefault, kNilOptions);
					if (KERN_SUCCESS == result) {
						iprintf("%s = ", "IOFBCopyI2CInterfaceForBus");
						CFMutableDictionaryRef copy = DumpI2CProperties(dict);
						cprintf(";\n");
						if (CFDictionaryGetCount(copy)) {
							iprintf("%s properties (unparsed) = ", "IOFBCopyI2CInterfaceForBus");
							CFOutput(copy);
							cprintf("; // %s properties (unparsed)\n", "IOFBCopyI2CInterfaceForBus");
						}

						onenum(64, IOI2CInterfaceID , "0x%llx")
						onenum(32, IOI2CBusType , "%u")
						onenum(32, IOI2CTransactionTypes , "0x%x")
						onenum(32, IOI2CSupportedCommFlags , "0x%x")

						IOI2CConnectRef i2cconnect;
						result = IOI2CInterfaceOpen(i2cservice, kNilOptions, &i2cconnect);
						if (KERN_SUCCESS == result) {
							IOI2CRequest_10_6 request;

							/*
								I2C Slave Address Pair/Address
											Specification
								3Ah 74h/75h RESERVED for HDCP (Primary Link Port)
								3Bh 76h/77h RESERVED for HDCP (Secondary Link Port)
								40h 80h/81h RESERVED for DisplayPort (Dual-Mode Video Adapter)
								50h A0h/A1h EDID
								52h A4h/A5h DisplayID
								54h A8h/A9h RESERVED for HDMI (see §10.4 "Status and Control Data Channel" in HDMI 2.0 spec)
								30h 60h     E-DDC Segment Pointer (see Section 2.2.5)
								37h 6Eh/6Fh RESERVED for DDC/CI communication (e.g., MCCS)
							*/

							if (0)
							{ // EDID from DDC
								// With this method, it is assumed that the EDID will always be
								// sent. However, only 256 bytes of the EDID will be sent
								// probably because larger EDIDs are part of E-DDC which uses
								// a segment pointer.
								//
								// The first read might not be from the beginning of the EDID,
								// so you have to search for the 00ffffffffffff00 bytes to find
								// the beginning of the EDID.
								
								iprintf("EDID from DDC = ");
								int numBlocks = 4;
								for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++) {

									UInt8 data[128];

									bzero(&request, sizeof(request));

									request.replyTransactionType = kIOI2CSimpleTransactionType;
									request.replyAddress = 0xA1; // 0x50(EDID) << 1 + 1(read)
									request.replyBytes = (uint32_t)sizeof(data);
									request.replyBuffer = (vm_address_t)data;
									result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &request);
									usleep(kDelayDDCEDIDReply);

									if (KERN_SUCCESS == result) {
										//iprintf("Read result 0x%x, 0x%x bytes\n", request.result, request.replyBytes);
										if( kIOReturnSuccess == request.result) {
											for (int i = 0; i < request.replyBytes; i++) {
												cprintf("%02x", data[i]);
											}
											lf;
										}
										else {
											cprintf("(request %s)", DumpOneReturn(resultStr, sizeof(resultStr), request.result));
										}
									}
									else {
										cprintf("(IOI2CSendRequest %s)", DumpOneReturn(resultStr, sizeof(resultStr), result));
									}
								} // for blockNdx
								lf;
							} // EDID from DDC

							if (1)
							{ // EDID from E-DDC method 1
								// https://github.com/apple-oss-distributions/IOGraphics/blob/main/tools/itvview.c
								
								for (int comm = 0; comm <= ((TRYSUBADDRESS * val_IOI2CSupportedCommFlags) & kIOI2CUseSubAddressCommFlag); comm += kIOI2CUseSubAddressCommFlag) {

									iprintf("EDID from E-DDC (old method) = ");
									int numBlocks = 1;
									for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++) {

										UInt8 block1[128];
										UInt8 data[128];

										if (blockNdx >= 2) {
											
											/*
												We are attempting an E-DDC Sequential Read from the E-DDC spec.
												We can't make this work - we can't read beyond block 2 because IOI2CSendRequest sends
												stop bits between setting the segment pointer (60h) and sending the DDC write address (A0h).
												The E-DDC spec clearly shows that only a start bit should exist between setting the
												segment pointer and sending the DDC write address.
											*/
											
											bzero(&request, sizeof(request));
											request.sendTransactionType = kIOI2CSimpleTransactionType;
											request.sendAddress = 0x60; // 0x30(Segment pointer) << 1 + 0(write)
											request.sendBytes = 0x01;
											request.sendBuffer = (vm_address_t)data;
											data[0] = blockNdx >> 1; // E-DDC Random Read in 128 byte chuncks of 256 byte segments
											if (comm) {
												request.commFlags = comm;
												request.sendSubAddress = 0x51;
											}

											result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &request);
											if (result) {
												cprintf("(segment IOI2CSendRequest %s)", DumpOneReturn(resultStr, sizeof(resultStr), result));
											}
											if (request.result) {
												cprintf("(segment request %s)", DumpOneReturn(resultStr, sizeof(resultStr), request.result));
											}
											usleep(kDelayEDDCPointerSegment);
										}
										{
											bzero(&request, sizeof(request));
											request.sendTransactionType = kIOI2CSimpleTransactionType;
											request.sendAddress = 0xA0; // 0x50(EDID) << 1 + 0(write)
											request.sendBytes = 0x01;
											request.sendBuffer = (vm_address_t)data;
											data[0] = (blockNdx & 1) * 128; // word offset ; Doing DDC Sequential Read of 128 byte blocks (2 blocks in each 256 byte segment)
											if (comm) {
												request.commFlags = comm;
												request.sendSubAddress = 0x51;
											}

											request.replyTransactionType = kIOI2CSimpleTransactionType;
											request.replyAddress = 0xA1; // 0x50(EDID) << 1 + 1(read)
											request.replyBytes = (uint32_t)sizeof(data);
											request.replyBuffer = (vm_address_t)data;
											result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &request);
											usleep(kDelayEDDCEDIDReply);
										}

										if (KERN_SUCCESS == result) {
											//iprintf("Read result 0x%x, 0x%x bytes\n", request.result, request.replyBytes);
											if( kIOReturnSuccess == request.result) {
												if (blockNdx == 0) {
													numBlocks += data[126];
													memcpy(block1, data, sizeof(block1));
												}
												if (blockNdx == 0 || memcmp(block1, data, sizeof(block1))) {
													for (int i = 0; i < request.replyBytes; i++) {
														cprintf("%02x", data[i]);
													}
												}
												else {
													cprintf("(cannot read more)");
												}
											}
											else {
												cprintf("(request %s)", DumpOneReturn(resultStr, sizeof(resultStr), request.result));
											}
										}
										else {
											cprintf("(IOI2CSendRequest %s)", DumpOneReturn(resultStr, sizeof(resultStr), result));
										}
									} // for blockNdx
									lf;
								} // for comm
							} // EDID from E-DDC method 1

							if (1)
							{ // EDID from E-DDC method 2

								for (int comm = 0; comm <= ((TRYSUBADDRESS * val_IOI2CSupportedCommFlags) & kIOI2CUseSubAddressCommFlag); comm += kIOI2CUseSubAddressCommFlag) {

									iprintf("EDID from E-DDC              = ");
									int numBlocks = 1;
									for (int blockNdx = 0; blockNdx < numBlocks; blockNdx += (blockNdx >= 2 ? 2 : 1)) {

										UInt8 block1[128];
										UInt8 data[256];

										if (blockNdx >= 2) {
											/*
												In this method, we'll set the segment pointer and hope to read
												that block of EDID from A1h without sending an offset to A0h.

												This is not a method described in the E-DDC spec. I suppose it
												resembles a combination of an E-DDC method and the "DDC Read
												at the Current Address" method.
											 
												Tested in Monterey.
												Works with Nvidia GTX 680 and Intel 630.
												Does not work with AMD W5700.
											*/
											
											bzero(&request, sizeof(request));
											request.sendTransactionType = kIOI2CSimpleTransactionType;
											request.sendAddress = 0x60; // 0x30(Segment pointer) << 1 + 0(write)
											request.sendBytes = 0x01;
											request.sendBuffer = (vm_address_t)data;
											data[0] = blockNdx >> 1; // E-DDC Random Read in 128 byte chuncks of 256 byte segments
											if (comm) {
												request.commFlags = comm;
												request.sendSubAddress = 0x51;
											}

											request.replyTransactionType = kIOI2CSimpleTransactionType;
											request.replyAddress = 0xA1; // 0x50(EDID) << 1 + 1(read)
											request.replyBytes = 128 * (((numBlocks - blockNdx) > 2) ? 2 : 1);
											request.replyBuffer = (vm_address_t)data;
											//request.minReplyDelay = 200;
											result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &request);
											usleep(kDelayEDDCPointerSegment);
										} // if blockNdx >= 2
										else
										{
											bzero(&request, sizeof(request));
											request.sendTransactionType = kIOI2CSimpleTransactionType;
											request.sendAddress = 0xA0; // 0x50(EDID) << 1 + 0(write)
											request.sendBytes = 0x01;
											request.sendBuffer = (vm_address_t)data;
											data[0] = (blockNdx & 1) * 128; // word offset ; Doing DDC Sequential Read of 128 byte blocks (2 blocks in each 256 byte segment)
											if (comm) {
												request.commFlags = comm;
												request.sendSubAddress = 0x51;
											}

											request.replyTransactionType = kIOI2CSimpleTransactionType;
											request.replyAddress = 0xA1; // 0x50(EDID) << 1 + 1(read)
											request.replyBytes = 128;
											request.replyBuffer = (vm_address_t)data;
											//request.minReplyDelay = 100;
											result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &request);
											usleep(kDelayEDDCEDIDReply);
										}

										if (KERN_SUCCESS == result) {
											//iprintf("Read result 0x%x, 0x%x bytes\n", request.result, request.replyBytes);
											if( kIOReturnSuccess == request.result) {
												if (blockNdx == 0) {
													numBlocks += data[126];
													memcpy(block1, data, sizeof(block1));
												}
												for (int subBlock = 0; subBlock < request.replyBytes / 128; subBlock++) {
													if (blockNdx + subBlock == 0 || memcmp(block1, data + subBlock * 128, sizeof(block1))) {
														for (int i = 0 + subBlock * 128; i < request.replyBytes; i++) {
															cprintf("%02x", data[i]);
														}
													}
													else {
														cprintf("(cannot read more)");
														break;
													}
												}
											}
											else {
												cprintf("(request %s)", DumpOneReturn(resultStr, sizeof(resultStr), request.result));
											}
										}
										else {
											cprintf("(IOI2CSendRequest %s)", DumpOneReturn(resultStr, sizeof(resultStr), result));
										}

									} // for blockNdx
									lf;
								} // for comm
							} // EDID from E-DDC method 2
							
							if (0)
							{ // DDC Identification Request // described in Access Bus 3.0 spec
								iprintf("Identification Request = {\n"); INDENT
								
								/*
									I don't know of anything that responds to identification request or if I'm doing it right.
								*/

								for (int address = 0; address <= 255; address++) {
									UInt8 identificationBytes[32+6];
									bzero(identificationBytes, sizeof(identificationBytes));

									bzero(&request, sizeof(request));
									request.sendTransactionType = kIOI2CSimpleTransactionType;
									request.sendAddress = 0x6E; // Destination address (DDC/CI)
									UInt8 senddata[] = {
										(UInt8)address, // Source address
										0x81, // Length
										0xf1, // Identification request op code
										0x00, // Checksum
									};
									request.sendBytes = (uint32_t)sizeof(senddata);
									request.sendBuffer = (vm_address_t)senddata;
									ddcsetchecksum(&request);

									if (FORCEI2C || !(val_IOI2CTransactionTypes & (1 << kIOI2CDDCciReplyTransactionType))) {
										result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &request);
										usleep(kDelayDDCIdentificationRequest);
										if (result) {
											iprintf("(IOI2CSendRequest %s)\n", DumpOneReturn(resultStr, sizeof(resultStr), result));
											break;
										}
										bzero(&request, sizeof(request));
										request.replyTransactionType = kIOI2CSimpleTransactionType;
									}
									else {
										request.replyTransactionType = kIOI2CDDCciReplyTransactionType;
										request.minReplyDelay = MicrosecondsToAbsoluteTime(kDelayDDCIdentificationRequest);
									}

									request.replyAddress = 0x6F; // Source address (DDC/CI)
									request.replyBytes = (uint32_t)sizeof(identificationBytes);
									request.replyBuffer = (vm_address_t)identificationBytes;
									result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &request);
									usleep(kDelayDDCIdentificationReply);

									if (result) {
										iprintf("(IOI2CSendRequest %s)\n", DumpOneReturn(resultStr, sizeof(resultStr), result));
										break;
									}

									if (ddcreplyisgood(&request, true, 0x6e, identificationBytes, -1)) {
										iprintf("identification string[%02Xh] = (", address);
										int numbytes = identificationBytes[1] & 0x7f;
										for (int i = 0; i < numbytes + 3; i++) {
											cprintf("%02x", identificationBytes[i]);
										}
										cprintf(" = \"%*.s\");\n", numbytes, identificationBytes + 3);
									}

								} while (0);

								OUTDENT iprintf("}; // Identification Request\n");
							} // DDC Identification Request

							{ // DDC Timing Report
								iprintf("Timing Report = { ");

								do {
									UInt8 timingReportBytes[9];
									bzero(timingReportBytes, sizeof(timingReportBytes));

									bzero(&request, sizeof(request));
									request.sendTransactionType = kIOI2CSimpleTransactionType;
									request.sendAddress = 0x6E; // Destination address (DDC/CI)
									UInt8 senddata[] = {
										0x51, // Source address
										0x81, // Length
										0x07, // timingReport request op code
										0x00, // Checksum
									};
									request.sendBytes = (uint32_t)sizeof(senddata);
									request.sendBuffer = (vm_address_t)senddata;
									ddcsetchecksum(&request);

									if (FORCEI2C || !(val_IOI2CTransactionTypes & (1 << kIOI2CDDCciReplyTransactionType))) {
										result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &request);
										usleep(kDelayDDCTimingRequest);
										if (result) {
											cprintf("(IOI2CSendRequest %s) ", DumpOneReturn(resultStr, sizeof(resultStr), result));
											break;
										}
										bzero(&request, sizeof(request));
										request.replyTransactionType = kIOI2CSimpleTransactionType;
									}
									else {
										request.replyTransactionType = kIOI2CDDCciReplyTransactionType;
										request.minReplyDelay = MicrosecondsToAbsoluteTime(kDelayDDCTimingRequest);
									}

									request.replyAddress = 0x6F; // Source address (DDC/CI)
									request.replyBytes = (uint32_t)sizeof(timingReportBytes);
									request.replyBuffer = (vm_address_t)timingReportBytes;
									result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &request);
									usleep(kDelayDDCTimingReply);

									if (result) {
										cprintf("(IOI2CSendRequest %s) ", DumpOneReturn(resultStr, sizeof(resultStr), result));
										break;
									}
									if (!ddcreplyisgood(&request, false, 0x6e, timingReportBytes, (int)sizeof(timingReportBytes)) || (timingReportBytes[1] & 0x7f) != 0x06 || timingReportBytes[2] != 0x4e) {
										cprintf("(unexpected data: ");
										for (int i = 0; i < request.replyBytes; i++) {
											cprintf("%02x", timingReportBytes[i]);
										}
										cprintf(") ");
										break;
									};
									
									for (int i = 0; i < request.replyBytes; i++) {
										cprintf("%02x", timingReportBytes[i]);
									}
									cprintf(" : { status:(%s%s%s%s%s) %.2fkHz %.2fHz%s } ",
										timingReportBytes[3] & 0x80 ? "Sync.Freq. out of Range," : "",
										timingReportBytes[3] & 0x40 ? "Unstable count," : "",
										timingReportBytes[3] & 0x02 ? "+hsync," : "-hsync,",
										timingReportBytes[3] & 0x01 ? "+vsync," : "-vsync",
										UNKNOWN_FLAG(timingReportBytes[3] & 0x3c),
										((timingReportBytes[4] << 8) | timingReportBytes[5]) / 100.0,
										((timingReportBytes[6] << 8) | timingReportBytes[7]) / 100.0,
										timingReportBytes[1] & 0x80 ? " (expected 06h instead of 86h for second byte)" : ""
									);

								} while (0);

								cprintf("};\n");
							} // DDC Timing Report

							if (1)
							{ // DDC VCP Capabilities
								iprintf("VCP Capabilities = {\n"); INDENT

								int vcpCapabilitiesOffset = 0;
								int vcpCapabilitiesMaxSize = 1000;
								char *vcpCapabilitiesBytes = (char *)malloc(vcpCapabilitiesMaxSize);
								
								while (1) {
									UInt8 chunkdata[32+6];
									bzero(chunkdata, sizeof(chunkdata));
									result = kIOReturnSuccess;
									bool ddcReplyIsBad = false;
									
									for (int attempt = 0; attempt < 3; attempt++) {
										ddcReplyIsBad = false;
										result = kIOReturnSuccess;
										
										bzero(&request, sizeof(request));
										request.sendTransactionType = kIOI2CSimpleTransactionType;
										request.sendAddress = 0x6E; // Destination address (DDC/CI)
										UInt8 senddata[] = {
											0x51, // Source address
											0x83, // Length
											0xf3, // Capabilities request op code
											(UInt8)(vcpCapabilitiesOffset >> 8), // Offset value High byte
											(UInt8)(vcpCapabilitiesOffset & 0xff), // Offset value Low byte
											0x00, // Checksum
										};
										request.sendBytes = (uint32_t)sizeof(senddata);
										request.sendBuffer = (vm_address_t)senddata;
										ddcsetchecksum(&request);

										if (FORCEI2C || !(val_IOI2CTransactionTypes & (1 << kIOI2CDDCciReplyTransactionType))) {
											result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &request);
											usleep(kDelayDDCVCPCapRequest);
											if (result) {
												continue;
											}
											bzero(&request, sizeof(request));
											request.replyTransactionType = kIOI2CSimpleTransactionType;
										}
										else {
											request.replyTransactionType = kIOI2CDDCciReplyTransactionType;
											request.minReplyDelay = MicrosecondsToAbsoluteTime(kDelayDDCVCPCapRequest);
										}

										request.replyAddress = 0x6F; // Source address (DDC/CI)
										request.replyBytes = (uint32_t)sizeof(chunkdata);
										request.replyBuffer = (vm_address_t)chunkdata;
										result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &request);
										usleep(kDelayDDCVCPCapReply);

										if (result) {
											continue;
										}

										if (!ddcreplyisgood(&request, true, 0x6e, chunkdata, -1) || chunkdata[2] != 0xE3 || ((chunkdata[3] << 8) | chunkdata[4]) != vcpCapabilitiesOffset) {
											ddcReplyIsBad = true;
											continue;
										};

										break;
									} // for attempt

									if (result) {
										iprintf("(IOI2CSendRequest %s)\n", DumpOneReturn(resultStr, sizeof(resultStr), result));
										break;
									}
									if (ddcReplyIsBad) {
										iprintf("(unexpected data at offset %d: ", vcpCapabilitiesOffset);
										for (int i = 0; i < request.replyBytes; i++) {
											cprintf("%02x", chunkdata[i]);
										}
										cprintf("),\n");
										break;
									}
									
									int chunksize = (chunkdata[1] & 0x7f) - 3;
									if (!chunksize) break;

									if (vcpCapabilitiesOffset + chunksize > vcpCapabilitiesMaxSize) {
										vcpCapabilitiesMaxSize += 1000;
										vcpCapabilitiesBytes = (char *)realloc(vcpCapabilitiesBytes, vcpCapabilitiesMaxSize);
									}
									memcpy(vcpCapabilitiesBytes + vcpCapabilitiesOffset, chunkdata + 5, chunksize);
									vcpCapabilitiesOffset += chunksize;
								} // while vcp string chunksize
								
								if (vcpCapabilitiesOffset) {
									if (vcpCapabilitiesBytes[vcpCapabilitiesOffset-1] != 0) {
										iprintf("(last byte at offset %d is not null)\n", vcpCapabilitiesOffset - 1);
										if (vcpCapabilitiesOffset + 1 > vcpCapabilitiesMaxSize) {
											vcpCapabilitiesMaxSize += 1;
											vcpCapabilitiesBytes = (char *)realloc(vcpCapabilitiesBytes, vcpCapabilitiesMaxSize);
										}
										vcpCapabilitiesBytes[vcpCapabilitiesOffset] = 0;
										vcpCapabilitiesOffset++;
									}
									if (strlen(vcpCapabilitiesBytes) != vcpCapabilitiesOffset - 1) {
										iprintf("(expected string length %d)\n", vcpCapabilitiesOffset - 1);
										iprintf("VCP bytes = ");
										for (int i = 0; i < vcpCapabilitiesOffset; i++) {
											cprintf("%02x", vcpCapabilitiesBytes[i]);
										}
										cprintf(";\n");
									}
									iprintf("VCP string = \"%s\";\n", vcpCapabilitiesBytes);
									iprintf("VCP parsed = {\n"); INDENT
									parsevcp(0, vcpCapabilitiesBytes, i2cconnect, val_IOI2CTransactionTypes);
									OUTDENT iprintf("} // VCP parsed\n");
								}
								
								free (vcpCapabilitiesBytes);
								OUTDENT iprintf("}; // VCP Capabilities\n");
							} // DDC VCP Capabilities

							if (0) { // EDID from DDC/CI MCCS
								
								// Can't test this without a display that supports 0x78 opcode
								
								/*
									I2C Secondary Address
												  Display Dependent Device Type
																		 Example

									0xF0h / F1h   Pointer                Touch Screen, Light pen or Remote Control Track Ball
									0xF2h / F3h   Audio Device           Speaker / Microphone
									0xF4h / F5h   Serial Communication   Home Network IF (power line modem)
									0xF6h / F7h   Calibration Device     Luminance Probe or Colorimeter
									0xF8h / F9h   Input Device           IR keyboard and remote control pad (shared IR channel)
									0xFAh / FBh   Reserved               Reserved for future use
									0xFCh / FDh   Reserved               Reserved for future use
									0xFEh / FFh   Reserved               Reserved for future use

									I2C Secondary Address
												   I2C Device
									0x12h / 13h    Smart Battery Charger
									0x14h / 15h    Smart Battery Selector
									0x16h / 17h    Smart Battery
									0x80h / 81h    Audio Processor
									0x40h / 41h    PAL / NTSC Decoder
									0xA0h / A1h    DDC2B Monitor (memory)
								 */
								
								iprintf("EDID from DDC/CI MCCS = {\n"); INDENT
								int numBlocks = 3;
								for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++) {

									UInt8 block1[128];
									UInt8 data[256];
									bzero(data, sizeof(data));

									for (int chunkNdx = 0; chunkNdx < 4; chunkNdx++) {
										UInt8 chunkdata[32+6];
										bzero(chunkdata, sizeof(chunkdata));

										bzero(&request, sizeof(request));
										request.sendTransactionType = kIOI2CSimpleTransactionType;
										request.sendAddress = 0x6E; // Destination address (DDC/CI)
										UInt8 senddata[] = {
											0x51, // Source address
											0x84, // Length
											0xe2, // Table read op code
											0x78, // VCP opcode for EDID
											(UInt8)blockNdx, // Offset value High byte
											(UInt8)(chunkNdx * 32), // Offset value Low byte
											0x00, // Checksum
										};
										//senddata[1] += sizeof(data) - 2;
										ddcsetchecksum(&request);
										request.sendBytes = (uint32_t)sizeof(senddata);
										request.sendBuffer = (vm_address_t)senddata;

										if (FORCEI2C || !(val_IOI2CTransactionTypes & (1 << kIOI2CDDCciReplyTransactionType))) {
											result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &request);
											usleep(kDelayMCCSEDIDRequest);
											if (result) {
												iprintf("(IOI2CSendRequest %s)\n", DumpOneReturn(resultStr, sizeof(resultStr), result));
												break;
											}
											bzero(&request, sizeof(request));
											request.replyTransactionType = kIOI2CSimpleTransactionType;
										}
										else {
											request.replyTransactionType = kIOI2CDDCciReplyTransactionType;
											request.minReplyDelay = MicrosecondsToAbsoluteTime(kDelayMCCSEDIDRequest);
										}

										request.replyAddress = 0x6F; // Source address (DDC/CI)
										request.replyBytes = (uint32_t)sizeof(chunkdata);
										request.replyBuffer = (vm_address_t)chunkdata;
										result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &request);
										usleep(kDelayMCCSEDIDReply);
										iprintf("(");
										for (int i = 0; i < request.replyBytes; i++) {
											cprintf("%02x", chunkdata[i]);
										}
										cprintf("),\n");
									} // for chunkNdx

									iprintf("block %d = { ", blockNdx);
									if (KERN_SUCCESS == result) {
										//iprintf("Read result 0x%x, 0x%x bytes\n", request.result, request.replyBytes);
										if( kIOReturnSuccess == request.result) {
											if (blockNdx == 0) {
												// numBlocks += data[126];
												memcpy(block1, data, sizeof(block1));
											}
											if (blockNdx == 0 || memcmp(block1, data, sizeof(block1))) {
												for (int i = 0; i < request.replyBytes; i++) {
													cprintf("%02x", data[i]);
												}
											}
											else {
												cprintf("(cannot read more)");
											}
										}
										else {
											cprintf("(request %s)", DumpOneReturn(resultStr, sizeof(resultStr), request.result));
										}
									}
									else {
										cprintf("(IOI2CSendRequest %s)", DumpOneReturn(resultStr, sizeof(resultStr), result));
									}
									cprintf(" }\n");
								} // for blockNdx
								OUTDENT iprintf("};\n\n");
							} // EDID from DDC/CI MCCS

							
							if (val_IOI2CTransactionTypes & (1 << kIOI2CDisplayPortNativeTransactionType))
							{ // DisplayPort
								iprintf("DisplayPort = {\n"); INDENT
								UInt8 path[16];
								DoOneDisplayPort(displayService, i2cconnect, path, 0, false);
								OUTDENT iprintf("}; // DisplayPort\n");
							} // DisplayPort

							result = IOI2CInterfaceClose(i2cconnect, kNilOptions);
						}
						else
						{
							iprintf("IOI2CInterfaceOpen error:%d\n", result);
						} // IOI2CInterfaceOpen
					}
					else {
						iprintf("IORegistryEntryCreateCFProperties error:%d\n", result);
					}
				} // IORegistryEntryCreateCFProperties
				else {
					iprintf("IOFBCopyI2CInterfaceForBus error:%d\n", result);
				} // IOFBCopyI2CInterfaceForBus

				OUTDENT iprintf("},\n");
			} // for iterfaceBus
			OUTDENT iprintf("}; // I2C Interfaces\n");
		} // if i2cInterfaceCount
	} // I2C
} // DumpDisplayService


void DumpAllDisplaysInfo (void) {
	CGDirectDisplayID onlineDisplays[20];
	uint32_t nModes[20]; // number of modes for each display
	CGSDisplayModeDescription * modes[20]; // all the modes for each display
	int modeAlias[20];
	
#define checkapi(v, x) \
do { \
	if (&x) \
		iprintf("%s is available since %s.\n", #x, v); \
	else \
		iprintf("%s is required for %s.\n", v, #x); \
} while(0)
	
#define checkapiandsdk(SDK, v, x) \
do { \
	bool gotit = false; \
	API_OR_SDK_AVAILABLE_BEGIN(SDK, x) { \
		gotit = true; \
	} API_OR_SDK_AVAILABLE_END \
	if (gotit) \
		iprintf("%s is available since %s.\n", #x, v); \
	else \
		iprintf("%s is required for %s.\n", v, #x); \
} while(0)
	
#define checksdk(v, x) \
	iprintf("%s SDK is required for %s.\n", v, #x)
	
#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_5
	checkapiandsdk(10.5, "Mac OS X 10.5", CGDisplayRotation);
	checkapiandsdk(10.5, "Mac OS X 10.5", CGDisplayCopyColorSpace);
	checkapi("Mac OS X 10.5", DisplayServicesGetBrightness);
	checkapi("Mac OS X 10.5", DisplayServicesSetBrightness);
#else
	checksdk("Mac OS X 10.5", CGDisplayRotation);
	checksdk("Mac OS X 10.5", CGDisplayCopyColorSpace);
	checksdk("Mac OS X 10.5", DisplayServicesGetBrightness);
	checksdk("Mac OS X 10.5", DisplayServicesSetBrightness);
#endif

#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_6
	checkapiandsdk(10.6, "Mac OS X 10.6", CGDisplayCopyDisplayMode);
	checkapiandsdk(10.6, "Mac OS X 10.6", CGDisplayCopyAllDisplayModes);
#else
	checksdk("Mac OS X 10.6", CGDisplayCopyDisplayMode);
	checksdk("Mac OS X 10.6", CGDisplayCopyAllDisplayModes);
#endif

#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_8
	checkapiandsdk(10.8, "OS X 10.8", kCGDisplayShowDuplicateLowResolutionModes);
#else
	checksdk("OS X 10.8", kCGDisplayShowDuplicateLowResolutionModes);
#endif

#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_11
	checkapi("OS X 10.11", CGSEnableHDR);
	checkapi("OS X 10.11", CGSIsHDREnabled);
	checkapi("OS X 10.11", CGSIsHDRSupported);
#else
	checksdk("OS X 10.11", CGSEnableHDR);
	checksdk("OS X 10.11", CGSIsHDREnabled);
	checksdk("OS X 10.11", CGSIsHDRSupported);
#endif

#if MAC_OS_X_VERSION_SDK >= MAC_OS_VERSION_11_0
	checkapi("macOS 11", SLSDisplaySetHDRModeEnabled);
	checkapi("macOS 11", SLSDisplayIsHDRModeEnabled);
	checkapi("macOS 11", SLSDisplaySupportsHDRMode);
#else
	checksdk("macOS 11", SLSDisplaySetHDRModeEnabled);
	checksdk("macOS 11", SLSDisplayIsHDRModeEnabled);
	checksdk("macOS 11", SLSDisplaySupportsHDRMode);
#endif

#if MAC_OS_X_VERSION_SDK >= MAC_OS_VERSION_12_0
	checkapi("macOS 12", SLSIsDisplayModeVRR);
	checkapi("macOS 12", SLSIsDisplayModeProMotion);
	checkapi("macOS 12", SLSGetDisplayModeMinRefreshRate);
#else
	checksdk("macOS 12", SLSIsDisplayModeVRR);
	checksdk("macOS 12", SLSIsDisplayModeProMotion);
	checksdk("macOS 12", SLSGetDisplayModeMinRefreshRate);
#endif
	
	iprintf("\n");
	
	uint32_t displayCount;
	CGGetOnlineDisplayList(20, onlineDisplays, &displayCount);
	iprintf("%d Online Monitors found\n", displayCount);
	//CGSConnectionID connectionID = CGSMainConnectionID();
	CGDirectDisplayID mainDisplay = CGMainDisplayID();
	iprintf("Main display: 0x%x\n", mainDisplay);

	iprintf("-----------------------------------------------------\n");
	iprintf("DISPLAYS = {\n"); INDENT
	for (uint32_t displayIndex = 0; displayIndex < displayCount; displayIndex++) {
		CGDirectDisplayID display = onlineDisplays[displayIndex];

		CGSGetNumberOfDisplayModes(display, &nModes[displayIndex]);
		modes[displayIndex] = (CGSDisplayModeDescription *)malloc(nModes[displayIndex] * sizeof(CGSDisplayModeDescription));
		if (!modes[displayIndex]) {
			iprintf("Error: Not enough memory for %d modes\n", nModes[displayIndex]);
			nModes[displayIndex] = 0;
		}
		else {
			bzero(modes[displayIndex], nModes[displayIndex] * sizeof(CGSDisplayModeDescription));
		}

#if 0
		int lastLength = 0;
		int lengthsCount = 0;
		int supportedLengths[20];
		for (int i = 4; i < sizeof(CGSDisplayModeDescription); i++) {
			memset(&modes[displayIndex][0], 0xab, sizeof(CGSDisplayModeDescription));
			CGError result = CGSGetDisplayModeDescriptionOfLength(display, 0, &modes[displayIndex][0], i);
			iprintf("i=%d=%03x b8:size=%08x result:%08x\n", i, i, modes[displayIndex][0].size, result);
		}
#endif
		
		int aliasCounts[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		for (int i = 0; i < nModes[displayIndex]; i++) {
			CGError result = CGSGetDisplayModeDescriptionOfLength(display, i, &modes[displayIndex][i], (int)sizeof(CGSDisplayModeDescription));
			if (result == kCGErrorSuccess) {
				aliasCounts[(modes[displayIndex][i].DisplayModeID >> 12) & 15]++;
			}
		}

		int maxAlias = 0;
		int maxAliasCount = 0;
		for (int i = 0; i < 15; i++) {
			if (aliasCounts[i] > maxAliasCount) {
				maxAliasCount = aliasCounts[i];
				maxAlias = i;
			}
		}
		modeAlias[displayIndex] = maxAlias;

		iprintf("Monitor[%d] = {\n", displayIndex); INDENT
		iprintf("modeAlias = %x;\n", modeAlias[displayIndex]);
		iprintf("DisplayID = 0x%x;\n", display);
		iprintf("PrimaryID = 0x%x;\n", CGDisplayPrimaryDisplay(display)); // The primary display in the mirror set. If display is not hardware-mirrored, this function simply returns display.
		iprintf("MirrorsID = 0x%x;\n", CGDisplayMirrorsDisplay(display)); // Returns the primary display in the mirroring set. Returns kCGNullDirectDisplay if the specified display is actually the primary display or is not in a mirroring set.
		iprintf("vendorNumber = 0x%x;\n", CGDisplayVendorNumber(display)); // A vendor number for the monitor associated with the specified display. If I/O Kit cannot identify the monitor, kDisplayVendorIDUnknown is returned. If there is no monitor associated with the display, 0xFFFFFFFF is returned.
		iprintf("modelNumber = 0x%x;\n", CGDisplayModelNumber(display)); // A model number for the monitor associated with the specified display. If I/O Kit can’t identify the monitor, kDisplayProductIDGeneric is returned. If no monitor is connected, a value of 0xFFFFFFFF is returned.
		iprintf("serialNumber = %u;\n", CGDisplaySerialNumber(display)); // A serial number for the monitor associated with the specified display, or a constant to indicate an exception—see the discussion below.
		iprintf("unitNumber = %d;\n", CGDisplayUnitNumber(display)); // A logical unit number for the specified display.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		iprintf("IOService = 0x%x;\n", CGDisplayIOServicePort(display)); // Return the IOKit service port of a display.
#pragma clang diagnostic pop
		iprintf("Main = %s;\n", CGDisplayIsMain(display) ? "true" : "false"); // If true, the specified display is currently the main display; otherwise, false.
		iprintf("Active = %s;\n", CGDisplayIsActive(display) ? "true" : "false"); // If true, the specified display is active; otherwise, false.
		iprintf("Online = %s;\n", CGDisplayIsOnline(display) ? "true" : "false"); // If true, the specified display is connected; otherwise, false.
		iprintf("Asleep = %s;\n", CGDisplayIsAsleep(display) ? "true" : "false"); // If YES, the specified display is in sleep mode; otherwise, NO.
		iprintf("Stereo = %s;\n", CGDisplayIsStereo(display) ? "true" : "false"); // If true, the specified display is running in a stereo graphics mode; otherwise, false.
		iprintf("Builtin = %s;\n", CGDisplayIsBuiltin(display) ? "true" : "false"); // If true, the specified display is considered to be a built-in display; otherwise, false.
		iprintf("Quartz Extreme = %s;\n", CGDisplayUsesOpenGLAcceleration(display) ? "true" : "false"); // If true, Quartz Extreme is used to render in the specified display; otherwise, false.
		iprintf("InMirrorSet = %s;\n", CGDisplayIsInMirrorSet(display) ? "true" : "false"); // If true, the specified display is a member of a software or hardware mirroring set; otherwise, false.
		iprintf("InHWMirrorSet = %s;\n", CGDisplayIsInHWMirrorSet(display) ? "true" : "false"); // If true, the specified display is a member of a hardware mirroring set; otherwise, false.
		iprintf("AlwaysInMirrorSet = %s;\n", CGDisplayIsAlwaysInMirrorSet(display) ? "true" : "false"); // If true, the specified display is in a mirroring set and cannot be removed from this set.
		CGRect bounds = CGDisplayBounds(display); // The bounds of the display, expressed as a rectangle in the global display coordinate space (relative to the upper-left corner of the main display).
#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_5
		API_OR_SDK_AVAILABLE_BEGIN(10.5, CGDisplayRotation) {
			iprintf("Rotation = %g°;\n", CGDisplayRotation(display)); // The rotation angle of the display in degrees, or 0 if the display is not valid. // available starting 10.5
		} API_OR_SDK_AVAILABLE_END
#endif
		iprintf("Bounds (l,t,w,h) = { %g, %g, %g, %g };\n", bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height);
		CGSize screenSize = CGDisplayScreenSize(display); // The size of the specified display in millimeters, or 0 if the display is not valid.
		iprintf("Size = %g x %g mm;\n", screenSize.width, screenSize.height);
		
#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_5
		float brightness = -1234.0f;
		int err = 123456789;
		if (&DisplayServicesGetBrightness) {
			err = DisplayServicesGetBrightness(display, &brightness);
			iprintf("Brightness = %g (err:%d)%s;\n", brightness, err, (brightness == -1234.0f) ? " (did not get brightness)" : "");
		}
#endif

#if MAC_OS_X_VERSION_SDK > MAC_OS_X_VERSION_10_6
		bool HDRSupportedSLS = false;
		bool HDRSupportedCGS = false;
#if MAC_OS_X_VERSION_SDK >= MAC_OS_VERSION_11_0
		bool HDREnabledSLS = false;
#endif
#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_11
		bool HDREnabledCGS = false;
#endif

#if MAC_OS_X_VERSION_SDK >= MAC_OS_VERSION_11_0
		if (SLSDisplaySupportsHDRMode) {
			iprintf("SupportsHDR(SLS) = %d;\n", HDRSupportedSLS = SLSDisplaySupportsHDRMode(display));
		}
#endif
#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_11
		if (CGSIsHDRSupported) {
			iprintf("SupportsHDR(CGS) = %d;\n", HDRSupportedCGS = CGSIsHDRSupported(display));
		}
#endif

		if (HDRSupportedSLS || HDRSupportedCGS) {
#if MAC_OS_X_VERSION_SDK >= MAC_OS_VERSION_11_0
			if (SLSDisplayIsHDRModeEnabled)  iprintf("HDREnabled(SLS) = %d;\n", HDREnabledSLS = SLSDisplayIsHDRModeEnabled(display));
#endif
#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_11
			if (CGSIsHDREnabled           )  iprintf("HDREnabled(CGS) = %d;\n", HDREnabledCGS =            CGSIsHDREnabled(display)); // returns 1 even for displays that don't support HDR. Maybe it's 1 if any display supports HDR?
#endif
	#if 0
			CGDisplayConfigRef config;
			result = CGBeginDisplayConfiguration(&config);
			if (!result) {
				//iprintf("SetBrightness to 0 (err: %d)\n", err = DisplayServicesSetBrightness(display, 0.0)); // we don't need to set brightness to change HDR setting.
#if MAC_OS_X_VERSION_SDK >= MAC_OS_VERSION_11_0
				if (SLSDisplaySetHDRModeEnabled) iprintf("ToggleHDR(SLS) = %x;\n", SLSDisplaySetHDRModeEnabled(display, HDREnabledSLS ? 0 : 1, 0, 0));
				else
#endif
#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_11
				if (CGSEnableHDR          ) iprintf("ToggleHDR(CGS) = %x;\n",     CGSEnableHDR           (display, HDREnabledCGS ? 0 : 1, 0, 0)); // I don't think CGSEnableHDR works - needs more investigation to find out how Catalina enables HDR.
#endif
				CGCompleteDisplayConfiguration(config, kCGConfigureForSession);
				//iprintf("SetBrightness to %g (err: %d)\n", brightness, err = DisplayServicesSetBrightness(display, brightness)); // we don't need to set brightness to change HDR setting.
#if MAC_OS_X_VERSION_SDK >= MAC_OS_VERSION_11_0
				if (SLSDisplayIsHDRModeEnabled)  iprintf("HDREnabled(SLS) = %d;\n", HDREnabledSLS = SLSDisplayIsHDRModeEnabled(display));
#endif
#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_11
				if (CGSIsHDREnabled           )  iprintf("HDREnabled(CGS) = %d;\n", HDREnabledCGS =            CGSIsHDREnabled(display)); // returns 1 even for displays that don't support HDR. Maybe it's 1 if any display supports HDR?
#endif
			}
	#endif
		}
#endif

#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_5
		API_OR_SDK_AVAILABLE_BEGIN(10.5, CGDisplayCopyColorSpace) {
			iprintf("ColorSpace = { ");
			CGColorSpaceRef space = CGDisplayCopyColorSpace(display); // available starting 10.5
			CFOutput(space);
			CGColorSpaceRelease(space);
			cprintf(" };\n");
		} API_OR_SDK_AVAILABLE_END
#endif

		OUTDENT iprintf("}, // Monitor[%d]\n", displayIndex);
	}
	OUTDENT iprintf("}; // DISPLAYS\n");

	{
		iprintf("-----------------------------------------------------\n");
		iprintf("IOSERVICE = {\n"); INDENT
		
		io_service_t service;
		
		for (uint32_t displayIndex = 0; displayIndex < displayCount; displayIndex++) {
			CGDirectDisplayID display = onlineDisplays[displayIndex];
			char *servicePath = NULL;
			service = 0;
			CGError result = CGSServiceForDisplayNumber(display, &service);
			if(result == kCGErrorSuccess) {
				servicePath = GetServicePath(service);
			}
			iprintf("Monitor[%d]: %s%s= {\n", displayIndex, servicePath ? servicePath : "", servicePath ? " " : ""); INDENT
			if(result == kCGErrorSuccess) {
				DumpDisplayService(service, modeAlias[displayIndex], "IOFramebuffer");
			}
			OUTDENT iprintf("}, // Monitor[%d]: %s\n\n", displayIndex, servicePath ? servicePath : "");
			if (servicePath) free (servicePath);
		}
		
		/*
			Apple Silicon services:
			API prefix         : IOService class
			IOAVAudioDriver    : DCPAVAudioDriver
			IOAVAudioInterface : DCPAVAudioInterfaceProxy
			IOAVController     : DCPAVControllerProxy
			IOAVDevice         : DCPAVDeviceProxy
			IOAVService        : DCPAVServiceProxy
			IOAVVideoInterface : DCPAVVideoInterfaceProxy
			IODPController     : DCPDPControllerProxy
			IODPDevice         : DCPDPDeviceProxy
			IODPService        : DCPDPServiceProxy
		*/

		const char *serviceTypes[] = { "IOFramebuffer", "IODisplay", "IOMobileFramebuffer", "DCPAVServiceProxy", "DCPDPServiceProxy" };
		for (int serviceTypeNdx = 0; serviceTypeNdx < sizeof(serviceTypes) / sizeof(serviceTypes[0]); serviceTypeNdx++) {
			io_iterator_t iterator;
			kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching(serviceTypes[serviceTypeNdx]), &iterator);
			if (kr == KERN_SUCCESS) {
				for (; IOIteratorIsValid(iterator) && (service = IOIteratorNext(iterator)); IOObjectRelease(service)) {
					bool found = false;
					for (int i = 0; i < dumpedCount && !(found = (dumpedServices[i] == service)); i++) { }
					if (!found) {
						char *servicePath = GetServicePath(service);
						iprintf("%s: %s%s= {\n", serviceTypes[serviceTypeNdx], servicePath ? servicePath : "", servicePath ? " " : ""); INDENT
						DumpDisplayService(service, 0, serviceTypes[serviceTypeNdx]);
						OUTDENT iprintf("}, // %s: %s\n\n", serviceTypes[serviceTypeNdx], servicePath ? servicePath : "");
						if (servicePath) free (servicePath);
					}
				} // for io_service_t
				IOObjectRelease(iterator);
			}
		}
		OUTDENT iprintf("}; // IOSERVICE\n");

		for (int i = 0; i < dumpedCount; i++) {
			IOObjectRelease(dumpedServices[dumpedCount]);
			dumpedServices[dumpedCount] = 0;
		}
	}

	iprintf("-----------------------------------------------------\n");
	iprintf("CURRENT MODE = {\n"); INDENT
	for (uint32_t displayIndex = 0; displayIndex < displayCount; displayIndex++) {
		CGDirectDisplayID display = onlineDisplays[displayIndex];
		iprintf("Monitor[%d] = {\n", displayIndex); INDENT
		uint32_t modeNum;

		{
			CGSGetCurrentDisplayMode(display, &modeNum);
			iprintf("current mode by CGSGetCurrentDisplayMode (private) = %d = { ", modeNum);
			DumpOneDisplayModeDescription(&modes[displayIndex][modeNum], modeAlias[displayIndex]);
			cprintf(" };\n");
		}

#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_6
		API_OR_SDK_AVAILABLE_BEGIN(10.6, CGDisplayCopyDisplayMode) {
			CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(display);
			iprintf("current mode by CGDisplayCopyDisplayMode               %s= { ", modeNum < 10 ? "" : modeNum < 100 ? " " : modeNum < 1000 ? "  " : "   ");
			DumpOneCGDisplayMode(currentMode, modeAlias[displayIndex]);
			cprintf(" };\n");
			if (&CGDisplayModeRelease) CGDisplayModeRelease(currentMode);
		} API_OR_SDK_AVAILABLE_END
#endif

		{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
			CFDictionaryRef modeDictionary = CGDisplayCurrentMode(display); // CGDisplayCurrentMode returns NULL when HDR is enabled
#pragma clang diagnostic pop
			iprintf("current mode by CGDisplayCurrentMode (deprecated)      %s= { ", modeNum < 10 ? "" : modeNum < 100 ? " " : modeNum < 1000 ? "  " : "   ");
			DumpOneCGDisplayMode((CGDisplayModeRef)modeDictionary, modeAlias[displayIndex]);
			cprintf(" };\n");
		}

		iprintf("current mode info by CGDisplay* (deprecated)           %s= { %dx%d %dbpp %dbpc %dcpp rowbytes:%d };\n",
			modeNum < 10 ? "" : modeNum < 100 ? " " : modeNum < 1000 ? "  " : "   ",
			(int)CGDisplayPixelsWide(display), // The display width in pixel units.
			(int)CGDisplayPixelsHigh(display), // The display height in pixel units.
			(int)CGDisplayBitsPerPixel(display), // The number of bits used to represent a pixel in the framebuffer.
			(int)CGDisplayBitsPerSample(display), // The number of bits used to represent a pixel component such as a color value in the framebuffer.
			(int)CGDisplaySamplesPerPixel(display), // The number of color components used to represent a pixel.
			(int)CGDisplayBytesPerRow(display) // The number of bytes per row in the display. This number also represents the stride between pixels in the same column of the display.
		);
		
		OUTDENT iprintf("}, // Monitor[%d]\n", displayIndex);
	}
	OUTDENT iprintf("}; // CURRENT MODE\n");

	iprintf("-----------------------------------------------------\n");
	iprintf("ALL MODES = {\n"); INDENT
	for (uint32_t displayIndex = 0; displayIndex < displayCount; displayIndex++) {
		CGDirectDisplayID display = onlineDisplays[displayIndex];
		iprintf("Monitor[%d] = {\n", displayIndex); INDENT

		{ // CGDisplayAvailableModes/CGDisplayCurrentMode
			/*
				CGDisplayAvailableModes/CGDisplayCurrentMode will include a set of modes for 8bpc framebuffer or 10bpc but not both.
				bpc is 8bpc even when doing 10bpc framebuffer or HDR.
				Usually the only difference between 8bpc and 10bpc modes is the mode number. It is even for 8bpc. The 10bpc mode number is 8bpc mode number plus 1.
			*/
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
			CFArrayRef availableModes = CGDisplayAvailableModes(display);
#pragma clang diagnostic pop
			iprintf("CGDisplayAvailableModes (deprecated) (%d modes) = {\n", (int)CFArrayGetCount(availableModes)); INDENT
			for ( int i = 0; i < CFArrayGetCount( availableModes ); i++ ) {
				CGDisplayModeRef displayMode = (CGDisplayModeRef)CFArrayGetValueAtIndex( availableModes, i );
				iprintf("{ ");
				DumpOneCGDisplayMode(displayMode, modeAlias[displayIndex]);
				cprintf(" },\n");
			}
			OUTDENT iprintf("}; // CGDisplayAvailableModes\n");
		} // CGDisplayAvailableModes
		
#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_6
		API_OR_SDK_AVAILABLE_BEGIN(10.6, CGDisplayCopyAllDisplayModes) // not available in 10.5 and earlier
		{ // CGDisplayCopyAllDisplayModes
			/*
				CGDisplayCopyAllDisplayModes/CGDisplayCopyDisplayMode is same as CGDisplayAvailableModes/CGDisplayCurrentMode above but includes the following 7 fields and there is double the modes - one for 8bpc framebuffer and another for 10bpc

					kCGDisplayModeIsTelevisionOutput = 0; // actually, this flag is included for CGDisplayAvailableModes/CGDisplayCurrentMode in Monterey but not included in Catalina
					kCGDisplayModeIsInterlaced = 0;
					kCGDisplayModeIsStretched = 0;
					kCGDisplayModeIsUnavailable = 0;
					kCGDisplayModeSuitableForUI = 1;
					DepthFormat = 4;                                      DepthFormat = 8;
					PixelEncoding = "--------RRRRRRRRGGGGGGGGBBBBBBBB";   PixelEncoding = "--RRRRRRRRRRGGGGGGGGGGBBBBBBBBBB";

				The double modes have 8bpc first (even) and 10bpc second (odd) with a mode number 1 greater than the 8bpc mode number like this:
					Mode = 0;                                             Mode = 1;
			*/

			CFStringRef key = NULL;
			CFArrayRef availableModes;
			{
				CFStringRef keys[1] = { NULL };
				CFBooleanRef values[1] = { kCFBooleanTrue }; // include HiDPI modes (kCGDisplayResolution > 1) and include multiple bit depths like 10bpc instead of just 8bpc)

#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_8
				API_OR_SDK_AVAILABLE_BEGIN(10.8, kCGDisplayShowDuplicateLowResolutionModes) {
					keys[0] = kCGDisplayShowDuplicateLowResolutionModes; // [0]	CFStringRef	"kCGDisplayResolution"
				} API_OR_SDK_AVAILABLE_END
#endif
				if (keys[0] == NULL) {
					//key = CFSTR("kCGDisplayResolution"); // This doesn't work. Try CFStringCreate instead.
					key = CFStringCreateWithCString(kCFAllocatorDefault, "kCGDisplayResolution", kCFStringEncodingUTF8); // This also doesn't work. We'll just warn the user.
					keys[0] = key;
				}

				CFDictionaryRef options = CFDictionaryCreate(kCFAllocatorDefault, (const void**) keys, (const void**) values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
				availableModes = CGDisplayCopyAllDisplayModes(display, options);
				CFRelease( options );
			}
			iprintf("CGDisplayCopyAllDisplayModes (%d modes) = {%s\n",
				(int)CFArrayGetCount(availableModes),
				key ? " // warning: kCGDisplayShowDuplicateLowResolutionModes may not work when AllRez is built with SDK < 10.8"
				: ""
			); INDENT
			if (key) CFRelease(key);

			//CFOutput(availableModes); // print them one at a time
			for ( int i = 0; i < CFArrayGetCount( availableModes ); i++ ) {
				CGDisplayModeRef displayMode = (CGDisplayModeRef)CFArrayGetValueAtIndex( availableModes, i );
				iprintf("{ ");
				DumpOneCGDisplayMode(displayMode, modeAlias[displayIndex]);
				cprintf(" },\n");
			}

			CFRelease( availableModes );
			OUTDENT iprintf("}; // CGDisplayCopyAllDisplayModes\n");
		} API_OR_SDK_AVAILABLE_END // CGDisplayCopyAllDisplayModes
#endif

		{ // CGSGetNumberOfDisplayModes
			/*
				CGSGetDisplayModeDescriptionOfLength is similar to CGDisplayCopyAllDisplayModes but includes some other fields.
				It adds modes that have the 0x200000 = kDisplayModeValidForMirroringFlag or the 0x01000000 = kDisplayModeValidForAirPlayFlag IOFlag set.
			*/
			iprintf("CGSGetDisplayModeDescriptionOfLength (%d modes) = {\n", nModes[displayIndex]); INDENT
			for (int i = 0; i < nModes[displayIndex]; i++) {
				iprintf("{ ");
				DumpOneDisplayModeDescription(&modes[displayIndex][i], modeAlias[displayIndex]);
				cprintf(" }");

#if MAC_OS_X_VERSION_SDK >= MAC_OS_VERSION_12_0
				bool VRR = false;
				bool ProMotion = false;
				float minRefreshRate = 0.0f;
				UInt32 result = 0;

				if (SLSIsDisplayModeVRR) VRR = SLSIsDisplayModeVRR(display, i);
				if (SLSIsDisplayModeProMotion) ProMotion = SLSIsDisplayModeProMotion(display, i);
				if (SLSGetDisplayModeMinRefreshRate) result = SLSGetDisplayModeMinRefreshRate(display, i, &minRefreshRate);

				if (VRR) cprintf(" VRR");
				if (ProMotion) cprintf(" ProMotion"); // Note: ProMotion is true only when VRR is true
				if (minRefreshRate) cprintf(" min:%gHz", minRefreshRate); // this is zero unless VRR is true
				if (result) cprintf(" result:%d", (uint32_t)result);
#endif
				cprintf(",\n");

				if (doSetDisplayModeTest) {
					if (modes[displayIndex][i].width == 2560) {
						CGDisplayConfigRef config;
						CGError result = CGBeginDisplayConfiguration(&config);
						if (!result) {
							result = CGSConfigureDisplayMode(config, display, i);
							if (result) {
								iprintf("Error configuring display mode %d\n", (uint32_t)result);
							}
							result = CGCompleteDisplayConfiguration(config, kCGConfigureForSession);
							if (result) {
								iprintf("Error completing configuration %d\n", (uint32_t)result);
							}
							else {
								uint32_t modeNum;
								CGSGetCurrentDisplayMode(display, &modeNum);
								if (modeNum != i) {
									iprintf("Set mode %d attempted but mode is %d\n", i, modeNum);
								}
								else {
									iprintf("Set mode successful\n");
								}
							}
						}
					}
				}

			}
			OUTDENT iprintf("}; // CGSGetDisplayModeDescriptionOfLength\n");
		} // CGSGetNumberOfDisplayModes

		OUTDENT iprintf("}, // Monitor[%d]\n", displayIndex);
	} // for displayIndex
	OUTDENT iprintf("}; // ALL MODES\n");
}

static void DisplayPortTest(void) {
	io_iterator_t iterator;
	kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching("IOFramebuffer"), &iterator);
	if (kr == KERN_SUCCESS) {
		for (io_service_t service; IOIteratorIsValid(iterator) && (service = IOIteratorNext(iterator)); IOObjectRelease(service)) {
			char *servicePath = GetServicePath(service);
			iprintf("IOFramebuffer: %s%s= {\n", servicePath ? servicePath : "", servicePath ? " " : ""); INDENT

			io_service_t ioFramebufferService = service;
			IOItemCount i2cInterfaceCount;
			IOReturn result;
			result = IOFBGetI2CInterfaceCount(ioFramebufferService, &i2cInterfaceCount);
			if (KERN_SUCCESS == result) {
				for (int interfaceBus = 0; interfaceBus < i2cInterfaceCount; interfaceBus++) {
					io_service_t i2cservice;
					result = IOFBCopyI2CInterfaceForBus(ioFramebufferService, interfaceBus, &i2cservice);
					if (KERN_SUCCESS == result) {
						CFMutableDictionaryRef dict = NULL;
						result = IORegistryEntryCreateCFProperties(i2cservice, &dict, kCFAllocatorDefault, kNilOptions);
						if (KERN_SUCCESS == result) {
							UInt32 val_IOI2CTransactionTypes = 0;
							CFTypeRef cf_IOI2CTransactionTypes = CFDictionaryGetValue(dict, CFSTR(kIOI2CTransactionTypesKey));
							if (cf_IOI2CTransactionTypes) {
								if (CFGetTypeID(cf_IOI2CTransactionTypes) == CFNumberGetTypeID()) {
									if (CFNumberGetValue((CFNumberRef)cf_IOI2CTransactionTypes, kCFNumberSInt32Type, &val_IOI2CTransactionTypes)) {

										IOI2CConnectRef i2cconnect;
										result = IOI2CInterfaceOpen(i2cservice, kNilOptions, &i2cconnect);
										if (KERN_SUCCESS == result) {
											
											if (val_IOI2CTransactionTypes & (1 << kIOI2CDisplayPortNativeTransactionType)) {
												//UInt8 path[16];
												//int pathLength = 0;
												//DoOneDisplayPort(ioFramebufferService, i2cconnect, path, 0, true);
												UInt8 *dpcd = (UInt8 *)malloc(0x100000);
												bzero(dpcd, sizeof(0x100000));
												do {
													result = dp_dpcd_read(i2cconnect, 0, 16, &dpcd[0]);
													if (result) break;
													result = dp_dpcd_read(i2cconnect, 0x20, 16, &dpcd[0x20]);
													if (result) break;

													int numPorts = dpcd[DP_DOWN_STREAM_PORT_COUNT] & DP_PORT_COUNT_MASK;
													if (numPorts) {
														const unsigned char* messages[] = {
//#if 0
															"\p\x16\xc2\xcf\x14\xac",
																// 10 c3 c6 14 00 9c

															"\p\x10\x02\xcb\x01\xd5",
																// 10 2d 8c 01 00 00 00 00 00 00 00 00 00 00 00 00
																// 00 00 00 00 04 90 c0 01 00 00 00 00 00 00 00 00
																// 00 00 00 00 00 00 00 00 00 00 00 02 00 00 00 a5
																//
																// 10 25 45 00 00 00 00 00 00 00 00 00 00 00 00 00
																// 00 00 00 23 c0 14 00 00 00 00 00 00 00 00 00 00
																// 00 00 00 00 00 00 00 73

															"\p\x10\x43\xc7\x10\x30\x46",
																// 10 47 c0 10 31 07 80 07 80 22 00 00 00 00 00 00
															
															"\p\x21\x30\x02\xc4\x01\xd5",
																// 20 30 2c 8c 01 00 00 00 00 00 00 00 00 00 00 00
																// 00 00 00 00 00 04 90 c0 21 c0 14 2d 6e 13 00 01
																// 00 00 00 2d 6e 13 00 01 00 00 00 00 02 00 00 b6
																//
																// 20 30 26 4f 00 00 00 00 00 00 00 00 00 00 00 00
																// 00 00 00 00 00 03 00 00 00 00 00 00 00 00 00 00
																// 00 00 00 00 00 00 00 00 00 a9
//#endif
															"\p\x10\x16\xc3\x21\x30\x00\x30\x10\x5a\x9b\xff\xff\x00\x00\x00\x00\x5a\x9b\xff\xff\x00\x00\x00\x00\x97", // 10 16 c3 21 30 00 30 10 5a 9b ff ff 00 00 00 00 5a 9b ff ff 00 00 00 00 97
																// 10 03 ce 21 03 14 00 00 00 00 00 00 00 00 00 00

															"\p\x21\x30\x43\xc8\x10\x10\xe2",
																// 20 30 47 c5 10 11 07 80 07 80 31 00 00 00 00 00
															
															"\p\x32\x31\x02\xc6\x01\xd5",
																// 30 31 2c 83 01 2d 6e 13 00 01 00 00 00 2d 6e 13
																// 00 01 00 00 00 04 90 c0 01 00 00 00 00 00 00 00
																// 00 00 00 00 00 00 00 00 00 00 00 00 02 00 00 21
																//
																// 30 31 26 40 00 00 00 00 00 00 00 00 00 00 00 00
																// 00 00 00 00 00 23 c0 12 bb ad 36 48 f2 87 ec 11
																// bf dc 40 8d 5c b4 0c 1b 00 d5
														
															"\p\x32\x31\x43\xca\x10\x30\x46",
																// 30 31 47 ca 10 31 07 80 07 80 22 00 00 00 00 00
//#endif
														};

														for (int i=0; i < sizeof(messages) / sizeof(UInt8*); i++) {
															iprintf("=====================================================================\n");
															Sideband_MSG_Body *body;
															int bodyLength;
															DpError dperr;
															gDumpSidebandMessage = kReq | kRep;
															UInt8 tmp[200];
															int len = messages[i][0];
															memcpy(tmp, &messages[i][1], len);
															result = mst_transaction(ioFramebufferService, i2cconnect, tmp, len, &body, &bodyLength, &dperr);
															gDumpSidebandMessage = 0;
															cprintf("\n");
//															if (result) break;
														
#if 0
															for (int port = 0; port < numPorts; port++) {
																path[pathLength] = port;
																iprintf("Port %d = {\n", port); INDENT
																DoOneDisplayPort(ioFramebufferService, i2cconnect, path, pathLength + 1, true);
																OUTDENT iprintf("}; // Port %d\n", port);
															} // for port
#endif
															//usleep(3000000);

														} // for messages
														return;
													} // if mst
												} while (0);
											} // if kIOI2CDisplayPortNativeTransactionType
											result = IOI2CInterfaceClose(i2cconnect, kNilOptions);
										} // IOI2CInterfaceOpen
									} // if CFNumberGetValue
								} // if CFNumberGetTypeID
							} // if cf_IOI2CTransactionTypes
						} // if IORegistryEntryCreateCFProperties
					} // if IOFBCopyI2CInterfaceForBus
				} // for iterfaceBus
			} // if IOFBGetI2CInterfaceCount

			OUTDENT iprintf("}, // IOFramebuffer: %s\n\n", servicePath ? servicePath : "");
			if (servicePath) free (servicePath);
		} // for io_service_t
		IOObjectRelease(iterator);
	}
} // DisplayPortTest

static void DisplayPortMessages(void) {
	{
		typedef struct {
			UInt32 address;
			const unsigned char *data;
		} SidebandMessage;
		
		SidebandMessage messages[] = {

			{ 0x01000, "\p\x16\xc2\xcf\x14\xac" },
			{ 0x01400, "\p\x10\xc3\xc6\x14\x00\x9c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x10\x02\xcb\x01\xd5" },
			{ 0x01400, "\p\x10\x2d\x8c\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01410, "\p\x00\x00\x00\x00\x04\x90\xc0\x01\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01420, "\p\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\xa5" },
			{ 0x01400, "\p\x10\x25\x45\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01410, "\p\x00\x00\x00\x23\xc0\x14\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01420, "\p\x00\x00\x00\x00\x00\x00\x00\x73" },
			{ 0x01000, "\p\x10\x43\xc7\x10\x30\x46" },
			{ 0x01400, "\p\x10\x47\xc0\x10\x31\x07\x80\x07\x80\x22\x00\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x21\x30\x02\xc4\x01\xd5" },
			{ 0x01400, "\p\x20\x30\x2c\x8c\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01410, "\p\x00\x00\x00\x00\x00\x04\x90\xc0\x21\xc0\x14\x2d\x6e\x13\x00\x01" },
			{ 0x01420, "\p\x00\x00\x00\x2d\x6e\x13\x00\x01\x00\x00\x00\x00\x02\x00\x00\xb6" },
			{ 0x01400, "\p\x20\x30\x26\x4f\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01410, "\p\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01420, "\p\x00\x00\x00\x00\x00\x00\x00\x00\x00\xa9" },
			{ 0x01000, "\p\x10\x16\xc3\x21\x30\x00\x30\x10\x5a\x9b\xff\xff\x00\x00\x00\x00" },
			{ 0x01010, "\p\x5a\x9b\xff\xff\x00\x00\x00\x00\x97" },
			{ 0x01400, "\p\x10\x03\xce\x21\x03\x14\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x21\x30\x43\xc8\x10\x10\xe2" },
			{ 0x01400, "\p\x20\x30\x47\xc5\x10\x11\x07\x80\x07\x80\x31\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x32\x31\x02\xc6\x01\xd5" },
			{ 0x01400, "\p\x30\x31\x2c\x83\x01\x2d\x6e\x13\x00\x01\x00\x00\x00\x2d\x6e\x13" },
			{ 0x01410, "\p\x00\x01\x00\x00\x00\x04\x90\xc0\x01\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01420, "\p\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x21" },
			{ 0x01400, "\p\x30\x31\x26\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01410, "\p\x00\x00\x00\x00\x00\x23\xc0\x12\xbb\xad\x36\x48\xf2\x87\xec\x11" },
			{ 0x01420, "\p\xbf\xdc\x40\x8d\x5c\xb4\x0c\x1b\x00\xd5" },
			{ 0x01000, "\p\x32\x31\x43\xca\x10\x30\x46" },
			{ 0x01400, "\p\x30\x31\x47\xca\x10\x31\x07\x80\x07\x80\x22\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x02\xc0\x01\xd5" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x86\x01\xbb\xad\x36\x48\xf2\x87\xec\x11\xbf\xdc" },
			{ 0x01410, "\p\x40\x8d\x5c\xb4\x0c\x1b\x03\x90\xc0\x38\x40\x12\x00\x00\x00\x00" },
			{ 0x01420, "\p\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x12\x31\x40\x98" },
			{ 0x01400, "\p\x40\x31\x30\x13\x4d\x11\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa" },
			{ 0x01410, "\p\xbb\xcc\xdd\xee\xff\x78\x00\x84" },
			{ 0x01000, "\p\x43\x31\x30\x43\xcc\x10\x80\x5f" },
			{ 0x01400, "\p\x40\x31\x30\x47\xc7\x10\x81\x07\x80\x07\x80\x9e\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x09\xc1\x22\x81\x50\x01\x00\x10\x50\x01\xbc" },
			{ 0x01400, "\p\x40\x31\x30\x05\xc4\x22\x08\x01\x00\xee\x00\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x09\xc1\x22\x81\x50\x01\x00\x10\x50\x80\x86" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x86\x22\x08\x80\x00\xff\xff\xff\xff\xff\xff\x00" },
			{ 0x01410, "\p\x10\xac\xbf\xa0\x42\x4c\x4c\x30\x23\x1a\x01\x04\xa5\x35\x1e\x78" },
			{ 0x01420, "\p\x3a\xe2\x45\xa8\x55\x4d\xa3\x26\x0b\x50\x54\xa5\x4b\x00\x71\x80" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x0d\x4f\x81\x80\xa9\xc0\xa9\x40\xd1\xc0\xe1\x00" },
			{ 0x01410, "\p\xd1\x00\x01\x01\xa3\x66\x00\xa0\xf0\x70\x1f\x80\x30\x20\x35\x00" },
			{ 0x01420, "\p\x0f\x28\x21\x00\x00\x1a\x00\x00\x00\xff\x00\x47\x33\x44\x37\xa4" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x0d\x46\x36\x38\x4f\x30\x4c\x4c\x42\x0a\x00\x00" },
			{ 0x01410, "\p\x00\xfc\x00\x44\x45\x4c\x4c\x20\x50\x32\x34\x31\x35\x51\x0a\x20" },
			{ 0x01420, "\p\x00\x00\x00\xfd\x00\x1d\x4c\x1e\x8c\x1e\x00\x0a\x20\x20\x20\x33" },
			{ 0x01400, "\p\x40\x31\x30\x06\x40\x20\x20\x20\x01\x4d\x3c\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x09\xc1\x22\x81\x50\x01\x80\x10\x50\x80\xdd" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x86\x22\x08\x80\x02\x03\x1d\xf1\x50\x10\x1f\x20" },
			{ 0x01410, "\p\x05\x14\x04\x13\x12\x11\x03\x02\x16\x15\x07\x06\x01\x23\x09\x1f" },
			{ 0x01420, "\p\x07\x83\x01\x00\x00\x56\x5e\x00\xa0\xa0\xa0\x29\x50\x30\x20\xa6" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x0d\x35\x00\x0f\x28\x21\x00\x00\x1a\x02\x3a\x80" },
			{ 0x01410, "\p\x18\x71\x38\x2d\x40\x58\x2c\x25\x00\x0f\x28\x21\x00\x00\x1e\x01" },
			{ 0x01420, "\p\x1d\x00\x72\x51\xd0\x1e\x20\x6e\x28\x55\x00\x0f\x28\x21\x00\x91" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x0d\x00\x1e\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01410, "\p\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01420, "\p\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xa5" },
			{ 0x01400, "\p\x40\x31\x30\x06\x40\x00\x00\x00\x00\x8a\xb9\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x43\xcc\x10\x10\xe2" },
			{ 0x01400, "\p\x40\x31\x30\x47\xc7\x10\x11\x04\xec\x04\xec\xce\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x09\xc1\x22\x11\x50\x01\x00\x10\x50\x01\xcb" },
			{ 0x01400, "\p\x40\x31\x30\x05\xc4\x22\x01\x01\x00\x8b\x00\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x09\xc1\x22\x11\x50\x01\x00\x10\x50\x80\xf1" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x86\x22\x01\x80\x00\xff\xff\xff\xff\xff\xff\x00" },
			{ 0x01410, "\p\x04\x72\xb1\x06\x53\x8f\x00\x85\x32\x1c\x01\x04\xb5\x3c\x22\x78" },
			{ 0x01420, "\p\x3b\x27\x11\xac\x51\x35\xb5\x26\x0e\x50\x54\x23\x48\x00\x81\x7c" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x0d\x40\x81\x80\x81\xc0\x81\x00\x95\x00\xb3\x00" },
			{ 0x01410, "\p\xd1\xc0\x01\x01\x4d\xd0\x00\xa0\xf0\x70\x3e\x80\x30\x20\x35\x00" },
			{ 0x01420, "\p\x55\x50\x21\x00\x00\x1a\xb4\x66\x00\xa0\xf0\x70\x1f\x80\x08\xe3" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x0d\x20\x18\x04\x55\x50\x21\x00\x00\x1a\x00\x00" },
			{ 0x01410, "\p\x00\xfd\x0c\x30\x90\xff\xff\x6b\x01\x0a\x20\x20\x20\x20\x20\x20" },
			{ 0x01420, "\p\x00\x00\x00\xfc\x00\x58\x56\x32\x37\x33\x4b\x0a\x20\x20\x20\xb4" },
			{ 0x01400, "\p\x40\x31\x30\x06\x40\x20\x20\x20\x02\x21\xcf\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x09\xc1\x22\x11\x50\x01\x80\x10\x50\x80\xaa" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x86\x22\x01\x80\x02\x03\x48\xf1\x51\x01\x03\x04" },
			{ 0x01410, "\p\x12\x13\x05\x14\x1f\x90\x07\x02\x5d\x5e\x5f\x60\x61\x3f\x23\x09" },
			{ 0x01420, "\p\x07\x07\x83\x01\x00\x00\xe2\x00\xc0\x6d\x03\x0c\x00\x20\x00\x16" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x0d\x38\x78\x20\x00\x60\x01\x02\x03\x68\x1a\x00" },
			{ 0x01410, "\p\x00\x01\x01\x30\x90\x00\xe3\x05\xe3\x01\xe4\x0f\x00\xc0\x00\xe6" },
			{ 0x01420, "\p\x06\x07\x01\x61\x56\x1c\x07\x82\x80\x54\x70\x38\x4d\x40\x08\x20" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x0d\x20\xf8\x0c\x56\x50\x21\x00\x00\x1a\x40\xe7" },
			{ 0x01410, "\p\x00\x6a\xa0\xa0\x6a\x50\x08\x20\x98\x04\x55\x50\x21\x00\x00\x1a" },
			{ 0x01420, "\p\x6f\xc2\x00\xa0\xa0\xa0\x55\x50\x30\x20\x35\x00\x55\x50\x21\x70" },
			{ 0x01400, "\p\x40\x31\x30\x06\x40\x00\x00\x1e\x00\xb0\x46\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x0d\xc6\x22\x12\x30\x01\x01\x10\x50\x01\x00\x10\x50" },
			{ 0x01010, "\p\x80\x03" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x86\x22\x01\x80\x70\x12\x79\x00\x00\x03\x01\x28" },
			{ 0x01410, "\p\x9a\xa0\x01\x84\xff\x0e\xa0\x00\x2f\x80\x21\x00\x6f\x08\x3e\x00" },
			{ 0x01420, "\p\x03\x00\x05\x00\xe0\xf6\x00\x04\x7f\x07\x59\x00\x2f\x80\x1f\xa0" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x0d\x00\x6f\x08\x19\x00\x01\x00\x03\x00\x26\x00" },
			{ 0x01410, "\p\x08\x07\x07\x03\x03\xe0\x7f\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01420, "\p\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x98" },
			{ 0x01400, "\p\x40\x31\x30\x2b\x0d\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01410, "\p\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01420, "\p\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01400, "\p\x40\x31\x30\x06\x40\x00\x00\x00\x94\x90\x8a\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x43\xcc\x24\x10\xcb" },
			{ 0x01400, "\p\x40\x31\x30\x43\xc0\x24\x10\xcb\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x46\xce\x11\x10\x01\x04\x27\xa4" },
			{ 0x01400, "\p\x40\x31\x30\x46\xc2\x11\x10\x01\x04\x38\x8b\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x46\xce\x11\x10\x01\x00\x00\x78" },
			{ 0x01400, "\p\x40\x31\x30\x46\xc2\x11\x10\x01\x00\x00\x78\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x43\xcc\x25\x10\xc0" },
			{ 0x01400, "\p\x40\x31\x30\x43\xc0\x25\x10\xc0\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x46\xce\x11\x10\x01\x02\x13\x96" },
			{ 0x01400, "\p\x40\x31\x30\x46\xc2\x11\x10\x01\x02\x1c\xeb\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x43\xcc\x24\x80\x76" },
			{ 0x01400, "\p\x40\x31\x30\x43\xc0\x24\x80\x76\x00\x00\x00\x00\x00\x00\x00\x00" },
			{ 0x01000, "\p\x43\x31\x30\x47\xcb\x11\x81\x02\x01\x0a\x00\x59" },
			{ 0x01400, "\p\x40\x31\x30\x46\xc2\x11\x80\x02\x01\x18\xad\x00\x00\x00\x00\x00" },

			{ 0x01000, "\p\x16\xc2\xcf\x14\xac\x11\x22\x33\x44\x55\x66\x77\x88" },
			{ 0x01400, "\p\x10\xc3\xc6\x14\x00\x9c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" }

		};

		for (int i=0; i < sizeof(messages) / sizeof(SidebandMessage); i++) {
			DumpOneDisplayPortMessage((UInt8*)&messages[i].data[1], messages[i].data[0], messages[i].address);
			cprintf("\n=============================\n");
		}
	}
} // DisplayPortMessages

int main(int argc, const char * argv[]) {
//	@autoreleasepool {
	const char* macOSName = DarwinMajorVersion() < 12 ? "Mac OS X" : DarwinMajorVersion() < 16 ? "OS X" : "macOS";

	printf("AllRez %s on %s %s (Darwin %d.%d.%d %s) built on %s at %s using SDK %d.%d.%d%s\n",
		ARCHITECTURE,
		macOSName,
		MacOSVersion(),
		DarwinMajorVersion(),
		DarwinMinorVersion(),
		DarwinRevision(),
		MachineType(),
		__DATE__,
		__TIME__,
		MAC_OS_X_VERSION_SDK / ((MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_10) ? 100 : 10000),
		(MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_10) ? (MAC_OS_X_VERSION_SDK % 100 / 10) : (MAC_OS_X_VERSION_SDK % 10000 / 100),
		MAC_OS_X_VERSION_SDK % ((MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_10) ? 10 : 100),
#ifdef MAX_TESTED_MAC_OS_X_VERSION_SDK
		"+"
#else
		""
#endif
	);

	if (argc > 1) {
		getchar();
	}

	if (doSetupIOFB) {
	
		if (IofbAvailable(testDisplayIndex)) {
			// Adjust settings in joevt Lilu / WhateverGreen patches
			
			//IofbSetDebugEnabled(true);
			//IofbSetAttributeForDisplay(kIOFBUnused, 'iofb', 'trac', kIOFBAll, kIOFBUnused); // enable all Trace control options // TRACE_MASK
			
			//IofbSetAttributeForDisplay(notTestDisplayIndex, 'iofb', 'i2cr', false, kIOFBUnused); // dump doi2cRequest
			//IofbSetAttributeForDisplay(notTestDisplayIndex, 'iofb', 'attr', false, kIOFBUnused); // dump get/set atributes
			//IofbSetAttributeForDisplay(notTestDisplayIndex, 'iofb', 'sbnd', false, kIOFBUnused); // don't use sideband property
			//IofbSetAttributeForDisplay(notTestDisplayIndex, 'iofb', 'vala', false, kIOFBUnused); // don't validate all modes
			
			//IofbSetAttributeForDisplay(testDisplayIndex, 'iofb', 'i2cr', true, kIOFBUnused); // dump doi2cRequest
			//IofbSetAttributeForDisplay(testDisplayIndex, 'iofb', 'attr', true, kIOFBUnused); // dump get/set atributes
			//IofbSetAttributeForDisplay(testDisplayIndex, 'iofb', 'sbnd', false, kIOFBUnused); // don't use sideband property
			//IofbSetAttributeForDisplay(testDisplayIndex, 'iofb', 'vala', true, kIOFBUnused); // validate all modes

			//IofbSetAttributeForDisplay(testDisplayIndex, 'iofb', kConnectionColorModesSupported      , kIOFBDontOverride, kIOFBUnused); // override set -> set
			//IofbSetAttributeForDisplay(testDisplayIndex, 'iofb', kConnectionColorDepthsSupported     , kIOFBDontOverride, kIOFBUnused); // override set -> set
			//IofbSetAttributeForDisplay(testDisplayIndex, 'iofb', kConnectionControllerDepthsSupported, kIOFBDontOverride, kIOFBUnused); // override get -> set
			//IofbSetAttributeForDisplay(testDisplayIndex, 'atfc', kIOFBConnectIndex0, kConnectionColorDepthsSupported, kIOFBAll);
			//IofbSetAttributeForDisplay(testDisplayIndex, 'atfc', kIOFBConnectIndex0, kConnectionColorModesSupported , kIOFBAll);
			
			// Can't poll the interrupt because the graphics driver may clear the interrupt after we read
			// only part of the first message - so the next part of the first message may be from the second message.
			// Instead, we must use the sideband flag which makes the WhateverGreen iofb patch read the entire
			// sideband message before the interrupt is cleared.
			// Actually, we can use the polling method for interrupts. Polling works because clearing the interrupt
			// doesn't do anything until after the entire reply is read, or the checksum and sequence number is
			// enough to determine if the reply is valid and we can retry if it is not.
			//IofbSetAttributeForDisplay(testDisplayIndex, 'iofb', 'sbnd', false, kIOFBUnused); // don't use sideband property
			
			IofbSetAttributeForDisplay(testDisplayIndex, 'iofb', 'vala', true, kIOFBUnused); // validate all modes
			IofbSetAttributeForDisplay(testDisplayIndex, 'iofb', kConnectionColorModesSupported,
				kIODisplayColorModeRGB | kIODisplayColorModeYCbCr422 | kIODisplayColorModeYCbCr444,
				kIOFBUnused
			); // override set -> set
			IofbSetAttributeForDisplay(testDisplayIndex, 'iofb', kConnectionColorDepthsSupported,
				kIODisplayRGBColorComponentBits6 | kIODisplayRGBColorComponentBits8 | kIODisplayRGBColorComponentBits10 |
				kIODisplayRGBColorComponentBits12 | kIODisplayRGBColorComponentBits14 | kIODisplayRGBColorComponentBits16 |
				kIODisplayYCbCr444ColorComponentBits6 | kIODisplayYCbCr444ColorComponentBits8 | kIODisplayYCbCr444ColorComponentBits10 |
				kIODisplayYCbCr444ColorComponentBits12 | kIODisplayYCbCr444ColorComponentBits14 | kIODisplayYCbCr444ColorComponentBits16 |
				kIODisplayYCbCr422ColorComponentBits6 | kIODisplayYCbCr422ColorComponentBits8 | kIODisplayYCbCr422ColorComponentBits10 |
				kIODisplayYCbCr422ColorComponentBits12 | kIODisplayYCbCr422ColorComponentBits14 | kIODisplayYCbCr422ColorComponentBits16,
				kIOFBUnused
			); // override set -> set
			IofbSetAttributeForDisplay(testDisplayIndex, 'iofb', kConnectionControllerDepthsSupported,
				kIODisplayRGBColorComponentBits6 | kIODisplayRGBColorComponentBits8 | kIODisplayRGBColorComponentBits10 |
				kIODisplayRGBColorComponentBits12 | kIODisplayRGBColorComponentBits14 | kIODisplayRGBColorComponentBits16 |
				kIODisplayYCbCr444ColorComponentBits6 | kIODisplayYCbCr444ColorComponentBits8 | kIODisplayYCbCr444ColorComponentBits10 |
				kIODisplayYCbCr444ColorComponentBits12 | kIODisplayYCbCr444ColorComponentBits14 | kIODisplayYCbCr444ColorComponentBits16 |
				kIODisplayYCbCr422ColorComponentBits6 | kIODisplayYCbCr422ColorComponentBits8 | kIODisplayYCbCr422ColorComponentBits10 |
				kIODisplayYCbCr422ColorComponentBits12 | kIODisplayYCbCr422ColorComponentBits14 | kIODisplayYCbCr422ColorComponentBits16,
				kIOFBUnused
			); // override get -> set
		}
	}

#if 0
		// Test DisplayPort sideband message super long path
		{
			UInt8 path[] = { 15,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
			UInt32 msgLength;
			UInt8 *reqdata = mst_encode_dpcd_read(path, 15, &msgLength, 0x12345, 16);
			iprintf("msgLength=%d\n", msgLength);
			DumpOneDisplayPortMessage(reqdata, msgLength, 0x1000);
			cprintf("\n");
		}
#endif
		
#if 1
		if (doAttributeTest) {
			//DoAttributeTest(testDisplayIndex);
			DoAttributeTest(0);
			DoAttributeTest(1);
		}
		if (doEdidOverrideTest) {
			DoEDIDOverrideTest();
		}
		if (doParseTest) {
			DisplayPortMessages();
		}
		
		if (doDisplayPortTest) {
			DisplayPortTest();
		}
		if (doDumpAll) {
			DumpAllDisplaysInfo();
		}

#endif

		if (doIogDiagnose) {

			if (DarwinMajorVersion() >= 17 && DarwinMajorVersion() <= 18) { // 10.13 and 10.14
				iprintf("iogdiagnose 10.14.3 = { // (supports 10.13.4, Report version up to 6)\n"); INDENT
				iogdiagnose6(false, NULL);
				OUTDENT iprintf("}; // iogdiagnose 10.14.3\n");
			}

#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_13

			if (DarwinMajorVersion() == 18) { // 10.14
				iprintf("iogdiagnose 10.14.6 = { // (supports 10.14.4, Report version 7 to 9)\n"); INDENT
				iogdiagnose9(false, NULL);
				OUTDENT iprintf("}; // iogdiagnose 10.14.6\n");
			}

			if (DarwinMajorVersion() == 19) { // 10.15
				iprintf("iogdiagnose 10.15.7 = { // (supports 10.15, Report version 7 to 9)\n"); INDENT
				iogdiagnose_10_15_7(false, NULL);
				OUTDENT iprintf("}; // iogdiagnose 10.15.7\n");
			}

			if (DarwinMajorVersion() >= 20) { // 11.0
				iprintf("iogdiagnose 13.2 = { // (supports 11.0, Report version 7 to 9)\n"); INDENT
				iogdiagnose(false, NULL);
				OUTDENT iprintf("}; // iogdiagnose 13.2\n");
			}
#endif
		}

	//	} // @autoreleasepool
	return 0;
} // main
