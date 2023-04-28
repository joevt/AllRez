//
//  AppleMisc.cpp
//  AllRez
//
//  Created by joevt on 2023-03-09.
//

#include "AppleMisc.h"
#include <sys/sysctl.h>
#include <stdio.h>
#include <strings.h>
#include <CoreFoundation/CFBundle.h>
#include <mach-o/dyld.h>
#include <dlfcn.h>

#define ConvertPointer(to, from) \
do { if (sizeof(to) < sizeof(from)) (to) = 0; else if (sizeof(to) == 8) *(uint64_t*)(&(to)) = (uint64_t)(from); else *(uint32_t*)(&(to)) = (uint32_t)(uint64_t)(from); } while (0)

#define ConvertIOI2CRequest(from, to, final) \
do { \
if (!final) bzero((to), sizeof(*(to))); \
(to)->commFlags            = (from)->commFlags                     ; \
if (!final) ConvertPointer((to)->completion, (from)->completion)   ; \
(to)->minReplyDelay        = (from)->minReplyDelay                 ; \
(to)->replyAddress         = (from)->replyAddress                  ; \
if (!final) ConvertPointer((to)->replyBuffer, (from)->replyBuffer) ; \
(to)->replyBytes           = (unsigned int)(from)->replyBytes      ; \
(to)->replySubAddress      = (from)->replySubAddress               ; \
(to)->replyTransactionType = (from)->replyTransactionType          ; \
(to)->result               = (from)->result                        ; \
(to)->sendAddress          = (from)->sendAddress                   ; \
if (!final) ConvertPointer((to)->sendBuffer, (from)->sendBuffer)   ; \
(to)->sendBytes            = (unsigned int)(from)->sendBytes       ; \
(to)->sendSubAddress       = (from)->sendSubAddress                ; \
(to)->sendTransactionType  = (from)->sendTransactionType           ; \
} while (0)

static char ghwmachine[20];

const char * MachineType() {
	if (!ghwmachine[0]) {
		size_t data_len = sizeof(ghwmachine) - 1;
		if (sysctlbyname("hw.machine", &ghwmachine, &data_len, NULL, 0)) {
			snprintf(ghwmachine, sizeof(ghwmachine), "unknownArch");
		}
		else if (!strcmp(ghwmachine, "Power Macintosh")) {
			snprintf(ghwmachine, sizeof(ghwmachine), "ppc");
		}
	}
	return ghwmachine;
}

static int gDarwinMajorVersion = -1;
static int gDarwinMinorVersion = 0;
static int gDarwinRevision = 0;
static char gMacOSVersion[20];

void GetDarwinVersion() {
	char osversion[32];
	size_t osversion_len = sizeof(osversion) - 1;
	int osversion_name[] = { CTL_KERN, KERN_OSRELEASE };

	if (sysctl(osversion_name, 2, osversion, &osversion_len, NULL, 0) == -1) {
		printf("sysctl() failed\n");
		return;
	}

	if (sscanf(osversion, "%u.%u.%u", &gDarwinMajorVersion, &gDarwinMinorVersion, &gDarwinRevision) != 3) {
		if (sscanf(osversion, "%u.%u", &gDarwinMajorVersion, &gDarwinMinorVersion) != 2) {
			printf("sscanf() failed\n");
			return;
		}
	}
}

int DarwinMajorVersion() {
	if (gDarwinMajorVersion < 0) {
		GetDarwinVersion();
	}
	return gDarwinMajorVersion;
}

int DarwinMinorVersion() {
	if (gDarwinMajorVersion < 0) {
		GetDarwinVersion();
	}
	return gDarwinMinorVersion;
}

int DarwinRevision() {
	if (gDarwinMajorVersion < 0) {
		GetDarwinVersion();
	}
	return gDarwinRevision;
}

char * MacOSVersion() {
	if (!gMacOSVersion[0]) {
		char osversion[32];
		size_t osversion_len = sizeof(osversion) - 1;

		if (sysctlbyname("kern.osproductversion", osversion, &osversion_len, NULL, 0) == 0) {
			snprintf(gMacOSVersion, sizeof(gMacOSVersion), "%.*s", (int)osversion_len, osversion);
			if (!strcmp(gMacOSVersion, "10.16")) {
				gMacOSVersion[0] = '\0';
			}
		}
		
		if (!gMacOSVersion[0]) {
			snprintf(gMacOSVersion, sizeof(gMacOSVersion), "%d.%d.%d",
				(DarwinMajorVersion() < 20) ? 10 : DarwinMajorVersion() - 9,
				(DarwinMajorVersion() == 1) ? (DarwinMinorVersion() == 4 ? 1 : 0) : (DarwinMajorVersion() < 20) ? DarwinMajorVersion() - 4 : DarwinMinorVersion(),
				(DarwinMajorVersion() < 20) ? DarwinMinorVersion() : DarwinRevision() // this is not always accurrate
			);
		}
	}
	return gMacOSVersion;
}

static IOReturn I2CSendRequest_10_5( IOI2CConnectRef connect, IOOptionBits options, IOI2CRequest_10_6 * request )
{
	kern_return_t kr;

	IOI2CBuffer buffer;

	if( request->sendBytes > kIOI2CInlineBufferBytes)
		return( kIOReturnOverrun );
	if( request->replyBytes > kIOI2CInlineBufferBytes)
		return( kIOReturnOverrun );

	if (request->sendAddress > 255) {
		return kIOReturnBadArgument;
	}

	if (request->replyAddress > 255) {
		return kIOReturnBadArgument;
	}

	#if defined(__LP64__)
	kr = IOConnectCallMethod(connect->connect, 0,       // Index
				NULL,    0, NULL,    0,                 // Input
				NULL, NULL, NULL, NULL);                // Output
	#else
	kr = IOConnectMethodScalarIScalarO( connect->connect, 0, 0, 0 );
	#endif
	if( kIOReturnSuccess != kr)
		return( kr );

	void *inlineBuffer;
	size_t len;
	uint32_t *replyBytes;
	
	if (!strcmp(MachineType(), "i386")) {
		ConvertIOI2CRequest(request, &buffer.buf_i386.request, 0);
		buffer.buf_i386.request.replyBuffer = 0;
		buffer.buf_i386.request.sendBuffer  = 0;
		replyBytes = &buffer.buf_i386.request.replyBytes;
		inlineBuffer = &buffer.buf_i386.inlineBuffer;
		len = sizeof(buffer.buf_i386);
	}
	else if (!strcmp(MachineType(), "ppc")) {
		ConvertIOI2CRequest(request, &buffer.buf_ppc.request, 0);
		buffer.buf_ppc.request.replyBuffer = 0;
		buffer.buf_ppc.request.sendBuffer  = 0;
		replyBytes = &buffer.buf_i386.request.replyBytes;
		inlineBuffer = &buffer.buf_ppc.inlineBuffer;
		len = sizeof(buffer.buf_ppc);
	}
	else {
		ConvertIOI2CRequest(request, &buffer.buf_def.request, 0);
		buffer.buf_def.request.replyBuffer = 0;
		buffer.buf_def.request.sendBuffer  = 0;
		replyBytes = &buffer.buf_i386.request.replyBytes;
		inlineBuffer = &buffer.buf_def.inlineBuffer;
		len = sizeof(buffer.buf_def);
	}

	if( request->sendBytes)
		bcopy( (void *) request->sendBuffer, inlineBuffer, request->sendBytes );

	#if defined(__LP64__)
	kr = IOConnectCallMethod(connect->connect, 2,       // Index
				NULL,    0, &buffer, len,               // Input
				NULL, NULL, &buffer, &len);             // Output
	#else
	kr = IOConnectMethodStructureIStructureO(connect->connect, 2, // Index
				len, &len, &buffer, &buffer);
	#endif

	if( *replyBytes)
		bcopy( inlineBuffer, (void *)  request->replyBuffer, *replyBytes );

	if (!strcmp(MachineType(), "i386")) {
		ConvertIOI2CRequest(&buffer.buf_i386.request, request, 1);
	}
	else if (!strcmp(MachineType(), "ppc")) {
		ConvertIOI2CRequest(&buffer.buf_ppc.request, request, 1);
	}
	else {
		ConvertIOI2CRequest(&buffer.buf_def.request, request, 1);
	}

	#if defined(__LP64__)
	IOConnectCallMethod(connect->connect, 1,       // Index
				NULL,    0, NULL,    0,            // Input
				NULL, NULL, NULL, NULL);           // Output
	#else
	IOConnectMethodScalarIScalarO( connect->connect, 1, 0, 0 );
	#endif

	return kr;
}

IOReturn UniversalI2CSendRequest( IOI2CConnectRef connect, IOOptionBits options, IOI2CRequest_10_6 * request )
{
	kern_return_t kr;
	if (DarwinMajorVersion() <= 9) { // 10.5
		#if 1
			kr = I2CSendRequest_10_5(connect, options, request);
		#else
			IOI2CRequest_10_5_0_user request_10_5_0;
			ConvertIOI2CRequest(request, &request_10_5_0, 0);
			kr = IOI2CSendRequest(connect, options, (IOI2CRequest*)&request_10_5_0);
			ConvertIOI2CRequest(&request_10_5_0, request, 1);
		#endif
	}
	else {
		kr = IOI2CSendRequest(connect, options, (IOI2CRequest*)request);
	}
	return kr;
}

void DumpAllBundles()
{
	CFBundleRef bundle = NULL;
	CFArrayRef allbundles = CFBundleGetAllBundles();
	CFIndex bundleCount = CFArrayGetCount(allbundles);
	for (int i = 0; i < bundleCount; i++) {
		bundle = (CFBundleRef)CFArrayGetValueAtIndex(allbundles, i);
		CFStringRef identifier = CFBundleGetIdentifier(bundle);
		if (identifier) {
			char identifierStr[256];
			CFStringGetCString(identifier, identifierStr, sizeof(identifierStr), kCFStringEncodingUTF8);
			printf("%d %s\n", i, identifierStr);
		}
		else {
			printf("%d no identifier\n", i);
		}
	}
}

void DumpAllImages()
{
	uint32_t imageCount = _dyld_image_count();
	for (int i = 0; i < imageCount; i++) {
		const char* image_name = _dyld_get_image_name(i);
		if (image_name) {
			printf("%d %s\n", i, image_name);
		}
		else {
			printf("%d no identifier\n", i);
		}
	}
}


static bool triedIODisplayForFramebuffer = false;
static IODisplayForFramebufferPtr gIODisplayForFramebuffer = NULL;

IODisplayForFramebufferPtr GetIODisplayForFramebufferPtr()
{
	if (!gIODisplayForFramebuffer && !triedIODisplayForFramebuffer) {
#if MAC_OS_X_VERSION_SDK == MAC_OS_X_VERSION_10_5
		// IODisplayForFramebuffer is exported in 10.1 to 10.4 and 10.6 and later but not 10.5.
		// This code is here for compiling with 10.5 sdk. It will work when Allrez10.5 is run on non-10.5 macOS versions.
		//DumpAllBundles();
		//DumpAllImages();
		CFBundleRef bundle = NULL;
		if ((bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.framework.IOKit")))) {
			gIODisplayForFramebuffer = (IODisplayForFramebufferPtr)CFBundleGetFunctionPointerForName(bundle, CFSTR("IODisplayForFramebuffer"));
			if (!gIODisplayForFramebuffer) {
				gIODisplayForFramebuffer = (IODisplayForFramebufferPtr)dlsym(RTLD_DEFAULT, "IODisplayForFramebuffer");
			}
		}
		triedIODisplayForFramebuffer = true;
#else
		gIODisplayForFramebuffer = &IODisplayForFramebuffer;
#endif
	}
	return gIODisplayForFramebuffer;
}


// https://github.com/apple-oss-distributions/IOKitUser/blob/IOKitUser-1445.71.1/graphics.subproj/IOGraphicsLib.c#L3284

// 837
static void
AdjustTimingForInterlace( IODetailedTimingInformationV2_12 * timing )
{
    timing->signalConfig          |= kIOInterlacedCEATiming;
    timing->verticalActive         = (timing->verticalActive << 1);
    timing->verticalBlanking       = (timing->verticalBlanking << 1) | 1;
    timing->verticalSyncPulseWidth = (timing->verticalSyncPulseWidth << 1);
    timing->verticalSyncOffset     = (timing->verticalSyncOffset << 1) | 1;
    timing->verticalBorderTop      = (timing->verticalBorderTop << 1);
    timing->verticalBorderBottom   = (timing->verticalBorderBottom << 1);
}

IOReturn
EDIDDescToDetailedTiming( EDID * edid, EDIDDetailedTimingDesc * desc,
                            IODetailedTimingInformationV2_12 * timing )
{
    bool interlaced;

    bzero( timing, sizeof( IODetailedTimingInformationV2_12 ) );

    if( !desc->clock)
        return( kIOReturnBadArgument );

    timing->signalConfig                = (edid->displayParams[0] & 16)
                                        ? kIOAnalogSetupExpected : 0;
    interlaced = (0 != (desc->flags & 0x80));

    timing->signalLevels                = (edid->displayParams[0] >> 5) & 3;

    timing->pixelClock                  = ((UInt64) OSReadLittleInt16(&desc->clock, 0))
                                        * 10000ULL;
    timing->maxPixelClock               = timing->pixelClock;
    timing->minPixelClock               = timing->pixelClock;

    timing->horizontalActive            = desc->horizActive
                                        | ((desc->horizHigh & 0xf0) << 4);
    timing->horizontalBlanking          = desc->horizBlanking
                                        | ((desc->horizHigh & 0x0f) << 8);

    timing->verticalActive              = desc->verticalActive
                                        | ((desc->verticalHigh & 0xf0) << 4);
    timing->verticalBlanking            = desc->verticalBlanking
                                        | ((desc->verticalHigh & 0x0f) << 8);

    timing->horizontalSyncOffset        = desc->horizSyncOffset
                                        | ((desc->syncHigh & 0xc0) << 2);
    timing->horizontalSyncPulseWidth    = desc->horizSyncWidth
                                        | ((desc->syncHigh & 0x30) << 4);

    timing->verticalSyncOffset          = ((desc->verticalSyncOffsetWidth & 0xf0) >> 4)
                                        | ((desc->syncHigh & 0x0c) << 2);
    timing->verticalSyncPulseWidth      = ((desc->verticalSyncOffsetWidth & 0x0f) >> 0)
                                        | ((desc->syncHigh & 0x03) << 4);

    timing->horizontalBorderLeft        = desc->horizBorder;
    timing->horizontalBorderRight       = desc->horizBorder;
    timing->verticalBorderTop           = desc->verticalBorder;
    timing->verticalBorderBottom        = desc->verticalBorder;

    timing->horizontalSyncConfig        = (desc->flags & 2)
                                        ? kIOSyncPositivePolarity : 0;
    timing->horizontalSyncLevel         = 0;
    timing->verticalSyncConfig          = (desc->flags & 4)
                                        ? kIOSyncPositivePolarity : 0;
    timing->verticalSyncLevel           = 0;

    if (interlaced)
        AdjustTimingForInterlace(timing);

    return( kIOReturnSuccess );
}

static void
MaxTimingRangeRec( IODisplayTimingRangeV1_12 * range )
{
	bzero( range, sizeof( IODisplayTimingRangeV1_12) );

	range->supportedSyncFlags                   = 0xffffffff;
	range->supportedSignalLevels                = 0xffffffff;
	range->supportedSignalConfigs               = 0xffffffff;

	range->maxFrameRate                         = 0xffffffff;
	range->maxLineRate                          = 0xffffffff;
	range->maxPixelClock                        = 0xffffffff;
	range->maxPixelError                        = 0xffffffff;

	range->maxHorizontalTotal                   = 0xffffffff;
	range->maxVerticalTotal                     = 0xffffffff;
	range->maxHorizontalActiveClocks            = 0xffffffff;
	range->maxHorizontalBlankingClocks          = 0xffffffff;
	range->maxHorizontalSyncOffsetClocks        = 0xffffffff;
	range->maxHorizontalPulseWidthClocks        = 0xffffffff;
	range->maxVerticalActiveClocks              = 0xffffffff;
	range->maxVerticalBlankingClocks            = 0xffffffff;
	range->maxVerticalSyncOffsetClocks          = 0xffffffff;
	range->maxVerticalPulseWidthClocks          = 0xffffffff;
	range->maxHorizontalBorderLeft              = 0xffffffff;
	range->maxHorizontalBorderRight             = 0xffffffff;
	range->maxVerticalBorderTop                 = 0xffffffff;
	range->maxVerticalBorderBottom              = 0xffffffff;

	range->charSizeHorizontalActive             = 1;
	range->charSizeHorizontalBlanking           = 1;
	range->charSizeHorizontalSyncOffset         = 1;
	range->charSizeHorizontalSyncPulse          = 1;
	range->charSizeVerticalActive               = 1;
	range->charSizeVerticalBlanking             = 1;
	range->charSizeVerticalSyncOffset           = 1;
	range->charSizeVerticalSyncPulse            = 1;
	range->charSizeHorizontalBorderLeft         = 1;
	range->charSizeHorizontalBorderRight        = 1;
	range->charSizeVerticalBorderTop            = 1;
	range->charSizeVerticalBorderBottom         = 1;
	range->charSizeHorizontalTotal              = 1;
	range->charSizeVerticalTotal                = 1;
}

Boolean
EDIDDescToDisplayTimingRangeRec( EDID * edid, EDIDGeneralDesc * desc,
								IODisplayTimingRangeV1_12 * range )
{
	UInt8 byte;

	if( !edid || (edid->version < 1) || (edid->revision < 1))
		return( false );

	if( desc->flag1 || desc->flag2 || desc->flag3)
		return( false );
	if( 0xfd != desc->type)
		return( false );

	MaxTimingRangeRec( range );

	byte = edid->displayParams[0];
	if (!(0x80 & byte))
	{
		range->supportedSignalLevels  = 1 << ((byte >> 5) & 3);
		range->supportedSyncFlags     = ((byte & 1) ? kIORangeSupportsVSyncSerration : 0)
									  | ((byte & 2) ? kIORangeSupportsSyncOnGreen : 0)
									  | ((byte & 4) ? kIORangeSupportsCompositeSync : 0)
									  | ((byte & 8) ? kIORangeSupportsSeparateSyncs : 0);
	}

	range->supportedSignalConfigs = kIORangeSupportsInterlacedCEATiming;

	range->minVerticalPulseWidthClocks   = 1;
	range->minHorizontalPulseWidthClocks = 1;

	range->minFrameRate  = desc->data[0];
	range->maxFrameRate  = desc->data[1];
	range->minLineRate   = desc->data[2] * 1000;
	range->maxLineRate   = desc->data[3] * 1000;
	range->maxPixelClock = desc->data[4] * 10000000ULL;

	range->minHorizontalActiveClocks = 640;
	range->minVerticalActiveClocks   = 480;

	if( range->minLineRate)
		range->maxHorizontalActiveClocks = (UInt32)(range->maxPixelClock / range->minLineRate);
	if( range->minFrameRate)
		range->maxVerticalActiveClocks   = (UInt32)(range->maxPixelClock / (range->minHorizontalActiveClocks * range->minFrameRate));

	return( true );
}
