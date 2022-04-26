//
//  oui.c
//  AllRez
//
//  Created by joevt on 2022-02-19.
//

#include "oui.h"

/*
# Create the ouilist.h file by executing the command below.
# There may be some duplicates that need to be commented out.

for thetype in oui cid; do
	[[ -f "/tmp/${thetype}.txt" ]] || curl -s "http://standards-oui.ieee.org/${thetype}/${thetype}.txt" > "/tmp/${thetype}.txt"
	tr '\r\t' '\n ' < "/tmp/${thetype}.txt" | sed -nE '/^(......)[ ]+\(base 16\) *(.*[^ ]) *$/s//oneoui(\1, "\2")/p'
done > "ouilist.h"
*/

char* convertoui(int oui) {
	#define oneoui(val, name) case 0x ## val: return name;
	switch(oui) {
		#include "ouilist.h"
		default: return "unknown";
	}
}
