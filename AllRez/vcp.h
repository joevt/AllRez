//
//  vcp.h
//  AllRez
//
//  Created by joevt on 2022-02-19.
//

#ifndef vcp_h
#define vcp_h

#include <CoreFoundation/CFBase.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <IOKit/IOTypes.h>
#include <IOKit/i2c/IOI2CInterface.h>

// These are in micoseconds

#define kDelayRequest					 10000
#define kDelayReply                      10000

#define kDelayDDCEDIDReply              kDelayReply

#define kDelayEDDCPointerSegment        kDelayRequest
#define kDelayEDDCEDIDReply             kDelayReply

#define kDelayMCCSEDIDRequest           kDelayRequest
#define kDelayMCCSEDIDReply             kDelayReply
	  
#define kDelayDDCIdentificationRequest  kDelayRequest
#define kDelayDDCIdentificationReply    kDelayReply

#define kDelayDDCTimingRequest          kDelayRequest
#define kDelayDDCTimingReply            kDelayReply
	  
#define kDelayDDCVCPCapRequest          kDelayRequest
#define kDelayDDCVCPCapReply            kDelayReply
	  
#define kDelayDDCFeatureRequest         kDelayRequest
#define kDelayDDCFeatureReply           kDelayReply


#define FORCEI2C 1 // don't use kIOI2CDDCciReplyTransactionType because some drivers (for my GTX 680 in Monterey) may treat minReplyDelay of 6000 as 6 seconds instead of 6 milliseconds

void ddcsetchecksum(IOI2CRequest_10_6_0 *request);
bool ddcreplyisgood(IOI2CRequest_10_6_0 *request, bool hasSize, UInt8 sendAddress, UInt8 *replyBuffer, int expectedSize);
long parsevcp(int level, char *vcps, IOI2CConnectRef i2cconnect, int val_IOI2CTransactionTypes);

#ifdef __cplusplus
}
#endif


#endif
