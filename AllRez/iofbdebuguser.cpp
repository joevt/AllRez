//
//  iofbdebuguser.cpp
//  AllRez
//
//  Created by joevt on 2022-05-19.
//

#include "AppleMisc.h"
#include "iofbdebuguser.h"
#include <CoreFoundation/CFNumber.h>
#include <CoreGraphics/CGDisplayConfiguration.h>
#include <IOKit/graphics/IOGraphicsTypes.h>
#include "printf.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <IOKit/i2c/IOI2CInterface.h>

#ifdef __cplusplus
}
#endif

IOReturn IofbSetAttributeForService(io_service_t ioFramebufferService, UInt32 category, UInt32 val1, UInt32 val2, UInt32 val3, UInt32 *valGet) {
	IOReturn result;
	IOItemCount i2cInterfaceCount;
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
							if (CFNumberGetValue((CFNumberRef)cf_IOI2CTransactionTypes, kCFNumberSInt32Type, (void*)&val_IOI2CTransactionTypes)) {

								IOI2CConnectRef i2cconnect;
								result = IOI2CInterfaceOpen(i2cservice, kNilOptions, &i2cconnect);
								if (KERN_SUCCESS == result) {
									
									if (val_IOI2CTransactionTypes & (1 << kIOI2CSimpleTransactionType)) {
										IOI2CRequest request;
										UInt32 buffer[] = { category, val1, val2, val3 };
										bzero(&request, sizeof(request));
										request.sendTransactionType = kIOI2CSimpleTransactionType;
										request.sendAddress = 0xfb;
										request.sendSubAddress = 0xfb;
										request.sendBuffer = (vm_address_t)&buffer;
										request.sendBytes = sizeof(buffer);
										UInt32 valGet0 = 0;
										if (valGet) {
											valGet0 = *valGet;
											request.replyTransactionType = kIOI2CSimpleTransactionType;
											request.replyAddress = 0xfb;
											request.replySubAddress = 0xfb;
											request.replyBuffer = (vm_address_t)&valGet0;
											request.replyBytes = sizeof(*valGet);
										}
										result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
										if (result == kIOReturnSuccess) {
											result = request.result;
										}
										
										if (!result && valGet) {
											*valGet = valGet0;
										}

#if 0
										if (result) {
											char resultStr[40];
											iprintf("IofbSetAttributeForService result:%s\n", DumpOneReturn(resultStr, sizeof(resultStr), request.result));
										}
#endif
									} // if kIOI2CDisplayPortNativeTransactionType
									
									IOI2CInterfaceClose(i2cconnect, kNilOptions);
								} // IOI2CInterfaceOpen
							} // if CFNumberGetValue
						} // if CFNumberGetTypeID
					} // if cf_IOI2CTransactionTypes
				} // if IORegistryEntryCreateCFProperties
			} // if IOFBCopyI2CInterfaceForBus
		} // for iterfaceBus
	} // if IOFBGetI2CInterfaceCount
	return result;
} // IofbSetAttributeForService

IOReturn IofbGetSetAttributeForDisplay(int displayIndex, UInt32 category, UInt32 val1, UInt32 val2, UInt32 val3, UInt32 *valGet) {
	CGDirectDisplayID onlineDisplays[20];
	uint32_t displayCount;
	CGGetOnlineDisplayList(20, onlineDisplays, &displayCount);
	
	if (displayIndex >= displayCount) {
		iprintf("displayIndex %d out of range\n", displayIndex);
		return kIOReturnBadArgument;
	}
	
	CGDirectDisplayID display = onlineDisplays[displayIndex];
	io_service_t ioFramebufferService;
	CGError result = CGSServiceForDisplayNumber(display, &ioFramebufferService);
	if (result) {
		iprintf("Error %x service for displayIndex %d\n", result, displayIndex);
		return result;
	}
	
	return IofbSetAttributeForService(ioFramebufferService, category, val1, val2, val3, valGet);
} // IofbGetSetAttributeForDisplay

IOReturn IofbSetAttributeForDisplay(int displayIndex, UInt32 category, UInt32 val1, UInt32 val2, UInt32 val3) {
	return IofbGetSetAttributeForDisplay(displayIndex, category, val1, val2, val3, NULL);
} // IofbSetAttributeForDisplay

IOReturn IofbGetAttributeForDisplay(int displayIndex, UInt32 category, UInt32 val1, UInt32 val2, UInt32 *valGet) {
	if (!valGet)
		return kIOReturnBadArgument;
	return IofbGetSetAttributeForDisplay(displayIndex, category, val1, val2, 0, valGet);
} // IofbSetAttributeForDisplay


void SetIofbDebugEnabled(bool debugEnabled) {
	IofbSetAttributeForDisplay(0, 'iofb', 'dbge', debugEnabled, 0);
} // SetIofbDebugEnabled

void DoAttributeTest(void) {
	SetIofbDebugEnabled(true);
	IofbSetAttributeForDisplay(1, 'atfc', 0, kConnectionColorDepthsSupported, -1);
	IofbSetAttributeForDisplay(1, 'atfc', 0, kConnectionColorModesSupported, -1);
#if 0
	IofbSetAttributeForDisplay(1, 'atfc', 0, kConnectionControllerColorDepth, kIODisplayRGBColorComponentBits6);
	IofbSetAttributeForDisplay(1, 'atfc', 0, kConnectionColorMode, kIODisplayColorModeYCbCr422);
	IofbSetAttributeForDisplay(1, 'atfc', 0, kConnectionControllerColorDepth, kIODisplayRGBColorComponentBits6);
	IofbSetAttributeForDisplay(1, 'atfc', 0, kConnectionColorMode, kIODisplayColorModeRGB); // changing color between 422 and RGB causes the screen to black out if the last depth does not match
	IofbSetAttributeForDisplay(1, 'atfc', 0, kConnectionControllerColorDepth, kIODisplayRGBColorComponentBits6);
#endif
	IofbSetAttributeForDisplay(1, 'atfc', 0, kConnectionControllerColorDepth, kIODisplayYCbCr422ColorComponentBits8);
	IofbSetAttributeForDisplay(1, 'atfc', 0, kConnectionColorMode, kIODisplayColorModeRGB);
	IofbSetAttributeForDisplay(1, 'atfc', 0, kConnectionControllerColorDepth, kIODisplayYCbCr422ColorComponentBits8);
	IofbSetAttributeForDisplay(1, 'atfc', 0, kConnectionColorMode, kIODisplayColorModeYCbCr422); // changing color between 422 and RGB causes the screen to black out if the last depth does not match
	IofbSetAttributeForDisplay(1, 'atfc', 0, kConnectionControllerColorDepth, kIODisplayYCbCr422ColorComponentBits8);
	
//  IofbSetAttributeForDisplay(1, 'atfc', 0, kConnectionControllerColorDepth, kIODisplayYCbCr422ColorComponentBits8);
	IofbSetAttributeForDisplay(1, 'atfc', 0, kConnectionFlushParameters, 1);

#if 0
kIODisplayColorModeReserved   = 0x00000000,
kIODisplayColorModeRGB        = 0x00000001,
kIODisplayColorModeYCbCr422   = 0x00000010,
kIODisplayColorModeYCbCr444   = 0x00000100,
kIODisplayColorModeRGBLimited = 0x00001000,
kIODisplayColorModeAuto       = 0x10000000,

case kConnectionColorDepthsSupported:
case kConnectionControllerDepthsSupported:
case kConnectionControllerColorDepth:
	scnprintf(buf, bufSize, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
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
#endif

} // DoAttributeTest
