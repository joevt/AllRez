//
//  printf.h
//  AllRez
//
//  Created by joevt on 2022-02-19.
//

#ifndef printf_h
#define printf_h

#include <stdio.h>
#include <IOKit/IOReturn.h>
#include <CoreFoundation/CFBase.h>

#define FILEOUT stdout
#define OUTDENT indent -= 4;
#define INDENT indent += 4;
#define iprintf(s,...) fprintf(FILEOUT, "%*s" s, indent, "", ##__VA_ARGS__)
#define cprintf(s,...) fprintf(FILEOUT, s, ##__VA_ARGS__)
#define lf cprintf("\n")

#ifdef __cplusplus
extern "C" {
#endif

int scnprintf(char * str, size_t size, const char * format, ...) __printflike(3, 4);

char * myprintf(const char *format, ...) __printflike(1, 2);

char * DumpOneReturn(char * buf, size_t bufSize, int64_t val);
const char * GetIOReturnStr(char * buf, size_t bufSize, int64_t val);
void CFOutput(CFTypeRef val);

#ifdef __cplusplus
}
#endif


#define UNKNOWN_FLAG( x) (x) ? myprintf("?0x%llx,", uint64_t(x)) : ""
#define UNKNOWN_VALUE(x)       myprintf("?0x%llx", uint64_t(x))

extern int indent;

#endif /* printf_h */
