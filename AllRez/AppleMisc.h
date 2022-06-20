//
//  AppleMisc.h
//  AllRez
//
//  Created by Joe van Tunen on 2022-05-19.
//

#ifndef AppleMisc_h
#define AppleMisc_h

#include <IOKit/graphics/IOGraphicsTypes.h>
#include <CoreGraphics/CGDisplayConfiguration.h>
#include <IOKit/graphics/IOGraphicsTypesPrivate.h>


#define kConnectionUnderscan 'pscn'
#define min(x,y) (((x)<(y)) ? (x) : (y))


// https://github.com/apple-oss-distributions/IOGraphics/blob/main/IOGraphicsFamily/IOKit/graphics/IODisplay.h
// 101
enum {
	kIODisplayNumPowerStates = 4,
	kIODisplayMaxPowerState  = kIODisplayNumPowerStates - 1
};

// https://github.com/apple-oss-distributions/IOGraphics/blob/main/IOGraphicsFamily/IOKit/graphics/IOFramebuffer.h
// 113
// IOFramebufferNotificationHandler events
enum {
	kIOFBNotifyDisplayModeWillChange    = 1,
	kIOFBNotifyDisplayModeDidChange     = 2,

	kIOFBNotifyWillSleep        = 3,
	kIOFBNotifyDidWake          = 4,

	kIOFBNotifyDidPowerOff      = 5,
	kIOFBNotifyWillPowerOn      = 6,

	kIOFBNotifyDidSleep         = kIOFBNotifyDidPowerOff,
	kIOFBNotifyWillWake         = kIOFBNotifyWillPowerOn,

	kIOFBNotifyWillPowerOff     = 7,
	kIOFBNotifyDidPowerOn       = 8,

	kIOFBNotifyWillChangeSpeed  = 9,
	kIOFBNotifyDidChangeSpeed   = 10,

	kIOFBNotifyHDACodecWillPowerOn  = 11,   // since IOGRAPHICSTYPES_REV 68
	kIOFBNotifyHDACodecDidPowerOn   = 12,   // since IOGRAPHICSTYPES_REV 68

	kIOFBNotifyHDACodecWillPowerOff = 13,   // since IOGRAPHICSTYPES_REV 68
	kIOFBNotifyHDACodecDidPowerOff  = 14,   // since IOGRAPHICSTYPES_REV 68

	kIOFBNotifyClamshellChange  = 20,

	kIOFBNotifyCaptureChange    = 30,

	kIOFBNotifyOnlineChange     = 40,

	kIOFBNotifyDisplayDimsChange = 50,

	kIOFBNotifyProbed           = 60,

	kIOFBNotifyVRAMReady        = 70,

	kIOFBNotifyWillNotify       = 80,
	kIOFBNotifyDidNotify        = 81,

	// <rdar://problem/32063590> IOGraphics needs to send WSAAWillExitDefer and WSAADidExitDefer instead of single message
	kIOFBNotifyWSAAWillEnterDefer   = 90,   // since IOGRAPHICSTYPES_REV 53
	kIOFBNotifyWSAAWillExitDefer    = 91,   // since IOGRAPHICSTYPES_REV 53

	kIOFBNotifyWSAADidEnterDefer    = 92,   // since IOGRAPHICSTYPES_REV 53
	kIOFBNotifyWSAADidExitDefer     = 93,   // since IOGRAPHICSTYPES_REV 53

	kIOFBNotifyWSAAEnterDefer       = kIOFBNotifyWSAAWillEnterDefer,
	kIOFBNotifyWSAAExitDefer        = kIOFBNotifyWSAAWillExitDefer,

	kIOFBNotifyTerminated       = 100, // since IOGRAPHICSTYPES_REV 49
};

// https://github.com/apple-oss-distributions/IOGraphics/blob/main/IOGraphicsFamily/IOFramebuffer.cpp

// 153
enum
{
	kIOFBEventCaptureSetting     = 0x00000001,
	kIOFBEventDisplayDimsSetting = 0x00000002,
	kIOFBEventReadClamshell	 	 = 0x00000004,
	kIOFBEventResetClamshell	 = 0x00000008,
	kIOFBEventEnableClamshell    = 0x00000010,
	kIOFBEventProbeAll			 = 0x00000020,
	kIOFBEventDisplaysPowerState = 0x00000040,
	kIOFBEventSystemPowerOn      = 0x00000080,
	kIOFBEventVBLMultiplier      = 0x00000100,
};

// 167
enum
{
	fg    = 1,
	bg    = 2,
	fgOff = 3,
	bgOff = 4,
};

// 1309
// IOFBController computed states
enum {
	kIOFBDidWork 			 = 0x00000001,
    kIOFBWorking             = 0x00000002,
	kIOFBPaging   			 = 0x00000004,
	kIOFBWsWait   			 = 0x00000008,
	kIOFBDimmed   			 = 0x00000010,
	kIOFBServerSentPowerOff  = 0x00000020,	// any fb ws notified asleep
	kIOFBServerAckedPowerOn  = 0x00000040,	// any fb ws state awake
	kIOFBServerAckedPowerOff = 0x00000080,	// any fb ws state asleep
	kIOFBCaptured 			 = 0x00000100,
	kIOFBDimDisable 		 = 0x00000200,
	kIOFBDisplaysChanging 	 = 0x00001000,
};

// 1324
// IOFBController work states for fDidWork and fAsyncWork
enum {
	kWorkStateChange = 0x00000001,
	kWorkPower       = 0x00000002,
	kWorkSuspend     = 0x00000004,
};

// 8152
enum
{
	gMux_Message                = 'gMUX',
	gMux_WillSwitch             = 0,
	gMux_DidSwitch              = 1,
	gMux_DidNotSwitch           = 2,
};

// 9849
#define kTempAttribute	'thrm'

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

	// https://github.com/apple-oss-distributions/IOKitUser/blob/rel/IOKitUser-1445/graphics.subproj/IOGraphicsLib.c
	kIOFBSWOfflineDisplayModeID = (IODisplayModeID) 0xffffff00,
	kArbModeIDSeedMask           = (IODisplayModeID) 0x00007000, //  (connectRef->arbModeIDSeed is incremented everytime IOFBBuildModeList is called) // https://github.com/apple-oss-distributions/IOKitUser/blob/rel/IOKitUser-1445/graphics.subproj/IOGraphicsLibInternal.h
};


enum // 244
{
	// values for graphic-options & kIOMirrorDefaultAttribute
//  kIOMirrorDefault        = 0x00000001,
//  kIOMirrorForced         = 0x00000002,
//	kIOGPlatformYCbCr       = 0x00000004,
	kIOFBDesktopModeAllowed = 0x00000008,   // https://github.com/apple-oss-distributions/IOGraphics/blob/rel/IOGraphics-305/IOGraphicsFamily/IOFramebuffer.cpp // gIOFBDesktopModeAllowed
//  kIOMirrorHint           = 0x00010000,
//	kIOMirrorNoAutoHDMI     = 0x00000010,
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

//#include <IOKit/ndrvsupport/IONDRVLibraries.h>
//#include <CarbonCore/MacErrors.h>

/* NameRegistry error codes */
enum {
	nrLockedErr                         = -2536,
	nrNotEnoughMemoryErr                = -2537,
	nrInvalidNodeErr                    = -2538,
	nrNotFoundErr                       = -2539,
	nrNotCreatedErr                     = -2540,
	nrNameErr                           = -2541,
	nrNotSlotDeviceErr                  = -2542,
	nrDataTruncatedErr                  = -2543,
	nrPowerErr                          = -2544,
	nrPowerSwitchAbortErr               = -2545,
	nrTypeMismatchErr                   = -2546,
	nrNotModifiedErr                    = -2547,
	nrOverrunErr                        = -2548,
	nrResultCodeBase                    = -2549,
	nrPathNotFound                      = -2550,    /* a path component lookup failed */
	nrPathBufferTooSmall                = -2551,    /* buffer for path is too small */
	nrInvalidEntryIterationOp           = -2552,    /* invalid entry iteration operation */
	nrPropertyAlreadyExists             = -2553,    /* property already exists */
	nrIterationDone                     = -2554,    /* iteration operation is done */
	nrExitedIteratorScope               = -2555,    /* outer scope of iterator was exited */
	nrTransactionAborted                = -2556,        /* transaction was aborted */

	gestaltUndefSelectorErr             = -5551 /*undefined selector was passed to Gestalt*/
};

//#include <IOKit/ndrvsupport/IONDRVLibraries.h>
//#include <CarbonCore/MacErrors.h>

enum {
  paramErr                      = -50,  /*error in user parameter list*/
  noHardwareErr                 = -200, /*Sound Manager Error Returns*/
  notEnoughHardwareErr          = -201, /*Sound Manager Error Returns*/
  userCanceledErr               = -128,
  qErr                          = -1,   /*queue element not found during deletion*/
  vTypErr                       = -2,   /*invalid queue element*/
  corErr                        = -3,   /*core routine number out of range*/
  unimpErr                      = -4,   /*unimplemented core routine*/
  SlpTypeErr                    = -5,   /*invalid queue element*/
  seNoDB                        = -8,   /*no debugger installed to handle debugger command*/
  controlErr                    = -17,  /*I/O System Errors*/
  statusErr                     = -18,  /*I/O System Errors*/
  readErr                       = -19,  /*I/O System Errors*/
  writErr                       = -20,  /*I/O System Errors*/
  badUnitErr                    = -21,  /*I/O System Errors*/
  unitEmptyErr                  = -22,  /*I/O System Errors*/
  openErr                       = -23,  /*I/O System Errors*/
  closErr                       = -24,  /*I/O System Errors*/
  dRemovErr                     = -25,  /*tried to remove an open driver*/
  dInstErr                      = -26,   /*DrvrInstall couldn't find driver in resources*/

  badCksmErr                    = -69,  /*addr mark checksum didn't check*/
};

#ifdef __cplusplus
extern "C" {
#endif

// CoreGraphics private APIs with support for scaled (retina) display modes
CGError CGSGetCurrentDisplayMode(CGDirectDisplayID display, uint32_t* modeNum);
CGError CGSConfigureDisplayMode(CGDisplayConfigRef config, CGDirectDisplayID display, uint32_t modeNum);
CGError CGSGetNumberOfDisplayModes(CGDirectDisplayID display, uint32_t* nModes);
CGError CGSGetDisplayModeDescriptionOfLength(CGDirectDisplayID display, int idx, CGSDisplayModeDescription* mode, int length);
CGError CGSServiceForDisplayNumber(CGDirectDisplayID display, io_service_t *service);
CGError CGSDisplayDeviceForDisplayNumber(CGDirectDisplayID display, io_service_t *service);

bool SLSIsDisplayModeVRR(CGDirectDisplayID display, uint32_t modeNum) __attribute__((weak_import));
bool SLSIsDisplayModeProMotion(CGDirectDisplayID display, uint32_t modeNum) __attribute__((weak_import));
UInt32 SLSGetDisplayModeMinRefreshRate(CGDirectDisplayID display, uint32_t modeNum, float *minRefreshRate) __attribute__((weak_import));

CGError SLSDisplaySetHDRModeEnabled(CGDirectDisplayID display, bool enable, int, int)  __attribute__((weak_import));
bool SLSDisplayIsHDRModeEnabled(CGDirectDisplayID display) __attribute__((weak_import));
bool SLSDisplaySupportsHDRMode(CGDirectDisplayID display) __attribute__((weak_import));

CGError CGSEnableHDR(CGDirectDisplayID display, bool enable, int, int)  __attribute__((weak_import));
bool CGSIsHDREnabled(CGDirectDisplayID display) __attribute__((weak_import));
bool CGSIsHDRSupported(CGDirectDisplayID display) __attribute__((weak_import));

extern int DisplayServicesGetBrightness(CGDirectDisplayID display, float *brightness)  __attribute__((weak_import)); // probably doesn't return an error
extern int DisplayServicesSetBrightness(CGDirectDisplayID display, float brightness)  __attribute__((weak_import));

size_t CGDisplayBitsPerPixel(CGDirectDisplayID display);
size_t CGDisplayBitsPerSample(CGDirectDisplayID display);
size_t CGDisplaySamplesPerPixel(CGDirectDisplayID display);
size_t CGDisplayBytesPerRow(CGDirectDisplayID display);

#ifdef __cplusplus
}
#endif


#endif /* AppleMisc_h */
