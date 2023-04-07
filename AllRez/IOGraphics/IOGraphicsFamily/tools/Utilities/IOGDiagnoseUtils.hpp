//
//  IOGDiagnoseUtils.hpp
//  IOGDiagnoseUtils
//
//  Created by Jérémy Tran on 8/8/17.
//

#ifndef IOGDiagnoseUtils_hpp
#define IOGDiagnoseUtils_hpp

#if defined(IOGD530_66)
#include <cstdlib>
#include <vector>
#endif

#include "iokit.h"

#if defined(IOGD530_66)
#include "IOGraphicsDiagnose.h"
#include "GTraceTypes.hpp"
#endif

#if defined(IOGD530_66)
kern_return_t openDiagnostics(IOConnect* diagConnectP, char **errmsgP);

IOReturn fetchGTraceBuffers(const IOConnect& diag,
                            std::vector<GTraceBuffer>* traceBuffersP);
/*!
 @brief Populate a pre-allocated IOGDiagnose pointer with data coming from
        IOGraphics. Also populates a vector of GTraceEntry tables.

 @param reportP An allocated pointer to a IOGDiagnose struct.
 @param reportLength The memory length of the passed pointer. Should be greater
                     or equal to @c sizeof(IOGDiagnose).
 @param errmsgP Pointer to a string used by the function to return error
                messages. Must be freed by callee.
 */

// If errmsgP is set then the caller is required to free returned string
kern_return_t iogDiagnose(const IOConnect& diag,
                          IOGDiagnose* reportP, size_t reportLength,
                          std::vector<GTraceBuffer>* traceBuffersP,
                          char** errmsgP);
#else
kern_return_t openDiagnostics(IOConnect* diagConnectP, const char **errmsgP);
kern_return_t openGTrace(IOConnect* gtraceConnectP, const char **errmsgP);
#endif
#endif // IOGDiagnoseUtils_hpp
