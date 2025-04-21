//
//  main.cpp
//  dumpdpcd
//
//  Created by joevt on 2025-04-21.
//
//=================================================================================================================================

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dpcd.h"

int main(int argc, const char * argv[]) {
	if (argc <= 1)
		return -1;

	FILE *thefile = fopen(argv[1], "rb");

	uint8_t *dpcd = (uint8_t*)malloc(0x100000);
	if (!dpcd)
		return -1;
	memset(dpcd, 0, 0x100000);
	fread(dpcd, 0x100000, 1, thefile);

	parsedpcd(dpcd);

	free(dpcd);
	return 0;
} // main
