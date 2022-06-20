//
//  printf.c
//  AllRez
//
//  Created by joevt on 2022-02-19.
//
#include <stdarg.h>
#include "printf.h"
#include "AppleMisc.h"
#include <IOKit/IOMessage.h>

int indent = 0;

int scnprintf(char * str, size_t size, const char * format, ...) {
	va_list args;
	va_start(args, format);
	size_t result = vsnprintf(str, size, format, args);
	va_end(args);
	if (result > size)
		result = size;
	return (int)result;
}

char * myprintf(const char *format, ...) {
	char *unknownValue = (char *)malloc(20);
	va_list vl;
	va_start(vl, format);
	vsnprintf(unknownValue, 20, format, vl);
	va_end(vl);
	return unknownValue;
}

void CFOutput(CFTypeRef val) {
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

const char * GetIOReturnStr(char * buf, size_t bufSize, int64_t val) {

	const char* str;
#define onereturn(x) case x: str = #x; break;
	
	switch (val) {
		onereturn(kIOReturnSuccess)
		onereturn(kIOReturnError)
		onereturn(kIOReturnNoMemory)
		onereturn(kIOReturnNoResources)
		onereturn(kIOReturnIPCError)
		onereturn(kIOReturnNoDevice)
		onereturn(kIOReturnNotPrivileged)
		onereturn(kIOReturnBadArgument)
		onereturn(kIOReturnLockedRead)
		onereturn(kIOReturnLockedWrite)
		onereturn(kIOReturnExclusiveAccess)
		onereturn(kIOReturnBadMessageID)
		onereturn(kIOReturnUnsupported)
		onereturn(kIOReturnVMError)
		onereturn(kIOReturnInternalError)
		onereturn(kIOReturnIOError)
		onereturn(kIOReturnCannotLock)
		onereturn(kIOReturnNotOpen)
		onereturn(kIOReturnNotReadable)
		onereturn(kIOReturnNotWritable)
		onereturn(kIOReturnNotAligned)
		onereturn(kIOReturnBadMedia)
		onereturn(kIOReturnStillOpen)
		onereturn(kIOReturnRLDError)
		onereturn(kIOReturnDMAError)
		onereturn(kIOReturnBusy)
		onereturn(kIOReturnTimeout)
		onereturn(kIOReturnOffline)
		onereturn(kIOReturnNotReady)
		onereturn(kIOReturnNotAttached)
		onereturn(kIOReturnNoChannels)
		onereturn(kIOReturnNoSpace)
		onereturn(kIOReturnPortExists)
		onereturn(kIOReturnCannotWire)
		onereturn(kIOReturnNoInterrupt)
		onereturn(kIOReturnNoFrames)
		onereturn(kIOReturnMessageTooLarge)
		onereturn(kIOReturnNotPermitted)
		onereturn(kIOReturnNoPower)
		onereturn(kIOReturnNoMedia)
		onereturn(kIOReturnUnformattedMedia)
		onereturn(kIOReturnUnsupportedMode)
		onereturn(kIOReturnUnderrun)
		onereturn(kIOReturnOverrun)
		onereturn(kIOReturnDeviceError)
		onereturn(kIOReturnNoCompletion)
		onereturn(kIOReturnAborted)
		onereturn(kIOReturnNoBandwidth)
		onereturn(kIOReturnNotResponding)
		onereturn(kIOReturnIsoTooOld)
		onereturn(kIOReturnIsoTooNew)
		onereturn(kIOReturnNotFound)
		onereturn(kIOReturnInvalid)

		onereturn(kIOMessageServiceIsTerminated)
		onereturn(kIOMessageServiceIsSuspended)
		onereturn(kIOMessageServiceIsResumed)
		onereturn(kIOMessageServiceIsRequestingClose)
		onereturn(kIOMessageServiceIsAttemptingOpen)
		onereturn(kIOMessageServiceWasClosed)
		onereturn(kIOMessageServiceBusyStateChange)
		onereturn(kIOMessageConsoleSecurityChange)
		onereturn(kIOMessageServicePropertyChange)
		onereturn(kIOMessageCopyClientID)
		onereturn(kIOMessageSystemCapabilityChange)
		onereturn(kIOMessageDeviceSignaledWakeup)
		onereturn(kIOMessageDeviceWillPowerOff)
		onereturn(kIOMessageDeviceHasPoweredOn)
		onereturn(kIOMessageSystemWillPowerOff)
		onereturn(kIOMessageSystemWillRestart)
		onereturn(kIOMessageSystemPagingOff)
		onereturn(kIOMessageCanSystemSleep)
		onereturn(kIOMessageSystemWillNotSleep)
		onereturn(kIOMessageSystemWillSleep)
		onereturn(kIOMessageSystemWillPowerOn)
		onereturn(kIOMessageSystemHasPoweredOn)
		onereturn(kIOMessageCanDevicePowerOff)
		onereturn(kIOMessageDeviceWillNotPowerOff)
		onereturn(kIOMessageSystemWillNotPowerOff)
		onereturn(kIOMessageCanSystemPowerOff)
		onereturn(kIOMessageDeviceWillPowerOn)
		onereturn(kIOMessageDeviceHasPoweredOff)
		onereturn(kIOMessageGraphicsNotifyTerminated)
		onereturn(kIOMessageGraphicsProbeAccelerator)
		onereturn(kIOMessageGraphicsDeviceEject)
		onereturn(kIOMessageGraphicsDeviceEjectFinalize)
		onereturn(kIOMessageGraphicsDeviceEjectCancel)
			
		onereturn(nrLockedErr)
		onereturn(nrNotEnoughMemoryErr)
		onereturn(nrInvalidNodeErr)
		onereturn(nrNotFoundErr)
		onereturn(nrNotCreatedErr)
		onereturn(nrNameErr)
		onereturn(nrNotSlotDeviceErr)
		onereturn(nrDataTruncatedErr)
		onereturn(nrPowerErr)
		onereturn(nrPowerSwitchAbortErr)
		onereturn(nrTypeMismatchErr)
		onereturn(nrNotModifiedErr)
		onereturn(nrOverrunErr)
		onereturn(nrResultCodeBase)
		onereturn(nrPathNotFound)
		onereturn(nrPathBufferTooSmall)
		onereturn(nrInvalidEntryIterationOp)
		onereturn(nrPropertyAlreadyExists)
		onereturn(nrIterationDone)
		onereturn(nrExitedIteratorScope)
		onereturn(nrTransactionAborted)
		onereturn(gestaltUndefSelectorErr)
		onereturn(paramErr)
		onereturn(noHardwareErr)
		onereturn(notEnoughHardwareErr)
		onereturn(userCanceledErr)
		onereturn(qErr)
		onereturn(vTypErr)
		onereturn(corErr)
		onereturn(unimpErr)
		onereturn(SlpTypeErr)
		onereturn(seNoDB)
		onereturn(controlErr)
		onereturn(statusErr)
		onereturn(readErr)
		onereturn(writErr)
		onereturn(badUnitErr)
		onereturn(unitEmptyErr)
		onereturn(openErr)
		onereturn(closErr)
		onereturn(dRemovErr)
		onereturn(dInstErr)
		onereturn(badCksmErr)
		default: str = buf; snprintf(buf, bufSize, "?%#llx", val); break;
	}
#undef onereturn
	return str;
} // GetIOReturnStr

char * DumpOneReturn(char * buf, size_t bufSize, int64_t val)
{
	char * result = buf;
	if (val == kIOReturnSuccess)
		snprintf(buf, bufSize, "");
	else {
		char resultStr[40];
		snprintf(buf, bufSize, " result:%s", GetIOReturnStr(resultStr, sizeof(resultStr), val));
	}
	return result;
} // DumpOneReturn
