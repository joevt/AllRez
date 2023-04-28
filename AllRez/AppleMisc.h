//
//  AppleMisc.h
//  AllRez
//
//  Created by joevt on 2022-05-19.
//

#ifndef AppleMisc_h
#define AppleMisc_h

#include "MacOSMacros.h"
#include <CoreGraphics/CGDisplayConfiguration.h> // must include before IOGraphicsTypes
#include <IOKit/graphics/IOGraphicsTypes.h>
#include <IOKit/graphics/IOGraphicsTypesPrivate.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <IOKit/i2c/IOI2CInterface.h>

#ifdef __cplusplus
}
#endif

#define kConnectionUnderscan 'pscn'

#ifndef MIN
#define MIN(x,y) (((x)<(y)) ? (x) : (y))
#endif

// https://github.com/apple-oss-distributions/IOGraphics/blob/IOGraphics-596.1/IOGraphicsFamily/IOKit/graphics/IOGraphicsInterfaceTypes.h

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_5
#define IO_FOUR_CHAR_CODE(x) (x)
enum {
// 212
	kIO64BGRAPixelFormat       = IO_FOUR_CHAR_CODE('B16I'), /* 64 bit bgra  */
	kIO64RGBAFloatPixelFormat  = IO_FOUR_CHAR_CODE('B16F'), /* 64 bit rgba  */
	kIO128RGBAFloatPixelFormat = IO_FOUR_CHAR_CODE('B32F')  /* 128 bit rgba float */
};
#endif

// https://github.com/apple-oss-distributions/IOGraphics/blob/IOGraphics-596.1/IOGraphicsFamily/IOKit/graphics/IOGraphicsTypes.h

// 145
struct IODisplayModeInformation_10_8 {
	UInt32                      nominalWidth;
	UInt32                      nominalHeight;
	IOFixed1616                 refreshRate;
	IOIndex                     maxDepthIndex;
	UInt32                      flags;
	union {
		UInt32                  reserved_10_1[ 4 ];
		struct {
			UInt16				imageWidth;
			UInt16				imageHeight;
			UInt32              reserved_10_8[ 3 ];
		};
	};
};
typedef struct IODisplayModeInformation_10_8 IODisplayModeInformation_10_8;


#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_9
enum {
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_5
// 174
	kDisplayModeValidateAgainstDisplay  = 0x00002000,
	kDisplayModeValidForMirroringFlag   = 0x00200000,
#endif
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_8
	kDisplayModeAcceleratorBackedFlag   = 0x00400000,
	kDisplayModeValidForHiResFlag       = 0x00800000,
	kDisplayModeValidForAirPlayFlag     = 0x01000000,
#endif
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_9
	kDisplayModeNativeFlag              = 0x02000000
#endif
};
#endif

// 256
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_15
enum {
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_6
	kIOPowerStateAttribute              = 'pwrs',
	kIODriverPowerAttribute             = 'dpow',
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_10
	kIOWindowServerActiveAttribute      = 'wsrv',
#endif
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_8
	kIOFBDisplayPortTrainingAttribute   = 'dpta',
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_13
	kIOFBDisplayState                   = 'dstt',
	kIOFBVariableRefreshRate            = 'vrr?',
	kIOFBLimitHDCPAttribute             = 'hdcp',
#endif
	
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_13_2
	kIOFBLimitHDCPStateAttribute        = 'sHDC',
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_13
	kIOFBStop                           = 'stop',
	kIOFBRedGammaScaleAttribute         = 'gslr',    // as of IOGRAPHICSTYPES_REV 54
	kIOFBGreenGammaScaleAttribute       = 'gslg',    // as of IOGRAPHICSTYPES_REV 54
	kIOFBBlueGammaScaleAttribute        = 'gslb',    // as of IOGRAPHICSTYPES_REV 54
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_14
	kIOFBHDRMetaDataAttribute           = 'hdrm',    // as of IOGRAPHICSTYPES_REV 64
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_15
	kIOBuiltinPanelPowerAttribute       = 'pnlp',    // as of IOGRAPHICSTYPES_REV 71
#endif
};
#endif

// 327
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_13
enum {
	kIOFBDisplayState_AlreadyActive     = (1 << 0),
	kIOFBDisplayState_RestoredProfile   = (1 << 1),
	kIOFBDisplayState_PipelineBlack     = (1 << 2),
	kIOFBDisplayState_Mask              = (kIOFBDisplayState_AlreadyActive |
										   kIOFBDisplayState_RestoredProfile |
										   kIOFBDisplayState_PipelineBlack)
};
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_13_2
// 336
enum {
	// States
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_12
	kIOWSAA_Unaccelerated       = 0,    // CPU rendering/access only, no GPU access
	kIOWSAA_Accelerated         = 1,    // GPU rendering/access only, no CPU mappings
	kIOWSAA_From_Accelerated    = 2,    // Transitioning from GPU to CPU
	kIOWSAA_To_Accelerated      = 3,    // Transitioning from CPU to GPU
	kIOWSAA_Sleep               = 4,
	//kIOWSAA_Hibernate           = kIOWSAA_Sleep,
	kIOWSAA_DriverOpen          = 5,    // Reserved
#endif
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_13_2
	kIOWSAA_StateMask           = 0xF,
#endif
	// Bits
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_12
	kIOWSAA_Transactional       = 0x10,  // If this bit is present, transition is to/from transactional operation model.
	// These attributes are internal
	kIOWSAA_DeferStart          = 0x100,
	kIOWSAA_DeferEnd            = 0x200,
#endif
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_13_2
	kIOWSAA_NonConsoleDevice    = 0x400,    // If present, associated FB is non-console.  See ERS for further details.
#endif
	//kIOWSAA_Reserved            = 0xF0000000
};
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_14
// 356
// IOFBNS prefix is IOFramebuffer notifyServer
enum {
	kIOFBNS_Rendezvous           = 0x87654321,  // Note sign-bit is 1 here.

	kIOFBNS_MessageMask         = 0x0000000f,
	kIOFBNS_Sleep               = 0x00,
	kIOFBNS_Wake                = 0x01,
	kIOFBNS_Doze                = 0x02,
#ifndef _OPEN_SOURCE_
	// <rdar://problem/39199290> IOGraphics: Add 10 second timeout to IODW DIM
	// policy.
	// Enum of Bitfields for Windows server notification msgh_id, which is a
	// integer_t signed type.
	kIOFBNS_Dim                 = 0x03,
	kIOFBNS_UnDim               = 0x04,
#endif // !_OPEN_SOURCE_

	// For Wake messages this field contains the current kIOFBDisplayState as
	// returned by attribute 'kIOFBDisplayState'
	kIOFBNS_DisplayStateMask    = 0x00000f00,
	kIOFBNS_DisplayStateShift   = 8,

	// Message Generation Count, top-bit i.e. sign-bit is always 0 for normal
	// messages, see kIOFBNS_Rendezvous is the only exception and it doesn't
	// encode a generation count.
	kIOFBNS_GenerationMask      = 0x7fff0000,
	kIOFBNS_GenerationShift     = 16,
};
#endif

// 376
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_8
enum {
	//kIOMirrorIsPrimary                  = 0x80000000,
	//kIOMirrorHWClipped                  = 0x40000000,
	kIOMirrorIsMirrored                 = 0x20000000
};
#endif

// 465
struct IODetailedTimingInformationV2_12 {
	union {
		UInt32      __reservedA_10_1[5];            // Init to 0
		struct { // 10.4
			UInt32      __reservedA_10_4[3];            // Init to 0
			UInt32      horizontalScaledInset;          // pixels
			UInt32      verticalScaledInset;            // lines
		};
	};

	UInt32      scalerFlags;

	UInt32      horizontalScaled;
	UInt32      verticalScaled;

	UInt32      signalConfig;
	UInt32      signalLevels;

	UInt64      pixelClock;                     // Hz

	UInt64      minPixelClock;                  // Hz - With error what is slowest actual clock
	UInt64      maxPixelClock;                  // Hz - With error what is fasted actual clock

	UInt32      horizontalActive;               // pixels
	UInt32      horizontalBlanking;             // pixels
	UInt32      horizontalSyncOffset;           // pixels
	UInt32      horizontalSyncPulseWidth;       // pixels

	UInt32      verticalActive;                 // lines
	UInt32      verticalBlanking;               // lines
	UInt32      verticalSyncOffset;             // lines
	UInt32      verticalSyncPulseWidth;         // lines

	UInt32      horizontalBorderLeft;           // pixels
	UInt32      horizontalBorderRight;          // pixels
	UInt32      verticalBorderTop;              // lines
	UInt32      verticalBorderBottom;           // lines

	UInt32      horizontalSyncConfig;
	UInt32      horizontalSyncLevel;            // Future use (init to 0)
	UInt32      verticalSyncConfig;
	UInt32      verticalSyncLevel;              // Future use (init to 0)
	union {
		UInt32      __reservedB_10_1[8];			// Init to 0
		struct { // 10.3.9
			UInt32      numLinks;
			union {
				UInt32      __reservedB_10_3_9[7];			// Init to 0
				struct { // 10.13
					UInt32      verticalBlankingExtension;      // lines (AdaptiveSync: 0 for non-AdaptiveSync support)
					union {
						UInt32      __reservedB_10_13[6];           // Init to 0
						struct { // 10.14
							UInt16      pixelEncoding;
							UInt16      bitsPerColorComponent;
							UInt16      colorimetry;
							UInt16      dynamicRange;
							union {
								UInt32      __reservedB_10_14[4];           // Init to 0
								struct { // 10.15
									UInt16      dscCompressedBitsPerPixel;
									UInt16      dscSliceHeight;
									UInt16      dscSliceWidth;
									union {
										UInt16      __reservedB_10_15[5];           // Init to 0
										struct { // 12.3
											UInt16      verticalBlankingMaxStretchPerFrame;
											UInt16      verticalBlankingMaxShrinkPerFrame;
											UInt16      __reservedB_12_3[3];            // Init to 0
										};
									};
								};
							};
						};
					};
				};
			};
		};
	};
};
typedef struct IODetailedTimingInformationV2_12 IODetailedTimingInformationV2_12;

//554
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_14
enum {
	kIOPixelEncodingNotSupported    = 0x0000,
	kIOPixelEncodingRGB444          = 0x0001,
	kIOPixelEncodingYCbCr444        = 0x0002,
	kIOPixelEncodingYCbCr422        = 0x0004,
	kIOPixelEncodingYCbCr420        = 0x0008
};
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_14
enum {
	kIOBitsPerColorComponentNotSupported    = 0x0000,
	kIOBitsPerColorComponent6               = 0x0001,
	kIOBitsPerColorComponent8               = 0x0002,
	kIOBitsPerColorComponent10              = 0x0004,
	kIOBitsPerColorComponent12              = 0x0008,
	kIOBitsPerColorComponent16              = 0x0010
};
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_14
enum {
	kIOColorimetryNotSupported  = 0x0000,
	kIOColorimetryNativeRGB     = 0x0001,
	kIOColorimetrysRGB          = 0x0002,
	kIOColorimetryDCIP3         = 0x0004,
	kIOColorimetryAdobeRGB      = 0x0008,
	kIOColorimetryxvYCC         = 0x0010,
	kIOColorimetryWGRGB         = 0x0020,
	kIOColorimetryBT601         = 0x0040,
	kIOColorimetryBT709         = 0x0080,
	kIOColorimetryBT2020        = 0x0100,
	kIOColorimetryBT2100        = 0x0200
};
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_VERSION_11_0
enum {
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_14
	// dynamicRange - should be in sync with "supportedDynamicRangeModes" enum below
	kIODynamicRangeNotSupported         = 0x0000,
	kIODynamicRangeSDR                  = 0x0001,
	kIODynamicRangeHDR10                = 0x0002,
	kIODynamicRangeDolbyNormalMode      = 0x0004,
	kIODynamicRangeDolbyTunnelMode      = 0x0008,
	kIODynamicRangeTraditionalGammaHDR  = 0x0010,
#endif
#if MAC_OS_X_VERSION_SDK < MAC_OS_VERSION_11_0
	kIODynamicRangeTraditionalGammaSDR  = 0x0020,   // as of IOGRAPHICSTYPES_REV 72
#endif
};
#endif

// 691
struct IODisplayTimingRangeV1_12
{
	UInt32      __reservedA[2];                 // Init to 0
	UInt32      version;                        // Init to 0
	UInt32      __reservedB[5];                 // Init to 0

	UInt64      minPixelClock;                  // Min dot clock in Hz
	UInt64      maxPixelClock;                  // Max dot clock in Hz

	UInt32      maxPixelError;                  // Max dot clock error
	UInt32      supportedSyncFlags;
	UInt32      supportedSignalLevels;

	union {
		UInt32		__reservedC_10_3_0[1];			// Init to 0
		UInt32      supportedSignalConfigs;  // 10.3.9
	};

	UInt32      minFrameRate;                   // Hz
	UInt32      maxFrameRate;                   // Hz
	UInt32      minLineRate;                    // Hz
	UInt32      maxLineRate;                    // Hz

	UInt32      maxHorizontalTotal;             // Clocks - Maximum total (active + blanking)
	UInt32      maxVerticalTotal;               // Clocks - Maximum total (active + blanking)
	UInt32      __reservedD[2];                 // Init to 0

	UInt8       charSizeHorizontalActive;
	UInt8       charSizeHorizontalBlanking;
	UInt8       charSizeHorizontalSyncOffset;
	UInt8       charSizeHorizontalSyncPulse;

	UInt8       charSizeVerticalActive;
	UInt8       charSizeVerticalBlanking;
	UInt8       charSizeVerticalSyncOffset;
	UInt8       charSizeVerticalSyncPulse;

	UInt8       charSizeHorizontalBorderLeft;
	UInt8       charSizeHorizontalBorderRight;
	UInt8       charSizeVerticalBorderTop;
	UInt8       charSizeVerticalBorderBottom;

	UInt8       charSizeHorizontalTotal;                // Character size for active + blanking
	UInt8       charSizeVerticalTotal;                  // Character size for active + blanking
	UInt16      __reservedE[1];                         // Reserved (Init to 0)

	UInt32      minHorizontalActiveClocks;
	UInt32      maxHorizontalActiveClocks;
	UInt32      minHorizontalBlankingClocks;
	UInt32      maxHorizontalBlankingClocks;

	UInt32      minHorizontalSyncOffsetClocks;
	UInt32      maxHorizontalSyncOffsetClocks;
	UInt32      minHorizontalPulseWidthClocks;
	UInt32      maxHorizontalPulseWidthClocks;

	UInt32      minVerticalActiveClocks;
	UInt32      maxVerticalActiveClocks;
	UInt32      minVerticalBlankingClocks;
	UInt32      maxVerticalBlankingClocks;

	UInt32      minVerticalSyncOffsetClocks;
	UInt32      maxVerticalSyncOffsetClocks;
	UInt32      minVerticalPulseWidthClocks;
	UInt32      maxVerticalPulseWidthClocks;

	UInt32      minHorizontalBorderLeft;
	UInt32      maxHorizontalBorderLeft;
	UInt32      minHorizontalBorderRight;
	UInt32      maxHorizontalBorderRight;

	UInt32      minVerticalBorderTop;
	UInt32      maxVerticalBorderTop;
	UInt32      minVerticalBorderBottom;
	UInt32      maxVerticalBorderBottom;
	union {
		UInt32	__reservedF_10_3_0[8];			// Init to 0
		struct { // 10.3.9
			UInt32      maxNumLinks;                       // number of links, if zero, assume link 1
			UInt32      minLink0PixelClock;                // min pixel clock for link 0 (kHz)
			UInt32      maxLink0PixelClock;                // max pixel clock for link 0 (kHz)
			UInt32      minLink1PixelClock;                // min pixel clock for link 1 (kHz)
			UInt32      maxLink1PixelClock;                // max pixel clock for link 1 (kHz)
			union {
				UInt32      __reservedF_10_3_9[3];                 // Init to 0
				struct { // 10.14
					UInt16      supportedPixelEncoding;
					UInt16      supportedBitsPerColorComponent;
					UInt16      supportedColorimetryModes;
					UInt16      supportedDynamicRangeModes;

					UInt32      __reservedF_10_14[1];                    // Init to 0
				};
			};
		};
	};
};
typedef struct IODisplayTimingRangeV1_12 IODisplayTimingRangeV1_12;

struct IODisplayTimingRangeV2_12
{
	IODisplayTimingRangeV1_12 v1;

	UInt64      maxBandwidth;
	UInt32      dscMinSliceHeight;
	UInt32      dscMaxSliceHeight;
	UInt32      dscMinSliceWidth;
	UInt32      dscMaxSliceWidth;
	UInt32      dscMinSlicePerLine;
	UInt32      dscMaxSlicePerLine;
	UInt16      dscMinBPC;
	UInt16      dscMaxBPC;
	UInt16      dscMinBPP;
	UInt16      dscMaxBPP;
	UInt8       dscVBR;
	UInt8       dscBlockPredEnable;
	UInt32      __reservedC_10_15[6];
};

typedef struct IODisplayTimingRangeV2_12 IODisplayTimingRangeV2_12;

// 905
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_14
enum {
	// supportedPixelEncoding
	kIORangePixelEncodingNotSupported   = 0x0000,
	kIORangePixelEncodingRGB444         = 0x0001,
	kIORangePixelEncodingYCbCr444       = 0x0002,
	kIORangePixelEncodingYCbCr422       = 0x0004,
	kIORangePixelEncodingYCbCr420       = 0x0008,
};

enum {
	// supportedBitsPerColorComponent
	kIORangeBitsPerColorComponentNotSupported   = 0x0000,
	kIORangeBitsPerColorComponent6              = 0x0001,
	kIORangeBitsPerColorComponent8              = 0x0002,
	kIORangeBitsPerColorComponent10             = 0x0004,
	kIORangeBitsPerColorComponent12             = 0x0008,
	kIORangeBitsPerColorComponent16             = 0x0010,
};

enum {
	// supportedColorimetry
	kIORangeColorimetryNotSupported     = 0x0000,
	kIORangeColorimetryNativeRGB        = 0x0001,
	kIORangeColorimetrysRGB             = 0x0002,
	kIORangeColorimetryDCIP3            = 0x0004,
	kIORangeColorimetryAdobeRGB         = 0x0008,
	kIORangeColorimetryxvYCC            = 0x0010,
	kIORangeColorimetryWGRGB            = 0x0020,
	kIORangeColorimetryBT601            = 0x0040,
	kIORangeColorimetryBT709            = 0x0080,
	kIORangeColorimetryBT2020           = 0x0100,
	kIORangeColorimetryBT2100           = 0x0200,
};
#endif

// 939
#if MAC_OS_X_VERSION_SDK < MAC_OS_VERSION_11_0
enum {
	// supportedDynamicRangeModes - should be in sync with "dynamicRange" enum above
	//kIORangeDynamicRangeNotSupported        = 0x0000,
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_14
	kIORangeDynamicRangeSDR                 = 0x0001,
	kIORangeDynamicRangeHDR10               = 0x0002,
	kIORangeDynamicRangeDolbyNormalMode     = 0x0004,
	kIORangeDynamicRangeDolbyTunnelMode     = 0x0008,
	kIORangeDynamicRangeTraditionalGammaHDR = 0x0010,
#endif
	kIORangeDynamicRangeTraditionalGammaSDR = 0x0020,   // as of IOGRAPHICSTYPES_REV 72
};
#endif

// 958
#if MAC_OS_X_VERSION_SDK < MAC_OS_VERSION_12_0
enum {
	// supportedSyncFlags
	//kIORangeSupportsSeparateSyncs        = 0x00000001,
	//kIORangeSupportsSyncOnGreen          = 0x00000002,
	//kIORangeSupportsCompositeSync        = 0x00000004,
	//kIORangeSupportsVSyncSerration       = 0x00000008,
	kIORangeSupportsVRR                  = 0x00000010   // since IOGRAPHICSTYPES_REV 76
};

// 966
enum {
	// supportedSignalConfigs
	//kIORangeSupportsInterlacedCEATiming            = 0x00000004,
	//kIORangeSupportsInterlacedCEATimingWithConfirm = 0x00000008,
	kIORangeSupportsMultiAlignedTiming             = 0x00000040     // since IOGRAPHICSTYPES_REV 75
};
#endif

// 972
#if MAC_OS_X_VERSION_SDK < MAC_OS_VERSION_12_0
enum {
	// signalConfig
	//kIODigitalSignal          = 0x00000001,
	//kIOAnalogSetupExpected    = 0x00000002,
	//kIOInterlacedCEATiming    = 0x00000004,
	//kIONTSCTiming             = 0x00000008,
	//kIOPALTiming              = 0x00000010,
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_15
	kIODSCBlockPredEnable     = 0x00000020,
#endif
#if MAC_OS_X_VERSION_SDK < MAC_OS_VERSION_12_0
	kIOMultiAlignedTiming     = 0x00000040, // since IOGRAPHICSTYPES_REV 73
#endif
};
#endif

// 1047
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_15
enum {
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_6
// 1055
	kConnectionCheckEnable              = 'cena',
	//kConnectionProbe                    = 'prob',

// 1066
	kConnectionRedGammaScale            = 'rgsc',
	kConnectionGreenGammaScale          = 'ggsc',
	kConnectionBlueGammaScale           = 'bgsc',
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_15
// 1057
	kConnectionIgnore                   = '\0igr',
#endif

// 1069
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_9
	kConnectionGammaScale               = 'gsc ',
	kConnectionFlushParameters          = 'flus',
#endif
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_10
	kConnectionVBLMultiplier            = 'vblm',
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_6
// 1074
	kConnectionHandleDisplayPortEvent   = 'dpir',

	kConnectionPanelTimingDisable       = 'pnlt',

	kConnectionColorMode                = 'cyuv',
	kConnectionColorModesSupported      = 'colr',
	kConnectionColorDepthsSupported     = ' bpc',

	kConnectionControllerDepthsSupported = '\0grd',

	kConnectionControllerDitherControl   = '\0gdc',
	kConnectionDisplayFlags              = 'dflg',

// 1083
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_6
	kConnectionControllerColorDepth      = '\0dpd',
#endif

#if MAC_OS_X_VERSION_SDK <= MAC_OS_X_VERSION_10_5 || IOGRAPHICSTYPES_REV < 28
// 1088
	kConnectionEnableAudio               = 'aud ',
#endif
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_9
	kConnectionAudioStreaming            = 'auds',
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_14
	kConnectionStartOfFrameTime          = 'soft',  // as of IOGRAPHICSTYPES_REV 65
#endif
};
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_6
// 1112
// kConnectionHandleDisplayPortEvent values
enum {
	kIODPEventStart                             = 1,
	kIODPEventIdle                              = 2,

	kIODPEventForceRetrain                      = 3,

	kIODPEventRemoteControlCommandPending       = 256,
	kIODPEventAutomatedTestRequest              = 257,
	kIODPEventContentProtection                 = 258,
	kIODPEventMCCS                              = 259,
	kIODPEventSinkSpecific                      = 260
};
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_6
// 1126
#define kIODisplayAttributesKey         "IODisplayAttributes"

#endif

enum {
// 1132
	kIODisplaySelectedColorModeKey4cc   = 'cmod'
};

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_6
enum // 1152
{
	// kConnectionColorDepthsSupported attribute
	kIODisplayRGBColorComponentBitsUnknown       = 0x00000000,
	kIODisplayRGBColorComponentBits6             = 0x00000001,
	kIODisplayRGBColorComponentBits8             = 0x00000002,
	kIODisplayRGBColorComponentBits10            = 0x00000004,
	kIODisplayRGBColorComponentBits12            = 0x00000008,
	kIODisplayRGBColorComponentBits14            = 0x00000010,
	kIODisplayRGBColorComponentBits16            = 0x00000020,

	kIODisplayYCbCr444ColorComponentBitsUnknown  = 0x00000000,
	kIODisplayYCbCr444ColorComponentBits6        = 0x00000100,
	kIODisplayYCbCr444ColorComponentBits8        = 0x00000200,
	kIODisplayYCbCr444ColorComponentBits10       = 0x00000400,
	kIODisplayYCbCr444ColorComponentBits12       = 0x00000800,
	kIODisplayYCbCr444ColorComponentBits14       = 0x00001000,
	kIODisplayYCbCr444ColorComponentBits16       = 0x00002000,

	kIODisplayYCbCr422ColorComponentBitsUnknown  = 0x00000000,
	kIODisplayYCbCr422ColorComponentBits6        = 0x00010000,
	kIODisplayYCbCr422ColorComponentBits8        = 0x00020000,
	kIODisplayYCbCr422ColorComponentBits10       = 0x00040000,
	kIODisplayYCbCr422ColorComponentBits12       = 0x00080000,
	kIODisplayYCbCr422ColorComponentBits14       = 0x00100000,
	kIODisplayYCbCr422ColorComponentBits16       = 0x00200000,
};

enum
{
	// kConnectionDitherControl attribute
	kIODisplayDitherDisable          = 0x00000000,
	kIODisplayDitherSpatial          = 0x00000001,
	kIODisplayDitherTemporal         = 0x00000002,
	kIODisplayDitherFrameRateControl = 0x00000004,
	kIODisplayDitherDefault          = 0x00000080,
	kIODisplayDitherAll              = 0x000000FF,
	kIODisplayDitherRGBShift         = 0,
	kIODisplayDitherYCbCr444Shift    = 8,
	kIODisplayDitherYCbCr422Shift    = 16,
};

enum
{
	// kConnectionDisplayFlags attribute
	kIODisplayNeedsCEAUnderscan      = 0x00000001,
};

#endif


// 1223
enum {
	// connection types for IOServiceOpen
	//kIOFBServerConnectType              = 0,
	//kIOFBSharedConnectType              = 1,
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_13 || MAC_OS_X_VERSION_SDK > MAC_OS_X_VERSION_10_14
	kIOFBDiagnoseConnectType            = 2,
#endif
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_15
	kIOGDiagnoseGTraceType              = 11452,  // On Display Wrangler
	kIOGDiagnoseConnectType             = 38744,
#endif
};

struct IOHardwareCursorDescriptor_32bit {
   UInt16               majorVersion;
   UInt16               minorVersion;
   UInt32               height;
   UInt32               width;
   UInt32               bitDepth;                       // bits per pixel, or a QD/QT pixel type
   UInt32               maskBitDepth;                   // unused
   UInt32               numColors;                      // number of colors in the colorMap. ie.
   UInt32               colorEncodings; // 32-bit pointer to array of UInt32
   UInt32               flags;
   UInt32               supportedSpecialEncodings;
   UInt32               specialEncodings[16];
};
typedef struct IOHardwareCursorDescriptor_32bit IOHardwareCursorDescriptor_32bit;

struct IOHardwareCursorDescriptor_64bit {
   UInt16               majorVersion;
   UInt16               minorVersion;
   UInt32               height;
   UInt32               width;
   UInt32               bitDepth;                       // bits per pixel, or a QD/QT pixel type
   UInt32               maskBitDepth;                   // unused
   UInt32               numColors;                      // number of colors in the colorMap. ie.
   UInt64               colorEncodings; // 64-bit pointer to array of UInt32
   UInt32               flags;
   UInt32               supportedSpecialEncodings;
   UInt32               specialEncodings[16];
};
typedef struct IOHardwareCursorDescriptor_64bit IOHardwareCursorDescriptor_64bit;

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_9
// 1407
enum {
	kIOTimingIDVESA_1152x864_75hz    = 215,     /* 1152x864  (75 Hz) VESA timing. */
};
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_6
enum { // 1494
	kIOFBAVSignalTypeUnknown = 0x00000000,
	kIOFBAVSignalTypeVGA     = 0x00000001,
	kIOFBAVSignalTypeDVI     = 0x00000002,
	kIOFBAVSignalTypeHDMI    = 0x00000008,
	kIOFBAVSignalTypeDP      = 0x00000010,
};
#endif

//1676
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_12
#define kIODisplayBrightnessProbeKey        "brightness-probe"
#define kIODisplayLinearBrightnessProbeKey  "linear-brightness-probe"
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_7
#define kIODisplayLinearBrightnessKey       "linear-brightness"
#define kIODisplayUsableLinearBrightnessKey "usable-linear-brightness"
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_10
#define kIODisplayBrightnessFadeKey         "brightness-fade"
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_6
// 1697
#define kIODisplaySpeakerVolumeKey              "speaker-volume"
#define kIODisplaySpeakerSelectKey              "speaker-select"
#define kIODisplayMicrophoneVolumeKey           "microphone-volume"
#define kIODisplayAmbientLightSensorKey         "ambient-light-sensor"
#define kIODisplayAudioMuteAndScreenBlankKey    "audio-mute-and-screen-blank"
#define kIODisplayAudioTrebleKey                "audio-treble"
#define kIODisplayAudioBassKey                  "audio-bass"
#define kIODisplayAudioBalanceLRKey             "audio-balance-LR"
#define kIODisplayAudioProcessorModeKey         "audio-processor-mode"
#define kIODisplayPowerModeKey                  "power-mode"
#define kIODisplayManufacturerSpecificKey       "manufacturer-specific"
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_7
#define kIODisplayPowerStateKey       			"dsyp"

#define kIODisplayControllerIDKey				"IODisplayControllerID"
#define kIODisplayCapabilityStringKey       	"IODisplayCapabilityString"
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_6
#define kIODisplayRedGammaScaleKey      "rgsc"
#define kIODisplayGreenGammaScaleKey    "ggsc"
#define kIODisplayBlueGammaScaleKey     "bgsc"
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_10
#define kIODisplayGammaScaleKey         "gsc "
#endif

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_6
#define kIODisplayParametersFlushKey    "flush"
#endif

// https://github.com/apple-oss-distributions/IOGraphics/blob/IOGraphics-596.1/IOGraphicsFamily/IOKit/graphics/IOGraphicsTypesPrivate.h

enum { // 30
	// This is the ID given to a programmable timing used at boot time
	k1 = kIODisplayModeIDBootProgrammable, // = (IODisplayModeID)0xFFFFFFFB,
	// Lowest (unsigned) DisplayModeID reserved by Apple
	k2 = kIODisplayModeIDReservedBase, // = (IODisplayModeID)0x80000000

	// https://github.com/apple-oss-distributions/IOKitUser/blob/IOKitUser-1445.71.1/graphics.subproj/IOGraphicsLib.c
	kIOFBSWOfflineDisplayModeID = (IODisplayModeID) 0xffffff00,
	kArbModeIDSeedMask           = (IODisplayModeID) 0x00007000, //  (connectRef->arbModeIDSeed is incremented everytime IOFBBuildModeList is called) // https://github.com/apple-oss-distributions/IOKitUser/blob/IOKitUser-1445.71.1/graphics.subproj/IOGraphicsLibInternal.h
};

// 156
#define detailedTimingModeID_10_1            __reservedA_10_1[0]

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

// https://github.com/apple-oss-distributions/IOKitUser/blob/IOKitUser-1445.71.1/graphics.subproj/IOGraphicsLibInternal.h

// 76
enum {
	// skips all the various checks and always installs
	kScaleInstallAlways         = 0x00000001,
	// disables the install of a stretched version if the aspect is different
	kScaleInstallNoStretch      = 0x00000002,
	// install resolution untransformed
	kScaleInstallNoResTransform = 0x00000004,
	// install resolution on mirror dependents of this display
	kScaleInstallMirrorDeps     = 0x00000008
};

// 128
struct IOFBOvrDimensions {
	UInt32              width;
	UInt32              height;
	IOOptionBits        setFlags; // kDisplayModeSafeFlag
	IOOptionBits        clearFlags;
};
typedef struct IOFBOvrDimensions IOFBOvrDimensions;

// 136
struct EDIDDetailedTimingDesc {
	UInt16      clock;
	UInt8       horizActive;
	UInt8       horizBlanking;
	UInt8       horizHigh;
	UInt8       verticalActive;
	UInt8       verticalBlanking;
	UInt8       verticalHigh;
	UInt8       horizSyncOffset;
	UInt8       horizSyncWidth;
	UInt8       verticalSyncOffsetWidth;
	UInt8       syncHigh;
	UInt8       horizImageSize;
	UInt8       verticalImageSize;
	UInt8       imageSizeHigh;
	UInt8       horizBorder;
	UInt8       verticalBorder;
	UInt8       flags;
};
typedef struct EDIDDetailedTimingDesc EDIDDetailedTimingDesc;

struct EDIDGeneralDesc {
	UInt16      flag1;
	UInt8       flag2;
	UInt8       type;
	UInt8       flag3;
	UInt8       data[13];
};
typedef struct EDIDGeneralDesc EDIDGeneralDesc;

union EDIDDesc {
	EDIDDetailedTimingDesc      timing;
	EDIDGeneralDesc             general;
};
typedef union EDIDDesc EDIDDesc;

struct EDID {
	UInt8       header[8];
	UInt8       vendorProduct[4];
	UInt8       serialNumber[4];
	UInt8       weekOfManufacture;
	UInt8       yearOfManufacture;
	UInt8       version;
	UInt8       revision;
	UInt8       displayParams[5];
	UInt8       colorCharacteristics[10];
	UInt8       establishedTimings[3];
	UInt16      standardTimings[8];
	EDIDDesc    descriptors[4];
	UInt8       extension;
	UInt8       checksum;
};
typedef struct EDID EDID;

// 524
typedef struct IOFBConnect * IOFBConnectRef;

#ifdef __cplusplus
extern "C" {
#endif
// this is actually __private_extern__ since 10.3 so you can't really use it
extern IOFBConnectRef
IOFBConnectToRef( io_connect_t connect ) __attribute__((weak_import));
#ifdef __cplusplus
}
#endif


// https://github.com/apple-oss-distributions/IOKitUser/blob/IOKitUser-1445.71.1/graphics.subproj/IOGraphicsLib.c

// 91
struct DMTimingOverrideRec {
	UInt32 timingOverrideVersion;
	UInt32 timingOverrideAttributes;   // flags
	UInt32 timingOverrideSetFlags;     // VDTimingInfoRec.csTimingFlags |= timingOverrideSetFlags
	UInt32 timingOverrideClearFlags;   // VDTimingInfoRec.csTimingFlags &= (~timingOverrideClearFlags)
	UInt32 timingOverrideReserved[16]; // reserved
};
typedef struct DMTimingOverrideRec      DMTimingOverrideRec;

#ifndef __DISPLAYS__ // ApplicationServices/QD/Displays.h
struct DMDisplayTimingInfoRec {
	UInt32 timingInfoVersion;
	UInt32 timingInfoAttributes;       // flags
	SInt32 timingInfoRelativeQuality;  // quality of the timing
	SInt32 timingInfoRelativeDefault;  // relative default of the timing
	UInt32 timingInfoReserved[16];     // reserved
};
typedef struct DMDisplayTimingInfoRec   DMDisplayTimingInfoRec;
#endif

// https://github.com/apple-oss-distributions/IOKitUser/blob/IOKitUser-2022.60.4/graphics.subproj/IOGraphicsLib.c

// 5420
struct IOI2CConnect
{
	io_connect_t connect;
};


#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_5
// https://github.com/apple-oss-distributions/xnu/blob/xnu-8792.81.2/iokit/IOKit/IOMessage.h
// 69
#define kIOMessageCopyClientID             iokit_common_msg(0x330)
#endif

// https://github.com/apple-oss-distributions/xnu/blob/xnu-8792.81.2/iokit/IOKit/pwr_mgt/IOPM.h

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_15_1
// 511
enum {
	//kIOPMSleepNow                 = (1 << 0),// put machine to sleep now
	//kIOPMAllowSleep               = (1 << 1),// allow idle sleep
	//kIOPMPreventSleep             = (1 << 2),// do not allow idle sleep
	//kIOPMPowerButton              = (1 << 3),// power button was pressed
	//kIOPMClamshellClosed          = (1 << 4),// clamshell was closed
	//kIOPMPowerEmergency           = (1 << 5),// battery dangerously low
	//kIOPMDisableClamshell         = (1 << 6),// do not sleep on clamshell closure
	//kIOPMEnableClamshell          = (1 << 7),// sleep on clamshell closure
	//kIOPMProcessorSpeedChange     = (1 << 8),// change the processor speed
	//kIOPMOverTemp                 = (1 << 9),// system dangerously hot
	//kIOPMClamshellOpened          = (1 << 10),// clamshell was opened
	//kIOPMDWOverTemp               = (1 << 11),// DarkWake thermal limits exceeded.
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_15
	kIOPMPowerButtonUp            = (1 << 12),// Power button up
#endif
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_15_1
	kIOPMProModeEngaged           = (1 << 13),// Fans entered 'ProMode'
	kIOPMProModeDisengaged        = (1 << 14) // Fans exited 'ProMode'
#endif
};
#endif


// https://github.com/apple-oss-distributions/IOGraphics/blob/IOGraphics-596.1/IOGraphicsFamily/IOKit/i2c/IOI2CInterface.h

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_6
enum {
// 42
	kIOI2CDisplayPortNativeTransactionType = 4,
};

enum {
// 141
	kIOI2CBusTypeDisplayPort    = 2
};

#endif


#pragma pack(push, 4)
struct IOI2CRequest_10_6
{
	IOOptionBits                sendTransactionType;
	IOOptionBits                replyTransactionType;
	uint32_t                    sendAddress;
	uint32_t                    replyAddress;
	uint8_t                     sendSubAddress;
	uint8_t                     replySubAddress;
	uint8_t                     __reservedA[2];

	uint64_t                    minReplyDelay;

	IOReturn                    result;
	IOOptionBits                commFlags;

#if defined(__LP64__)
	uint32_t                    __padA;
#else
	vm_address_t                sendBuffer;
#endif
	uint32_t                    sendBytes;

	uint32_t                    __reservedB[2];

#if defined(__LP64__)
	uint32_t                    __padB;
#else
	vm_address_t                replyBuffer;
#endif
	uint32_t                    replyBytes;

	IOI2CRequestCompletion      completion;
#if !defined(__LP64__)
	uint32_t                    __padC[5];
#else
	vm_address_t                sendBuffer;
	vm_address_t                replyBuffer;
#endif

	uint32_t                    __reservedC[10];
#ifdef __ppc__
	uint32_t                    __reservedD;
#endif
};
#pragma pack(pop)


// https://github.com/apple-oss-distributions/IOGraphics/blob/IOGraphics-497.1/IOGraphicsFamily/IOKit/i2c/IOI2CInterfacePrivate.h


#pragma pack(push, 4)

struct IOI2CRequest_10_5_kernel_i386 // for 10.4 and 10.5 kernel
{
	UInt64              __reservedA;
	IOReturn            result;
	uint32_t            completion;
	IOOptionBits        commFlags;
	uint64_t            minReplyDelay;
	uint8_t             sendAddress;
	uint8_t             sendSubAddress;
	uint8_t             __reservedB[2];
	IOOptionBits        sendTransactionType;
	uint32_t            sendBuffer;
	uint32_t            sendBytes;
	uint8_t             replyAddress;
	uint8_t             replySubAddress;
	uint8_t             __reservedC[2];
	IOOptionBits        replyTransactionType;
	uint32_t            replyBuffer;
	uint32_t            replyBytes;
	uint32_t            __reservedD[16];
}; // 124 bytes

struct IOI2CRequest_10_5_kernel_ppc // for 10.4 and 10.5 kernel
{
	UInt64              __reservedA;
	IOReturn            result;
	uint32_t            completion;
	IOOptionBits        commFlags;
	uint64_t            minReplyDelay;
	uint8_t             sendAddress;
	uint8_t             sendSubAddress;
	uint8_t             __reservedB[2];
	IOOptionBits        sendTransactionType;
	uint32_t            sendBuffer;
	uint32_t            sendBytes;
	uint8_t             replyAddress;
	uint8_t             replySubAddress;
	uint8_t             __reservedC[2];
	IOOptionBits        replyTransactionType;
	uint32_t            replyBuffer;
	uint32_t            replyBytes;
	uint32_t            __reservedD[16];
	uint32_t            __reservedE;
}; // 128 bytes

#pragma pack(pop)

#pragma pack(push, 4)

struct IOI2CRequest_10_5_user // for 10.4 and 10.5 user
{
	UInt64                  __reservedA;
	IOReturn                result;
	IOI2CRequestCompletion  completion;
	IOOptionBits            commFlags;
	uint64_t                minReplyDelay;
	UInt8                   sendAddress;
	UInt8                   sendSubAddress;
	UInt8                   __reservedB[2];
	IOOptionBits            sendTransactionType;
	vm_address_t            sendBuffer;
	IOByteCount             sendBytes;
	UInt8                   replyAddress;
	UInt8                   replySubAddress;
	UInt8                   __reservedC[2];
	IOOptionBits            replyTransactionType;
	vm_address_t            replyBuffer;
	IOByteCount             replyBytes;
	UInt32                  __reservedD[16];
};

#pragma pack(pop)

enum { kIOI2CInlineBufferBytes = 1024 };

struct IOI2CBuffer {
	union {
		struct {
			IOI2CRequest                   request;
			UInt8                          inlineBuffer[ kIOI2CInlineBufferBytes ];
		} buf_def;
		struct {
			IOI2CRequest_10_5_kernel_ppc   request;
			UInt8                          inlineBuffer[ kIOI2CInlineBufferBytes ];
		} buf_ppc;
		struct {
			IOI2CRequest_10_5_kernel_i386  request;
			UInt8                          inlineBuffer[ kIOI2CInlineBufferBytes ];
		} buf_i386;
	};
};

// https://github.com/robbertkl/ResolutionMenu/blob/master/Resolution%20Menu/DisplayModeMenuItem.m
// CoreGraphics DisplayMode struct used in private APIs
typedef struct {
	union {
		struct {
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
/* b8 */	uint32_t size; // 0 or 200 or 212
/* bc */	uint32_t refreshRate; // 59.875 : 16b.16b refresh rate fixed point (the fractional part might be flags (0001 or E002)? but it appears to be used as a fraction in the dictionary results)
			
/* c0 */	uint32_t IOFlags; // -> IOFramebufferInformation.flags kDisplayModeSafetyFlags
/* c4 */	IODisplayModeID DisplayModeID; // 0x8000xyyy

/* 200 10.5 */
/* c8 */	uint32_t PixelsWide; // actual pixels = double width for HiDPI
/* cc */	uint32_t PixelsHigh; // actual pixels = double height for HiDPI
			
/* d0 */	float resolution; // 1 = normal, 2 = HiDPI

/* 212 10.6 */
/* d4 */
		};
		uint8_t bytes[0x200]; // allow up to 512 bytes but no version of macOS currently has more than 212 bytes.
	};
} CGSDisplayModeDescription;

//#include <IOKit/ndrvsupport/IONDRVLibraries.h>
//#include <CarbonCore/MacErrors.h>

#if 0 // for 10.6
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
#endif

//#include <IOKit/ndrvsupport/IONDRVLibraries.h>
//#include <CarbonCore/MacErrors.h>

#if 0 // for 10.6
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
#endif 

// https://github.com/apple-oss-distributions/IOKitUser/blob/IOKitUser-1445.71.1/graphics.subproj/IODisplayLib.c

IOReturn EDIDDescToDetailedTiming( EDID * edid, EDIDDetailedTimingDesc * desc, IODetailedTimingInformationV2_12 * timing );
Boolean EDIDDescToDisplayTimingRangeRec( EDID * edid, EDIDGeneralDesc * desc, IODisplayTimingRangeV1_12 * range );

// ___________________________________________________________________________________________

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_6
// CGDirectDisplay.h
typedef struct CGDisplayMode *CGDisplayModeRef;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// CoreGraphics private APIs with support for scaled (retina) display modes
CGError CGSGetCurrentDisplayMode(CGDirectDisplayID display, uint32_t* modeNum) __attribute__((weak_import));
CGError CGSConfigureDisplayMode(CGDisplayConfigRef config, CGDirectDisplayID display, uint32_t modeNum) __attribute__((weak_import));
CGError CGSGetNumberOfDisplayModes(CGDirectDisplayID display, uint32_t* nModes) __attribute__((weak_import));
CGError CGSGetDisplayModeDescriptionOfLength(CGDirectDisplayID display, int idx, CGSDisplayModeDescription* mode, int length) __attribute__((weak_import));
CGError CGSServiceForDisplayNumber(CGDirectDisplayID display, io_service_t *service) __attribute__((weak_import));
CGError CGSDisplayDeviceForDisplayNumber(CGDirectDisplayID display, io_service_t *service) __attribute__((weak_import));

bool SLSIsDisplayModeVRR(CGDirectDisplayID display, uint32_t modeNum) __attribute__((weak_import));
bool SLSIsDisplayModeProMotion(CGDirectDisplayID display, uint32_t modeNum) __attribute__((weak_import));
UInt32 SLSGetDisplayModeMinRefreshRate(CGDirectDisplayID display, uint32_t modeNum, float *minRefreshRate) __attribute__((weak_import));

CGError SLSDisplaySetHDRModeEnabled(CGDirectDisplayID display, bool enable, int, int) __attribute__((weak_import));
bool SLSDisplayIsHDRModeEnabled(CGDirectDisplayID display) __attribute__((weak_import));
bool SLSDisplaySupportsHDRMode(CGDirectDisplayID display) __attribute__((weak_import));

CGError CGSEnableHDR(CGDirectDisplayID display, bool enable, int, int) __attribute__((weak_import));
bool CGSIsHDREnabled(CGDirectDisplayID display) __attribute__((weak_import));
bool CGSIsHDRSupported(CGDirectDisplayID display) __attribute__((weak_import));

int DisplayServicesGetBrightness(CGDirectDisplayID display, float *brightness) __attribute__((weak_import)); // probably doesn't return an error
int DisplayServicesSetBrightness(CGDirectDisplayID display, float brightness) __attribute__((weak_import));

size_t CGDisplayBitsPerPixel(CGDirectDisplayID display) __attribute__((weak_import));
size_t CGDisplayBitsPerSample(CGDirectDisplayID display) __attribute__((weak_import));
size_t CGDisplaySamplesPerPixel(CGDirectDisplayID display) __attribute__((weak_import));
size_t CGDisplayBytesPerRow(CGDirectDisplayID display) __attribute__((weak_import));

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_5
double CGDisplayRotation(CGDirectDisplayID display) __attribute__((weak_import)); // CG_AVAILABLE_STARTING(10.5);
CGColorSpaceRef CGDisplayCopyColorSpace(CGDirectDisplayID display) __attribute__((weak_import)); // CG_AVAILABLE_STARTING(10.5);
#endif
#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_6
CGDisplayModeRef CGDisplayCopyDisplayMode(CGDirectDisplayID display) __attribute__((weak_import)); // CG_AVAILABLE_STARTING(10.6);
void CGDisplayModeRelease(CGDisplayModeRef mode) __attribute__((weak_import)); // CG_AVAILABLE_STARTING(10.6);
CFTypeID CGDisplayModeGetTypeID(void) __attribute__((weak_import)); // CG_AVAILABLE_STARTING(10.6);
CFArrayRef CGDisplayCopyAllDisplayModes(CGDirectDisplayID display, CFDictionaryRef options) __attribute__((weak_import)); // CG_AVAILABLE_STARTING(10.6);
#endif

// IODisplayForFramebuffer exists in all versions of macOS in IOKit.Framework but is not declared external in Mac OS X 10.5.8
io_service_t IODisplayForFramebuffer(io_service_t framebuffer, IOOptionBits options) __attribute__((weak_import)); 

#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_5
kern_return_t
IOConnectCallMethod(
	mach_port_t	 connection,		// In
	uint32_t	 selector,		// In
	const uint64_t	*input,			// In
	uint32_t	 inputCnt,		// In
	const void      *inputStruct,		// In
	size_t		 inputStructCnt,	// In
	uint64_t	*output,		// Out
	uint32_t	*outputCnt,		// In/Out
	void		*outputStruct,		// Out
	size_t		*outputStructCnt)	// In/Out
__attribute__((weak_import));
#endif

IOReturn UniversalI2CSendRequest( IOI2CConnectRef connect, IOOptionBits options, IOI2CRequest_10_6 * request );
char * MacOSVersion();
const char * MachineType();
int DarwinMajorVersion();
int DarwinMinorVersion();
int DarwinRevision();

typedef io_service_t (*IODisplayForFramebufferPtr)(io_service_t framebuffer, IOOptionBits options);

IODisplayForFramebufferPtr GetIODisplayForFramebufferPtr();


#ifdef __cplusplus
}
#endif


#endif /* AppleMisc_h */
