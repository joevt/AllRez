//
//  printf.c
//  AllRez
//
//  Created by joevt on 2022-02-19.
//
#include "MacOSMacros.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "printf.h"
#include "AppleMisc.h"
#include <IOKit/IOMessage.h>
#include <CarbonCore/MacErrors.h>


#if MAC_OS_X_VERSION_SDK < MAC_OS_X_VERSION_10_7
#define kIOMessageConsoleSecurityChange    iokit_common_msg(0x128)
#define kIOMessageSystemCapabilityChange   iokit_common_msg(0x340)
#define kIOMessageDeviceSignaledWakeup     iokit_common_msg(0x350)
#define kIOMessageSystemPagingOff          iokit_common_msg(0x255)
#define kIOMessageDeviceWillPowerOn        iokit_common_msg(0x215)
#define kIOMessageDeviceHasPoweredOff      iokit_common_msg(0x225)
#endif


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

#if !defined(DUMPDPCD)
void CFOutput(CFTypeRef val) {
	if (val) {
		CFStringRef theinfo = CFCopyDescription(val); // can't really parse this if the format can change for every macOS
		if (theinfo) {
			size_t maxsize = CFStringGetMaximumSizeForEncoding(CFStringGetLength(theinfo), kCFStringEncodingUTF8);
			char *strinfo = (char *)malloc(maxsize);
			if (strinfo) {
				if (CFStringGetCString(theinfo, strinfo, maxsize, kCFStringEncodingUTF8)) {
					char *s;
					if (DarwinMajorVersion() < 19) { // earlier than 10.15
						size_t l;
						regmatch_t p[2];
						int reti;
						
						regex_t regex;
						
						reti = regcomp(&regex, "<CFString 0x[[:xdigit:]]+ \\[0x[[:xdigit:]]+\\]>\\{contents = \"([^\"]*)\"\\}", REG_EXTENDED);
						if (reti) {
							cprintf("? Could not compile regex1");
						}
						else {
							s = strinfo;
							l = maxsize;
							
							while (1) {
								reti = regexec(&regex, s, sizeof(p) / sizeof(p[0]), p, 0);
								if (!reti) {
									snprintf(s + p[0].rm_so, (size_t)(l - p[0].rm_so), "%.*s%s", (int)(p[1].rm_eo - p[1].rm_so), s + (size_t)p[1].rm_so, s + (size_t)p[0].rm_eo);
									s += p[0].rm_so + p[1].rm_eo - p[1].rm_so;
									l -= (size_t)(p[0].rm_so + p[1].rm_eo - p[1].rm_so);
								}
								else if (reti == REG_NOMATCH) {
									break;
								}
								else {
									char msgbuf[100];
									regerror(reti, &regex, msgbuf, sizeof(msgbuf));
									cprintf("? Regex1 match failed: %s", msgbuf);
								}
							}
							regfree(&regex);
						}
						
						reti = regcomp(&regex, "<CF[[:alpha:]]+ 0x[[:xdigit:]]+ \\[0x[[:xdigit:]]+\\]>", REG_EXTENDED);
						if (reti) {
							cprintf("? Could not compile regex2");
						}
						else {
							s = strinfo;
							l = maxsize;
							
							while (1) {
								reti = regexec(&regex, s, sizeof(p) / sizeof(p[0]), p, 0);
								if (!reti) {
									snprintf(s + p[0].rm_so, l - (size_t)p[0].rm_so, "%s", s + (size_t)p[0].rm_eo);
									s += p[0].rm_so;
									l -= (int)p[0].rm_so;
								}
								else if (reti == REG_NOMATCH) {
									break;
								}
								else {
									char msgbuf[100];
									regerror(reti, &regex, msgbuf, sizeof(msgbuf));
									cprintf("? Regex2 match failed: %s", msgbuf);
								}
							}
							regfree(&regex);
						}
					} // if earlier than 10.15

					// Output each line with extra indent
					char *next;
					size_t len = strlen(strinfo);
					if (len && strinfo[len-1] == '\n') {
						strinfo[len-1] = '\0';
					}
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
#endif

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
