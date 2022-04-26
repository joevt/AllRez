//
//  printf.h
//  AllRez
//
//  Created by joevt on 2022-02-19.
//

#ifndef printf_h
#define printf_h

#include <stdio.h>

extern int indent;

#define FILEOUT stdout
#define OUTDENT indent -= 4;
#define INDENT indent += 4;
#define iprintf(s,...) fprintf(FILEOUT, "%*s" s, indent, "", ##__VA_ARGS__)
#define cprintf(s,...) fprintf(FILEOUT, s, ##__VA_ARGS__)
#define lf cprintf("\n")

#endif /* printf_h */
