//
//  main.m
//  AllRez
//
//  Created by joevt on 2022-01-03.
//
//=================================================================================================================================
// Apple includes

//#include <Foundation/Foundation.h>

#include <CoreGraphics/CGDirectDisplay.h>
#include <CoreGraphics/CGDisplayConfiguration.h>
//#include <CoreGraphics/CoreGraphicsPrivate.h>

#include <IOKit/graphics/IOGraphicsLib.h>
#include <IOKit/graphics/IOGraphicsTypes.h>
#include <IOKit/graphics/IOGraphicsInterfaceTypes.h>
//#include <IOKit/graphics/IOGraphicsTypesPrivate.h>
#include <IOKit/i2c/IOI2CInterface.h>

#include <IOKit/ndrvsupport/IOMacOSVideo.h>

//#include <Kernel/IOKit/ndrvsupport/IONDRVFramebuffer.h>
//#include <IOKitUser/graphics/IOGraphicsLibInternal.h>

// <IOKit/graphics/IOGraphicsTypes.h>
// kConnectionColorMode attribute
enum {
	kIODisplayColorModeReserved   = 0x00000000,
	kIODisplayColorModeRGB        = 0x00000001,
	kIODisplayColorModeYCbCr422   = 0x00000010,
	kIODisplayColorModeYCbCr444   = 0x00000100,
	kIODisplayColorModeRGBLimited = 0x00001000,
	kIODisplayColorModeAuto       = 0x10000000,
};


#define min(x,y) (((x)<(y)) ? (x) : (y))

// https://github.com/apple-oss-distributions/IOKitUser/blob/rel/IOKitUser-1445/graphics.subproj/IOGraphicsLibInternal.h
struct IOFBOvrDimensions {
	UInt32              width;
	UInt32              height;
	IOOptionBits        setFlags; // kDisplayModeSafeFlag
	IOOptionBits        clearFlags;
};
typedef struct IOFBOvrDimensions IOFBOvrDimensions;


// https://github.com/apple-oss-distributions/IOGraphics/blob/main/IOGraphicsFamily/IOKit/graphics/IOGraphicsTypesPrivate.h

enum { // 30
	// This is the ID given to a programmable timing used at boot time
	k1 = kIODisplayModeIDBootProgrammable, // = (IODisplayModeID)0xFFFFFFFB,
	// Lowest (unsigned) DisplayModeID reserved by Apple
	k2 = kIODisplayModeIDReservedBase, // = (IODisplayModeID)0x80000000

	// This is the ID given to a programmable timing used at boot time
	kIODisplayModeIDInvalid     = (IODisplayModeID) 0xFFFFFFFF,
	kIODisplayModeIDCurrent     = (IODisplayModeID) 0x00000000,
	kIODisplayModeIDAliasBase   = (IODisplayModeID) 0x40000000,
	
	// https://github.com/apple-oss-distributions/IOKitUser/blob/rel/IOKitUser-1445/graphics.subproj/IOGraphicsLib.c
	kIOFBSWOfflineDisplayModeID = (IODisplayModeID) 0xffffff00,
	kArbModeIDSeedMask           = (IODisplayModeID) 0x00007000, //  (connectRef->arbModeIDSeed is incremented everytime IOFBBuildModeList is called) // https://github.com/apple-oss-distributions/IOKitUser/blob/rel/IOKitUser-1445/graphics.subproj/IOGraphicsLibInternal.h
};

enum { // 36
	// options for IOServiceRequestProbe()
	kIOFBForceReadEDID                  = 0x00000100,
	kIOFBAVProbe                        = 0x00000200,
	kIOFBSetTransform                   = 0x00000400,
	kIOFBTransformShift                 = 16,
	kIOFBScalerUnderscan                = 0x01000000,
};

enum { // 45
	// transforms
	kIOFBRotateFlags                    = 0x0000000f,

	kIOFBSwapAxes                       = 0x00000001,
	kIOFBInvertX                        = 0x00000002,
	kIOFBInvertY                        = 0x00000004,

	kIOFBRotate0                        = 0x00000000,
	kIOFBRotate90                       = kIOFBSwapAxes | kIOFBInvertX,
	kIOFBRotate180                      = kIOFBInvertX  | kIOFBInvertY,
	kIOFBRotate270                      = kIOFBSwapAxes | kIOFBInvertY
};

enum { // 71
	// Controller attributes
	kIOFBSpeedAttribute                 = ' dgs',
	kIOFBWSStartAttribute               = 'wsup',
	kIOFBProcessConnectChangeAttribute  = 'wsch',
	kIOFBEndConnectChangeAttribute      = 'wsed',

	kIOFBMatchedConnectChangeAttribute  = 'wsmc',

	// Connection attributes
	kConnectionInTVMode                 = 'tvmd',
	kConnectionWSSB                     = 'wssb',

	kConnectionRawBacklight             = 'bklt',
	kConnectionBacklightSave            = 'bksv',

	kConnectionVendorTag                = 'vtag'
};

/*! @enum FramebufferConstants
	@constant kIOFBVRAMMemory The memory type for IOConnectMapMemory() to get the VRAM memory. Use a memory type equal to the IOPixelAperture index to get a particular pixel aperture.
*/
enum { // 106
	kIOFBVRAMMemory = 110
};

#define kIOFBGammaHeaderSizeKey         "IOFBGammaHeaderSize" // 110

#define kIONDRVFramebufferGenerationKey "IONDRVFramebufferGeneration"

#define kIOFramebufferOpenGLIndexKey    "IOFramebufferOpenGLIndex"

#define kIOFBCurrentPixelClockKey       "IOFBCurrentPixelClock"
#define kIOFBCurrentPixelCountKey       "IOFBCurrentPixelCount"
#define kIOFBCurrentPixelCountRealKey   "IOFBCurrentPixelCountReal"

#define kIOFBTransformKey               "IOFBTransform"
#define kIOFBRotatePrefsKey             "framebuffer-rotation"
#define kIOFBStartupTimingPrefsKey      "startup-timing"

#define kIOFBCapturedKey                "IOFBCaptured"

#define kIOFBMirrorDisplayModeSafeKey   "IOFBMirrorDisplayModeSafe"

#define kIOFBConnectInterruptDelayKey   "connect-interrupt-delay"

#define kIOFBUIScaleKey					"IOFBUIScale"

#define kIOGraphicsPrefsKey             "IOGraphicsPrefs"
#define kIODisplayPrefKeyKey            "IODisplayPrefsKey"
#define kIODisplayPrefKeyKeyOld         "IODisplayPrefsKeyOld"
#define kIOGraphicsPrefsParametersKey   "IOGraphicsPrefsParameters"
#define kIOGraphicsIgnoreParametersKey  "IOGraphicsIgnoreParameters" // 136

#define detailedTimingModeID            __reservedA[0] // 156

#define kIOFBDPDeviceIDKey          "dp-device-id" // 214
#define kIOFBDPDeviceTypeKey        "device-type"
#define kIOFBDPDeviceTypeDongleKey  "branch-device"

enum // 218
{
	kDPRegisterLinkStatus      = 0x200,
	kDPRegisterLinkStatusCount = 6,
	kDPRegisterServiceIRQ      = 0x201,
};

enum
{
	kDPLinkStatusSinkCountMask = 0x3f,
};

enum
{
	kDPIRQRemoteControlCommandPending = 0x01,
	kDPIRQAutomatedTestRequest        = 0x02,
	kDPIRQContentProtection           = 0x04,
	kDPIRQMCCS                        = 0x08,
	kDPIRQSinkSpecific                = 0x40,
};

enum // 244
{
	// values for graphic-options & kIOMirrorDefaultAttribute
//  kIOMirrorDefault       = 0x00000001,
//  kIOMirrorForced        = 0x00000002,
	kIOGPlatformYCbCr      = 0x00000004,
	kIOFBDesktopModeAllowed = 0x00000008,   // https://github.com/apple-oss-distributions/IOGraphics/blob/rel/IOGraphics-305/IOGraphicsFamily/IOFramebuffer.cpp // gIOFBDesktopModeAllowed
//  kIOMirrorHint          = 0x00010000,
	kIOMirrorNoAutoHDMI    = 0x00000010,
	kIOMirrorHint           = 0x00010000,	// https://github.com/apple-oss-distributions/IOGraphics/blob/rel/IOGraphics-585/IONDRVSupport/IONDRVFramebuffer.cpp
};


// https://github.com/robbertkl/ResolutionMenu/blob/master/Resolution%20Menu/DisplayModeMenuItem.m
// CoreGraphics DisplayMode struct used in private APIs
typedef struct {
	uint32_t mode; // mode index (0..CGSGetNumberOfDisplayModes - 1)
	uint32_t flags; // similar to IOFlags -> IOFramebufferInformation.flags kDisplayModeSafetyFlags
	uint32_t width; // "Looks like"
	uint32_t height; // "Looks like"
	uint32_t depthFormat; // 4 or 8

	uint32_t bytesPerRow; // width * bits per pixel / 8
	uint32_t bitsPerPixel; // 32
	uint32_t bitsPerSample; // 8 or 10
	uint32_t samplesPerPixel; // 3
	uint32_t intRefreshRate; // 60 integer refresh rate
	uint32_t horizontalResolution; // ??? dpi maybe
	uint32_t verticalResolution; // ??? dpi maybe

	IOPixelEncoding pixelEncoding; // --------RRRRRRRRGGGGGGGGBBBBBBBB
	uint64_t unknown2[8]; // 0
	uint16_t unknown0; // 0
	uint32_t unknown1; // 1
	uint32_t size; // 0xd4 = sizeof(CGSDisplayModeDescription)
	uint32_t refreshRate; // 59.875 : 16b.16b refresh rate fixed point (the fractional part might be flags (0001 or E002)? but it appears to be used as a fraction in the dictionary results)

	uint32_t IOFlags; // -> IOFramebufferInformation.flags kDisplayModeSafetyFlags
	IODisplayModeID DisplayModeID; // 0x8000xyyy

	uint32_t PixelsWide; // actual pixels = double width for HiDPI
	uint32_t PixelsHigh; // actual pixels = double height for HiDPI

	float resolution; // 1 = normal, 2 = HiDPI
} CGSDisplayModeDescription;

// CoreGraphics private APIs with support for scaled (retina) display modes
CGError CGSGetCurrentDisplayMode(CGDirectDisplayID display, uint32_t* modeNum);
CGError CGSConfigureDisplayMode(CGDisplayConfigRef config, CGDirectDisplayID display, uint32_t modeNum);
CGError CGSGetNumberOfDisplayModes(CGDirectDisplayID display, uint32_t* nModes);
CGError CGSGetDisplayModeDescriptionOfLength(CGDirectDisplayID display, int idx, CGSDisplayModeDescription* mode, int length);
CGError CGSServiceForDisplayNumber(CGDirectDisplayID display, io_service_t *service);
CGError CGSDisplayDeviceForDisplayNumber(CGDirectDisplayID display, io_service_t *service);

bool SLSIsDisplayModeVRR(CGDirectDisplayID display) __attribute__((weak_import));

CGError SLSDisplaySetHDRModeEnabled(CGDirectDisplayID display, bool enable, int, int)  __attribute__((weak_import));
bool SLSDisplayIsHDRModeEnabled(CGDirectDisplayID display) __attribute__((weak_import));
bool SLSDisplaySupportsHDRMode(CGDirectDisplayID display) __attribute__((weak_import));

CGError CGSEnableHDR(CGDirectDisplayID display, bool enable, int, int)  __attribute__((weak_import));
bool CGSIsHDREnabled(CGDirectDisplayID display) __attribute__((weak_import));
bool CGSIsHDRSupported(CGDirectDisplayID display) __attribute__((weak_import));

extern int DisplayServicesGetBrightness(CGDirectDisplayID display, float *brightness); // probably doesn't return an error
extern int DisplayServicesSetBrightness(CGDirectDisplayID display, float brightness);

size_t CGDisplayBitsPerPixel(CGDirectDisplayID display);
size_t CGDisplayBitsPerSample(CGDirectDisplayID display);
size_t CGDisplaySamplesPerPixel(CGDirectDisplayID display);
size_t CGDisplayBytesPerRow(CGDirectDisplayID display);


void KeyArrayCallback(const void *key, const void *value, void *context)
{
	CFArrayAppendValue(context, key);
}


//=================================================================================================================================
// Linux includes

#include <drm/dp/drm_dp_helper.h>
#include <drm/drm_hdcp.h>
#include "dpcd_defs.h"

//=================================================================================================================================
// Includes

#include "vcp.h"
#include "dpcd.h"
#include "printf.h"
#include "utilities.h"

//=================================================================================================================================
// Defines

// These are in micoseconds

#define kDelayDisplayPortReply 0

#define TRYSUBADDRESS 0 // I don't know when sub address is useful or not - better not try it

//=================================================================================================================================
// Utilities

static void CFOutput(CFTypeRef val) {
	if (val) {
		CFStringRef theinfo = CFCopyDescription(val); // can't really parse this if the format can change for every macOS
		if (theinfo) {
			size_t maxsize = CFStringGetMaximumSizeForEncoding(CFStringGetLength(theinfo), kCFStringEncodingUTF8);
			char *strinfo = (char *)malloc(maxsize);
			if (strinfo) {
				if (CFStringGetCString(theinfo, strinfo, maxsize, kCFStringEncodingUTF8)) {
					char *next;
					char *s;
					for (s = strinfo; ; s = next + 1) {
						next = strstr(s, "\n");
						if (!next) {
							cprintf("%s", s);
							break;
						}
						cprintf("%.*s", (int)(next - s + 1), s);
						iprintf("");
					}
				}
				else {
					cprintf("? (CFStringGetCString error)");
				}
				free(strinfo);
			}
			CFRelease(theinfo);
		}
		else {
			cprintf("? (CFCopyDescription error)");
		}
	}
	else {
		cprintf("(NULL)");
	}
}

//=================================================================================================================================

static void DumpOneID(CFNumberRef ID, int modeAlias) {
	UInt32 val;
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

char * myprintf(char *format, ...) {
	char *unknownValue = malloc(20);
	va_list vl;
	va_start(vl, format);
	vsnprintf(unknownValue, 20, format, vl);
	va_end(vl);
	return unknownValue;
}

#define UNKNOWN_FLAG( x) (x) ? myprintf("?0x%x,", (x)) : ""
#define UNKNOWN_VALUE(x)       myprintf("?0x%x", (x))

static bool DumpOneCursorInfo(CFDataRef IOFBOneCursorInfo, int compareNdx) {
	CFIndex size = CFDataGetLength(IOFBOneCursorInfo);

	if (size != sizeof(IOHardwareCursorDescriptor)) {
		cprintf("Unexpected size:%ld", (long)size);
		if (size >= sizeof(IOHardwareCursorDescriptor)) {
			cprintf(" ");
		}
	}

	if (size >= sizeof(IOHardwareCursorDescriptor)) {
		IOHardwareCursorDescriptor *info = (IOHardwareCursorDescriptor *)CFDataGetBytePtr(IOFBOneCursorInfo);
		cprintf("{ version:%d.%d size:%dx%d depth:%s maskBitDepth:%x colors:%d colorEncodings:%llx flags:%x specialEncodings:(",
			info->majorVersion,
			info->minorVersion,
			info->height,
			info->width,
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
				
			info->maskBitDepth,                   // unused
			info->numColors,                      // number of colors in the colorMap. ie.
			(UInt64)info->colorEncodings,         // UInt32 *
			info->flags
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
				cprintf(":%06x", info->specialEncodings[i]);
				gotone = true;
			}
		}
		cprintf(") },\n");
	}

	return size == sizeof(IOHardwareCursorDescriptor);
} // DumpOneCursorInfo

static void DumpOneDetailedTimingInformationPtr(void *IOFBDetailedTiming, CFIndex size, int modeAlias) {
	switch (size) {
		case sizeof(IODetailedTimingInformationV1): cprintf("V1"); break;
		case sizeof(IODetailedTimingInformationV2): cprintf("V2"); break;
		default                                   : cprintf("Unexpected size:%ld", (long)size); break;
	}

	if (size >= sizeof(IODetailedTimingInformationV2))
	{
		char hexDigits[] = "0123456789abcdef";
		hexDigits[modeAlias] = '.';
		
		IODetailedTimingInformationV2 *timing = (IODetailedTimingInformationV2 *)IOFBDetailedTiming;

		char refresh[40];
		if (timing->verticalBlankingExtension) {
			snprintf(refresh, sizeof(refresh), "%.3fHz (min:%.3fHz)",
				timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * (timing->verticalActive + timing->verticalBlanking)),
				timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * (timing->verticalActive + timing->verticalBlanking + timing->verticalBlankingExtension))
			);
		}
		else {
			snprintf(refresh, sizeof(refresh), "%.3fHz", timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * (timing->verticalActive + timing->verticalBlanking)));
		}

		cprintf(" id:0x%04x%c%03x %dx%d@%s %.3fkHz %.3fMHz (errMHz %g,%g)  h(%d %d %d %s%s)  v(%d %d %d %s%s)  border(h%d:%d v%d:%d)  active:%dx%d %s inset:%dx%d flags(%s%s%s%s%s%s%s%s%s%s%s%s%s%s) signal(%s%s%s%s%s%s%s%s) levels:%s links:%d " \
			"vbext:%d vbstretch:%d vbshrink:%d encodings(%s%s%s%s%s) bpc(%s%s%s%s%s%s) colorimetry(%s%s%s%s%s%s%s%s%s%s%s) dynamicrange(%s%s%s%s%s%s%s) dsc(%dx%d %gbpp) %s%s%s%s%s%s%s%s%s%s",

			timing->detailedTimingModeID >> 16, // mode
			hexDigits[(timing->detailedTimingModeID >> 12) & 15],
			timing->detailedTimingModeID & 0x0fff,

			timing->horizontalScaled ? timing->horizontalScaled : timing->horizontalActive,
			timing->verticalScaled ? timing->verticalScaled : timing->verticalActive,

			refresh, // Hz
			timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * 1000.0), // kHz
			timing->pixelClock / 1000000.0,

			((SInt64)timing->minPixelClock - (SInt64)timing->pixelClock) / 1000000.0, // Hz - With error what is slowest actual clock
			((SInt64)timing->maxPixelClock - (SInt64)timing->pixelClock) / 1000000.0, // Hz - With error what is fasted actual clock
			  
			timing->horizontalSyncOffset,           // pixels
			timing->horizontalSyncPulseWidth,       // pixels
			timing->horizontalBlanking - timing->horizontalSyncOffset - timing->horizontalSyncPulseWidth, // pixels

			timing->horizontalSyncConfig == kIOSyncPositivePolarity ? "+" :
			timing->horizontalSyncConfig == 0 ? "-" :
			UNKNOWN_VALUE(timing->horizontalSyncConfig),

			timing->horizontalSyncLevel == 0 ? "" :
			UNKNOWN_VALUE(timing->horizontalSyncLevel), // Future use (init to 0)

			timing->verticalSyncOffset,             // lines
			timing->verticalSyncPulseWidth,         // lines
			timing->verticalBlanking - timing->verticalSyncOffset - timing->verticalSyncPulseWidth, // lines

			timing->verticalSyncConfig == kIOSyncPositivePolarity ? "+" :
			timing->verticalSyncConfig == 0 ? "-" :
			UNKNOWN_VALUE(timing->verticalSyncConfig),

			timing->verticalSyncLevel == 0 ? "" :
			UNKNOWN_VALUE(timing->verticalSyncLevel), // Future use (init to 0)

			timing->horizontalBorderLeft,           // pixels
			timing->horizontalBorderRight,          // pixels
			timing->verticalBorderTop,              // lines
			timing->verticalBorderBottom,           // lines

			timing->horizontalActive,               // pixels
			timing->verticalActive,                 // lines

			(timing->horizontalScaled && timing->verticalScaled) ? "(scaled)" :
			((!timing->horizontalScaled) != (!timing->verticalScaled)) ? "(scaled?)" : "(not scaled)",

			timing->horizontalScaledInset,          // pixels
			timing->verticalScaledInset,            // lines

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
			UNKNOWN_VALUE(timing->signalLevels),

			timing->numLinks,

			timing->verticalBlankingExtension,      // lines (AdaptiveSync: 0 for non-AdaptiveSync support)
			timing->verticalBlankingMaxStretchPerFrame,
			timing->verticalBlankingMaxShrinkPerFrame,

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
			UNKNOWN_FLAG(timing->dynamicRange & 0xffc0),

			timing->dscSliceWidth,
			timing->dscSliceHeight,
			timing->dscCompressedBitsPerPixel / 16.0,
			
				timing->__reservedA[1] ? " reservedA1:" : "",
				timing->__reservedA[1] ? UNKNOWN_VALUE(timing->__reservedA[1]) : "",
				timing->__reservedA[2] ? " reservedA2:" : "",
				timing->__reservedA[2] ? UNKNOWN_VALUE(timing->__reservedA[2]) : "",
				timing->__reservedB[0] ? " reservedB0:" : "",
				timing->__reservedB[0] ? UNKNOWN_VALUE(timing->__reservedB[0]) : "",
				timing->__reservedB[1] ? " reservedB1:" : "",
				timing->__reservedB[1] ? UNKNOWN_VALUE(timing->__reservedB[1]) : "",
				timing->__reservedB[2] ? " reservedB2:" : "",
				timing->__reservedB[2] ? UNKNOWN_VALUE(timing->__reservedB[2]) : ""
		);
	} else if (size >= sizeof(IODetailedTimingInformationV1)) {
		IODetailedTimingInformationV1 *timing = (IODetailedTimingInformationV1 *)IOFBDetailedTiming;
		cprintf(" %dx%d@%.3fHz %.3fkHz %.3fMHz  h(%d %d %d)  v(%d %d %d)  border(h%d v%d)",
			timing->horizontalActive,               // pixels
			timing->verticalActive,                 // lines
			timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * (timing->verticalActive + timing->verticalBlanking)), // Hz
			timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * 1000.0), // kHz
			timing->pixelClock / 1000000.0,

			timing->horizontalSyncOffset,           // pixels
			timing->horizontalSyncWidth,            // pixels
			timing->horizontalBlanking - timing->horizontalSyncOffset - timing->horizontalSyncWidth, // pixels

			timing->verticalSyncOffset,             // lines
			timing->verticalSyncWidth,              // lines
			timing->verticalBlanking - timing->verticalSyncOffset - timing->verticalSyncWidth, // lines

			timing->horizontalBorder,       // pixels
			timing->verticalBorder          // lines
		);
	}
} // DumpOneDetailedTimingInformationPtr

static IODetailedTimingInformationV2 *detailedTimingsArr = NULL;
static CFIndex detailedTimingsCount = 0;

static bool DumpOneDetailedTimingInformation(CFDataRef IOFBDetailedTiming, int compareNdx, int modeAlias) {
	CFIndex size = CFDataGetLength(IOFBDetailedTiming);
	IODetailedTimingInformationV2 timing;
	bzero(&timing, sizeof(timing));
	memcpy(&timing, CFDataGetBytePtr(IOFBDetailedTiming), min(sizeof(timing), CFDataGetLength(IOFBDetailedTiming)));
	bool omitted = false;
	if (compareNdx < 0 ||
		memcmp(&timing, &detailedTimingsArr[compareNdx], sizeof(timing) )
	) {
		if (compareNdx >= 0) {
			iprintf("[%d] = {", compareNdx);
		}
		DumpOneDetailedTimingInformationPtr(&timing, size, modeAlias);
		if (compareNdx >= 0) {
			cprintf("};\n");
		}
	}
	else {
		omitted = true;
	}
	return omitted;
} // DumpOneDetailedTimingInformation

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

static void DumpOneAppleID(CFNumberRef appleTimingIDRef) {
	IOAppleTimingID appleTimingID;
	CFNumberGetValue(appleTimingIDRef, kCFNumberSInt32Type, &appleTimingID);
	cprintf("%d:%s", appleTimingID, GetOneAppleTimingID(appleTimingID));
} // DumpOneAppleID

static void DumpOneDisplayTimingRange(CFDataRef IOFBTimingRange) {
	CFIndex size = CFDataGetLength(IOFBTimingRange);
	switch (size) {
		case sizeof(IODisplayTimingRangeV1): cprintf("V1"); break;
		case sizeof(IODisplayTimingRangeV2): cprintf("V2"); break;
		default                            : cprintf("Unexpected size:%ld", (long)size); break;
	}
	
	if (size >= sizeof(IODisplayTimingRangeV1))
	{
		IODisplayTimingRangeV1 *range = (IODisplayTimingRangeV1 *)CFDataGetBytePtr(IOFBTimingRange);
		cprintf(
			" version:%d %d…%dHz %.3f…%.3fkHz %.3f…%.3f±%.3fMHz sync(%s%s%s%s%s%s) levels(%s%s%s%s%s) signal(%s%s%s%s) " \
			"(total,active,blank,frontp,syncw,border1,border2)(charsize(h(%d,%d,%d,%d,%d,%d,%d) v(%d,%d,%d,%d,%d,%d,%d)) pixels(h(%d,%d…%d,%d…%d,%d…%d,%d…%d,%d…%d,%d…%d) v(%d,%d…%d,%d…%d,%d…%d,%d…%d,%d…%d,%d…%d))) " \
			"links(#:%d 0:%.3f…%.3f 1:%.3f…%.3f MHz) encodings(%s%s%s%s%s) bpc(%s%s%s%s%s%s) colorimetry(%s%s%s%s%s%s%s%s%s%s%s) dynamicrange(%s%s%s%s%s%s%s)" \
			"%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",

			range->version,                       // Init to 0

			range->minFrameRate,                   // Hz
			range->maxFrameRate,                   // Hz
			range->minLineRate / 1000.0,           // Hz
			range->maxLineRate / 1000.0,           // Hz
			range->minPixelClock / 1000000.0,      // Min dot clock in Hz
			range->maxPixelClock / 1000000.0,      // Max dot clock in Hz
			range->maxPixelError / 1000000.0,      // Max dot clock error

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
			UNKNOWN_FLAG(range->supportedSignalLevels & 0xfffffff0),

			(range->supportedSignalConfigs & kIORangeSupportsInterlacedCEATiming           ) ?            "interlaced CEA," : "",
			(range->supportedSignalConfigs & kIORangeSupportsInterlacedCEATimingWithConfirm) ? "interlaced CEA w/ confirm," : "",
			(range->supportedSignalConfigs & kIORangeSupportsMultiAlignedTiming            ) ?             "multi aligned," : "",
			UNKNOWN_FLAG(range->supportedSignalConfigs & 0xffffffb3),

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

			range->maxHorizontalTotal,             // Clocks - Maximum total (active + blanking)
			range->minHorizontalActiveClocks,
			range->maxHorizontalActiveClocks,
			range->minHorizontalBlankingClocks,
			range->maxHorizontalBlankingClocks,
			range->minHorizontalSyncOffsetClocks,
			range->maxHorizontalSyncOffsetClocks,
			range->minHorizontalPulseWidthClocks,
			range->maxHorizontalPulseWidthClocks,
			range->minHorizontalBorderLeft,
			range->maxHorizontalBorderLeft,
			range->minHorizontalBorderRight,
			range->maxHorizontalBorderRight,

			range->maxVerticalTotal,               // Clocks - Maximum total (active + blanking)
			range->minVerticalActiveClocks,
			range->maxVerticalActiveClocks,
			range->minVerticalBlankingClocks,
			range->maxVerticalBlankingClocks,
			range->minVerticalSyncOffsetClocks,
			range->maxVerticalSyncOffsetClocks,
			range->minVerticalPulseWidthClocks,
			range->maxVerticalPulseWidthClocks,
			range->minVerticalBorderTop,
			range->maxVerticalBorderTop,
			range->minVerticalBorderBottom,
			range->maxVerticalBorderBottom,

			range->maxNumLinks,                       // number of links, if zero, assume link 1
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
			UNKNOWN_FLAG(range->supportedDynamicRangeModes & 0xffc0),

			range->__reservedA[0] ? " reservedA0:" : "",
			range->__reservedA[0] ? UNKNOWN_VALUE(range->__reservedA[0]) : "",
			range->__reservedA[1] ? " reservedA1:" : "",
			range->__reservedA[1] ? UNKNOWN_VALUE(range->__reservedA[1]) : "",
			range->__reservedB[0] ? " reservedB0:" : "",
			range->__reservedB[0] ? UNKNOWN_VALUE(range->__reservedB[0]) : "",
			range->__reservedB[1] ? " reservedB1:" : "",
			range->__reservedB[1] ? UNKNOWN_VALUE(range->__reservedB[1]) : "",
			range->__reservedB[2] ? " reservedB2:" : "",
			range->__reservedB[2] ? UNKNOWN_VALUE(range->__reservedB[2]) : "",
			range->__reservedB[3] ? " reservedB3:" : "",
			range->__reservedB[3] ? UNKNOWN_VALUE(range->__reservedB[3]) : "",
			range->__reservedB[4] ? " reservedB4:" : "",
			range->__reservedB[4] ? UNKNOWN_VALUE(range->__reservedB[4]) : "",
			range->__reservedD[0] ? " reservedD0:" : "",
			range->__reservedD[0] ? UNKNOWN_VALUE(range->__reservedD[0]) : "",
			range->__reservedD[1] ? " reservedD1:" : "",
			range->__reservedD[1] ? UNKNOWN_VALUE(range->__reservedD[1]) : "",
			range->__reservedE    ? " reservedE:" : "",
			range->__reservedE    ? UNKNOWN_VALUE(range->__reservedE   ) : "",
			range->__reservedF[0] ? " reservedF0:" : "",
			range->__reservedF[0] ? UNKNOWN_VALUE(range->__reservedF[0]) : ""
		);
	}

	if (size >= sizeof(IODisplayTimingRangeV2))
	{
		IODisplayTimingRangeV2 *range = (IODisplayTimingRangeV2 *)CFDataGetBytePtr(IOFBTimingRange);
		cprintf(" dsc(%.3fGbps%s slice:%dx%d…%dx%d slice/line:%d…%d %d…%dbpc %d…%dbpp VBR:%s BlockPred:%s)%s%s%s%s%s%s%s%s%s%s%s%s",

			range->maxBandwidth / 1000000000.0,
			((range->maxBandwidth >> 32) == 0xffffffff) ? "?(32-bit sign extension error)" : "",
			
			range->dscMinSliceWidth,
			range->dscMinSliceHeight,
			range->dscMaxSliceWidth,
			range->dscMaxSliceHeight,
			range->dscMinSlicePerLine,
			range->dscMaxSlicePerLine,

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
				"?not zero or one",

			range->__reservedC[0] ? " reservedC0:" : "",
			range->__reservedC[0] ? UNKNOWN_VALUE(range->__reservedC[0]) : "",
			range->__reservedC[1] ? " reservedC1:" : "",
			range->__reservedC[1] ? UNKNOWN_VALUE(range->__reservedC[1]) : "",
			range->__reservedC[2] ? " reservedC2:" : "",
			range->__reservedC[2] ? UNKNOWN_VALUE(range->__reservedC[2]) : "",
			range->__reservedC[3] ? " reservedC3:" : "",
			range->__reservedC[3] ? UNKNOWN_VALUE(range->__reservedC[3]) : "",
			range->__reservedC[4] ? " reservedC4:" : "",
			range->__reservedC[4] ? UNKNOWN_VALUE(range->__reservedC[4]) : "",
			range->__reservedC[5] ? " reservedC5:" : "",
			range->__reservedC[5] ? UNKNOWN_VALUE(range->__reservedC[5]) : ""
		);
	}
} // DumpOneDisplayTimingRange

static void DumpOneDisplayScalerInfo(CFDataRef IOFBScalerInfo) {
	CFIndex size = CFDataGetLength(IOFBScalerInfo);

	if (size != sizeof(IODisplayScalerInformation)) {
		cprintf("Unexpected size:%ld", (long)size);
		if (size >= sizeof(IODisplayScalerInformation)) {
			cprintf(" ");
		}
	}
	
	if (size >= sizeof(IODisplayScalerInformation))
	{
		IODisplayScalerInformation *info = (IODisplayScalerInformation *)CFDataGetBytePtr(IOFBScalerInfo);
		cprintf(
			"version:%d maxPixels:%dx%d options(%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s) %x.%x.%x.%x.%x.%x.%x.%x",

			info->version,                       // Init to 0
			info->maxHorizontalPixels,
			info->maxVerticalPixels,

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

			info->__reservedA[0], // Init to 0

			info->__reservedB[0], // Init to 0
			info->__reservedB[1], // Init to 0

			info->__reservedC[0], // Init to 0
			info->__reservedC[1], // Init to 0
			info->__reservedC[2], // Init to 0
			info->__reservedC[3], // Init to 0
			info->__reservedC[4]  // Init to 0
		);
	}

} // DumpOneDisplayScalerInfo

static void DumpOneTimingInformation(CFDataRef IOTimingInformationData, int modeAlias) {
	CFIndex size = CFDataGetLength(IOTimingInformationData);
	if (size < offsetof(IOTimingInformation, detailedInfo)) {
		cprintf("Unexpected size:%ld", (long)size);
	}
	else {
		IOTimingInformation *info = (IOTimingInformation *)CFDataGetBytePtr(IOTimingInformationData);
		cprintf("appleTimingID = %s; flags = %s%s%s; DetailedTimingInformation = { ",
			GetOneAppleTimingID(info->appleTimingID),
			info->flags & kIODetailedTimingValid ? "DetailedTimingValid," : "",
			info->flags & kIOScalingInfoValid    ?    "ScalingInfoValid," : "",
			UNKNOWN_FLAG(info->flags & 0x3fffffff)
		);
		DumpOneDetailedTimingInformationPtr(&info->detailedInfo, size - offsetof(IOTimingInformation, detailedInfo), modeAlias);
		cprintf("}");
	}
} // DumpOneTimingInformation

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

static char * GetOneFlagsStr(UInt64 flags) {
	char * flagsstr = malloc(1000);
	if (flagsstr) {
		snprintf(flagsstr, 1000, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
			flags & kDisplayModeValidFlag              ?                  "Valid," : "",
			flags & kDisplayModeSafeFlag               ?                   "Safe," : "",
			flags & kDisplayModeDefaultFlag            ?                "Default," : "",
			flags & kDisplayModeAlwaysShowFlag         ?             "AlwaysShow," : "",
			flags & kDisplayModeNotResizeFlag          ?              "NotResize," : "",
			flags & kDisplayModeRequiresPanFlag        ?            "RequiresPan," : "",
			flags & kDisplayModeInterlacedFlag         ?             "Interlaced," : "",
			flags & kDisplayModeNeverShowFlag          ?              "NeverShow," : "",
			flags & kDisplayModeSimulscanFlag          ?              "Simulscan," : "",
			flags & kDisplayModeNotPresetFlag          ?              "NotPreset," : "",
			flags & kDisplayModeBuiltInFlag            ?                "BuiltIn," : "",
			flags & kDisplayModeStretchedFlag          ?              "Stretched," : "",
			flags & kDisplayModeNotGraphicsQualityFlag ?     "NotGraphicsQuality," : "",
			flags & kDisplayModeValidateAgainstDisplay ? "ValidateAgainstDisplay," : "",
			flags & kDisplayModeTelevisionFlag         ?             "Television," : "",
			flags & kDisplayModeValidForMirroringFlag  ?      "ValidForMirroring," : "",
			flags & kDisplayModeAcceleratorBackedFlag  ?      "AcceleratorBacked," : "",
			flags & kDisplayModeValidForHiResFlag      ?          "ValidForHiRes," : "",
			flags & kDisplayModeValidForAirPlayFlag    ?        "ValidForAirPlay," : "",

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
	return flagsstr;
} // GetOneFlagsStr

static void DumpOneDisplayModeInformation(CFDataRef displayModeInformation) {
	CFIndex size = CFDataGetLength(displayModeInformation);

	if (size != sizeof(IODisplayModeInformation)) {
		cprintf("Unexpected size:%ld", (long)size);
		if (size >= sizeof(IODisplayModeInformation)) {
			cprintf(" ");
		}
	}

	if (size >= sizeof(IODisplayModeInformation))
	{
		IODisplayModeInformation *info = (IODisplayModeInformation *)CFDataGetBytePtr(displayModeInformation);
		char * flagsstr = GetOneFlagsStr(info->flags);
		cprintf("%dx%d@%.3fHz maxdepth:%d flags:%s imagesize:%dx%dmm%s%s%s%s%s%s",
			info->nominalWidth,
			info->nominalHeight,
			info->refreshRate / 65536.0,
			info->maxDepthIndex,
			flagsstr,
			info->imageWidth,
			info->imageHeight,
			info->reserved[ 0 ] ? " reserved0:" : "",
			info->reserved[ 0 ] ? UNKNOWN_VALUE(info->reserved[ 0 ]) : "",
			info->reserved[ 1 ] ? " reserved1:" : "",
			info->reserved[ 1 ] ? UNKNOWN_VALUE(info->reserved[ 1 ]) : "",
			info->reserved[ 2 ] ? " reserved2:" : "",
			info->reserved[ 2 ] ? UNKNOWN_VALUE(info->reserved[ 2 ]) : ""
		);
		free(flagsstr);
	}
} // DumpOneDisplayModeInformation

static void DumpOneDisplayModeDescription(CGSDisplayModeDescription *mode, int modeAlias) {
	char *flagsstr1 = GetOneFlagsStr(mode->IOFlags);
	char *flagsstr2 = GetOneFlagsStr(mode->flags);

	char hexDigits[] = "0123456789abcdef";
	hexDigits[modeAlias] = '.';

	cprintf("%d: id:0x%04x%c%03x %dx%d@%.3fHz %dHz (dens=%.1f) pixels:%dx%d resolution:%dx%d %dbpp %dbpc %dcpp rowbytes:%d IOFlags:(%s) flags:(%s) depthFormat:%d encoding:%s refreshRate.unk0.unk1:%08x.%04x.%08x%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		mode->mode,
			
		(UInt32)mode->DisplayModeID >> 16,
		hexDigits[(mode->DisplayModeID >> 12) & 15],
		mode->DisplayModeID & 0x0fff,
			
		mode->width, mode->height, mode->refreshRate / 65536.0, mode->intRefreshRate, mode->resolution, mode->PixelsWide, mode->PixelsHigh, mode->horizontalResolution, mode->verticalResolution,
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

	if (mode->size >= sizeof(CGSDisplayModeDescription)) {
		cprintf("unexpected size:%d expected:%ld", mode->size, sizeof(CGSDisplayModeDescription));
	}
} // DumpOneDisplayModeDescription

static CFDictionaryRef CGDisplayModeToDict(CGDisplayModeRef mode) {
	typedef struct CGDisplayMode {
		UInt8 bytes[16];
		CFDictionaryRef dict;
	} CGDisplayMode;
	
	CFDictionaryRef dict = NULL;
	if (mode) {
		dict = ((CGDisplayMode *)mode)->dict;
	}
	return dict;
}

#define onefloat(_field, _format) \
	float val_ ## _field = 0.0; \
	char str_ ## _field[20]; \
	CFTypeRef cf_ ## _field = CFDictionaryGetValue(dict, CFSTR(#_field)); \
	if (cf_ ## _field) { \
		if (CFGetTypeID(cf_ ## _field) == CFNumberGetTypeID()) { \
			if (copy) CFDictionaryRemoveValue(copy, CFSTR(#_field)); \
			CFNumberGetValue(cf_ ## _field, kCFNumberFloat32Type, &val_ ## _field); \
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
			if (CFNumberGetValue(cf_ ## _field, kCFNumberSInt ## _size ## Type, &val_ ## _field)) \
				snprintf(str_ ## _field, sizeof(str_ ## _field), _format, val_ ## _field); \
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
			CFStringGetCString(cf_ ## _field, str_ ## _field, sizeof(str_ ## _field), kCFStringEncodingUTF8); \
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

	if (CFGetTypeID(mode) == CGDisplayModeGetTypeID()) {
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

	if (CFGetTypeID(dict) == CFArrayGetTypeID()) {
		CFArrayRef arr = (CFArrayRef)dict;
		if (CFArrayGetCount(arr) != 1) {
			cprintf("not a single display mode\n");
			return;
		}
		dict = (CFDictionaryRef)CFArrayGetValueAtIndex(arr, 0);
	}

	if (CFGetTypeID(dict) != CFDictionaryGetTypeID()) {
		cprintf("not a CGDisplayModeRef\n");
		return;
	}

	CFMutableDictionaryRef copy = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, dict);

	onenum(32, BitsPerPixel, "%d")
	onenum(32, BitsPerSample, "%d")
	onenum(32, Height, "%d")
	onenum(32, IODisplayModeID, "0x%08x")
	onenum(32, IOFlags, "%x")
	onenum(32, Mode, "%d")
	onefloat(RefreshRate, "%.3f")
	onenum(32, SamplesPerPixel, "%d")
	onenum(32, Width, "%d")
	onenum(32, kCGDisplayBytesPerRow, "%d")
	onenum(32, kCGDisplayHorizontalResolution, "%d")
	onenum(32, kCGDisplayPixelsHigh, "%d")
	onenum(32, kCGDisplayPixelsWide, "%d")
	onefloat(kCGDisplayResolution, "%.1f")
	onenum(32, kCGDisplayVerticalResolution, "%d")
	onenum(32, UsableForDesktopGUI, "%d")
	onenum(32, kCGDisplayModeIsSafeForHardware, "%d")
	onenum(32, kCGDisplayModeIsTelevisionOutput, "%d")

	onenum(32, kCGDisplayModeIsInterlaced, "%d")
	onenum(32, kCGDisplayModeIsStretched, "%d")
	onenum(32, kCGDisplayModeIsUnavailable, "%d")
	onenum(32, kCGDisplayModeSuitableForUI, "%d")
	onenum(32, DepthFormat, "%d")
	onestr(PixelEncoding)

	
	char hexDigits[] = "0123456789abcdef";
	hexDigits[modeAlias] = '.';
	
	if (str_IODisplayModeID[0] == '0') {
		snprintf(str_IODisplayModeID, sizeof(str_IODisplayModeID), "0x%04x%c%03x",
			val_IODisplayModeID >> 16,
			hexDigits[(val_IODisplayModeID >> 12) & 15],
			val_IODisplayModeID & 0x0fff
		);
	}
	
	char * flagsstr = GetOneFlagsStr(val_IOFlags);
	cprintf("%s: id:%s %sx%s@%sHz %.0fHz (dens=%s) pixels:%sx%s resolution:%sx%s %sbpp %sbpc %scpp rowbytes:%s IOFlags:(%s) flags:(%s%s%s%s%s%s%s%s%s%s%s%s%s%s) depthFormat:%s encoding:%s",
		str_Mode, str_IODisplayModeID,
		str_Width, str_Height, str_RefreshRate, val_RefreshRate, str_kCGDisplayResolution, str_kCGDisplayPixelsWide, str_kCGDisplayPixelsHigh, str_kCGDisplayHorizontalResolution, str_kCGDisplayVerticalResolution,
		str_BitsPerPixel, str_BitsPerSample, str_SamplesPerPixel, str_kCGDisplayBytesPerRow,
		flagsstr,
		oneflag(UsableForDesktopGUI             , "gui usable" ),
		oneflag(kCGDisplayModeIsSafeForHardware , "hw safe"    ),
		oneflag(kCGDisplayModeIsTelevisionOutput, "tv out"     ),

		oneflag(kCGDisplayModeIsInterlaced      , "interlaced" ),
		oneflag(kCGDisplayModeIsStretched       , "stretched"  ),
		oneflag(kCGDisplayModeIsUnavailable     , "unavailable"),
		oneflag(kCGDisplayModeSuitableForUI     , "ui suitable"),
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
		char str[256];
		CFStringGetCString(key, str, sizeof(str), CFStringGetSystemEncoding());

		SInt32 numValue;
		CFNumberGetValue(value, kCFNumberSInt32Type, &numValue);
		iprintf("%s = %s;\n", str,
			numValue == 1 ?  "true" :
			numValue == 0 ? "false" :
			UNKNOWN_VALUE(numValue)
		);
		CFDictionaryRemoveValue((CFMutableDictionaryRef)context, key);
	}
}

static void DoAllBooleans(CFMutableDictionaryRef dict) {
	CFDictionaryApplyFunction(dict, printBooleanKeys, dict);
}

uint8_t crc4(const uint8_t * data, size_t NumberOfNibbles)
{
	uint8_t BitMask      = 0x80;
	uint8_t BitShift     = 7;
	uint8_t ArrayIndex   = 0;
	int     NumberOfBits = (int)NumberOfNibbles * 4;
	uint8_t Remainder    = 0;

	while (NumberOfBits != 0)
	{
		NumberOfBits--;
		   Remainder <<= 1;
		   Remainder |= (data[ArrayIndex] & BitMask) >> BitShift;
		   BitMask >>= 1;
		   BitShift--;
		   if (BitMask == 0)
		   {
			   BitMask  = 0x80;
			   BitShift = 7;
			   ArrayIndex++;
		   }
		   if ((Remainder & 0x10) == 0x10)
		   {
			   Remainder ^= 0x13;
		   }
	}
	NumberOfBits = 4;
	while (NumberOfBits != 0)
	{
		NumberOfBits--;
		Remainder <<= 1;
		if ((Remainder & 0x10) != 0)
		{
			Remainder ^= 0x13;
		}
	}

   return Remainder;
} // crc4

uint8_t crc8(const uint8_t * data, uint8_t NumberOfBytes)
{
	uint8_t  BitMask      = 0x80;
	uint8_t  BitShift     = 7;
	uint8_t  ArrayIndex   = 0;
	uint16_t NumberOfBits = NumberOfBytes * 8;
	uint16_t Remainder    = 0;

	while (NumberOfBits != 0)
	{
		NumberOfBits--;
		Remainder <<= 1;
		Remainder |= (data[ArrayIndex] & BitMask) >> BitShift;
		BitMask >>= 1;
		BitShift--;
		if (BitMask == 0)
		{
			BitMask  = 0x80;
			BitShift = 7;
			ArrayIndex++;
		}
		if ((Remainder & 0x100) == 0x100)
		{
			Remainder ^= 0xD5;
		}
	}
	NumberOfBits = 8;
	while (NumberOfBits != 0)
	{
		NumberOfBits--;
		Remainder <<= 1;
		if ((Remainder & 0x100) != 0)
		{
			Remainder ^= 0xD5;
		}
	}
   return Remainder & 0xFF;
} // crc8

static void DoOneDisplayPort(IOI2CConnectRef i2cconnect, UInt8 *inpath, int pathLength) {
	// the first number in the path is the port number
	
	IOI2CRequest request;
	IOReturn result = kIOReturnSuccess;
	UInt8 path[16];
	memcpy(path, inpath, pathLength);
	
	UInt8 *dpcd = malloc(0x100000);
	if (dpcd) {
		bzero(dpcd, 0x100000);

		int dpcdRangeNdx;
		for (dpcdRangeNdx = 0; dpcdranges[dpcdRangeNdx] >= 0; dpcdRangeNdx += 2) {
			bool hasError = false;
			int dpcdAddr;
			for (dpcdAddr = dpcdranges[dpcdRangeNdx]; dpcdAddr < dpcdranges[dpcdRangeNdx + 1]; dpcdAddr += DP_AUX_MAX_PAYLOAD_BYTES) {
				for (int attempt = 0; attempt < 3; attempt++) {

					if (pathLength) {
						struct I2C_Transaction {
							UInt8 Write_I2C_Device_Identifier : 7; // LSB
							UInt8 zero : 1; // MSB
							UInt8 Number_Of_Bytes_To_Write;
							UInt8 I2C_Data_To_Write; // unknown length;
						};
						
						struct Remote_I2C_Read2 {
							UInt8 I2C_Transaction_Delay : 4; // LSB
							UInt8 No_Stop_Bit : 1;
							UInt8 zeros : 3; // MSB
						};
						
						typedef struct __attribute__((packed)) __attribute__((aligned(1))) {
							UInt8 Request_Identifier : 7; // LSB copy of same in Request Message Transaction
							UInt8 Reply_Type : 1; // MSB DP_SIDEBAND_REPLY_ACK=0 or DP_SIDEBAND_REPLY_NAK=1
							union Reply_Data {
								struct {
									guid_t Global_Unique_Identifier;
									UInt8 Reason_For_NAK; // DP_NAK_INVALID_READ should actually be renamed DP_NAK_INVALID_RAD
									UInt8 NAK_Data;
								} NAK; // 1
								struct {
									union {
										struct {
											UInt8 Port_Number : 4; // LSB
											UInt8 zeros : 4; // MSB
											UInt8 Number_Of_Bytes_Read;
											UInt8 Data_Read[1];
											UInt8 end;
										} Remote_DPCD_Read;
										struct {
											UInt8 Port_Number : 4; // LSB
											UInt8 zeros : 4; // MSB
											UInt8 Number_Of_Bytes_Read;
											UInt8 Data_Read[1];
											UInt8 end;
										} Remote_I2C_Read;
									} ACK_Data;
								} ACK; // 0
							} data;
						} Message_Transaction_Reply;

						typedef struct __attribute__((packed)) __attribute__((aligned(1))) {
							struct Sideband_MSG_Header {
								UInt8 Link_Count_Remaining : 4; // LSB init to Link_Count_Total
								UInt8 Link_Count_Total : 4; // MSB

								UInt8 Relative_Address[8]; // 16 nibbles nibble[0] is MSB of byte[0]
							} header;
						}  Sideband_MSG1;

						typedef struct __attribute__((packed)) __attribute__((aligned(1))) {
							struct Sideband_MSG_Header2 {
								UInt8 Sideband_MSG_Body_Length : 6; // LSB
								UInt8 Path_Message : 1;
								UInt8 Broadcast_Message : 1; // MSB

								UInt8 Sideband_MSG_Header_CRC : 4; // LSB // bit(0..3)
								UInt8 Message_Sequence_No : 1; // bit(4)
								UInt8 zero : 1; // bit(5)
								UInt8 End_Of_Message_Transaction : 1; // bit(6)
								UInt8 Start_Of_Message_Transaction : 1; // MSB // bit(7)
							} header;

							struct Sideband_MSG_Body {
								struct Message_Transacion_Request {
									UInt8 Request_Identifier : 7; // LSB
									UInt8 zero : 1; // MSB
									union Request_Data {
										union {
											struct __attribute__((packed)) __attribute__((aligned(1))) {
												UInt32 Number_Of_Bytes_To_Read : 8  __attribute__((packed)) __attribute__((aligned(1))); // LSB of 32 bits
												UInt32 DPCD_Address : 20;
												UInt32 Port_Number : 4; // MSB of 32 bits
												UInt8 end;
											} bits;
											UInt32 raw __attribute__((packed)) __attribute__((aligned(1))); // use this to swap the bits
										} Remote_DPCD_Read;
										struct {
											UInt8 Port_Number : 4; // LSB
											UInt8 Number_Of_I2C_Transactions : 2; // MSB
											UInt8 zeros : 2; // MSB
											struct I2C_Transaction Transactions; // ...
										} Remote_I2C_Read;
									} data;
								} request;
							} body;
						} Sideband_MSG2;

						typedef struct __attribute__((packed)) __attribute__((aligned(1))) {
							struct Sideband_MSG_Body_2 {
								UInt8 Sideband_MSG_Body_CRC;
							} body;
							UInt8 end;
						} Sideband_MSG3;
						
						UInt8 senddata[10];
						UInt8 replydata[32];
						memset(senddata, 0xff, sizeof(senddata));
						memset(replydata, 0xff, sizeof(replydata));

						Sideband_MSG1 *msg1 = (void*)&senddata;
						Sideband_MSG2 *msg2 = (void*)&msg1->header.Relative_Address + pathLength / 2; // pathlength 1 & 2 = 1 byte, pathlength 3 & 4 = 2 bytes, ... (since path[0] is port number)
						Sideband_MSG3 *msg3 = (void*)&msg2->body.request.data.Remote_DPCD_Read.bits.end;

						msg1->header.Link_Count_Total = pathLength;
						msg1->header.Link_Count_Remaining = pathLength - 1;
						bzero(&msg1->header.Relative_Address, pathLength / 2);
						for (int i = 0; i < pathLength - 1; i++) {
							msg1->header.Relative_Address[i / 2] |= path[i + 1] << ((i^1) * 4);
						}
						msg2->header.Broadcast_Message = 0;
						msg2->header.Path_Message = 0;
						msg2->header.Sideband_MSG_Body_Length = (void*)&msg3->end - (void*)&msg2->body; // the CRC is part of the MSG_Body_Length
						
						msg2->header.Start_Of_Message_Transaction = 1;
						msg2->header.End_Of_Message_Transaction = 1;
						msg2->header.zero = 0;
						msg2->header.Message_Sequence_No = 0;
						msg2->header.Sideband_MSG_Header_CRC = crc4(senddata, ((void*)&msg2->body - (void*)&msg1->header) * 2 - 1); // 2 nibbles per byte - exclude the nibble for the 4-bit CRC
						
						msg2->body.request.zero = 0;
						msg2->body.request.Request_Identifier = DP_REMOTE_DPCD_READ;
						msg2->body.request.data.Remote_DPCD_Read.bits.Port_Number = path[0];
						msg2->body.request.data.Remote_DPCD_Read.bits.DPCD_Address = dpcdAddr;
						msg2->body.request.data.Remote_DPCD_Read.bits.Number_Of_Bytes_To_Read = DP_AUX_MAX_PAYLOAD_BYTES;
						msg2->body.request.data.Remote_DPCD_Read.raw = CFSwapInt32HostToBig(msg2->body.request.data.Remote_DPCD_Read.raw);
						msg3->body.Sideband_MSG_Body_CRC = crc8((void*)&msg2->body, msg2->header.Sideband_MSG_Body_Length);

						bzero(&request, sizeof(request));
						request.sendTransactionType = kIOI2CDisplayPortNativeTransactionType;
						request.sendAddress = DP_SIDEBAND_MSG_DOWN_REQ_BASE;
						request.sendBuffer = (vm_address_t) msg1;
						request.sendBytes = (UInt32)((void*)&msg3->end - (void*)&msg1->header);

						if (!dpcdAddr) {
							iprintf("{\n"); INDENT
							iprintf("(message:");
							for (int i = 0; i < request.sendBytes; i++) {
								cprintf(" %02X", ((UInt8*)request.sendBuffer)[i]);
							}
							cprintf(")\n");
							iprintf("(lct=%d lcr=%d dst=",
								msg1->header.Link_Count_Total,
								msg1->header.Link_Count_Remaining
							);
							for (int i = 0; i < msg1->header.Link_Count_Total - 1; i++) {
								cprintf("%s%d", i > 0 ? "." : "", (msg1->header.Relative_Address[i / 2] >> ((i & 1) ? 4 : 0)) & 15);
							}
							cprintf(" broadcast=%d path=%d len=%d somt=%d eomt=%d seq=%d crc=0x%x)\n",
								msg2->header.Broadcast_Message,
								msg2->header.Path_Message,
								msg2->header.Sideband_MSG_Body_Length,
								msg2->header.Start_Of_Message_Transaction,
								msg2->header.End_Of_Message_Transaction,
								msg2->header.Message_Sequence_No,
								msg2->header.Sideband_MSG_Header_CRC
							);

							iprintf("(type=");
							switch (msg2->body.request.Request_Identifier) {
								case DP_GET_MSG_TRANSACTION_VERSION:
									cprintf("GET_MSG_TRANSACTION_VERSION");
									break;
								case DP_LINK_ADDRESS:
									cprintf("LINK_ADDRESS");
									break;
								case DP_CONNECTION_STATUS_NOTIFY:
									cprintf("CONNECTION_STATUS_NOTIFY");
									break;
								case DP_ENUM_PATH_RESOURCES:
									cprintf("ENUM_PATH_RESOURCES");
									break;
								case DP_ALLOCATE_PAYLOAD:
									cprintf("ALLOCATE_PAYLOAD");
									break;
								case DP_QUERY_PAYLOAD:
									cprintf("QUERY_PAYLOAD");
									break;
								case DP_RESOURCE_STATUS_NOTIFY:
									cprintf("RESOURCE_STATUS_NOTIFY");
									break;
								case DP_CLEAR_PAYLOAD_ID_TABLE:
									cprintf("CLEAR_PAYLOAD_ID_TABLE");
									break;
								case DP_REMOTE_DPCD_READ:
									cprintf("REMOTE_DPCD_READ");
									msg2->body.request.data.Remote_DPCD_Read.raw = CFSwapInt32HostToBig(msg2->body.request.data.Remote_DPCD_Read.raw);
									cprintf(" port=%d, dpcd=%05x bytestoread=%d crc=0x%02x",
										msg2->body.request.data.Remote_DPCD_Read.bits.Port_Number,
										msg2->body.request.data.Remote_DPCD_Read.bits.DPCD_Address,
										msg2->body.request.data.Remote_DPCD_Read.bits.Number_Of_Bytes_To_Read,
										msg3->body.Sideband_MSG_Body_CRC
									);
									msg2->body.request.data.Remote_DPCD_Read.raw = CFSwapInt32HostToBig(msg2->body.request.data.Remote_DPCD_Read.raw);
									break;
								case DP_REMOTE_DPCD_WRITE:
									cprintf("REMOTE_DPCD_WRITE");
									break;
								case DP_REMOTE_I2C_READ:
									cprintf("REMOTE_I2C_READ");
									break;
								case DP_REMOTE_I2C_WRITE:
									cprintf("REMOTE_I2C_WRITE");
									break;
								case DP_POWER_UP_PHY:
									cprintf("POWER_UP_PHY");
									break;
								case DP_POWER_DOWN_PHY:
									cprintf("POWER_DOWN_PHY");
									break;
								case DP_SINK_EVENT_NOTIFY:
									cprintf("SINK_EVENT_NOTIFY");
									break;
								case DP_QUERY_STREAM_ENC_STATUS:
									cprintf("QUERY_STREAM_ENC_STATUS");
									break;
								default:
									cprintf("?0x%x", msg2->body.request.Request_Identifier);
									break;
							} // switch
							cprintf(")\n");
						} // if !dpcdAddr

						/*
						result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
						usleep(kDelayDisplayPortReply);
						if (result || request.result) {
							continue;
						}
						*/

						Message_Transaction_Reply *reply = (void*)&replydata;
						//NSLog(@"hello");
						for (int poll = 0; poll < 1 * (dpcdAddr ? 1 : 1000); poll++) {
							if (poll > 0) bzero(&request, sizeof(request));
							request.replyTransactionType = kIOI2CDisplayPortNativeTransactionType;
							request.replyAddress = DP_SIDEBAND_MSG_DOWN_REP_BASE;
							request.replyBuffer = (vm_address_t) replydata;
							request.replyBytes = 1;
							result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
							//usleep(kDelayDisplayPortReply);
							if (result || request.result) {
								continue;
							}
							if (reply->Request_Identifier == msg2->body.request.Request_Identifier) {
								break;
							}
							else {
								result = -1;
								continue;
							}
						}
						if (!dpcdAddr) {
							iprintf("(reply:");
							for (int i = 0; i < DP_AUX_MAX_PAYLOAD_BYTES; i++) {
								cprintf(" %02X", replydata[i]);
							}
							cprintf(")\n");

							switch (reply->Reply_Type) {
								case DP_SIDEBAND_REPLY_NAK:
									iprintf("(reply_type=NAK guid=%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x reason=",
										*(UInt32*)((void*)&reply->data.NAK.Global_Unique_Identifier + 0),
										*(UInt16*)((void*)&reply->data.NAK.Global_Unique_Identifier + 4),
										*(UInt16*)((void*)&reply->data.NAK.Global_Unique_Identifier + 6),
										*(UInt16*)((void*)&reply->data.NAK.Global_Unique_Identifier + 8),
										*(UInt8*)((void*)&reply->data.NAK.Global_Unique_Identifier + 10),
										*(UInt8*)((void*)&reply->data.NAK.Global_Unique_Identifier + 11),
										*(UInt8*)((void*)&reply->data.NAK.Global_Unique_Identifier + 12),
										*(UInt8*)((void*)&reply->data.NAK.Global_Unique_Identifier + 13),
										*(UInt8*)((void*)&reply->data.NAK.Global_Unique_Identifier + 14),
										*(UInt8*)((void*)&reply->data.NAK.Global_Unique_Identifier + 15)
									);
									switch (reply->data.NAK.Reason_For_NAK) {
										case DP_NAK_WRITE_FAILURE:
											cprintf("DP_NAK_WRITE_FAILURE");
											break;
										case DP_NAK_INVALID_READ:
											cprintf("DP_NAK_INVALID_READ");
											break;
										case DP_NAK_CRC_FAILURE:
											cprintf("DP_NAK_CRC_FAILURE");
											break;
										case DP_NAK_BAD_PARAM:
											cprintf("DP_NAK_BAD_PARAM");
											break;
										case DP_NAK_DEFER:
											cprintf("DP_NAK_DEFER");
											break;
										case DP_NAK_LINK_FAILURE:
											cprintf("DP_NAK_LINK_FAILURE");
											break;
										case DP_NAK_NO_RESOURCES:
											cprintf("DP_NAK_NO_RESOURCES");
											break;
										case DP_NAK_DPCD_FAIL:
											cprintf("DP_NAK_DPCD_FAIL");
											break;
										case DP_NAK_I2C_NAK:
											cprintf("DP_NAK_I2C_NAK");
											break;
										case DP_NAK_ALLOCATE_FAIL:
											cprintf("DP_NAK_ALLOCATE_FAIL");
											break;
										default:
											cprintf("DP_NAK_ALLOCATE_FAIL");
											break;
									}
									break;
								case DP_SIDEBAND_REPLY_ACK:
									iprintf("(reply_type=ACK type=");
									switch (reply->Request_Identifier) {
										case DP_GET_MSG_TRANSACTION_VERSION:
											cprintf("GET_MSG_TRANSACTION_VERSION");
											break;
										case DP_LINK_ADDRESS:
											cprintf("LINK_ADDRESS");
											break;
										case DP_CONNECTION_STATUS_NOTIFY:
											cprintf("CONNECTION_STATUS_NOTIFY");
											break;
										case DP_ENUM_PATH_RESOURCES:
											cprintf("ENUM_PATH_RESOURCES");
											break;
										case DP_ALLOCATE_PAYLOAD:
											cprintf("ALLOCATE_PAYLOAD");
											break;
										case DP_QUERY_PAYLOAD:
											cprintf("QUERY_PAYLOAD");
											break;
										case DP_RESOURCE_STATUS_NOTIFY:
											cprintf("RESOURCE_STATUS_NOTIFY");
											break;
										case DP_CLEAR_PAYLOAD_ID_TABLE:
											cprintf("CLEAR_PAYLOAD_ID_TABLE");
											break;
										case DP_REMOTE_DPCD_READ:
											cprintf("REMOTE_DPCD_READ");
											/*
											UInt8 Port_Number : 4; // LSB
											UInt8 zeros : 4; // MSB
											UInt8 Number_Of_Bytes_Read;
											UInt8 Data_Read[1];
											*/
											break;
										case DP_REMOTE_DPCD_WRITE:
											cprintf("REMOTE_DPCD_WRITE");
											break;
										case DP_REMOTE_I2C_READ:
											cprintf("REMOTE_I2C_READ");
											/*
											UInt8 Port_Number : 4; // LSB
											UInt8 zeros : 4; // MSB
											UInt8 Number_Of_Bytes_Read;
											UInt8 Data_Read[1];
											*/
											break;
										case DP_REMOTE_I2C_WRITE:
											cprintf("REMOTE_I2C_WRITE");
											break;
										case DP_POWER_UP_PHY:
											cprintf("POWER_UP_PHY");
											break;
										case DP_POWER_DOWN_PHY:
											cprintf("POWER_DOWN_PHY");
											break;
										case DP_SINK_EVENT_NOTIFY:
											cprintf("SINK_EVENT_NOTIFY");
											break;
										case DP_QUERY_STREAM_ENC_STATUS:
											cprintf("QUERY_STREAM_ENC_STATUS");
											break;
										default:
											cprintf("?0x%x", reply->Request_Identifier);
											break;
									} // switch reply->Request_Identifier
									break;
							} // switch reply->Reply_Type

							cprintf(")\n");
							OUTDENT iprintf("}\n");
						} // if !dpcdAddr

						if (reply->Request_Identifier == msg2->body.request.Request_Identifier) {
							bzero(&request, sizeof(request));
							request.replyTransactionType = kIOI2CDisplayPortNativeTransactionType;
							request.replyAddress = DP_SIDEBAND_MSG_DOWN_REP_BASE + DP_AUX_MAX_PAYLOAD_BYTES;
							request.replyBuffer = (vm_address_t) replydata + DP_AUX_MAX_PAYLOAD_BYTES;
							request.replyBytes = DP_AUX_MAX_PAYLOAD_BYTES;
							result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
							//usleep(kDelayDisplayPortReply);
							if (result || request.result) {
								continue;
							}
						}
						else {
							result = -1;
							continue;
						}

					}
					else {
						bzero(&request, sizeof(request));
						request.replyTransactionType = kIOI2CDisplayPortNativeTransactionType;
						request.replyAddress = dpcdAddr;
						request.replyBuffer = (vm_address_t) &dpcd[dpcdAddr];
						request.replyBytes = DP_AUX_MAX_PAYLOAD_BYTES;
						result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
						usleep(kDelayDisplayPortReply);
						if (result || request.result) {
							continue;
						}
					}

					break;
				} // for attempt

				hasError = false;
				if (result) {
					iprintf("(%05xh: request error:%x)\n", dpcdAddr, result);
					hasError = true;
				}
				if (request.result) {
					iprintf("(%05xh: IOI2CSendRequest error:%x)\n", dpcdAddr, request.result);
					hasError = true;
				}
				if (hasError && dpcdAddr == 0) {
					break; // if we can't read dpcd 00000h then it's probably not a DisplayPort device
				}
			} // for dpcdAddr
			if (hasError && dpcdAddr == 0) {
				break;
			}
		} // for dpcdRangeNdx
		if (dpcdRangeNdx > 0) {
			parsedpcd(dpcd);
			
			if (dpcd[DP_MSTM_CAP] & DP_MST_CAP) { // 0x021   /* 1.2 */

				int numPorts = dpcd[DP_DOWN_STREAM_PORT_COUNT] & DP_PORT_COUNT_MASK;
				for (int port = 0; port < numPorts; port++) {
					path[pathLength] = port;
					iprintf("Port %d = {\n", port); INDENT
					DoOneDisplayPort(i2cconnect, path, pathLength + 1);
					OUTDENT iprintf("}; // Port %d\n", port);
				} // for port

			} // if mst
		} // if dpcd

		free(dpcd);
	} // if dpcd
} // DoOneDisplayPort

CFMutableDictionaryRef DumpDisplayParameters(char *parametersName, CFDictionaryRef dictDisplayParameters0) {
	iprintf("%s = {\n", parametersName); INDENT

	CFMutableDictionaryRef dictDisplayParameters = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, dictDisplayParameters0);
	if (dictDisplayParameters) {
		CFIndex itemCount = CFDictionaryGetCount(dictDisplayParameters);

		const void **keys   = malloc(itemCount * sizeof(void*));
		const void **values = malloc(itemCount * sizeof(void*));
		CFDictionaryGetKeysAndValues(dictDisplayParameters, keys, values);

		for (int i = 0; i < itemCount; i++) {
			char key[50];
			if (CFStringGetCString(keys[i], key , sizeof(key), kCFStringEncodingUTF8)) {
				if (CFGetTypeID(values[i]) == CFDictionaryGetTypeID()) {
					CFMutableDictionaryRef parameterDict = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, values[i]);
					if (parameterDict) {
						CFNumberRef min = CFDictionaryGetValue(parameterDict, CFSTR(kIODisplayMinValueKey));
						CFNumberRef max = CFDictionaryGetValue(parameterDict, CFSTR(kIODisplayMaxValueKey));
						CFNumberRef val = CFDictionaryGetValue(parameterDict, CFSTR(kIODisplayValueKey   ));
						if (min && max && val && CFGetTypeID(min) == CFNumberGetTypeID() && CFGetTypeID(max) == CFNumberGetTypeID() && CFGetTypeID(val) == CFNumberGetTypeID()) {
							SInt32 val_min = 0;
							SInt32 val_max = 0;
							SInt32 val_val = 0;
							CFNumberGetValue(min, kCFNumberSInt32Type, &val_min);
							CFNumberGetValue(max, kCFNumberSInt32Type, &val_max);
							CFNumberGetValue(val, kCFNumberSInt32Type, &val_val);
							
							char *name;
							bool isFixedPoint = 0;
							
							/**/ if(!strcmp(key, kIODisplayBrightnessProbeKey        )) { name = "kIODisplayBrightnessProbeKey"; }
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

							else name = key;
							
							if (isFixedPoint) {
								iprintf("%s = %.5f (%g…%g);\n", name, val_val / 65536.0, val_min / 65536.0, val_max / 65536.0);
							}
							else {
								iprintf("%s = %d (%d…%d);\n", name, val_val, val_min, val_max);
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

void DumpOneIODisplayAttributes(CFMutableDictionaryRef dictDisplayInfo)
{
	typedef struct IODisplayAttributesRec {
		union {
			char c[4];
			UInt32 v;
		} attribute;
		UInt32 value;
	} IODisplayAttributesRec;

	CFDataRef IODisplayAttributesData = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kIODisplayAttributesKey));
	
	if (IODisplayAttributesData && CFGetTypeID(IODisplayAttributesData) == CFDataGetTypeID()) {
		iprintf("IODisplayAttributes = [\n"); INDENT
		
		if (CFDataGetLength(IODisplayAttributesData) % sizeof(IODisplayAttributesRec) != 0) {
			iprintf("DisplayPixelDimensions = unexpected size %ld\n", CFDataGetLength(IODisplayAttributesData));
		}
		else {
			IODisplayAttributesRec *p = (IODisplayAttributesRec *)CFDataGetBytePtr(IODisplayAttributesData);
			for (int i = 0; i < CFDataGetLength(IODisplayAttributesData) / sizeof(IODisplayAttributesRec); i++) {
				char buf[200];
				
				char* name;
				switch (p[i].attribute.v) {
					case kIOFBSpeedAttribute                 : name = "kIOFBSpeedAttribute                 "; break;
					case kIOFBWSStartAttribute               : name = "kIOFBWSStartAttribute               "; break;
					case kIOFBProcessConnectChangeAttribute  : name = "kIOFBProcessConnectChangeAttribute  "; break;
					case kIOFBEndConnectChangeAttribute      : name = "kIOFBEndConnectChangeAttribute      "; break;
					case kIOFBMatchedConnectChangeAttribute  : name = "kIOFBMatchedConnectChangeAttribute  "; break;
					// Connection attributesk
					case kConnectionInTVMode                 : name = "kConnectionInTVMode                 "; break;
					case kConnectionWSSB                     : name = "kConnectionWSSB                     "; break;
					case kConnectionRawBacklight             : name = "kConnectionRawBacklight             "; break;
					case kConnectionBacklightSave            : name = "kConnectionBacklightSave            "; break;
					case kConnectionVendorTag                : name = "kConnectionVendorTag                "; break;

					case kConnectionFlags                    : name = "kConnectionFlags                    ";
						
					case kConnectionSyncEnable               : name = "kConnectionSyncEnable               "; break;
					case kConnectionSyncFlags                : name = "kConnectionSyncFlags                "; break;
					
					case kConnectionSupportsAppleSense       : name = "kConnectionSupportsAppleSense       "; break;
					case kConnectionSupportsLLDDCSense       : name = "kConnectionSupportsLLDDCSense       "; break;
					case kConnectionSupportsHLDDCSense       : name = "kConnectionSupportsHLDDCSense       "; break;
					case kConnectionEnable                   : name = "kConnectionEnable                   "; break;
					case kConnectionCheckEnable              : name = "kConnectionCheckEnable              "; break;
					case kConnectionProbe                    : name = "kConnectionProbe                    "; break;
					case kConnectionIgnore                   : name = "kConnectionIgnore                   "; break;
					case kConnectionChanged                  : name = "kConnectionChanged                  "; break;
					case kConnectionPower                    : name = "kConnectionPower                    "; break;
					case kConnectionPostWake                 : name = "kConnectionPostWake                 "; break;
					case kConnectionDisplayParameterCount    : name = "kConnectionDisplayParameterCount    "; break;
					case kConnectionDisplayParameters        : name = "kConnectionDisplayParameters        "; break;
					case kConnectionOverscan                 : name = "kConnectionOverscan                 "; break;
					case kConnectionVideoBest                : name = "kConnectionVideoBest                "; break;

					case kConnectionRedGammaScale            : name = "kConnectionRedGammaScale            "; break;
					case kConnectionGreenGammaScale          : name = "kConnectionGreenGammaScale          "; break;
					case kConnectionBlueGammaScale           : name = "kConnectionBlueGammaScale           "; break;
					case kConnectionGammaScale               : name = "kConnectionGammaScale               "; break;
					case kConnectionFlushParameters          : name = "kConnectionFlushParameters          "; break;

					case kConnectionVBLMultiplier            : name = "kConnectionVBLMultiplier            "; break;

					case kConnectionHandleDisplayPortEvent   : name = "kConnectionHandleDisplayPortEvent   "; break;

					case kConnectionPanelTimingDisable       : name = "kConnectionPanelTimingDisable       "; break;

					case kConnectionColorMode                : name = "kConnectionColorMode                "; break;
					case kConnectionColorModesSupported      : name = "kConnectionColorModesSupported      "; break;
					case kConnectionColorDepthsSupported     : name = "kConnectionColorDepthsSupported     "; break;

					case kConnectionControllerDepthsSupported: name = "kConnectionControllerDepthsSupported"; break;
					case kConnectionControllerColorDepth     : name = "kConnectionControllerColorDepth     "; break;
					case kConnectionControllerDitherControl  : name = "kConnectionControllerDitherControl  "; break;

					case kConnectionDisplayFlags             : name = "kConnectionDisplayFlags             "; break;

					case kConnectionEnableAudio              : name = "kConnectionEnableAudio              "; break;
					case kConnectionAudioStreaming           : name = "kConnectionAudioStreaming           "; break;

					case kConnectionStartOfFrameTime         : name = "kConnectionStartOfFrameTime         "; break;
					default                                  : name = "?                                   "; break;
				}

				switch (p[i].attribute.v) {
					case kConnectionFlags                    : name = "kConnectionFlags                    ";
						snprintf(buf, sizeof(buf), "%s%s%s",
							p[i].value & kIOConnectionBuiltIn    ?    "BuiltIn," : "",
							p[i].value & kIOConnectionStereoSync ? "StereoSync," : "",
							UNKNOWN_FLAG(p[i].value & 0xffff77ff)
						); break;

					case kConnectionSyncFlags:
						snprintf(buf, sizeof(buf), "%s%s%s%s%s%s%s%s%s",
							p[i].value & kIOHSyncDisable          ?          "HSyncDisable," : "",
							p[i].value & kIOVSyncDisable          ?          "VSyncDisable," : "",
							p[i].value & kIOCSyncDisable          ?          "CSyncDisable," : "",
							p[i].value & kIONoSeparateSyncControl ? "NoSeparateSyncControl," : "",
							p[i].value & kIOTriStateSyncs         ?         "TriStateSyncs," : "",
							p[i].value & kIOSyncOnBlue            ?            "SyncOnBlue," : "",
							p[i].value & kIOSyncOnGreen           ?           "SyncOnGreen," : "",
							p[i].value & kIOSyncOnRed             ?             "SyncOnRed," : "",
							UNKNOWN_FLAG(p[i].value & 0xffffff00)
						); break;

					case kConnectionHandleDisplayPortEvent:
						snprintf(buf, sizeof(buf), "%s",
							p[i].value == kIODPEventStart                       ?                       "Start" :
							p[i].value == kIODPEventIdle                        ?                        "Idle" :
							p[i].value == kIODPEventForceRetrain                ?                "ForceRetrain" :
							p[i].value == kIODPEventRemoteControlCommandPending ? "RemoteControlCommandPending" :
							p[i].value == kIODPEventAutomatedTestRequest        ?        "AutomatedTestRequest" :
							p[i].value == kIODPEventContentProtection           ?           "ContentProtection" :
							p[i].value == kIODPEventMCCS                        ?                        "MCCS" :
							p[i].value == kIODPEventSinkSpecific                ?                "SinkSpecific" :
							p[i].value == kIODPEventStart                       ?                       "Start" :
							UNKNOWN_VALUE(p[i].value)
						); break;

					case kConnectionColorMode:
					case kConnectionColorModesSupported:
						snprintf(buf, sizeof(buf), "%s%s%s%s%s%s%s",
							p[i].value == kIODisplayColorModeReserved  ?    "Reserved" : "",
							p[i].value & kIODisplayColorModeRGB        ?        "RGB," : "",
							p[i].value & kIODisplayColorModeYCbCr422   ?        "422," : "",
							p[i].value & kIODisplayColorModeYCbCr444   ?        "444," : "",
							p[i].value & kIODisplayColorModeRGBLimited ? "RGBLimited," : "",
							p[i].value & kIODisplayColorModeAuto       ?       "Auto," : "",
							UNKNOWN_FLAG(p[i].value & 0xefffeeee)
						); break;

					case kConnectionColorDepthsSupported:
					case kConnectionControllerDepthsSupported:
					case kConnectionControllerColorDepth:
						snprintf(buf, sizeof(buf), "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
							p[i].value == kIODisplayRGBColorComponentBitsUnknown ? "Unknown" : "",
							p[i].value & kIODisplayRGBColorComponentBits6        ?  "RGB 6," : "",
							p[i].value & kIODisplayRGBColorComponentBits8        ?  "RGB 8," : "",
							p[i].value & kIODisplayRGBColorComponentBits10       ? "RGB 10," : "",
							p[i].value & kIODisplayRGBColorComponentBits12       ? "RGB 12," : "",
							p[i].value & kIODisplayRGBColorComponentBits14       ? "RGB 14," : "",
							p[i].value & kIODisplayRGBColorComponentBits16       ? "RGB 16," : "",
							p[i].value & kIODisplayYCbCr444ColorComponentBits6   ?  "444 6," : "",
							p[i].value & kIODisplayYCbCr444ColorComponentBits8   ?  "444 8," : "",
							p[i].value & kIODisplayYCbCr444ColorComponentBits10  ? "444 10," : "",
							p[i].value & kIODisplayYCbCr444ColorComponentBits12  ? "444 12," : "",
							p[i].value & kIODisplayYCbCr444ColorComponentBits14  ? "444 14," : "",
							p[i].value & kIODisplayYCbCr444ColorComponentBits16  ? "444 16," : "",
							p[i].value & kIODisplayYCbCr422ColorComponentBits6   ?  "422 6," : "",
							p[i].value & kIODisplayYCbCr422ColorComponentBits8   ?  "422 8," : "",
							p[i].value & kIODisplayYCbCr422ColorComponentBits10  ? "422 10," : "",
							p[i].value & kIODisplayYCbCr422ColorComponentBits12  ? "422 12," : "",
							p[i].value & kIODisplayYCbCr422ColorComponentBits14  ? "422 14," : "",
							p[i].value & kIODisplayYCbCr422ColorComponentBits16  ? "422 16," : "",
							UNKNOWN_FLAG(p[i].value & 0xffc00000)
						); break;

					case kConnectionControllerDitherControl:
						snprintf(buf, sizeof(buf), "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
							((p[i].value >> kIODisplayDitherRGBShift     ) & 0xff) == kIODisplayDitherDisable         ?           "RGB Disabled" : "",
							((p[i].value >> kIODisplayDitherRGBShift     ) & 0xff) == kIODisplayDitherAll             ?                "RGB All" : "",
							((p[i].value >> kIODisplayDitherRGBShift     ) & 0xff) & kIODisplayDitherSpatial          ?           "RGB Spatial," : "",
							((p[i].value >> kIODisplayDitherRGBShift     ) & 0xff) & kIODisplayDitherTemporal         ?          "RGB Temporal," : "",
							((p[i].value >> kIODisplayDitherRGBShift     ) & 0xff) & kIODisplayDitherFrameRateControl ?  "RGB FrameRateControl," : "",
							((p[i].value >> kIODisplayDitherRGBShift     ) & 0xff) & kIODisplayDitherDefault          ?           "RGB Default," : "",
							((p[i].value >> kIODisplayDitherRGBShift     ) & 0xff) & 0x70                             ?                 "RGB ?," : "",
							((p[i].value >> kIODisplayDitherYCbCr444Shift) & 0xff) == kIODisplayDitherDisable         ?           "444 Disabled" : "",
							((p[i].value >> kIODisplayDitherYCbCr444Shift) & 0xff) == kIODisplayDitherAll             ?                "444 All" : "",
							((p[i].value >> kIODisplayDitherYCbCr444Shift) & 0xff) & kIODisplayDitherSpatial          ?           "444 Spatial," : "",
							((p[i].value >> kIODisplayDitherYCbCr444Shift) & 0xff) & kIODisplayDitherTemporal         ?          "444 Temporal," : "",
							((p[i].value >> kIODisplayDitherYCbCr444Shift) & 0xff) & kIODisplayDitherFrameRateControl ?  "444 FrameRateControl," : "",
							((p[i].value >> kIODisplayDitherYCbCr444Shift) & 0xff) & kIODisplayDitherDefault          ?           "444 Default," : "",
							((p[i].value >> kIODisplayDitherYCbCr444Shift) & 0xff) & 0x70                             ?                 "444 ?," : "",
							((p[i].value >> kIODisplayDitherYCbCr422Shift) & 0xff) == kIODisplayDitherDisable         ?           "422 Disabled" : "",
							((p[i].value >> kIODisplayDitherYCbCr422Shift) & 0xff) == kIODisplayDitherAll             ?                "422 All" : "",
							((p[i].value >> kIODisplayDitherYCbCr422Shift) & 0xff) & kIODisplayDitherSpatial          ?           "422 Spatial," : "",
							((p[i].value >> kIODisplayDitherYCbCr422Shift) & 0xff) & kIODisplayDitherTemporal         ?          "422 Temporal," : "",
							((p[i].value >> kIODisplayDitherYCbCr422Shift) & 0xff) & kIODisplayDitherFrameRateControl ?  "422 FrameRateControl," : "",
							((p[i].value >> kIODisplayDitherYCbCr422Shift) & 0xff) & kIODisplayDitherDefault          ?           "422 Default," : "",
							((p[i].value >> kIODisplayDitherYCbCr422Shift) & 0xff) & 0x70                             ?                 "422 ?," : "",
							UNKNOWN_FLAG(p[i].value & 0xff000000)
						); break;
						
					case kConnectionDisplayFlags:
						snprintf(buf, sizeof(buf), "%s%s",
							p[i].value == kIODisplayNeedsCEAUnderscan ? "NeedsCEAUnderscan," : "",
								 UNKNOWN_FLAG(p[i].value & 0xfffffffe)
						); break;

					default:
						snprintf(buf, sizeof(buf), "");
				}
				
				char attributename[20]; attributename[0] = '\0';
				int x = 0;
				if (p[i].attribute.c[3]) x += snprintf(attributename + x, sizeof(attributename) - x, "%c", p[i].attribute.c[3]); else x += snprintf(attributename + x, sizeof(attributename) - x, "ø");
				if (p[i].attribute.c[2]) x += snprintf(attributename + x, sizeof(attributename) - x, "%c", p[i].attribute.c[2]); else x += snprintf(attributename + x, sizeof(attributename) - x, "ø");
				if (p[i].attribute.c[1]) x += snprintf(attributename + x, sizeof(attributename) - x, "%c", p[i].attribute.c[1]); else x += snprintf(attributename + x, sizeof(attributename) - x, "ø");
				if (p[i].attribute.c[0]) x += snprintf(attributename + x, sizeof(attributename) - x, "%c", p[i].attribute.c[0]); else x += snprintf(attributename + x, sizeof(attributename) - x, "ø");
				iprintf("[%d] = { %s %s = 0x%08x : %s },\n", i, name, attributename, p[i].value, buf);
			}
			CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kIODisplayAttributesKey));
		}
		OUTDENT iprintf("]; // IODisplayAttributes\n");
	} // if IODisplayAttributes

} // DumpOneIODisplayAttributes



void DumpDisplayInfo(CFMutableDictionaryRef dictDisplayInfo)
{
	SInt32 numValue;
	CFNumberRef num;

	{ // DisplayVendorID
		num = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplayVendorID));
		if (num) {
			CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
			iprintf("DisplayVendorID = %d (0x%04x) %s\"%c%c%c\";\n", (int)numValue, (int)numValue,
					numValue & 0xffff8000 ? UNKNOWN_VALUE(numValue & 0xffff8000) : "",
					((numValue >> 10) & 31) + '@',
					((numValue >>  5) & 31) + '@',
					((numValue >>  0) & 31) + '@'
			);
			CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kDisplayVendorID));
		}	} // DisplayVendorID
	
	{ // DisplayProductID
		num = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplayVendorID));
		num = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplayProductID));
		if (num) {
			CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
			iprintf("DisplayProductID = %d (0x%04x);\n", (int)numValue, (int)numValue);
			CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kDisplayProductID));
		}
	} // DisplayProductID

	{ // DisplayProductName

		// Show the preferred DisplayProductName
		{
			CFDictionaryRef names = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplayProductName));
			if (names) {
				CFArrayRef langKeys = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
				CFDictionaryApplyFunction(names, KeyArrayCallback, (void *) langKeys);
				CFArrayRef orderLangKeys = CFBundleCopyPreferredLocalizationsFromArray(langKeys);
				CFRelease(langKeys);
				if (orderLangKeys && CFArrayGetCount(orderLangKeys)) {
					char            cName[256];
					CFStringRef     langKey;
					CFStringRef     localName;

					langKey = CFArrayGetValueAtIndex(orderLangKeys, 0);
					localName = CFDictionaryGetValue(names, langKey);
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
			CFDictionaryRef DisplayProductName0 = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplayProductName));
			if (DisplayProductName0) {
				DisplayProductName = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, DisplayProductName0);

				CFIndex itemCount = CFDictionaryGetCount(DisplayProductName);

				const void **keys   = malloc(itemCount * sizeof(void*));
				const void **values = malloc(itemCount * sizeof(void*));
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
							CFStringGetCString(values[i], name, sizeof(name), kCFStringEncodingUTF8) &&
							CFStringGetCString(keys  [i], key , sizeof(key ), kCFStringEncodingUTF8))
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

	{ // IODisplayConnectFlags
		CFDataRef IODisplayConnectFlagsData = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kIODisplayConnectFlagsKey));
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
		CFDictionaryRef IODisplayAttributesDict0 = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kIODisplayAttributesKey));
		
		if (IODisplayAttributesDict0 && CFGetTypeID(IODisplayAttributesDict0) == CFDictionaryGetTypeID()) {
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
		} else {
			DumpOneIODisplayAttributes(dictDisplayInfo);
		}
	} // IODisplayAttributes

	{ // IODisplayParameters
		CFDictionaryRef dictDisplayParameters0 = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kIODisplayParametersKey));
		if (dictDisplayParameters0 && CFGetTypeID(dictDisplayParameters0) == CFDictionaryGetTypeID()) {
			CFMutableDictionaryRef dictDisplayParameters = DumpDisplayParameters("IODisplayParameters", dictDisplayParameters0);
			if (dictDisplayParameters) {
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kIODisplayParametersKey));
			}
		}
	} // IODisplayParameters

	{ // IODisplayEDID
		CFDataRef IODisplayEDIDData = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kIODisplayEDIDKey));
		if (IODisplayEDIDData) {
			if (CFDataGetLength(IODisplayEDIDData)) {
				iprintf("IODisplayEDID%s = ", CFDictionaryGetCountOfKey(dictDisplayInfo, CFSTR(kIODisplayEDIDOriginalKey)) ? "        " : "");
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
		CFDataRef IODisplayEDIDOriginalData = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kIODisplayEDIDOriginalKey));
		if (IODisplayEDIDOriginalData) {
			if (CFDataGetLength(IODisplayEDIDOriginalData)) {
				iprintf("IODisplayEDIDOriginal = ");
				const UInt8 *p = CFDataGetBytePtr(IODisplayEDIDOriginalData);
				for (int i = 0; i < CFDataGetLength(IODisplayEDIDOriginalData); i++) {
					cprintf("%02x", p[i]);
				}
				lf;
			}
			CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kIODisplayEDIDOriginalKey));
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


// https://github.com/apple-oss-distributions/IOKitUser/blob/main/graphics.subproj/IODisplayTest.c
void DumpDisplayService(io_service_t ioFramebufferService, int modeAlias)
{
	CFNumberRef         num;
	CGError             err;
	SInt32              numValue;

	CFDictionaryRef dictDisplayInfo0 = IODisplayCreateInfoDictionary(ioFramebufferService, kNilOptions);
	CFMutableDictionaryRef dictDisplayInfo = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, dictDisplayInfo0);
	CFRelease(dictDisplayInfo0);

	if (dictDisplayInfo) { // DisplayInfo
		iprintf("DisplayInfo = {\n"); INDENT

		DumpDisplayInfo(dictDisplayInfo);

		{ // DisplayHorizontalImageSize && DisplayVerticalImageSize
			CFNumberRef h = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplayHorizontalImageSize));
			CFNumberRef v = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplayVerticalImageSize));
			if (h || v) {
				SInt32 hval = -1;
				SInt32 vval = -1;
				CFNumberGetValue(h, kCFNumberSInt32Type, &hval);
				CFNumberGetValue(v, kCFNumberSInt32Type, &vval);
				iprintf("DisplayHorizontalImageSize, DisplayVerticalImageSize = %dx%dmm;\n", hval, vval);
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kDisplayHorizontalImageSize));
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kDisplayVerticalImageSize));
			}
		} // DisplayHorizontalImageSize

		{ // DisplaySubPixelLayout
			num = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplaySubPixelLayout));
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
			num = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplaySubPixelConfiguration));
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
			num = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplaySubPixelShape));
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
			CFDataRef displayPixelDimensionsData = CFDictionaryGetValue(dictDisplayInfo, CFSTR("DisplayPixelDimensions"));
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
			num = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kIOFBTransformKey));
			if (num) {
				CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
				iprintf("IOFBTransform = ");
				DumpOneTransform(numValue);
				cprintf(";\n");
				CFDictionaryRemoveValue(dictDisplayInfo, CFSTR(kIOFBTransformKey));
			}
		} // IOFBTransform
		
		{ // graphic-options
			num = CFDictionaryGetValue(dictDisplayInfo, CFSTR("graphic-options"));
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
				float val_ ## C ## Point ## X = 0.0; \
				CFNumberRef num_ ## C ## Point ## X = CFDictionaryGetValue(dictDisplayInfo, CFSTR(kDisplay ## C ## Point ## X)); \
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

		iprintf("DisplayInfo properties (unparsed) = ");
		CFOutput(dictDisplayInfo);
		cprintf("; // DisplayInfo properties (unparsed)\n");

		OUTDENT iprintf("}; // DisplayInfo\n\n");
		CFRelease(dictDisplayInfo);
	} // DisplayInfo
	
	CFDictionaryRef dictDisplayParameters = NULL;
	if (KERN_SUCCESS == IODisplayCopyParameters(ioFramebufferService, kNilOptions, &dictDisplayParameters)) {
		// same as IODisplay/IODisplayParameters
		DumpDisplayParameters("IODisplayCopyParameters", dictDisplayParameters);
		cprintf("\n");
		CFRelease(dictDisplayParameters);
	}

	iprintf("IOFramebuffer 0x%0x = {\n", ioFramebufferService); INDENT
	{
		CFMutableDictionaryRef IOFBProperties = NULL;
		if (!IORegistryEntryCreateCFProperties(ioFramebufferService, &IOFBProperties, kCFAllocatorDefault, kNilOptions)) {
			if (IOFBProperties) {
				
				{ // IOFBConfig
					CFMutableDictionaryRef IOFBConfig = NULL;
					CFDictionaryRef IOFBConfig0 = CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBConfigKey));
					if (IOFBConfig0) IOFBConfig = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, IOFBConfig0);
					if (IOFBConfig) {
						iprintf("IOFBConfig = {\n"); INDENT
						
						{ // IOFBModes
							CFMutableArrayRef IOFBModes = NULL;
							CFArrayRef IOFBModes0 = CFDictionaryGetValue(IOFBConfig, CFSTR(kIOFBModesKey));
							if (IOFBModes0) IOFBModes = CFArrayCreateMutableCopy(kCFAllocatorDefault, 0, IOFBModes0);
							if (IOFBModes && CFArrayGetCount(IOFBModes)) {
								iprintf("IOFBModes = [\n"); INDENT
								detailedTimingsCount = CFArrayGetCount(IOFBModes);
								detailedTimingsArr = malloc(detailedTimingsCount * sizeof(*detailedTimingsArr));
								if (detailedTimingsArr) { bzero(detailedTimingsArr, detailedTimingsCount * sizeof(*detailedTimingsArr)); }
								for (int i = 0, j = 0; i < detailedTimingsCount; i++) {
									CFMutableDictionaryRef IOFBMode = NULL;
									CFDictionaryRef IOFBMode0 = CFArrayGetValueAtIndex(IOFBModes, j);
									if (IOFBMode0) IOFBMode = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, IOFBMode0);
									iprintf("{");
									if (IOFBMode) {
										CFTypeRef val;
										{ val = CFDictionaryGetValue(IOFBMode, CFSTR(kIOFBModeTMKey));  if (val) { cprintf(" DetailedTimingInformation = { "); DumpOneDetailedTimingInformation(val, -1, modeAlias); cprintf("};");
											memcpy(&detailedTimingsArr[i], CFDataGetBytePtr(val), min(sizeof(*detailedTimingsArr), CFDataGetLength(val))); CFDictionaryRemoveValue(IOFBMode, CFSTR(kIOFBModeTMKey)); } }
										{ val = CFDictionaryGetValue(IOFBMode, CFSTR(kIOFBModeDMKey));  if (val) { cprintf(   " DisplayModeInformation = { "); DumpOneDisplayModeInformation   (val); cprintf(" };"); CFDictionaryRemoveValue(IOFBMode, CFSTR(kIOFBModeDMKey)); } }
										{ val = CFDictionaryGetValue(IOFBMode, CFSTR(kIOFBModeAIDKey)); if (val) { cprintf(              " AppleTimingID = "); DumpOneAppleID                  (val); cprintf(";");   CFDictionaryRemoveValue(IOFBMode, CFSTR(kIOFBModeAIDKey)); } }
										{ val = CFDictionaryGetValue(IOFBMode, CFSTR(kIOFBModeIDKey));  if (val) { cprintf(                         " ID = "); DumpOneID            (val, modeAlias); cprintf(";"); if (CFDictionaryGetCount(IOFBMode) == 1) { CFDictionaryRemoveValue(IOFBMode, CFSTR(kIOFBModeIDKey)); } } }
	//									{ val = CFDictionaryGetValue(IOFBMode, CFSTR(kIOFBModeDFKey));  if (val) { dumpproc((void*)val); } }
	//									{ val = CFDictionaryGetValue(IOFBMode, CFSTR(kIOFBModePIKey));  if (val) { dumpproc((void*)val); } }
	//									{ val = CFDictionaryGetValue(IOFBMode, CFSTR(kIOFBModeIDKey));  if (val) { dumpproc((void*)val); } }
	//									{ val = CFDictionaryGetValue(IOFBMode, CFSTR(kIOFBModeDMKey));  if (val) { dumpproc((void*)val); } }
										if (CFDictionaryGetCount(IOFBMode) > 0) {
											CFArraySetValueAtIndex(IOFBModes, j++, IOFBMode);
										}
										else {
											CFArrayRemoveValueAtIndex(IOFBModes, j);
										}
									}
									cprintf(" },\n");
								} // for
								OUTDENT iprintf("]; // IOFBModes\n");
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
							CFArrayRef IOFBDetailedTimings0 = CFDictionaryGetValue(IOFBConfig, CFSTR(kIOFBDetailedTimingsKey));
							if (IOFBDetailedTimings0) IOFBDetailedTimings = CFArrayCreateMutableCopy(kCFAllocatorDefault, 0, IOFBDetailedTimings0);
							if (IOFBDetailedTimings && CFArrayGetCount(IOFBDetailedTimings)) {
								iprintf("IOFBDetailedTimings = [\n"); INDENT
								int dups = 0;
								CFIndex count = CFArrayGetCount(IOFBDetailedTimings);
								for (int i = 0, j = 0; i < count; i++) {
									CFDataRef IOFBDetailedTiming = CFArrayGetValueAtIndex(IOFBDetailedTimings, j);
									if (IOFBDetailedTiming) {
										dups += DumpOneDetailedTimingInformation(IOFBDetailedTiming, i, modeAlias) ? 1 : 0;
									}
									CFArrayRemoveValueAtIndex(IOFBDetailedTimings, j);
								}
								if (dups) {
									iprintf("%d duplicates\n", dups);
								}
								OUTDENT iprintf("]; // IOFBDetailedTimings\n");
								if (CFArrayGetCount(IOFBDetailedTimings)) {
									CFDictionarySetValue(IOFBConfig, CFSTR(kIOFBDetailedTimingsKey), IOFBDetailedTimings);
								}
								else {
									CFDictionaryRemoveValue(IOFBConfig, CFSTR(kIOFBDetailedTimingsKey));
								}
							}
						} // IOFBDetailedTimings

						{ // IOFB0Hz (suppressRefresh)
							num = CFDictionaryGetValue(IOFBConfig, CFSTR("IOFB0Hz"));
							if (num) {
								CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
								iprintf("IOFB0Hz (suppressRefresh) = %s;\n", numValue == 1 ? "true" : numValue == 0 ? "false" : UNKNOWN_VALUE(numValue));
							}
							CFDictionaryRemoveValue(IOFBConfig, CFSTR("IOFB0Hz"));
						} // IOFB0Hz (suppressRefresh)

						{ // IOFBmHz (detailedRefresh)
							num = CFDictionaryGetValue(IOFBConfig, CFSTR("IOFBmHz"));
							if (num) {
								CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
								iprintf("IOFBmHz (detailedRefresh) = %s;\n", numValue == 1 ? "true" : numValue == 0 ? "false" : UNKNOWN_VALUE(numValue));
								CFDictionaryRemoveValue(IOFBConfig, CFSTR("IOFBmHz"));
							}
						} // IOFBmHz (detailedRefresh)

						{ // IOFBmir (displayMirror)
							num = CFDictionaryGetValue(IOFBConfig, CFSTR("IOFBmir"));
							if (num) {
								CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
								iprintf("IOFBmir (displayMirror) = %s\n", numValue == 1 ? "true" : numValue == 0 ? "false" : UNKNOWN_VALUE(numValue));
								CFDictionaryRemoveValue(IOFBConfig, CFSTR("IOFBmir"));
							}
						} // IOFBmir (displayMirror)

						{ // IOFBScalerUnderscan (useScalerUnderscan)
							num = CFDictionaryGetValue(IOFBConfig, CFSTR("IOFBmir"));
							if (num) {
								CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
								iprintf("IOFBScalerUnderscan (useScalerUnderscan) = %s;\n", numValue == 1 ? "true" : numValue == 0 ? "false" : UNKNOWN_VALUE(numValue));
								CFDictionaryRemoveValue(IOFBConfig, CFSTR("IOFBmir"));
							}
						} // IOFBScalerUnderscan (useScalerUnderscan)

						{ // IOFBtv (addTVFlag)
							num = CFDictionaryGetValue(IOFBConfig, CFSTR("IOFBmir"));
							if (num) {
								CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
								iprintf("IOFBtv (addTVFlag) = %s;\n", numValue == 1 ? "true" : numValue == 0 ? "false" : UNKNOWN_VALUE(numValue));
								CFDictionaryRemoveValue(IOFBConfig, CFSTR("IOFBmir"));
							}
						} // IOFBtv (addTVFlag)

						{ // dims (IOFBOvrDimensions)
							CFDataRef IOFBOvrDimensionsData = CFDictionaryGetValue(IOFBConfig, CFSTR("dims"));
							if (IOFBOvrDimensionsData) {
								if (CFDataGetLength(IOFBOvrDimensionsData) != sizeof(IOFBOvrDimensions)) {
									iprintf("dims (IOFBOvrDimensions) = unexpected size %ld\n", CFDataGetLength(IOFBOvrDimensionsData));
								}
								else {
									IOFBOvrDimensions *p = (IOFBOvrDimensions *)CFDataGetBytePtr(IOFBOvrDimensionsData);
									char *flagsstr1 = GetOneFlagsStr(p->setFlags);
									char *flagsstr2 = GetOneFlagsStr(p->clearFlags);
									iprintf("dims (IOFBOvrDimensions) = %dx%d setFlags:%s clearFlags:%s;\n",
										p->width, p->height, flagsstr1, flagsstr2
									);
									free(flagsstr1);
									free(flagsstr2);
									CFDictionaryRemoveValue(IOFBConfig, CFSTR("dims"));
								}
							}
						} // dims (IOFBOvrDimensions)



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
					CFArrayRef IOFBCursorInfo = CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBCursorInfoKey));
					if (IOFBCursorInfo) {
						CFMutableDictionaryRef IOFBCursorInfoNew = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL, NULL);
						CFIndex itemCount = CFArrayGetCount(IOFBCursorInfo);
						if (itemCount) {
							iprintf("IOFBCursorInfo = [\n"); INDENT
							for (SInt32 i = 0; i < itemCount; i++) {
								bool good = false;
								CFDataRef IOFBOneCursorInfo = CFArrayGetValueAtIndex(IOFBCursorInfo, i);
								if (IOFBOneCursorInfo && CFGetTypeID(IOFBOneCursorInfo) == CFDataGetTypeID()) {
									iprintf("[%d] = ", i);
									good = DumpOneCursorInfo(IOFBOneCursorInfo, i);
								}
								if (!good) {
									if (IOFBOneCursorInfo) CFRetain(IOFBOneCursorInfo);
									//CFStringRef arrayIndex = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("[%d]"), i);
									CFNumberRef arrayIndex = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &i);
									CFDictionaryAddValue(IOFBCursorInfoNew, arrayIndex, IOFBOneCursorInfo);
								}
							}
							OUTDENT iprintf("]; // IOFBCursorInfo\n");
							
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
					CFArrayRef IOFBDetailedTimings0 = CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBDetailedTimingsKey));
					if (IOFBDetailedTimings0) IOFBDetailedTimings = CFArrayCreateMutableCopy(kCFAllocatorDefault, 0, IOFBDetailedTimings0);
					if (IOFBDetailedTimings && CFArrayGetCount(IOFBDetailedTimings)) {
						iprintf("IOFBDetailedTimings = [\n"); INDENT
						int dups = 0;
						CFIndex count = CFArrayGetCount(IOFBDetailedTimings);
						for (int i = 0, j = 0; i < count; i++) {
							CFDataRef IOFBDetailedTiming = CFArrayGetValueAtIndex(IOFBDetailedTimings, j);
							if (IOFBDetailedTiming) {
								dups += DumpOneDetailedTimingInformation(IOFBDetailedTiming, i, modeAlias) ? 1 : 0;
							}
							CFArrayRemoveValueAtIndex(IOFBDetailedTimings, j);
						}
						if (dups) {
							iprintf("%d duplicates\n", dups);
						}
						OUTDENT iprintf("]; // IOFBDetailedTimings\n");
						if (CFArrayGetCount(IOFBDetailedTimings)) {
							CFDictionarySetValue(IOFBProperties, CFSTR(kIOFBDetailedTimingsKey), IOFBDetailedTimings);
						}
						else {
							CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBDetailedTimingsKey));
						}
					}
				} // IOFBDetailedTimings

				{ // IOFBProbeOptions
					num = CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBProbeOptionsKey));
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
					CFDataRef IOFBScalerInfo = CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBScalerInfoKey));
					if (IOFBScalerInfo) {
						iprintf("IOFBScalerInfo = { ");
						DumpOneDisplayScalerInfo(IOFBScalerInfo);
						cprintf(" };\n");
						CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBScalerInfoKey));
					}
				} //IOFBScalerInfo

				{ // IOFBTimingRange
					CFDataRef IOFBTimingRange = CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBTimingRangeKey));
					if (IOFBTimingRange) {
						iprintf("IOFBTimingRange = { ");
						DumpOneDisplayTimingRange(IOFBTimingRange);
						cprintf(" };\n");
						CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBTimingRangeKey));
					}
				} //IOFBTimingRange

				{ // IOFBTransform
					num = CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBTransformKey));
					if (num) {
						CFNumberGetValue(num, kCFNumberSInt32Type, &numValue);
						iprintf("IOFBTransform = ");
						DumpOneTransform(numValue);
						cprintf(";\n");
						CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBTransformKey));
					}
				} // IOFBTransform

				{ // startup-timing
					CFDataRef IOTimingInformationData = CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBStartupTimingPrefsKey));
					if (IOTimingInformationData) {
						iprintf("startup-timing = { ");
						DumpOneTimingInformation(IOTimingInformationData, modeAlias);
						cprintf(" };\n");
						CFDictionaryRemoveValue(IOFBProperties, CFSTR(kIOFBStartupTimingPrefsKey));
					}
				} // startup-timing

				{ // IOFBI2CInterfaceIDs
					CFArrayRef IOFBI2CInterfaceIDs = CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBI2CInterfaceIDsKey));
					if (IOFBI2CInterfaceIDs && CFGetTypeID(IOFBI2CInterfaceIDs) == CFArrayGetTypeID()) {
						iprintf("IOFBI2CInterfaceIDs = (\n"); INDENT
						CFIndex itemCount = CFArrayGetCount(IOFBI2CInterfaceIDs);
						for (int i = 0; i < itemCount; i++) {
							num = CFArrayGetValueAtIndex(IOFBI2CInterfaceIDs, i);
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
					CFArrayRef IOFBI2CInterfaceInfo = CFDictionaryGetValue(IOFBProperties, CFSTR(kIOFBI2CInterfaceInfoKey));
					if (IOFBI2CInterfaceInfo && CFGetTypeID(IOFBI2CInterfaceInfo) == CFArrayGetTypeID()) {
						iprintf("IOFBI2CInterfaceInfo = (\n"); INDENT
						CFIndex itemCount = CFArrayGetCount(IOFBI2CInterfaceInfo);
						for (int i = 0; i < itemCount; i++) {
							CFDictionaryRef dict = CFArrayGetValueAtIndex(IOFBI2CInterfaceInfo, i);
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
				
#define OneNumber0(_name, _format, _calc) \
				{ \
					num = CFDictionaryGetValue(IOFBProperties, CFSTR(_name)); \
					if (num && CFGetTypeID(num) == CFNumberGetTypeID()) { \
						CFNumberGetValue(num, kCFNumberSInt32Type, &numValue); \
						iprintf("%s = " _format ";\n", _name, _calc); \
						CFDictionaryRemoveValue(IOFBProperties, CFSTR(_name)); \
					} \
				}
#define OneNumber(_name, _format) OneNumber0(_name, _format, numValue)

#define OneDataNum(_name, _bits, _format) \
				{ \
					CFDataRef data = CFDictionaryGetValue(IOFBProperties, CFSTR(_name)); \
					if (data && CFGetTypeID(data) == CFDataGetTypeID()) { \
						const UInt8* thebytes = CFDataGetBytePtr(data); \
						CFIndex len = CFDataGetLength(data); \
						if (thebytes && len * 8 == _bits) { \
							iprintf("%s = " _format ";\n", _name, *((SInt ## _bits *)thebytes)); \
							CFDictionaryRemoveValue(IOFBProperties, CFSTR(_name)); \
						} \
					} \
				}

				OneNumber0("IOFBCurrentPixelClock", "%g MHz", numValue / 1000000.0) // kIOFBCurrentPixelClockKey
				OneDataNum("IOFBUIScale", 32, "%d") // kIOFBUIScaleKey
				DoAllBooleans(IOFBProperties);
				OneDataNum("ATY,fb_linebytes", 32, "%d")
				OneDataNum("ATY,fb_offset", 64, "0x%llx")
				OneDataNum("ATY,fb_size", 64, "%lld")
				OneDataNum("ATY,fb_maxshrink", 16, "%hd")
				OneDataNum("ATY,fb_maxstretch", 16, "%hd")
				OneNumber("ATY,fb_minVTotal", "%d")
				OneNumber("ATY,fb_maxVTotal", "%d")

				iprintf("IOFramebuffer properties (unparsed) = ");
				CFOutput(IOFBProperties);
				cprintf("; // IOFramebuffer properties (unparsed)\n");

				CFRelease(IOFBProperties);
			}
		}
	} // IOFramebuffer
	OUTDENT iprintf("}; // IOFramebuffer\n\n");

	io_service_t ioDisplayService;
	if( (ioDisplayService = IODisplayForFramebuffer( ioFramebufferService, kNilOptions)))
	{
		iprintf("IODisplay 0x%0x = {\n", ioDisplayService); INDENT
		CFMutableDictionaryRef IODProperties = NULL;
		if (!IORegistryEntryCreateCFProperties(ioDisplayService, &IODProperties, kCFAllocatorDefault, kNilOptions)) {
			if (IODProperties) {

				DumpDisplayInfo(IODProperties);

				{ // AppleDisplayType
					num = CFDictionaryGetValue(IODProperties, CFSTR(kAppleDisplayTypeKey));
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
					num = CFDictionaryGetValue(IODProperties, CFSTR(kAppleSenseKey));
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
		
		OUTDENT iprintf("}; // IODisplay\n\n");
		IOObjectRelease(ioDisplayService);
	} // IODisplay

	{ // IOFramebufferOpen
		iprintf("IOFramebufferOpen = {\n"); INDENT
		task_port_t selfTask = mach_task_self();
		io_connect_t ioFramebufferConnect;
		IOFramebufferOpen(ioFramebufferService, selfTask, kIOFBSharedConnectType, &ioFramebufferConnect); //
		if (ioFramebufferConnect) {
			/*
			IOFBConnectRef connectRef = IOFBConnectToRef( ioFramebufferConnect );
			if (connectRef) {
				// We can't get IOFBConnectRef unless we are WindowServer which does kIOFBServerConnectType to spawn IOFramebufferUserClient instead of kIOFBSharedConnectType to IOFramebufferUserClient
			}
			*/

			{ // StdFBShmem_t
				StdFBShmem_t *fbshmem;
				mach_vm_size_t size;

				err = IOConnectMapMemory(
					ioFramebufferConnect,
					kIOFBCursorMemory,
					selfTask,
					(mach_vm_address_t*)&fbshmem,
					&size,
					kIOMapAnywhere | kIOMapDefaultCache | kIOMapReadOnly
				);
				if (KERN_SUCCESS == err) {
					iprintf("kIOFBCursorMemory = { size:%lld version:%d location:%dx%d };\n", size, fbshmem->version, fbshmem->cursorLoc.x, fbshmem->cursorLoc.y);
					IOConnectUnmapMemory(ioFramebufferConnect, kIOFBCursorMemory, selfTask, (mach_vm_address_t)fbshmem);
				} // if IOConnectMapMemory

				err = IOConnectMapMemory(
					ioFramebufferConnect,
					kIOFBVRAMMemory,
					selfTask,
					(mach_vm_address_t*)&fbshmem,
					&size,
					kIOMapAnywhere | kIOMapDefaultCache | kIOMapReadOnly
				);
				if (KERN_SUCCESS == err) {
					iprintf("kIOFBVRAMMemory = { size:%gMB };\n", size / (1024 * 1024.0));
					IOConnectUnmapMemory(ioFramebufferConnect, kIOFBCursorMemory, selfTask, (mach_vm_address_t)fbshmem);
				} // if IOConnectMapMemory
			} // StdFBShmem_t
			
			IOServiceClose(ioFramebufferConnect);
		}
		OUTDENT iprintf("}; // IOFramebufferOpen\n\n");
	} // IOFramebufferOpen
	
	{ // I2C
		IOItemCount i2cInterfaceCount;
		IOReturn result;
		result = IOFBGetI2CInterfaceCount(ioFramebufferService, &i2cInterfaceCount);
		if (KERN_SUCCESS == result) {
			iprintf("I2C Interfaces = {\n"); INDENT
			for (int interfaceBus = 0; interfaceBus < i2cInterfaceCount; interfaceBus++) {
				io_service_t i2cservice;
				iprintf("[%d] = {\n", interfaceBus); INDENT
				result = IOFBCopyI2CInterfaceForBus(ioFramebufferService, interfaceBus, &i2cservice);
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
						onenum(32, IOI2CBusType , "%d")
						onenum(32, IOI2CTransactionTypes , "0x%x")
						onenum(32, IOI2CSupportedCommFlags , "0x%x")

						IOI2CConnectRef i2cconnect;
						result = IOI2CInterfaceOpen(i2cservice, kNilOptions, &i2cconnect);
						if (KERN_SUCCESS == result) {
							IOI2CRequest request;

							/*
								I2C Slave Address Pair/Address
										Specification
								74h/75h RESERVED for HDCP (Primary Link Port)
								76h/77h RESERVED for HDCP (Secondary Link Port)
								80h/81h RESERVED for DisplayPort (Dual-Mode Video Adapter)
								A0h/A1h EDID
								A4h/A5h DisplayID
								A8h/A9h RESERVED for HDMI
								60h     E-DDC Segment Pointer (see Section 2.2.5)
								6Eh/6Fh RESERVED for DDC/CI communication (e.g., MCCS)
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
									request.replyBytes = sizeof(data);
									request.replyBuffer = (vm_address_t)data;
									result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
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
											cprintf("(request error:%x)", request.result);
										}
									}
									else {
										cprintf("(IOI2CSendRequest error:%x)", result);
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

											result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
											if (result) {
												cprintf("(segment IOI2CSendRequest error:%x)", result);
											}
											if (request.result) {
												cprintf("(segment request error:%x)", request.result);
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
											request.replyBytes = sizeof(data);
											request.replyBuffer = (vm_address_t)data;
											result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
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
												cprintf("(request error:%x)", request.result);
											}
										}
										else {
											cprintf("(IOI2CSendRequest error:%x)", result);
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
											 
												Tested in Montery.
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
											result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
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
											result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
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
												cprintf("(request error:%x)", request.result);
											}
										}
										else {
											cprintf("(IOI2CSendRequest error:%x)", result);
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
										address, // Source address
										0x81, // Length
										0xf1, // Identification request op code
										0x00, // Checksum
									};
									request.sendBytes = sizeof(senddata);
									request.sendBuffer = (vm_address_t)senddata;
									ddcsetchecksum(&request);

									if (FORCEI2C || !(val_IOI2CTransactionTypes & (1 << kIOI2CDDCciReplyTransactionType))) {
										result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
										usleep(kDelayDDCIdentificationRequest);
										if (result) {
											iprintf("(IOI2CSendRequest error:%x)\n", result);
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
									request.replyBytes = sizeof(identificationBytes);
									request.replyBuffer = (vm_address_t)identificationBytes;
									result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
									usleep(kDelayDDCIdentificationReply);

									if (result) {
										iprintf("(IOI2CSendRequest error:%x)\n", result);
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
									request.sendBytes = sizeof(senddata);
									request.sendBuffer = (vm_address_t)senddata;
									ddcsetchecksum(&request);

									if (FORCEI2C || !(val_IOI2CTransactionTypes & (1 << kIOI2CDDCciReplyTransactionType))) {
										result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
										usleep(kDelayDDCTimingRequest);
										if (result) {
											cprintf("(IOI2CSendRequest error:%x) ", result);
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
									request.replyBytes = sizeof(timingReportBytes);
									request.replyBuffer = (vm_address_t)timingReportBytes;
									result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
									usleep(kDelayDDCTimingReply);

									if (result) {
										cprintf("(IOI2CSendRequest error:%x) ", result);
										break;
									}
									if (!ddcreplyisgood(&request, false, 0x6e, timingReportBytes, sizeof(timingReportBytes)) || (timingReportBytes[1] & 0x7f) != 0x06 || timingReportBytes[2] != 0x4e) {
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
								char *vcpCapabilitiesBytes = malloc(vcpCapabilitiesMaxSize);
								
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
											vcpCapabilitiesOffset >> 8, // Offset value High byte
											vcpCapabilitiesOffset & 0xff, // Offset value Low byte
											0x00, // Checksum
										};
										request.sendBytes = sizeof(senddata);
										request.sendBuffer = (vm_address_t)senddata;
										ddcsetchecksum(&request);

										if (FORCEI2C || !(val_IOI2CTransactionTypes & (1 << kIOI2CDDCciReplyTransactionType))) {
											result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
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
										request.replyBytes = sizeof(chunkdata);
										request.replyBuffer = (vm_address_t)chunkdata;
										result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
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
										iprintf("(IOI2CSendRequest error:%x)\n", result);
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
										vcpCapabilitiesBytes = realloc(vcpCapabilitiesBytes, vcpCapabilitiesMaxSize);
									}
									memcpy(vcpCapabilitiesBytes + vcpCapabilitiesOffset, chunkdata + 5, chunksize);
									vcpCapabilitiesOffset += chunksize;
								} // while vcp string chunksize
								
								if (vcpCapabilitiesOffset) {
									if (vcpCapabilitiesBytes[vcpCapabilitiesOffset-1] != 0) {
										iprintf("(last byte at offset %d is not null)\n", vcpCapabilitiesOffset - 1);
										if (vcpCapabilitiesOffset + 1 > vcpCapabilitiesMaxSize) {
											vcpCapabilitiesMaxSize += 1;
											vcpCapabilitiesBytes = realloc(vcpCapabilitiesBytes, vcpCapabilitiesMaxSize);
										}
										vcpCapabilitiesBytes[vcpCapabilitiesOffset] = 0;
										vcpCapabilitiesOffset++;
									}
									if (strnlen(vcpCapabilitiesBytes, vcpCapabilitiesOffset) != vcpCapabilitiesOffset - 1) {
										iprintf("(expected string length %d)\n", vcpCapabilitiesOffset - 1);
										iprintf("VCP bytes = ");
										for (int i = 0; i < vcpCapabilitiesOffset; i++) {
											cprintf("%02x", vcpCapabilitiesBytes[i]);
										}
										cprintf(";\n");
									}
									iprintf("VCP string = \"%s\";\n", vcpCapabilitiesBytes);
									parsevcp(0, vcpCapabilitiesBytes, i2cconnect, val_IOI2CTransactionTypes);
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
											blockNdx, // Offset value High byte
											chunkNdx * 32, // Offset value Low byte
											0x00, // Checksum
										};
										//senddata[1] += sizeof(data) - 2;
										ddcsetchecksum(&request);
										request.sendBytes = sizeof(senddata);
										request.sendBuffer = (vm_address_t)senddata;

										if (FORCEI2C || !(val_IOI2CTransactionTypes & (1 << kIOI2CDDCciReplyTransactionType))) {
											result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
											usleep(kDelayMCCSEDIDRequest);
											if (result) {
												iprintf("(IOI2CSendRequest error:%x)\n", result);
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
										request.replyBytes = sizeof(chunkdata);
										request.replyBuffer = (vm_address_t)chunkdata;
										result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
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
											cprintf("(request error:%x)", request.result);
										}
									}
									else {
										cprintf("(IOI2CSendRequest error:%x)", result);
									}
									cprintf(" }\n");
								} // for blockNdx
								OUTDENT iprintf("};\n\n");
							} // EDID from DDC/CI MCCS

							
							if (val_IOI2CTransactionTypes & (1 << kIOI2CDisplayPortNativeTransactionType))
							{ // DisplayPort
								iprintf("DisplayPort = {\n"); INDENT
								UInt8 path[16];
								DoOneDisplayPort(i2cconnect, path, 0);
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
			OUTDENT iprintf("}; // I2C Interfaces\n\n");
		}
	} // I2C
} // DumpDisplayService


char * GetServicePath(io_service_t device)
{
	// for an ioregistry device, return an ioregistry path string
	char *ioregPath = malloc(4096);
	ioregPath[0] = 0;

	char temp[4096];
	kern_return_t kr;
	io_service_t leaf_device = device;
	
	while (device) {
		io_iterator_t parentIterator = 0;
		if (IOObjectConformsTo(device, "IOPlatformExpertDevice"))
		{
			sprintf(temp, "%s%s", ioregPath[0] ? "/" : "", ioregPath);
			strcpy(ioregPath, temp);
			kr = KERN_ABORTED; // no more parents
		}
		else
		{
			io_name_t name;
			io_name_t locationInPlane;
			const char *deviceLocation = NULL, *functionLocation = NULL;
			unsigned int deviceInt = 0, functionInt = 0;
			int len;
			name[0] = '\0';
			
			IORegistryEntryGetName(device, name);
			if (IORegistryEntryGetLocationInPlane(device, kIOServicePlane, locationInPlane) == KERN_SUCCESS) {
				len = sprintf(temp, "%s@%s", name, locationInPlane);
				deviceLocation = strtok(locationInPlane, ",");
				functionLocation = strtok(NULL, ",");
				if (deviceLocation != NULL) deviceInt = (unsigned int)strtol(deviceLocation, NULL, 16);
				if (functionLocation != NULL) functionInt = (unsigned int)strtol(functionLocation, NULL, 16);
			}
			else {
				len = sprintf(temp, "%s", name);
			}
			sprintf(temp + len, "%s%s", ioregPath[0] ? "/" : "", ioregPath);
			strcpy(ioregPath, temp);

			kr = IORegistryEntryGetParentIterator(device, kIOServicePlane, &parentIterator);
		} // !IOPlatformExpertDevice
	
		if (device != leaf_device) IOObjectRelease(device);
		if (kr != KERN_SUCCESS)
			break;
		device = IOIteratorNext(parentIterator);
	} // while device
	return ioregPath;
} // GetServicePath


void DumpAllDisplaysInfo (void) {
	CGDirectDisplayID onlineDisplays[20];
	io_service_t onlineServices[20];
	uint32_t nModes[20]; // number of modes for each display
	CGSDisplayModeDescription * modes[20]; // all the modes for each display
	int modeAlias[20];
	
	uint32_t displayCount;
	CGGetOnlineDisplayList(20, onlineDisplays, &displayCount);
	cprintf("%d Online Monitors found\n", displayCount);
	//CGSConnectionID connectionID = CGSMainConnectionID();
	CGDirectDisplayID mainDisplay = CGMainDisplayID();
	cprintf("Main display: 0x%x\n", mainDisplay);

	cprintf("-----------------------------------------------------\n");
	iprintf("DISPLAYS = [\n"); INDENT
	for (uint32_t displayIndex = 0; displayIndex < displayCount; displayIndex++) {
		CGDirectDisplayID display = onlineDisplays[displayIndex];

		CGSGetNumberOfDisplayModes(display, &nModes[displayIndex]);
		modes[displayIndex] = malloc(nModes[displayIndex] * sizeof(CGSDisplayModeDescription));
		if (!modes[displayIndex]) {
			iprintf("Error: Not enough memory for %d modes\n", nModes[displayIndex]);
			nModes[displayIndex] = 0;
		}

		int aliasCounts[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		for (int i = 0; i < nModes[displayIndex]; i++) {
			const int size = sizeof (CGSDisplayModeDescription);
			CGError result = CGSGetDisplayModeDescriptionOfLength(display, i, &modes[displayIndex][i], size);
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
		iprintf("serialNumber = %d;\n", CGDisplaySerialNumber(display)); // A serial number for the monitor associated with the specified display, or a constant to indicate an exception—see the discussion below.
		iprintf("unitNumber = %d;\n", CGDisplayUnitNumber(display)); // A logical unit number for the specified display.
		iprintf("IOService = 0x%x;\n", CGDisplayIOServicePort(display)); // Return the IOKit service port of a display.
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
		iprintf("Rotation = %g°;\n", CGDisplayRotation(display)); // The rotation angle of the display in degrees, or 0 if the display is not valid.
		iprintf("Bounds (l,t,w,h) = { %g, %g, %g, %g };\n", bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height);
		CGSize screenSize = CGDisplayScreenSize(display); // The size of the specified display in millimeters, or 0 if the display is not valid.
		iprintf("Size = %g x %g mm;\n", screenSize.width, screenSize.height);
		
		float brightness = 1;
		int err;
		err = DisplayServicesGetBrightness(display, &brightness);
		iprintf("Brightness = %g (err:%d);\n", brightness, err);

		if (SLSIsDisplayModeVRR)         iprintf("IsModeVRR = %d;\n", SLSIsDisplayModeVRR(display));

		bool HDRSupportedSLS = false;
		bool HDRSupportedCGS = false;
		bool HDREnabledSLS = false;
		bool HDREnabledCGS = false;

		if (SLSDisplaySupportsHDRMode)   iprintf("SupportsHDR(SLS) = %d;\n", HDRSupportedSLS = SLSDisplaySupportsHDRMode(display));
		if (CGSIsHDRSupported        )   iprintf("SupportsHDR(CGS) = %d;\n", HDRSupportedCGS =   CGSIsHDRSupported      (display));

		if (HDRSupportedSLS || HDRSupportedCGS) {
			if (SLSDisplayIsHDRModeEnabled)  iprintf("HDREnabled(SLS) = %d;\n", HDREnabledSLS = SLSDisplayIsHDRModeEnabled(display));
			if (CGSIsHDREnabled           )  iprintf("HDREnabled(CGS) = %d;\n", HDREnabledCGS =            CGSIsHDREnabled(display)); // returns 1 even for displays that don't support HDR. Maybe it's 1 if any display supports HDR?
#if 0
			CGDisplayConfigRef config;
			result = CGBeginDisplayConfiguration(&config);
			if (!result) {
				//iprintf("SetBrightness to 0 (err: %d)\n", err = DisplayServicesSetBrightness(display, 0.0)); // we don't need to set brightness to change HRD setting.
				if (SLSDisplaySetHDRModeEnabled) iprintf("ToggleHDR(SLS) = %x;\n", SLSDisplaySetHDRModeEnabled(display, HDREnabledSLS ? 0 : 1, 0, 0));
				else if (CGSEnableHDR          ) iprintf("ToggleHDR(CGS) = %x;\n",     CGSEnableHDR           (display, HDREnabledCGS ? 0 : 1, 0, 0)); // I don't think CGSEnableHDR works - needs more investigation to find out how Catalina enables HDR.
				CGCompleteDisplayConfiguration(config, kCGConfigureForSession);
				//iprintf("SetBrightness to %g (err: %d)\n", brightness, err = DisplayServicesSetBrightness(display, brightness)); // we don't need to set brightness to change HRD setting.
				if (SLSDisplayIsHDRModeEnabled)  iprintf("HDREnabled(SLS) = %d;\n", HDREnabledSLS = SLSDisplayIsHDRModeEnabled(display));
				if (CGSIsHDREnabled           )  iprintf("HDREnabled(CGS) = %d;\n", HDREnabledCGS =            CGSIsHDREnabled(display)); // returns 1 even for displays that don't support HDR. Maybe it's 1 if any display supports HDR?
			}
#endif
		}

		iprintf("ColorSpace = { ");
		CGColorSpaceRef space = CGDisplayCopyColorSpace(display);
		CFOutput(space);
		CGColorSpaceRelease(space);
		cprintf(" };\n");
		
		OUTDENT iprintf("}, // Monitor[%d]\n", displayIndex);
	}
	OUTDENT iprintf("]; // DISPLAYS\n");

	cprintf("-----------------------------------------------------\n");
	iprintf("IOSERVICE = [\n"); INDENT
	for (uint32_t displayIndex = 0; displayIndex < displayCount; displayIndex++) {
		CGDirectDisplayID display = onlineDisplays[displayIndex];
		char *servicePath = NULL;
		CGError result = CGSServiceForDisplayNumber(display, &onlineServices[displayIndex]);
		if(result == kCGErrorSuccess) {
			servicePath = GetServicePath(onlineServices[displayIndex]);
		}
		iprintf("Monitor[%d]: %s%s= {\n", displayIndex, servicePath ? servicePath : "", servicePath ? " " : ""); INDENT
		if(result == kCGErrorSuccess) {
			DumpDisplayService(onlineServices[displayIndex], modeAlias[displayIndex]);
		}
		OUTDENT iprintf("}, // Monitor[%d]: %s\n\n", displayIndex, servicePath ? servicePath : "");
		if (servicePath) free (servicePath);
	}

	char *serviceTypes[] = { "IOFramebuffer", "IOMobileFramebuffer", "DCPAVServiceProxy", "DCPDPServiceProxy" };
	for (int serviceTypeNdx = 0; serviceTypeNdx < sizeof(serviceTypes) / sizeof(serviceTypes[0]); serviceTypeNdx++)
	{
		io_iterator_t iterator;
		kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching(serviceTypes[serviceTypeNdx]), &iterator);
		if (kr == KERN_SUCCESS) {
			for (io_service_t device; IOIteratorIsValid(iterator) && (device = IOIteratorNext(iterator)); IOObjectRelease(device))
			{
				bool found = false;
				for (uint32_t displayIndex = 0; !found && displayIndex < displayCount; displayIndex++) {
					if (onlineServices[displayIndex] == device) {
						found = true;
					}
				}
				if (!found) {
					char *servicePath = GetServicePath(device);
					iprintf("%s: %s%s= {\n", serviceTypes[serviceTypeNdx], servicePath ? servicePath : "", servicePath ? " " : ""); INDENT
					DumpDisplayService(device, 0);
					OUTDENT iprintf("}, // %s: %s\n\n", serviceTypes[serviceTypeNdx], servicePath ? servicePath : "");
					if (servicePath) free (servicePath);
				}
			}
			IOObjectRelease(iterator);
		}
	}
	
	OUTDENT iprintf("]; // IOSERVICE\n");

	cprintf("-----------------------------------------------------\n");
	iprintf("CURRENT MODE = [\n"); INDENT
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

		{
			CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(display);
			iprintf("current mode by CGDisplayCopyDisplayMode               %s= { ", modeNum < 10 ? "" : modeNum < 100 ? " " : modeNum < 1000 ? "  " : "   ");
			DumpOneCGDisplayMode(currentMode, modeAlias[displayIndex]);
			cprintf(" };\n");
			CGDisplayModeRelease(currentMode);
		}

		{
			CFDictionaryRef modeDictionary = CGDisplayCurrentMode(display); // CGDisplayCurrentMode returns NULL when HDR is enabled
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
	OUTDENT iprintf("]; // CURRENT MODE\n");

	cprintf("-----------------------------------------------------\n");
	iprintf("ALL MODES = [\n"); INDENT
	for (uint32_t displayIndex = 0; displayIndex < displayCount; displayIndex++) {
		CGDirectDisplayID display = onlineDisplays[displayIndex];
		iprintf("Monitor[%d] = [\n", displayIndex); INDENT

		{ // CGDisplayAvailableModes/CGDisplayCurrentMode
			/*
				CGDisplayAvailableModes/CGDisplayCurrentMode will include a set of modes for 8bpc framebuffer or 10bpc but not both.
				bpc is 8bpc even when doing 10bpc framebuffer or HDR.
				Usually the only difference between 8bpc and 10bpc modes is the mode number. It is even for 8bpc. The 10bpc mode number is 8bpc mode number plus 1.
			*/
			CFArrayRef availableModes = CGDisplayAvailableModes(display);
			iprintf("CGDisplayAvailableModes (deprecated) (%d modes) = [\n", (int)CFArrayGetCount(availableModes)); INDENT
			for ( int i = 0; i < CFArrayGetCount( availableModes ); i++ ) {
				CGDisplayModeRef displayMode = (CGDisplayModeRef) CFArrayGetValueAtIndex( availableModes, i );
				iprintf("{ ");
				DumpOneCGDisplayMode(displayMode, modeAlias[displayIndex]);
				cprintf(" },\n");
			}
			OUTDENT iprintf("]; // CGDisplayAvailableModes\n");
		} // CGDisplayAvailableModes
		
		{ // CGDisplayCopyAllDisplayModes
			/*
				CGDisplayCopyAllDisplayModes/CGDisplayCopyDisplayMode is same as CGDisplayAvailableModes/CGDisplayCurrentMode above but includes the following 7 fields and there is double the modes - one for 8bpc framebuffer and another for 10bpc

					kCGDisplayModeIsTelevisionOutput = 0; // actually, this flag is included for CGDisplayAvailableModes/CGDisplayCurrentMode in Montery but not included in Catalina
					kCGDisplayModeIsInterlaced = 0;
					kCGDisplayModeIsStretched = 0;
					kCGDisplayModeIsUnavailable = 0;
					kCGDisplayModeSuitableForUI = 1;
					DepthFormat = 4;                                      DepthFormat = 8;
					PixelEncoding = "--------RRRRRRRRGGGGGGGGBBBBBBBB";   PixelEncoding = "--RRRRRRRRRRGGGGGGGGGGBBBBBBBBBB";

				The double modes have 8bpc first (even) and 10bpc second (odd) with a mode number 1 greater than the 8bpc mode number like this:
					Mode = 0;                                             Mode = 1;
			*/

			CFStringRef keys[1] = { kCGDisplayShowDuplicateLowResolutionModes };
			CFBooleanRef values[1] = { kCFBooleanTrue };			// include HiDPI modes (kCGDisplayResolution > 1) and include multiple bit depths like 10bpc instead of just 8bpc)
			CFDictionaryRef options = CFDictionaryCreate(kCFAllocatorDefault, (const void**) keys, (const void**) values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
			CFArrayRef availableModes = CGDisplayCopyAllDisplayModes(display, options);

			iprintf("CGDisplayCopyAllDisplayModes (%d modes) = [\n", (int)CFArrayGetCount(availableModes)); INDENT

			//CFOutput(availableModes); // print them one at a time
			for ( int i = 0; i < CFArrayGetCount( availableModes ); i++ ) {
				CGDisplayModeRef displayMode = (CGDisplayModeRef) CFArrayGetValueAtIndex( availableModes, i );
				iprintf("{ ");
				DumpOneCGDisplayMode(displayMode, modeAlias[displayIndex]);
				cprintf(" },\n");
			}

			CFRelease( options );
			CFRelease( availableModes );
			OUTDENT iprintf("]; // CGDisplayCopyAllDisplayModes\n");
		} // CGDisplayCopyAllDisplayModes

		{ // CGSGetNumberOfDisplayModes
			/*
				CGSGetDisplayModeDescriptionOfLength is similar to CGDisplayCopyAllDisplayModes but includes some other fields.
				It adds modes that have the 0x200000 = kDisplayModeValidForMirroringFlag or the 0x01000000 = kDisplayModeValidForAirPlayFlag IOFlag set.
			*/
			iprintf("CGSGetDisplayModeDescriptionOfLength (%d modes) = [\n", nModes[displayIndex]); INDENT
			for (int i = 0; i < nModes[displayIndex]; i++) {
				iprintf("{ ");
				DumpOneDisplayModeDescription(&modes[displayIndex][i], modeAlias[displayIndex]);
				cprintf(" },\n");
			}
			OUTDENT iprintf("]; // CGSGetDisplayModeDescriptionOfLength\n");
		} // CGSGetNumberOfDisplayModes

		OUTDENT iprintf("], // Monitor[%d]\n", displayIndex);
	} // for displayIndex
	OUTDENT iprintf("]; // ALL MODES\n");
}

int main(int argc, const char * argv[]) {
	@autoreleasepool {
		DumpAllDisplaysInfo();
	} // @autoreleasepool
	return 0;
} // main
