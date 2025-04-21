//
//  IOGDiagnoseUtils.cpp
//  IOGDiagnoseUtils
//
//  Created by Jérémy Tran on 8/8/17.
//

#define IOGD530_14

#include "IOGDiagnoseUtils.h"

#include <string.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/graphics/IOGraphicsTypes.h>
#include "AppleMisc.h"

kern_return_t iogDiagnose(IOGDiagnose * reportP,
                          size_t        reportLength,
                          uint64_t      version,
                          const char ** errmsgP)
{
    const char *  errstr = NULL;
    kern_return_t err = kIOReturnError;
    io_iterator_t iterator = IO_OBJECT_NULL;

    if (!reportP)
    {
        errstr = "NULL IOGDiagnose pointer argument passed";
        err = kIOReturnBadArgument;
        goto exit;
    }
    if (sizeof(*reportP) > reportLength)
    {
        errstr = "reportLength argument not big enough to contain a IOGDiagnose object";
        err = kIOReturnBadArgument;
        goto exit;
    }
    err = IOServiceGetMatchingServices(kIOMasterPortDefault,
                                       IOServiceMatching("IOFramebuffer"),
                                       &iterator);

    if (kIOReturnSuccess != err)
    {
        errstr = "IOService matching of IOFramebuffer failed";
        goto exit;
    }

    if (!IOIteratorIsValid(iterator))
    {
        errstr = "IOIterator became invalid, cannot proceed forward";
        err = kIOReturnError;
    }
    else
    {
        io_service_t framebuffer = IOIteratorNext(iterator);
        if (IO_OBJECT_NULL == framebuffer)
        {
            errstr = "IOGDiagnose failed, no framebuffers found";
            err = kIOReturnNotFound;
        }
        else
        {
            io_connect_t connect = IO_OBJECT_NULL;
            err = IOServiceOpen(framebuffer, mach_task_self(), kIOFBDiagnoseConnectType, &connect);
            if (kIOReturnSuccess != err)
            {
                errstr = "IOServiceOpen on IOFramebuffer failed";
            }
            else
            {
                bzero(reportP, reportLength);

                reportP->version = version;
                reportP->tokenSize = (uint32_t)sizeof(reportP->tokenBuffer);

#if MAC_OS_X_VERSION_SDK >= MAC_OS_X_VERSION_10_5
                uint64_t    scalerParams[] = {reportLength, version};
                uint32_t    scalerParamsCount = ((uint32_t)sizeof(scalerParams) / (uint32_t)sizeof(scalerParams[0]));

                API_OR_SDK_AVAILABLE_BEGIN(10.5, IOConnectCallMethod) {
                    err = IOConnectCallMethod(connect, kIOGSharedInterface_IOGDiagnose,
                                             scalerParams, scalerParamsCount, NULL, 0,
                                             NULL, NULL, reportP, &reportLength);
                } API_OR_SDK_AVAILABLE_ELSE {
                    err = kIOReturnUnsupported;
                } API_OR_SDK_AVAILABLE_END
#endif
                if (kIOReturnSuccess != err)
                {
                    errstr = "IOGDiagnose failed";
                }
                IOServiceClose(connect);
            }
            IOObjectRelease(framebuffer);
        }
    }
    IOObjectRelease(iterator);

exit:
    if (errstr && errmsgP)
    {
        *errmsgP = errstr;
    }
    return err;
}
