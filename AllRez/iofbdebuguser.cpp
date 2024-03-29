//
//  iofbdebuguser.cpp
//  AllRez
//
//  Created by joevt on 2022-05-19.
//

#include "MacOSMacros.h"
#include "iofbdebuguser.h"
#include <CoreFoundation/CFNumber.h>
#include <ApplicationServices/ApplicationServices.h>
#include "AppleMisc.h"
#include <IOKit/graphics/IOGraphicsTypes.h>
#include "printf.h"
#ifdef __cplusplus
#include <cerrno>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <IOKit/IOTypes.h>
#include <IOKit/i2c/IOI2CInterface.h>

#ifdef __cplusplus
}
#endif

IOReturn IoFbDoRequest(io_service_t ioFramebufferService, UInt8 *sendBuf, UInt32 sendSize, UInt8 *replyBuf, UInt32 replySize) {
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
										IOI2CRequest_10_6 request;
										bzero(&request, sizeof(request));
										request.sendTransactionType = kIOI2CSimpleTransactionType;
										request.sendAddress = 0xfb;
										request.sendSubAddress = 0xfb;
										request.sendBuffer = (vm_address_t)sendBuf;
										request.sendBytes = sendSize;
										if (replyBuf) {
											request.replyTransactionType = kIOI2CSimpleTransactionType;
											request.replyAddress = 0xfb;
											request.replySubAddress = 0xfb;
											request.replyBuffer = (vm_address_t)replyBuf;
											request.replyBytes = replySize;
										}
										result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &request);
										if (result == kIOReturnSuccess) {
											result = request.result;
										}
#if 0
										if (result) {
											char resultStr[40];
											iprintf("IoFbDoRequest result:%s\n", DumpOneReturn(resultStr, sizeof(resultStr), request.result));
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

IOReturn IofbGetService(int displayIndex, io_service_t *ioFramebufferService) {
	CGDirectDisplayID onlineDisplays[20];
	uint32_t displayCount;
	CGError result = CGGetOnlineDisplayList(20, onlineDisplays, &displayCount);
	if (result)
		return result;
	
	if (displayIndex >= displayCount) {
		iprintf("displayIndex %d out of range\n", displayIndex);
		return kIOReturnBadArgument;
	}
	
	io_service_t ioFramebufferServiceTemp = IO_OBJECT_NULL;
	if (!ioFramebufferService)
		ioFramebufferService = &ioFramebufferServiceTemp;
	
	CGDirectDisplayID display = onlineDisplays[displayIndex];
	result = CGSServiceForDisplayNumber(display, ioFramebufferService);
	if (result) {
		iprintf("Error %x service for displayIndex %d\n", result, displayIndex);
		*ioFramebufferService = IO_OBJECT_NULL;
	}
	return result;
}

IOReturn IofbSetAttributeForService(io_service_t ioFramebufferService, UInt32 category, UInt32 val1, UInt32 val2, UInt32 val3, UInt32 *valGet) {
	UInt32 buffer[] = { category, val1, val2, val3 };
	UInt32 valGet0 = 0;
	if (valGet)
		valGet0 = *valGet;
	IOReturn result = IoFbDoRequest(ioFramebufferService, (UInt8*)buffer, (UInt32)sizeof(buffer), valGet ? (UInt8*)&valGet0 : NULL, valGet ? (UInt32)sizeof(*valGet) : 0);
	if (!result && valGet) {
		*valGet = valGet0;
	}
	return result;
} // IofbSetAttributeForService

IOReturn IofbSetEDIDOverride(UInt8 *orgEDID, UInt32 orgSize, UInt8 *newEDID, UInt32 newSize) {
	io_service_t ioFramebufferService = IO_OBJECT_NULL;
	IOReturn result = IofbGetService(0, &ioFramebufferService);
	if (!result) {
		UInt32 bufferSize = (UInt32)sizeof(UInt32) * 5 + orgSize + newSize;
		UInt32* buf = (UInt32*)malloc(bufferSize);
		if (!buf) return errno;
		buf[0] = 'iofb';
		buf[1] = 'edid';
		buf[2] = orgSize;
		buf[3] = newSize;
		memcpy(&buf[4], orgEDID, orgSize);
		memcpy(((UInt8*)(&buf[4])) + orgSize, newEDID, newSize);
		result = IoFbDoRequest(ioFramebufferService, (UInt8*)buf, bufferSize, NULL, 0);
	}
	return result;
} // IofbSetEDIDOverride

IOReturn IofbGetSetAttributeForDisplay(int displayIndex, UInt32 category, UInt32 val1, UInt32 val2, UInt32 val3, UInt32 *valGet) {
	io_service_t ioFramebufferService = IO_OBJECT_NULL;
	IOReturn result = IofbGetService(displayIndex, &ioFramebufferService);
	if (!result) {
		result = IofbSetAttributeForService(ioFramebufferService, category, val1, val2, val3, valGet);
	}
	return result;
} // IofbGetSetAttributeForDisplay

IOReturn IofbSetAttributeForDisplay(int displayIndex, UInt32 category, UInt32 val1, UInt32 val2, UInt32 val3) {
	return IofbGetSetAttributeForDisplay(displayIndex, category, val1, val2, val3, NULL);
} // IofbSetAttributeForDisplay

IOReturn IofbGetAttributeForDisplay(int displayIndex, UInt32 category, UInt32 val1, UInt32 val2, UInt32 *valGet) {
	if (!valGet)
		return kIOReturnBadArgument;
	return IofbGetSetAttributeForDisplay(displayIndex, category, val1, val2, kIOFBUnused, valGet);
} // IofbSetAttributeForDisplay


void IofbSetDebugEnabled(bool debugEnabled) {
	IofbSetAttributeForDisplay(0, 'iofb', 'dbge', debugEnabled, kIOFBUnused);
} // IofbSetDebugEnabled

bool IofbAvailable(int displayIndex) {
	UInt32 iofbtest = 0;
	IofbGetAttributeForDisplay(displayIndex, 'iofb', 'iofb', kIOFBUnused, &iofbtest);
	return (iofbtest == 'iofb');
}

void IofbSetControllerColorModeAndDepth(int displayIndex, UInt32 mode, UInt32 depth) {
	// changing color between 422 and RGB causes the screen to black out if the last depth does not match

	UInt32 currentMode;
	IofbGetAttributeForDisplay(displayIndex, 'atfc', kIOFBConnectIndex0, kConnectionColorMode, &currentMode);
	UInt32 currentDepth;
	IofbGetAttributeForDisplay(displayIndex, 'atfc', kIOFBConnectIndex0, kConnectionControllerColorDepth, &currentDepth);

	UInt32 tempMode;
	switch (mode) {
		case kIODisplayColorModeYCbCr422 :
		case kIODisplayColorModeYCbCr444 : tempMode = kIODisplayColorModeRGB; break;
		default                          : tempMode = kIODisplayColorModeYCbCr422; break;
	}
	
	IofbSetAttributeForDisplay(displayIndex, 'atfc', kIOFBConnectIndex0, kConnectionControllerColorDepth, depth);
	IofbSetAttributeForDisplay(displayIndex, 'atfc', kIOFBConnectIndex0, kConnectionColorMode, tempMode);
	IofbSetAttributeForDisplay(displayIndex, 'atfc', kIOFBConnectIndex0, kConnectionControllerColorDepth, depth);
	IofbSetAttributeForDisplay(displayIndex, 'atfc', kIOFBConnectIndex0, kConnectionColorMode, mode);
	IofbSetAttributeForDisplay(displayIndex, 'atfc', kIOFBConnectIndex0, kConnectionControllerColorDepth, depth);
	IofbSetAttributeForDisplay(displayIndex, 'atfc', kIOFBConnectIndex0, kConnectionFlushParameters, true);
}

void DoAttributeTest(int displayIndex) {
	if (IofbAvailable(displayIndex)) {
		// works on iMac14,2 (NVIDIA GeForce GTX 780M 4 GB) but not MacPro3,1 (NVIDIA GeForce GTX 680 2 GB). The driver accepts the attributes but doesn't update the display.
		IofbSetControllerColorModeAndDepth(displayIndex, kIODisplayColorModeRGB     , kIODisplayRGBColorComponentBits10    ); // 30 bpp (default color depth for Nvidia Kepler)
		IofbSetControllerColorModeAndDepth(displayIndex, kIODisplayColorModeRGB     , kIODisplayRGBColorComponentBits8     ); // 24 bpp
		IofbSetControllerColorModeAndDepth(displayIndex, kIODisplayColorModeRGB     , kIODisplayRGBColorComponentBits6     ); // 18 bpp (low color depth = banding)
		IofbSetControllerColorModeAndDepth(displayIndex, kIODisplayColorModeYCbCr422, kIODisplayYCbCr422ColorComponentBits8); // 16 bpp (4:2:2 chroma subsampling = low horizontal color resolution)
	}
	else {
		iprintf("Iofb is not available.\n");
	}
} // DoAttributeTest

void DoEDIDOverrideTest() {
	UInt8 AcerXV273JleftPartial[] =
	{
		0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x04, 0x72, 0xb1, 0x06, 0x53, 0x8f, 0x00, 0x85,
		0x32, 0x1c, 0x01, 0x04, 0xb5, 0x3c, 0x22, 0x78, 0x3b, 0x27, 0x11, 0xac, 0x51, 0x35, 0xb5, 0x26,
		0x0e, 0x50, 0x54, 0x23, 0x48, 0x00, 0x81, 0x40, 0x81, 0x80, 0x81, 0xc0, 0x81, 0x00, 0x95, 0x00,
		0xb3, 0x00, 0xd1, 0xc0, 0x01, 0x01, 0x4d, 0xd0, 0x00, 0xa0, 0xf0, 0x70, 0x3e, 0x80, 0x30, 0x20,
		0x35, 0x00, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1a, 0xb4, 0x66, 0x00, 0xa0, 0xf0, 0x70, 0x1f, 0x80,
		0x08, 0x20, 0x18, 0x04, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0xfd, 0x0c, 0x30,
		0x78, 0xff, 0xff, 0x6b, 0x01, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc,
		0x00, 0x58, 0x56, 0x32, 0x37, 0x33, 0x4b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x02, 0x39,

		0x02, 0x03, 0x48, 0xf1, 0x51, 0x01, 0x03, 0x04, 0x12, 0x13, 0x05, 0x14, 0x1f, 0x90, 0x07, 0x02,
		0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x3f, 0x23, 0x09, 0x07, 0x07, 0x83, 0x01, 0x00, 0x00, 0xe2, 0x00,
		0xc0, 0x6d, 0x03, 0x0c, 0x00, 0x10, 0x00, 0x38, 0x78, 0x20, 0x00, 0x60, 0x01, 0x02, 0x03, 0x68,
		0x1a, 0x00, 0x00, 0x01, 0x01, 0x30, 0x78, 0x00, 0xe3, 0x05, 0xe3, 0x01, 0xe4, 0x0f, 0x00, 0xc0,
		0x00, 0xe6, 0x06, 0x07, 0x01, 0x61, 0x56, 0x1c, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d, 0x40,
		0x58, 0x2c, 0x45, 0x00, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1e, 0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0,
		0x1e, 0x20, 0x6e, 0x28, 0x55, 0x00, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1e, 0x6f, 0xc2, 0x00, 0xa0,
		0xa0, 0xa0, 0x55, 0x50, 0x30, 0x20, 0x35, 0x00, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1e, 0x00, 0x51,
	};

	UInt8 AcerXV273JrightPartial[] =
	{
		0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x04, 0x72, 0xb1, 0x06, 0x53, 0x8f, 0x00, 0x85,
		0x32, 0x1c, 0x01, 0x04, 0xb5, 0x3c, 0x22, 0x78, 0x3b, 0x27, 0x11, 0xac, 0x51, 0x35, 0xb5, 0x26,
		0x0e, 0x50, 0x54, 0x23, 0x48, 0x00, 0x81, 0x40, 0x81, 0x80, 0x81, 0xc0, 0x81, 0x00, 0x95, 0x00,
		0xb3, 0x00, 0xd1, 0xc0, 0x01, 0x01, 0x4d, 0xd0, 0x00, 0xa0, 0xf0, 0x70, 0x3e, 0x80, 0x30, 0x20,
		0x35, 0x00, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1a, 0xb4, 0x66, 0x00, 0xa0, 0xf0, 0x70, 0x1f, 0x80,
		0x08, 0x20, 0x18, 0x04, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0xfd, 0x0c, 0x30,
		0x78, 0xff, 0xff, 0x6b, 0x01, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc,
		0x00, 0x58, 0x56, 0x32, 0x37, 0x33, 0x4b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x02, 0x39,

		0x02, 0x03, 0x48, 0xf1, 0x51, 0x01, 0x03, 0x04, 0x12, 0x13, 0x05, 0x14, 0x1f, 0x90, 0x07, 0x02,
		0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x3f, 0x23, 0x09, 0x07, 0x07, 0x83, 0x01, 0x00, 0x00, 0xe2, 0x00,
		0xc0, 0x6d, 0x03, 0x0c, 0x00, 0x20, 0x00, 0x38, 0x78, 0x20, 0x00, 0x60, 0x01, 0x02, 0x03, 0x68,
		0x1a, 0x00, 0x00, 0x01, 0x01, 0x30, 0x78, 0x00, 0xe3, 0x05, 0xe3, 0x01, 0xe4, 0x0f, 0x00, 0xc8,
		0x00, 0xe6, 0x06, 0x07, 0x01, 0x61, 0x56, 0x1c, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d, 0x40,
		0x58, 0x2c, 0x45, 0x00, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1e, 0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0,
		0x1e, 0x20, 0x6e, 0x28, 0x55, 0x00, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1e, 0x6f, 0xc2, 0x00, 0xa0,
		0xa0, 0xa0, 0x55, 0x50, 0x30, 0x20, 0x35, 0x00, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1e, 0x00, 0x39,
	};

	UInt8 DellUP3218Kleft[] = {
		0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x10, 0xac, 0x47, 0x41, 0x4c, 0x34, 0x32, 0x30,
		0x27, 0x1c, 0x01, 0x04, 0xb5, 0x46, 0x27, 0x78, 0x3a, 0x76, 0x45, 0xae, 0x51, 0x33, 0xba, 0x26,
		0x0d, 0x50, 0x54, 0xa5, 0x4b, 0x00, 0x81, 0x00, 0xb3, 0x00, 0xd1, 0x00, 0xa9, 0x40, 0x81, 0x80,
		0xd1, 0xc0, 0x01, 0x01, 0x01, 0x01, 0x4d, 0xd0, 0x00, 0xa0, 0xf0, 0x70, 0x3e, 0x80, 0x30, 0x20,
		0x35, 0x00, 0xba, 0x89, 0x21, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0xff, 0x00, 0x46, 0x46, 0x4e,
		0x58, 0x4d, 0x38, 0x39, 0x50, 0x30, 0x32, 0x34, 0x4c, 0x0a, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x44,
		0x45, 0x4c, 0x4c, 0x20, 0x55, 0x50, 0x33, 0x32, 0x31, 0x38, 0x4b, 0x0a, 0x00, 0x00, 0x00, 0xfd,
		0x00, 0x18, 0x4b, 0x1e, 0xb4, 0x6c, 0x01, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x02, 0x59,

		0x02, 0x03, 0x1d, 0xf1, 0x50, 0x10, 0x1f, 0x20, 0x05, 0x14, 0x04, 0x13, 0x12, 0x11, 0x03, 0x02,
		0x16, 0x15, 0x07, 0x06, 0x01, 0x23, 0x09, 0x1f, 0x07, 0x83, 0x01, 0x00, 0x00, 0xa3, 0x66, 0x00,
		0xa0, 0xf0, 0x70, 0x1f, 0x80, 0x30, 0x20, 0x35, 0x00, 0xba, 0x89, 0x21, 0x00, 0x00, 0x1a, 0x56,
		0x5e, 0x00, 0xa0, 0xa0, 0xa0, 0x29, 0x50, 0x30, 0x20, 0x35, 0x00, 0xba, 0x89, 0x21, 0x00, 0x00,
		0x1a, 0x7c, 0x39, 0x00, 0xa0, 0x80, 0x38, 0x1f, 0x40, 0x30, 0x20, 0x3a, 0x00, 0xba, 0x89, 0x21,
		0x00, 0x00, 0x1a, 0xa8, 0x16, 0x00, 0xa0, 0x80, 0x38, 0x13, 0x40, 0x30, 0x20, 0x3a, 0x00, 0xba,
		0x89, 0x21, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47,

		0x70, 0x12, 0x79, 0x00, 0x00, 0x12, 0x00, 0x16, 0x82, 0x10, 0x00, 0x00, 0xff, 0x0e, 0xdf, 0x10,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x45, 0x4c, 0x47, 0x41, 0x4c, 0x34, 0x32, 0x30, 0x03, 0x01,
		0x50, 0x70, 0x92, 0x01, 0x84, 0xff, 0x1d, 0xc7, 0x00, 0x1d, 0x80, 0x09, 0x00, 0xdf, 0x10, 0x2f,
		0x00, 0x02, 0x00, 0x04, 0x00, 0xc1, 0x42, 0x01, 0x84, 0xff, 0x1d, 0xc7, 0x00, 0x2f, 0x80, 0x1f,
		0x00, 0xdf, 0x10, 0x30, 0x00, 0x02, 0x00, 0x04, 0x00, 0xa8, 0x4e, 0x01, 0x04, 0xff, 0x0e, 0xc7,
		0x00, 0x2f, 0x80, 0x1f, 0x00, 0xdf, 0x10, 0x61, 0x00, 0x02, 0x00, 0x09, 0x00, 0x97, 0x9d, 0x01,
		0x04, 0xff, 0x0e, 0xc7, 0x00, 0x2f, 0x80, 0x1f, 0x00, 0xdf, 0x10, 0x2f, 0x00, 0x02, 0x00, 0x09,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9e, 0x90,
	};

	UInt8 DellUP3218Kright[] = {
		0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x10, 0xac, 0x47, 0x41, 0x4c, 0x34, 0x32, 0x30,
		0x27, 0x1c, 0x01, 0x04, 0xb5, 0x46, 0x27, 0x78, 0x3a, 0x76, 0x45, 0xae, 0x51, 0x33, 0xba, 0x26,
		0x0d, 0x50, 0x54, 0xa5, 0x4b, 0x00, 0x81, 0x00, 0xb3, 0x00, 0xd1, 0x00, 0xa9, 0x40, 0x81, 0x80,
		0xd1, 0xc0, 0x01, 0x01, 0x01, 0x01, 0x4d, 0xd0, 0x00, 0xa0, 0xf0, 0x70, 0x3e, 0x80, 0x30, 0x20,
		0x35, 0x00, 0xba, 0x89, 0x21, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0xff, 0x00, 0x46, 0x46, 0x4e,
		0x58, 0x4d, 0x38, 0x39, 0x50, 0x30, 0x32, 0x34, 0x4c, 0x0a, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x44,
		0x45, 0x4c, 0x4c, 0x20, 0x55, 0x50, 0x33, 0x32, 0x31, 0x38, 0x4b, 0x0a, 0x00, 0x00, 0x00, 0xfd,
		0x00, 0x18, 0x4b, 0x1e, 0xb4, 0x6c, 0x01, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x02, 0x59,

		0x02, 0x03, 0x1d, 0xf1, 0x50, 0x10, 0x1f, 0x20, 0x05, 0x14, 0x04, 0x13, 0x12, 0x11, 0x03, 0x02,
		0x16, 0x15, 0x07, 0x06, 0x01, 0x23, 0x09, 0x1f, 0x07, 0x83, 0x01, 0x00, 0x00, 0xa3, 0x66, 0x00,
		0xa0, 0xf0, 0x70, 0x1f, 0x80, 0x30, 0x20, 0x35, 0x00, 0xba, 0x89, 0x21, 0x00, 0x00, 0x1a, 0x56,
		0x5e, 0x00, 0xa0, 0xa0, 0xa0, 0x29, 0x50, 0x30, 0x20, 0x35, 0x00, 0xba, 0x89, 0x21, 0x00, 0x00,
		0x1a, 0x7c, 0x39, 0x00, 0xa0, 0x80, 0x38, 0x1f, 0x40, 0x30, 0x20, 0x3a, 0x00, 0xba, 0x89, 0x21,
		0x00, 0x00, 0x1a, 0xa8, 0x16, 0x00, 0xa0, 0x80, 0x38, 0x13, 0x40, 0x30, 0x20, 0x3a, 0x00, 0xba,
		0x89, 0x21, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47,

		0x70, 0x12, 0x79, 0x00, 0x00, 0x12, 0x00, 0x16, 0x82, 0x10, 0x10, 0x00, 0xff, 0x0e, 0xdf, 0x10,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x45, 0x4c, 0x47, 0x41, 0x4c, 0x34, 0x32, 0x30, 0x03, 0x01,
		0x50, 0x70, 0x92, 0x01, 0x84, 0xff, 0x1d, 0xc7, 0x00, 0x1d, 0x80, 0x09, 0x00, 0xdf, 0x10, 0x2f,
		0x00, 0x02, 0x00, 0x04, 0x00, 0xc1, 0x42, 0x01, 0x84, 0xff, 0x1d, 0xc7, 0x00, 0x2f, 0x80, 0x1f,
		0x00, 0xdf, 0x10, 0x30, 0x00, 0x02, 0x00, 0x04, 0x00, 0xa8, 0x4e, 0x01, 0x04, 0xff, 0x0e, 0xc7,
		0x00, 0x2f, 0x80, 0x1f, 0x00, 0xdf, 0x10, 0x61, 0x00, 0x02, 0x00, 0x09, 0x00, 0x97, 0x9d, 0x01,
		0x04, 0xff, 0x0e, 0xc7, 0x00, 0x2f, 0x80, 0x1f, 0x00, 0xdf, 0x10, 0x2f, 0x00, 0x02, 0x00, 0x09,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8e, 0x90,
	};
	
	if (IofbAvailable(0)) {
		IofbSetEDIDOverride(AcerXV273JleftPartial, (UInt32)sizeof(AcerXV273JleftPartial), DellUP3218Kleft, (UInt32)sizeof(DellUP3218Kleft));
		IofbSetEDIDOverride(AcerXV273JrightPartial, (UInt32)sizeof(AcerXV273JrightPartial), DellUP3218Kright, (UInt32)sizeof(DellUP3218Kright));
	}
	else {
		iprintf("Iofb is not available.\n");
	}
}
