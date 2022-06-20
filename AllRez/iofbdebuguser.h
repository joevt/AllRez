//
//  iofbdebuguser.hpp
//  AllRez
//
//  Created by joevt on 2022-05-19.
//

#ifndef iofbdebuguser_hpp
#define iofbdebuguser_hpp

#include <IOKit/IOTypes.h>

IOReturn IofbSetAttributeForService(io_service_t ioFramebufferService, UInt32 category, UInt32 val1, UInt32 val2, UInt32 val3, UInt32 *valGet);

IOReturn IofbGetSetAttributeForDisplay(int displayIndex, UInt32 category, UInt32 val1, UInt32 val2, UInt32 val3, UInt32 *valGet);

IOReturn IofbSetAttributeForDisplay(int displayIndex, UInt32 category, UInt32 val1, UInt32 val2, UInt32 val3);

IOReturn IofbGetAttributeForDisplay(int displayIndex, UInt32 category, UInt32 val1, UInt32 val2, UInt32 *valGet);


void SetIofbDebugEnabled(bool debugEnabled);

void DoAttributeTest(void);

#endif /* iofbdebuguser_hpp */
