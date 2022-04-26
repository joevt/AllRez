//
//  vcp.c
//  AllRez
//
//  Created by joevt on 2022-02-19.
//

#include "vcp.h"
#include "printf.h"
#include "utilities.h"

typedef enum {
	kVCPDirectionRW,
	kVCPDirectionWO,
	kVCPDirectionRO,
	kVCPDirectionTBD,
} VCPDirection;

typedef enum {
	kVCPTypeT,
	kVCPTypeNC,
	kVCPTypeC,
	kVCPTypeTBD
} VCPType;

typedef struct {
	uint8_t sourceAddress;
	uint8_t length;
	uint8_t vcpFeatureReplyOpcode;
	uint8_t resultCode; // 00 = NoError; 01 = Unsupported VCP Code
	uint8_t vcpOpcode;
	uint8_t vcpTypeCode; // 00 = Set parameter; 01 = Momentary
	uint8_t MH;
	uint8_t ML;
	uint8_t SH;
	uint8_t SL;
	uint8_t checksum;
} VCPFeatureReply;

void ddcsetchecksum(IOI2CRequest *request) {
	// request.sendBytes[1] = 0x80 + request.sendBytes - 3; // Length
	UInt8 checksum = request->sendAddress;
	for (int i = 0; i < request->sendBytes - 1; i++) {
		checksum = checksum ^ ((UInt8*)(request->sendBuffer))[i];
	}
	((UInt8*)(request->sendBuffer))[request->sendBytes - 1] = checksum;
} // ddcsetchecksum

bool ddcreplyisgood(IOI2CRequest *request, bool hasSize, UInt8 sendAddress, UInt8 *replyBuffer, int expectedSize) {
	// zero length result: 6e80be
	if (request->result)
		return false;
	if (request->replyBytes < 3)
		return false;
	if (replyBuffer[0] != sendAddress)
		return false;
	int numBytes = hasSize ? (replyBuffer[1] & 0x7f) + 3 : request->replyBytes;
	if (numBytes > request->replyBytes)
		return false;
	if (expectedSize >= 0 && numBytes != expectedSize)
		return false;

	UInt8 checksum = 0x50;
	for (int i = 0; i < numBytes; i++) {
		checksum = checksum ^ replyBuffer[i];
	}
	if (checksum)
		return false;
	return true;
}

long parsevcp(int level, char *vcps, IOI2CConnectRef i2cconnect, int val_IOI2CTransactionTypes) {
	char *c = vcps;
	char *s; long slen;
	while (*c) {
		for (; *c && *c == ' '; c++);

		if (*c == '(') {
			c++;
			iprintf("{\n"); INDENT
			c += parsevcp(level + 1, c, i2cconnect, val_IOI2CTransactionTypes);
			OUTDENT iprintf("}\n");
			continue;
		}
		if (*c == ')') {
			if (level == 0) {
				iprintf("(unexpected closing parenthesis at %ld)\n", c - vcps);
			}
			return c - vcps + 1;
		}

		s = c;
		for (; *c && *c != '('; c++);
		slen = c - s;
		if (!*c && !slen) break;
		iprintf("%.*s = ", (int)slen, s);
		if (!*c) {
			cprintf("(unexpected end)\n");
			break;
		}
		c++;
		if (!strncasecmp("cmds", s, slen)) {
			cprintf("{\n"); INDENT
			while (1) {
				for (; *c && *c == ' '; c++);
				s = c;
				for (; *c && *c != ' ' && *c != ')'; c++);
				slen = c - s;
				if (!slen) break;
				iprintf("%.*s: ", (int)slen, s);
				
				if (0) {}
				#define oneddccmd(id, name) else if (!strncasecmp( #id, s, slen ) ) cprintf("%s\n", name);
				#include "ddccicommands.h"
				else cprintf("(unknown command)\n");
			}
			OUTDENT iprintf("};\n");
		}
		else if (!strncasecmp("vcp", s, slen)) {
			bool hasUserColorTemperatureIncrement = false;
			int UserColorTemperatureIncrement = 0;

			cprintf("{\n"); INDENT
			while (1) {

				//UInt8 *data = NULL; // for ddc/ci table reading
				char *vcpcodedescription = NULL;
				UInt8 vcpcode = 0xff;
				char cap[20];
				bool hascap = false;
				int caplen = 0;
				char errors[1000]; errors[0] = '\0';
				char *errptr = errors;
				VCPType vcptype;
				char *vcptypestr;
				VCPDirection vcpdirection;
				char *vcpdirectionstr;

				for (; *c && *c == ' '; c++);
				s = c;
				for (; *c && *c != ' ' && *c != ')' && *c != '('; c++);
				slen = c - s;
				if (!slen) break;

				if (*c == '(') {
					c++;
					hascap = true;
					while (1) {
						char *p; long plen;
						int b;
						
						for (; *c && *c == ' '; c++);
						p = c;
						for (; *c && *c != ' ' && *c != ')'; c++);
						plen = c - p;
						if (!plen) break;
						b = 0;
						if (!sscanf(p, "%X", &b)) {
							errptr += snprintf(errptr, sizeof(errors) - (errptr - errors), "(parse error at %ld) ", p - vcps);
						}
						if (caplen < sizeof(cap)) {
							cap[caplen] = b;
							caplen++;
						}
						else {
							errptr += snprintf(errptr, sizeof(errors) - (errptr - errors), "(too many capability bytes at %ld) ", p - vcps);
						}
					} // while vcp capabilities string
					if (*c) c++;
				} // if vcp capabilities string

				if (0) {}
				#define onevcpcode(id, description, direction, type, mandatory, version) \
				else if (!strncasecmp( #id, s, slen ) ) { \
					vcpcodedescription = description; \
					vcpcode = 0x ## id; \
					vcptype = kVCPType ## type; \
					vcpdirection = kVCPDirection ## direction; \
					vcptypestr = #type; \
					vcpdirectionstr = #direction; \
				}
				#include "vcpcodes.h"
				else {
					vcpcodedescription = "(unknown VCP code)";
					vcpcode = 0x00;
					vcpdirection = kVCPDirectionTBD;
					vcptype = kVCPTypeTBD;
					vcpdirectionstr = "Unknown";
					vcptypestr = "unknown";
				}
				
				bool hasFeatureReply = false;
				VCPFeatureReply featureReply;
				bzero(&featureReply, sizeof(featureReply));
				
				if (vcpdirection == kVCPDirectionRO || vcpdirection == kVCPDirectionRW) {
					IOI2CRequest request;
					IOReturn result = kIOReturnSuccess;
					bool ddcReplyIsBad = false;

					for (int attempt = 0; attempt < 3; attempt++) {
						ddcReplyIsBad = false;
						result = kIOReturnSuccess;
						
						if (vcptype == kVCPTypeC || vcptype == kVCPTypeNC) {
							bzero(&request, sizeof(request));
							request.sendTransactionType = kIOI2CSimpleTransactionType;
							request.sendAddress = 0x6E; // Destination address (DDC/CI)
							UInt8 senddata[] = {
								0x51, // Source address
								0x82, // Length
								0x01, // Get VCP Feature COMMAND
								vcpcode,
								0x00, // Checksum
							};
							request.sendBytes = sizeof(senddata);
							request.sendBuffer = (vm_address_t)senddata;
							ddcsetchecksum(&request);

							if (FORCEI2C || !(val_IOI2CTransactionTypes & (1 << kIOI2CDDCciReplyTransactionType))) {
								result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
								usleep(kDelayDDCFeatureRequest);
								if (result) {
									continue;
								}
								bzero(&request, sizeof(request));
								request.replyTransactionType = kIOI2CSimpleTransactionType;
							}
							else {
								request.replyTransactionType = kIOI2CDDCciReplyTransactionType;
								request.minReplyDelay = MicrosecondsToAbsoluteTime(kDelayDDCFeatureRequest);
							}

							request.replyAddress = 0x6F; // Source address (DDC/CI)
							request.replyBytes = sizeof(featureReply);
							request.replyBuffer = (vm_address_t)&featureReply;
							result = IOI2CSendRequest(i2cconnect, kNilOptions, &request);
							usleep(kDelayDDCFeatureReply);

							if (result) {
								continue;
							}
							else {
								hasFeatureReply = true;
								if (!ddcreplyisgood(&request, true, 0x6e, (UInt8*)&featureReply, sizeof(featureReply))
									|| featureReply.vcpFeatureReplyOpcode != 0x02
									|| featureReply.resultCode != 0x00
									|| featureReply.vcpOpcode != vcpcode
									|| (featureReply.vcpTypeCode != 0x00 && featureReply.vcpTypeCode != 0x01)
								) {
									ddcReplyIsBad = true;
									continue;
								};
							}
							break;
						} // vcp C or NC // continuous or non-continuous
						else if (vcptype == kVCPTypeT) { // vcp T // table
							
							
							break;
						} // vcp T // table
					} // for attempt
					
					if (result) {
						errptr += snprintf(errptr, sizeof(errors) - (errptr - errors), "(IOI2CSendRequest error:%d for vcp %02X) ", result, vcpcode);
					} else if (ddcReplyIsBad) {
						errptr += snprintf(errptr, sizeof(errors) - (errptr - errors), "(unexpected data for vcp %02X: ", vcpcode);
						for (int i = 0; i < request.replyBytes; i++) {
							errptr += snprintf(errptr, sizeof(errors) - (errptr - errors), "%02x", ((UInt8*)(&featureReply))[i]);
						}
						errptr += snprintf(errptr, sizeof(errors) - (errptr - errors), ") ");
					}
					
				} // if vcp RO or RW // supports read
				
				iprintf("%.*s: %s = { %s, %s, ", (int)slen, s, vcpcodedescription, vcpdirectionstr, vcptypestr);

				int sl;
				int sh;
				int sw;
				int s24;
				int s32;
				int ml;
				int mh;
				int mw;
				int m24;

				if (hascap) {
					cprintf("cap = { ");
					for (int i = 0; i < caplen; i++) {
						cprintf("%02X ", cap[i]);
					}
					cprintf(": ");

					switch (vcpcode) {
						case 0x14: // Select Color Preset
							for (int i = 0; i < caplen; i++) {
								sl = cap[i];
								cprintf("%s%s ",
									sl ==  1 ?                "sRGB" :
									sl ==  2 ?      "Display native" :
									sl ==  3 ?            "Warmer 4" :
									sl ==  4 ?            "Warmer 3" :
									sl ==  5 ?            "Warmer 2" :
									sl ==  6 ?            "Warmer 1" :
									sl ==  7 ?            "Cooler 1" :
									sl ==  8 ?            "Cooler 2" :
									sl ==  9 ?            "Cooler 3" :
									sl == 10 ?            "Cooler 4" :
									sl == 11 ?              "User 1" :
									sl == 12 ?              "User 2" :
									sl == 13 ?              "User 3" :
									/**/ "Reserved, must be ignored",
									i < caplen - 1 ? "," : ""
								);
							};
							break;
						case 0x60: // Input Select
							for (int i = 0; i < caplen; i++) {
								sl = cap[i];
								cprintf("%s%s ",
									sl ==  1 ?            "Analog video (R/G/B) 1" :
									sl ==  2 ?            "Analog video (R/G/B) 2" :
									sl ==  3 ?      "Digital video (TMDS) 1 DVI 1" :
									sl ==  4 ?      "Digital video (TMDS) 2 DVI 2" :
									sl ==  5 ?                 "Composite video 1" :
									sl ==  6 ?                 "Composite video 2" :
									sl ==  7 ?                         "S-video 1" :
									sl ==  8 ?                         "S-video 2" :
									sl ==  9 ?                           "Tuner 1" :
									sl == 10 ?                           "Tuner 2" :
									sl == 11 ?                           "Tuner 3" :
									sl == 12 ? "Component video (YPbPr / YCbCr) 1" :
									sl == 13 ? "Component video (YPbPr / YCbCr) 2" :
									sl == 14 ? "Component video (YPbPr / YCbCr) 3" :
									sl == 15 ?                     "DisplayPort 1" :
									sl == 16 ?                     "DisplayPort 2" :
									sl == 17 ?     "Digital Video (TMDS) 3 HDMI 1" :
									sl == 18 ?     "Digital Video (TMDS) 4 HDMI 2" :
									/**/                                     "?",
									i < caplen - 1 ? "," : ""
								);
							}
							break;
						case 0xcc: // OSD Language
							for (int i = 0; i < caplen; i++) {
								sl = cap[i];
								cprintf("%s%s ",
									sl ==  1 ? "Chinese (traditional / Hantai)" :
									sl ==  2 ?                        "English" :
									sl ==  3 ?                         "French" :
									sl ==  4 ?                         "German" :
									sl ==  5 ?                        "Italian" :
									sl ==  6 ?                       "Japanese" :
									sl ==  7 ?                         "Korean" :
									sl ==  8 ?          "Portuguese (Portugal)" :
									sl ==  9 ?                        "Russian" :
									sl == 10 ?                        "Spanish" :
									sl == 11 ?                        "Swedish" :
									sl == 12 ?                        "Turkish" :
									sl == 13 ?  "Chinese (simplified / Kantai)" :
									sl == 14 ?            "Portuguese (Brazil)" :
									sl == 15 ?                         "Arabic" :
									sl == 16 ?                      "Bulgarian" :
									sl == 17 ?                       "Croatian" :
									sl == 18 ?                          "Czech" :
									sl == 19 ?                         "Danish" :
									sl == 20 ?                          "Dutch" :
									sl == 21 ?                       "Estonian" :
									sl == 22 ?                        "Finnish" :
									sl == 23 ?                          "Greek" :
									sl == 24 ?                         "Hebrew" :
									sl == 25 ?                          "Hindi" :
									sl == 26 ?                      "Hungarian" :
									sl == 27 ?                        "Latvian" :
									sl == 28 ?                     "Lithuanian" :
									sl == 29 ?                      "Norwegian" :
									sl == 30 ?                         "Polish" :
									sl == 31 ?                       "Romanian" :
									sl == 32 ?                        "Serbian" :
									sl == 33 ?                         "Slovak" :
									sl == 34 ?                      "Slovenian" :
									sl == 35 ?                           "Thai" :
									sl == 36 ?                      "Ukrainian" :
									sl == 37 ?                     "Vietnamese" :
									/**/            "Reserved, must be ignored",
									i < caplen - 1 ? "," : ""
								);
							}
							break;
						case 0xaa: // Screen Orientation
							for (int i = 0; i < caplen; i++) {
								sl = cap[i];
								cprintf("%s%s ",
									sl ==   1 ?             "0°" :
									sl ==   2 ?            "90°" :
									sl ==   3 ?           "180°" :
									sl ==   4 ?           "270°" :
									sl == 255 ? "Not applicable" :
									/**/              "Reserved",
									i < caplen - 1 ? "," : ""
								);
							}
							break;

						case 0xdc: // Display Application
							for (int i = 0; i < caplen; i++) {
								sl = cap[i];
								cprintf("%s%s ",
									sl ==   0 ?                                        "Stand / default mode" :
									sl ==   1 ?                     "Productivity (e.g. office applications)" :
									sl ==   2 ?           "Mixed (e.g. internet with mix of text and images)" :
									sl ==   3 ?                                                       "Movie" :
									sl ==   4 ?                                                "User defined" :
									sl ==   5 ?                        "Games (e.g. games console / PC game)" :
									sl ==   6 ?                                   "Sports (e.g. fast action)" :
									sl ==   7 ?               "Professional (all signal processing disabled)" :
									sl ==   8 ? "Standard / default mode with intermediate power consumption" :
									sl ==   9 ?          "Standard / default mode with low power consumption" :
									sl ==  10 ?  "Demonstration (used for high visual impact in retail etc.)" :
									sl == 240 ?                                            "Dynamic contrast" :
									/**/                                          "Reserved, must be ignored",
									i < caplen - 1 ? "," : ""
								);
							}
							break;

					} // switch vcpcode for capabilities
					
					cprintf("}, ");
				}
				if (hasFeatureReply) {
					cprintf("");
					for (int i = 0; i < 5; i++) {
						cprintf("%02X", ((UInt8*)(&featureReply.vcpTypeCode))[i]);
					}
					cprintf(":{ ");

					// pre(s)ent and (m)ax values
					sl  =                                                                                featureReply.SL;
					sh  =                                                                                featureReply.SH;
					sw  =                                                      (featureReply.SH  << 8) | featureReply.SL;
					s24 =                           (((featureReply.ML  << 8) | featureReply.SH) << 8) | featureReply.SL;
					s32 = (((((featureReply.MH << 8) | featureReply.ML) << 8) | featureReply.SH) << 8) | featureReply.SL;

					ml  =                                                                                featureReply.ML;
					mh  =                                                                                featureReply.MH;
					mw  =                                                      (featureReply.MH  << 8) | featureReply.ML;
					m24 =                           (((featureReply.MH  << 8) | featureReply.ML) << 8) | featureReply.SH;
					
					switch(vcpcode) {
						case 0x02: // New Control Value
							cprintf("%s%s",
								m24 ? "?" : "", // spec says these should be 0 but my Cinema display has MHML = 0002.
								sl ==    1 ?                               "No new control value(s)" :
								sl ==    2 ? "New control value(s) have been changed (see FIFO 52h)" :
								sl == 0xff ?                          "No user controls are present" :
								/**/                                     "Reserved, must be ignored"
							);
							break;
						case 0x52: // Active Control
							{
								#define onevcpcode(id, description, direction, type, mandatory, version) sl == 0x ## id ? description :
								char * vstr =
									sl == 0xff ? "FIFO has been overrun" :
									sl == 0x00 ?         "FIFO is empty" :
									#include "vcpcodes.h"
									/**/            "(unknown VCP code)";
								cprintf("%s%02x: %s",
									(mw != 255 || sh) ? "?" : "",
									sl, vstr
								);
							}
							break;
						case 0x10: // Luminance
						case 0x12: // Contrast
						case 0x16: // Video Gain (Drive): Red
						case 0x18: // Video Gain (Drive): Green
						case 0x1a: // Video Gain (Drive): Blue
						case 0x59: // 6 Axis Saturation Control: Red
						case 0x5a: // 6 Axis Saturation Control: Yellow
						case 0x5b: // 6 Axis Saturation Control: Green
						case 0x5c: // 6 Axis Saturation Control: Cyan
						case 0x5d: // 6 Axis Saturation Control: Blue
						case 0x5e: // 6 Axis Saturation Control: Magenta
						case 0x9b: // 6 Axis Hue Control: Red
						case 0x9c: // 6 Axis Hue Control: Yellow
						case 0x9d: // 6 Axis Hue Control: Green
						case 0x9e: // 6 Axis Hue Control: Cyan
						case 0x9f: // 6 Axis Hue Control: Blue
						case 0xa0: // 6 Axis Hue Control: Magenta
							cprintf("%s%.1f%%",
								(!mw || mw < sw) ? "?" : "",
								(100.0 * sw) / (mw ? mw : 255.0)
							);
							break;
						case 0xb6: // Display Technology Type
							cprintf("Transducer: %s, Technology Implementation: %s",
								sl == 1 ?    "CRT (shadow mask)" :
								sl == 2 ? "CRT (aperture grill)" :
								sl == 3 ?  "LCD (active matrix)" :
								sl == 4 ?                 "LCoS" :
								sl == 5 ?               "Plasma" :
								sl == 6 ?                 "OLED" :
								sl == 7 ?                   "EL" :
								sl == 8 ? "Dynamic MEM e.g. DLP" :
								sl == 9 ? "Static MEM e.g. iMOD" :
								/**/ "Reserved, must be ignored",
								sh == 1 ?        "Direct View CRT" :
								sh == 2 ? "Direct View Flat Panel" :
								sh == 3 ?        "Projection Rear" :
								sh == 4 ?       "Projection Front" :
								sh == 5 ?           "Glasses Mono" :
								sh == 6 ?         "Glasses Stereo" :
								/**/   "Reserved, must be ignored"
							);
							break;
						case 0xc0: // Display Usage Time
							cprintf("%s%d hours",
								mh ? "?" : "", // spec says this should be 0 but my Cinema display has FFFF in MHML
								(mw == 0xffff) ? sw : s24
							);
							break;
						case 0xc9: // Display Firmware Level
							cprintf("%s%d.%d",
								(mw != 0xffff) ? "?" : "",
								sh,
								sl
							);
							break;
						case 0xdf: // VCP Version
							cprintf("%d.%d",
								sh,
								sl
							);
							break;
						case 0xca: // OSD / Button Control
							cprintf("%s%s %s",
								mw ? "?" : "", // spec says these should be 0 by my Cinema display has FFFF in MHML
								sl ==   0 ?                                       "The display does not support Host control of its OSD and may NOT report button “events”." :
								sl ==   1 ?             "Sink OSD & display control disabled, soft and predefined button “Host OSD” events enabled using (VCP 02h/52h/03h)." :
								sl ==   2 ? "Sink OSD & display control enabled if supported, soft and predefined button ”Host OSD” events enabled using (VCP 02h/52h/03h)." :
								sl ==   3 ?                                   "Sink OSD & display control disabled, soft and predefined buttons “Host OSD” events disabled." :
								sl == 255 ?                                                                                    "The display cannot supply this information." :
								/**/                                                                                                                                     "?",
								sh ==   0 ?        "The display does not support Host control of its Power function and may NOT report Power “events”." :
								sh ==   1 ?                               "Power button disabled, power button events enabled using (VCP 02h/52h/03h)." :
								sh ==   2 ?                                "Power button enabled, power button events enabled using (VCP 02h/52h/03h)." :
								sh ==   3 ?                                                      "Power button disabled, power button events disabled." :
								/**/                                                                                                                "?"
							);
							break;
						case 0xd6: // Power Mode
							cprintf("%s",
								sl == 1 ?       "DPM On, DPMS On" :
								sl == 2 ? "DPM Off, DPMS Standby" :
								sl == 3 ? "DPM Off, DPMS Suspend" :
								sl == 4 ?     "DPM Off, DPMS Off" :
								sl == 5 ? "Power off the display" :
								/**/  "Reserved, must be ignored"
							);
							break;
						case 0x0b: // User Color Temperature Increment
							cprintf("%s%d°K",
								sw < 1 || sw > 5000 ? "Invalid" : "",
								sw
							);
							hasUserColorTemperatureIncrement = true;
							UserColorTemperatureIncrement = sw;
							break;
						case 0x0c: // User Color Temperature
							cprintf("%s%d%s",
								mw < sw ? "?" : "",
								hasUserColorTemperatureIncrement ? sw * UserColorTemperatureIncrement + 3000 : sw,
								hasUserColorTemperatureIncrement ? "°K" : " * UserColorTemperatureIncrement + 3000°K"
							);
							break;
						case 0x14: // Select Color Preset
							if (mh) {
								cprintf("%s%s±%d%%",
									sl ==  1 ?                "sRGB" :
									sl ==  2 ?      "Display native" :
									sl ==  3 ?              "4000°K" :
									sl ==  4 ?              "5000°K" :
									sl ==  5 ?              "6500°K" :
									sl ==  6 ?              "7500°K" :
									sl ==  7 ?              "8200°K" :
									sl ==  8 ?              "9300°K" :
									sl ==  9 ?             "10000°K" :
									sl == 10 ?             "11500°K" :
									sl == 11 ?              "User 1" :
									sl == 12 ?              "User 2" :
									sl == 13 ?              "User 3" :
									/**/ "Reserved, must be ignored",
									mh > 10 ? "?" : "",
									mh
								);
							}
							else {
								cprintf("%s",
									sl ==  1 ?                "sRGB" :
									sl ==  2 ?      "Display native" :
									sl ==  3 ?            "Warmer 4" :
									sl ==  4 ?            "Warmer 3" :
									sl ==  5 ?            "Warmer 2" :
									sl ==  6 ?            "Warmer 1" :
									sl ==  7 ?            "Cooler 1" :
									sl ==  8 ?            "Cooler 2" :
									sl ==  9 ?            "Cooler 3" :
									sl == 10 ?            "Cooler 4" :
									sl == 11 ?              "User 1" :
									sl == 12 ?              "User 2" :
									sl == 13 ?              "User 3" :
									/**/ "Reserved, must be ignored"
								);
							}
							break;
						case 0x54: // Performance Preservation
							cprintf("supported:{%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s}, present:{%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s}",
								mw & 0x8000 ?                                                                             "Image “orbiting” mode., " : "",
								mw & 0x4000 ?                                                       "Low luminance mode with “active” video mode., " : "",
								mw & 0x2000 ?                                     "Slow luminance reduction when a static image is detected mode., " : "",
								mw & 0x1000 ?                                   "Slow luminance reduction when no user activity is detected mode., " : "",
								mw & 0x0800 ?                                                                                                "11?, " : "",
								mw & 0x0400 ?                                                                                                "10?, " : "",
								mw & 0x0200 ?                                                                                                 "9?, " : "",
								mw & 0x0100 ?                                                                                                 "8?, " : "",
								mw & 0x0080 ? "A white vertical bar (or line) moving slowly horizontally across the screen on a black background., " : "",
								mw & 0x0040 ?                                                            "A white image filling the display area., " : "",
								mw & 0x0020 ? "A black vertical bar (or line) moving slowly horizontally across the screen on a black background., " : "",
								mw & 0x0010 ?                    "Reverse video ... the displayed image is the inverse color of the source image., " : "",
								mw & 0x0008 ?                                                            "Display is active but video is blanked., " : "",
								mw & 0x0004 ?                                 "A gray scale pattern moving slowly horizontally across the screen., " : "",
								mw & 0x0002 ?                                                                                                 "1?, " : "",
								mw & 0x0001 ?                                                                                                 "0?, " : "",

								sw & 0x8000 ?                                                                             "Image “orbiting” mode., " : "",
								sw & 0x4000 ?                                                       "Low luminance mode with “active” video mode., " : "",
								sw & 0x2000 ?                                     "Slow luminance reduction when a static image is detected mode., " : "",
								sw & 0x1000 ?                                   "Slow luminance reduction when no user activity is detected mode., " : "",
								sw & 0x0800 ?                                                                                                "11?, " : "",
								sw & 0x0400 ?                                                                                                "10?, " : "",
								sw & 0x0200 ?                                                                                                 "9?, " : "",
								sw & 0x0100 ?                                                                                                 "8?, " : "",
								sw & 0x0080 ? "A white vertical bar (or line) moving slowly horizontally across the screen on a black background., " : "",
								sw & 0x0040 ?                                                            "A white image filling the display area., " : "",
								sw & 0x0020 ? "A black vertical bar (or line) moving slowly horizontally across the screen on a black background., " : "",
								sw & 0x0010 ?                    "Reverse video ... the displayed image is the inverse color of the source image., " : "",
								sw & 0x0008 ?                                                            "Display is active but video is blanked., " : "",
								sw & 0x0004 ?                                 "A gray scale pattern moving slowly horizontally across the screen., " : "",
								sw & 0x0002 ?                                                                                                 "1?, " : "",
								sw & 0x0001 ?                                                                                                 "0?, " : ""
							);
							break;
						case 0x60: // Input Select
							cprintf("%s%s",
								m24 ? "?" : "",
								sl ==  1 ?            "Analog video (R/G/B) 1" :
								sl ==  2 ?            "Analog video (R/G/B) 2" :
								sl ==  3 ?      "Digital video (TMDS) 1 DVI 1" :
								sl ==  4 ?      "Digital video (TMDS) 2 DVI 2" :
								sl ==  5 ?                 "Composite video 1" :
								sl ==  6 ?                 "Composite video 2" :
								sl ==  7 ?                         "S-video 1" :
								sl ==  8 ?                         "S-video 2" :
								sl ==  9 ?                           "Tuner 1" :
								sl == 10 ?                           "Tuner 2" :
								sl == 11 ?                           "Tuner 3" :
								sl == 12 ? "Component video (YPbPr / YCbCr) 1" :
								sl == 13 ? "Component video (YPbPr / YCbCr) 2" :
								sl == 14 ? "Component video (YPbPr / YCbCr) 3" :
								sl == 15 ?                     "DisplayPort 1" :
								sl == 16 ?                     "DisplayPort 2" :
								sl == 17 ?     "Digital Video (TMDS) 3 HDMI 1" :
								sl == 18 ?     "Digital Video (TMDS) 4 HDMI 2" :
								/**/                                       "?"
							);
							break;
						case 0x62: // Audio: Speaker Volume
							if (mw != 0)
								cprintf("?%.1f%%", (100.0 * sw) / mw); // this is against spec; assume continuous
							else if (sl > 0 && sl < 255)
								cprintf("%s%.1f%%", sh ? "?" : "", (100.0 * sl) / 254.0);
							else
								cprintf("%s%s",
									m24 ? "?" : "",
									sl ==   0 ? "Fixed (default) level" :
									sl == 254 ?                  "Mute" :
									/**/                            "?"
								);
							break;
						case 0xac: // Horizontal Frequency
							if (!s32)
								cprintf("The display is NOT synchronized to the video input signal.");
							if (s32 == 0xffffffff)
								cprintf("The display cannot synchronize or has determined the input frequency is out of range.");
							else
								cprintf("%.3fkHz", s32 / 1000.0);
							break;
						case 0xae: // Vertical Frequency
							if (!s32)
								cprintf("The display is NOT synchronized to the video input signal.");
							if (s32 == 0xffffffff)
								cprintf("The display cannot synchronize or has determined the input frequency is out of range.");
							else
								cprintf("%.2fHz", s32 / 100.0);
							break;
						case 0xc6: // Application Enable Key
						case 0xde: // Scratch Pad
							cprintf("0x%04x", sw);
							break;
						case 0xc8: // Display Controller ID
							cprintf("Manufacturer:%s Product:0x%06X",
								sl ==   0 ?                                         "Reserved" :
								sl ==   1 ?                                         "Conexant" :
								sl ==   2 ?                                "Genesis Microchip" :
								sl ==   3 ?                                         "Macronix" :
								sl ==   4 ?               "IDT (Integrated Device Technology)" :
								sl ==   5 ?                              "Mstar Semiconductor" :
								sl ==   6 ?                                            "Myson" :
								sl ==   7 ?                                          "Philips" :
								sl ==   8 ?                                       "PixelWorks" :
								sl ==   9 ?                            "RealTek Semiconductor" :
								sl ==  10 ?                                             "Sage" :
								sl ==  11 ?                                    "Silicon Image" :
								sl ==  12 ?                                        "SmartASIC" :
								sl ==  13 ?                               "STMicroelectronics" :
								sl ==  14 ?                                            "Topro" :
								sl ==  15 ?                                         "Trumpion" :
								sl ==  16 ?                                        "Welltrend" :
								sl ==  17 ?                                          "Samsung" :
								sl ==  18 ?                         "Novatek Microelectronics" :
								sl ==  19 ?                                              "STK" :
								sl ==  20 ?                               "Silicon Optix Inc." :
								sl ==  21 ?                                "Texas Instruments" :
								sl ==  22 ?                           "Analogix Semiconductor" :
								sl ==  23 ?                                     "Quantum Data" :
								sl ==  24 ?                               "NXP Semiconductors" :
								sl ==  25 ?                                         "Chrontel" :
								sl ==  26 ?                              "Parade Technologies" :
								sl ==  27 ?                                "THine Electronics" :
								sl ==  28 ?                                          "Trident" :
								sl ==  29 ?                                         "Micronas" :
								sl == 255 ? "Not defined – a manufacturer designed controller" :
								/**/                              "Reserved, must be ignored",
								m24
							);
							break;
						case 0xcc: // OSD Language
							cprintf("%s%s",
								m24 ? "?" : "",
								sl ==  1 ? "Chinese (traditional / Hantai)" :
								sl ==  2 ?                        "English" :
								sl ==  3 ?                         "French" :
								sl ==  4 ?                         "German" :
								sl ==  5 ?                        "Italian" :
								sl ==  6 ?                       "Japanese" :
								sl ==  7 ?                         "Korean" :
								sl ==  8 ?          "Portuguese (Portugal)" :
								sl ==  9 ?                        "Russian" :
								sl == 10 ?                        "Spanish" :
								sl == 11 ?                        "Swedish" :
								sl == 12 ?                        "Turkish" :
								sl == 13 ?  "Chinese (simplified / Kantai)" :
								sl == 14 ?            "Portuguese (Brazil)" :
								sl == 15 ?                         "Arabic" :
								sl == 16 ?                      "Bulgarian" :
								sl == 17 ?                       "Croatian" :
								sl == 18 ?                          "Czech" :
								sl == 19 ?                         "Danish" :
								sl == 20 ?                          "Dutch" :
								sl == 21 ?                       "Estonian" :
								sl == 22 ?                        "Finnish" :
								sl == 23 ?                          "Greek" :
								sl == 24 ?                         "Hebrew" :
								sl == 25 ?                          "Hindi" :
								sl == 26 ?                      "Hungarian" :
								sl == 27 ?                        "Latvian" :
								sl == 28 ?                     "Lithuanian" :
								sl == 29 ?                      "Norwegian" :
								sl == 30 ?                         "Polish" :
								sl == 31 ?                       "Romanian" :
								sl == 32 ?                        "Serbian" :
								sl == 33 ?                         "Slovak" :
								sl == 34 ?                      "Slovenian" :
								sl == 35 ?                           "Thai" :
								sl == 36 ?                      "Ukrainian" :
								sl == 37 ?                     "Vietnamese" :
								/**/            "Reserved, must be ignored"
							);
							break;
						case 0xaa: // Screen Orientation
							cprintf("%s",
								sl ==   1 ?             "0°" :
								sl ==   2 ?            "90°" :
								sl ==   3 ?           "180°" :
								sl ==   4 ?           "270°" :
								sl == 255 ? "Not applicable" :
								/**/              "Reserved"
							);
							break;
						case 0xb2: // Flat Panel Sub-Pixel Layout
							cprintf("%s",
								sl == 0 ?                                                                                           "Sub-pixel layout is not defined" :
								sl == 1 ?                                                                                        "Red / Green / Blue vertical stripe" :
								sl == 2 ?                                                                                      "Red / Green / Blue horizontal stripe" :
								sl == 3 ?                                                                                        "Blue / Green / Red vertical stripe" :
								sl == 4 ?                                                                                      "Blue / Green / Red horizontal stripe" :
								sl == 5 ? "Quad-pixel, a 2 x 2 sub-pixel structure with red at top left, blue at bottom right and green at top right and bottom left" :
								sl == 6 ? "Quad-pixel, a 2 x 2 sub-pixel structure with red at bottom left, blue at top right and green at top left and bottom right" :
								sl == 7 ?                                                                                                             "Delta (triad)" :
								sl == 8 ?                                                                    "Mosaic with interleaved sub-pixels of different colors" :
								/**/                                                                                                      "Reserved, must be ignored"
							);
							break;
						case 0xdc: // Display Application
							cprintf("%s",
								sl ==   0 ?                                        "Stand / default mode" :
								sl ==   1 ?                     "Productivity (e.g. office applications)" :
								sl ==   2 ?           "Mixed (e.g. internet with mix of text and images)" :
								sl ==   3 ?                                                       "Movie" :
								sl ==   4 ?                                                "User defined" :
								sl ==   5 ?                        "Games (e.g. games console / PC game)" :
								sl ==   6 ?                                   "Sports (e.g. fast action)" :
								sl ==   7 ?               "Professional (all signal processing disabled)" :
								sl ==   8 ? "Standard / default mode with intermediate power consumption" :
								sl ==   9 ?          "Standard / default mode with low power consumption" :
								sl ==  10 ?  "Demonstration (used for high visual impact in retail etc.)" :
								sl == 240 ?                                            "Dynamic contrast" :
								/**/                                          "Reserved, must be ignored"
							);
							break;
						case 0x8d: // Audio Mute / Screen Blank
							cprintf("%s, %s, %s, %s",
								mh == 0 ? "Screen Blanking NOT supported" :
								mh == 2 ?     "Screen Blanking supported" :
								/**/                                  "?",
								ml == 0 ?      "Audio Mute NOT supported" :
								ml == 2 ?          "Audio Mute supported" :
								/**/                                  "?",
								sh == 1 ?              "Blank the screen" :
								sh == 2 ?           "Un-blank the screen" :
								/**/          "Reserved, must be ignored",
								sl == 1 ?                "Mute the audio" :
								sl == 2 ?             "Un-mute the audio" :
								/**/          "Reserved, must be ignored"
							);
							break;

							
							

					} // switch vcpcode for present value
					
					cprintf(" }, ");
				}

				cprintf("},%s%s\n", errors[0] ? " // " : "", errors);
				
			} // while vcp
			OUTDENT iprintf("};\n");
		}
		else {
			s = c;
			for (; *c && *c != ')'; c++);
			slen = c - s;
			cprintf("\"%.*s\";\n", (int)slen, s);
		}
		if (*c) c++;
	} // while vcp string
	return c - vcps;
} // parsevcp
