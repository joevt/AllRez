//
//  dpcd.c
//  AllRez
//
//  Created by joevt on 2022-02-19.
//

#include "MacOSMacros.h"
#include <ApplicationServices/ApplicationServices.h>

#include "dpcd.h"

#include <drm/display/drm_dp_helper.h>
#include <drm/display/drm_hdcp.h>
#include "dpcd_defs.h"
#include "dc_dp_types.h"
#include "oui.h"
#include "printf.h"
#include "displayport.h"


int dpcdranges[] = {
		   
	// 0x00000 Receiver Capability Field
	0x00000, 0x000b0,

	// 0x00100 Link Configuration Field
	0x00100, 0x00130,
	0x00150, 0x00180,
	0x001a0, 0x001b0,
	0x001c0, 0x001d0,

	// 0x00200 Link/Sink Status Field
	0x00200, 0x00290,
	0x002c0, 0x00610,

	// 0x00300 Source Device-Specific Field

	// 0x00400 Sink Device-Specific Field

	// 0x00500 Branch Device Specific Field

	// 0x00600 Sink Control Field

	// 0x00700  RESERVED for eDP
	0x00700, 0x00710,
	0x00720, 0x00750,
	0x007a0, 0x007b0,

	// 0x00800 Usage to be Defined

	// 0x01000 Sideband MSG Buffers
	0x01000, 0x01020,
	0x01200, 0x01220,
	0x01400, 0x01420,
	0x01600, 0x01620,

	// 0x02000 ESI (Event Status Indicator) Field
	0x02000, 0x02010,

	// 0x02200 Extended Receiver Capability
	0x02200, 0x02220,
	0x02230, 0x02280,

	// 0x03000 Protocol Converter Extension
	0x03000, 0x03040,
	0x03050, 0x03060,
	0x03100, 0x03190,

//#warning remove this limit when done testing
#if 1
	// 0x68000 HDCP 1.3 and HDCP 2.2
	0x68000, 0x68040,
	0x680c0, 0x68100,

	// 0x69000 DP HDCP2.2 parameter offsets in DPCD address space
	0x69000, 0x69030,
	0x692a0, 0x692d0,
	0x692e0, 0x69300,
	0x69310, 0x69350,
	0x693e0, 0x69400,
	0x69470, 0x69480,
	0x69490, 0x694a0,
	0x69510, 0x69560,

	// 0x70000 Usage to be Defined

	// 0x80000 RESERVED Field for DPCP

	// 0x81000 Remote Command Pass-through Field

	// 0x81000 RESERVED
	// 0xf0000 LTTPR: Link Training (LT)-tunable PHY Repeaters
	0xf0000, 0xf02d0,
	// 0xFFFFF end
#endif
	-1
};

static bool ismemzero(UInt8 *start, UInt8 *stop) {
	for ( ; start < stop; start++) if (*start) return false;
	return true;
}

void parsedpcd(UInt8* dpcd) {
	int dpcdRangeNdx;
	iprintf("dpcd = {\n"); INDENT
	for (dpcdRangeNdx = 0; dpcdranges[dpcdRangeNdx] >= 0; dpcdRangeNdx += 2) {
		for (int dpcdAddr = dpcdranges[dpcdRangeNdx]; dpcdAddr < dpcdranges[dpcdRangeNdx + 1]; dpcdAddr += DP_AUX_MAX_PAYLOAD_BYTES) {
			bool hasBytes = false;
			for (int i = 0; i < DP_AUX_MAX_PAYLOAD_BYTES; i++) {
				if (dpcd[dpcdAddr + i]) {
					hasBytes = true;
					break;
				}
			}
			if (!hasBytes) {
				continue;
			}
			iprintf("%05xh:", dpcdAddr);
			for (int i = 0; i < DP_AUX_MAX_PAYLOAD_BYTES; i++) {
				cprintf(" %02x", dpcd[dpcdAddr + i]);
			}
			cprintf(" // ");
			for (int i = 0; i < DP_AUX_MAX_PAYLOAD_BYTES; i++) {
				cprintf("%c", (dpcd[dpcdAddr + i] >= ' ' && dpcd[dpcdAddr + i] <= '~') ? dpcd[dpcdAddr + i] : '.');
			}
			lf;
		}
	}

// return true if any dpcd registers in the range [start, stop) are zero.
#define iszero(start, stop) ismemzero(&dpcd[start], &dpcd[stop])
	
	char tempname[100];
	int val;
	const char * name;
	const char * tname;
	const char * flag;
	const char * comma;
	int i;

// begin new line (resets comma string)
#define olf lf; comma = ""

// return default register name (regname without the "DP_" prefix)
#define oname(regname) (tname = regname, tname + 3)

// continue printf; includes comma if a previous cp was used on the same line
#define cp(format, ...) do { cprintf("%s" format, comma, ##__VA_ARGS__); comma = ", "; } while (0)

// output linefeed, indent, register address, and register name; first comma is a space character
#define ofield(reg) olf; iprintf("%05xh %s:", reg, name); comma = " ";

// sets register name; if register value is not zero then output linefeed, indent, register address, and register name; and also execute the following block
#define onotzerofield(reg, regname) name = regname; if (val) { ofield(reg) } if (val)

// get register value (little-endian or big-endian)
#define d32(  reg) (d24(reg) | (d8((reg) + 3) << 24))
#define d24(  reg) (d16(reg) | (d8((reg) + 2) << 16))
#define d16(  reg) (d8 (reg) | (d8((reg) + 1) <<  8))
#define d8(   reg) dpcd[reg]
#define d32be(reg) ((d8(reg) << 24) | d24be(reg + 1))
#define d24be(reg) ((d8(reg) << 16) | d16be(reg + 1))
#define d16be(reg) ((d8(reg) <<  8) | d8(   reg + 1))

// get register value and do field with register name if not zero
#define ob32(  reg) val = d32(  reg); onotzerofield(reg, oname(#reg))
#define ob24(  reg) val = d24(  reg); onotzerofield(reg, oname(#reg))
#define ob16(  reg) val = d16(  reg); onotzerofield(reg, oname(#reg))
#define ob(    reg) val = d8(   reg); onotzerofield(reg, oname(#reg))
#define ob32be(reg) val = d32be(reg); onotzerofield(reg, oname(#reg))
#define ob24be(reg) val = d24be(reg); onotzerofield(reg, oname(#reg))
#define ob16be(reg) val = d16be(reg); onotzerofield(reg, oname(#reg))

// get register value and do field using specific register name
#define obn16( reg, n) val = d16(reg) ; name = n ; ofield(reg)
#define obn(   reg, n) val =  d8(reg) ; name = n ; ofield(reg)

// get register value and do field using specific register name (using printf formatting)
#define obf16( reg, n, ...) snprintf(tempname, sizeof(tempname), n, ##__VA_ARGS__); obn16(reg, tempname)
#define obf(   reg, n, ...) snprintf(tempname, sizeof(tempname), n, ##__VA_ARGS__); obn(  reg, tempname)

// get register value and do field with register name; indent and do block x
#define obl(   reg, x)                                                              obn(  reg, oname(#reg)) INDENT x ; OUTDENT
// get register value and do field with register name
#define obx16( reg)                                                                 obn16(reg, oname(#reg))
#define obx(   reg)                                                                 obn(  reg, oname(#reg))

// enumerations
	// switch on masked register value
	#define sw(mask) switch (val & mask)
	// do a case value
	#define oc(val, format, ...) case val : cp(format, ##__VA_ARGS__); comma = ", "; break;
	// do default
	#define od(format, ...) default : cp(format, ##__VA_ARGS__); comma = ", "; break;

// flag bits
	// do a flag bit with flag name (without "DP_" prefix)
	#define of(fv)  if (val & (fv)) { cp("%s", (flag = #fv, flag + 3)); comma = ", "; }
	// do a flag bit with flag names (without "DP_" prefix); first name for set, second name for not set.
	#define of2(fv, fn)  { cp("%s", (flag = (val & (fv)) ? #fv : #fn, flag + 3)); comma = ", "; }
	// do a flag bit using specific flag name (using printf formatting)
	#define ofs(fv, format, ...)  if (val & (fv)) { cp(format, ##__VA_ARGS__); comma = ", "; }
	// do unamed flag bits
	#define ofd(fv) if (val & (fv)) { cp("?0x%02x", val & (fv)); comma = ", "; }

// do one masked integer value in least significant bits
#define oim(iv) { cp("%d", val & iv); comma = ", "; }
// do one integer value
#define oi() { cp("%d", val); comma = ", "; }
// do one label (usually followed by some code to output the value)
#define lb(format, ...) { cp(format " =", ##__VA_ARGS__); comma = " "; }

// do one guid
#define oguid(guid) cp("%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x", d32be(guid), d16be(guid + 4), d16be(guid + 6), d16be(guid + 8), d8(guid + 10), d8(guid + 11), d8(guid + 12), d8(guid + 13), d8(guid + 14), d8(guid + 15))

// dump a range of bytes as hex if they are not zero
#define dumpnotzero(start, stop, ...) \
	do { \
		int _nexti; \
		for (int _i = start; _i < stop; _i = _nexti) { \
			_nexti = (stop <= start + 16) ? stop : (_i & -16) + 16; \
			if (_nexti > stop) _nexti = stop; \
			if (_i == start) { \
				dumpnotzero0(_i, _nexti, ##__VA_ARGS__) \
			} else { \
				dumpnotzero0(_i, _nexti) \
			} \
		} \
	} while(0);
#define dumpnotzero0(start, stop, ...) if (!iszero(start, stop)) { obf(start, "" __VA_ARGS__); for (int _j = start; _j < stop; _j++) cprintf(" %02x", d8(_j)); cprintf(" // "); for (int _j = start; _j < stop; _j++) cprintf("%c", (d8(_j) >= ' ' && d8(_j) <= '~') ? d8(_j) : '.'); }


	if (!iszero(0x00000, 0x00100)) { // looks like a valid dpcd
		olf;

		if (!iszero(0x00000, 0x00100)) {
			iprintf("Receiver Capability"); INDENT

			ob(DP_DPCD_REV) { // 0x000
				cp("%d.%d", (val >> 4) & 0xf, val & 0xf);
			}
			
			ob(DP_MAX_LINK_RATE) { // 0x001
				switch (val) {
					oc( 6, "RBR")
					oc(10, "HBR")
					oc(12, "3_24 (AppleVGA)")
					oc(20, "HBR2")
					oc(30, "HBR3")
					od("?%gGbps (unknown)", 0.27 * val)
				}
			}

			ob(DP_MAX_LANE_COUNT) { // 0x002
				oim(DP_MAX_LANE_COUNT_MASK)
				of(DP_ENHANCED_FRAME_CAP)
				of(DP_TPS3_SUPPORTED)
				ofd(1 << 5)
			}
			
			ob(DP_MAX_DOWNSPREAD) { // 0x003
				of(DP_MAX_DOWNSPREAD_0_5)
				of(DP_STREAM_REGENERATION_STATUS_CAP)
				ofd(0x3c)
				of(DP_NO_AUX_HANDSHAKE_LINK_TRAINING)
				of(DP_TPS4_SUPPORTED)
			}
			
			ob(DP_NORP) { // 0x004
				cp("%d", (val & 1) + 1);
				ofd(0xfe)
			}
			
			ob(DP_DOWNSTREAMPORT_PRESENT) { // 0x005
				of(DP_DWN_STRM_PORT_PRESENT)
				lb("PORT_TYPE")
				switch (val & DP_DWN_STRM_PORT_TYPE_MASK) {
					oc(DP_DWN_STRM_PORT_TYPE_DP, "DisplayPort")
					oc(DP_DWN_STRM_PORT_TYPE_ANALOG, "Analog VGA")
					oc(DP_DWN_STRM_PORT_TYPE_TMDS, "DVI or HDMI")
					oc(DP_DWN_STRM_PORT_TYPE_OTHER, "Other (no EDID)")
				}
				of(DP_FORMAT_CONVERSION)
				of(DP_DETAILED_CAP_INFO_AVAILABLE)
				ofd(0xe0)
			}
			int portsize = (val & DP_DETAILED_CAP_INFO_AVAILABLE) ? 4 : 1;

			if (d8(DP_DPCD_REV) >= 0x20) {
				ob(DP_MAIN_LINK_CHANNEL_CODING_CAP) { // 0x006
					ofd(0xff)
				}
			}
			else {
				ob(DP_MAIN_LINK_CHANNEL_CODING) { // 0x006
					of(DP_CAP_ANSI_8B10B)
					of(DP_CAP_ANSI_128B132B)
					ofd(0xfc)
				}
			}

			ob(DP_DOWN_STREAM_PORT_COUNT) { // 0x007
				oim(DP_PORT_COUNT_MASK)
				ofd(0x30)
				of(DP_MSA_TIMING_PAR_IGNORED)
				of(DP_OUI_SUPPORT)
			}

			ob(DP_RECEIVE_PORT_0_CAP_0) { // 0x008
				ofd(0x01)
				of(DP_LOCAL_EDID_PRESENT) // (1 << 1)
				of(DP_ASSOCIATED_TO_PRECEDING_PORT) // (1 << 2)
				ofd(0xf8)
			}

			ob(DP_RECEIVE_PORT_0_BUFFER_SIZE) { // 0x009
				cp("%d bytes per lane", (val + 1) * 32 );
			}

			ob(DP_RECEIVE_PORT_1_CAP_0) { // 0x00a
				ofd(0xf9)
				of(DP_LOCAL_EDID_PRESENT) // (1 << 1)
				of(DP_ASSOCIATED_TO_PRECEDING_PORT) // (1 << 2)
			}

			ob(DP_RECEIVE_PORT_1_BUFFER_SIZE) { // 0x00b
				cp("%d bytes per lane", (val + 1) * 32 );
			}

			ob(DP_I2C_SPEED_CAP) { // 0x00c    /* DPI */
				ofs(DP_I2C_SPEED_1K  , "1kbps"  ) // 0x01
				ofs(DP_I2C_SPEED_5K  , "5kbps"  ) // 0x02
				ofs(DP_I2C_SPEED_10K , "10kbps" ) // 0x04
				ofs(DP_I2C_SPEED_100K, "100kbps") // 0x08
				ofs(DP_I2C_SPEED_400K, "400kbps") // 0x10
				ofs(DP_I2C_SPEED_1M  , "1Mbps"  ) // 0x20
				ofd(0xc0)
			}

			ob(DP_EDP_CONFIGURATION_CAP) { // 0x00d   /* XXX 1.2? */
				of(DP_ALTERNATE_SCRAMBLER_RESET_CAP) // (1 << 0)
				of(DP_FRAMING_CHANGE_CAP) // (1 << 1)
				of(DP_DPCD_DISPLAY_CONTROL_CAPABLE) // (1 << 3) /* edp v1.2 or higher */
				ofd(0xf8)
			}

			ob(DP_TRAINING_AUX_RD_INTERVAL) { // 0x00e   /* XXX 1.2? */
				sw(DP_TRAINING_AUX_RD_MASK) { // 0x7F    /* DP 1.3 */
					oc(0, "100µs for Main Link Clock Recovery phase, 400µs for Main Link Channel Equalization phase and FAUX training")
					oc(1, "4ms all")
					oc(2, "8ms all")
					oc(3, "12ms all")
					oc(4, "16ms all")
					od("?%dms all (reserved)", (val & DP_TRAINING_AUX_RD_MASK) * 4)
				}
				of(DP_EXTENDED_RECEIVER_CAP_FIELD_PRESENT) // (1 << 7) /* DP 1.3 */
			}

			ob(DP_ADAPTER_CAP) { // 0x00f   /* 1.2 */
				of(DP_FORCE_LOAD_SENSE_CAP) // (1 << 0)
				of(DP_ALTERNATE_I2C_PATTERN_CAP) // (1 << 1)
				ofd(0xfc)
			}

			if (!iszero(DP_SUPPORTED_LINK_RATES, DP_FAUX_CAP)) {
				obx(DP_SUPPORTED_LINK_RATES) { // 0x010 /* eDP 1.4 */
					for (i = 0; i < DP_MAX_SUPPORTED_RATES; i++) {
						val = (d8(DP_SUPPORTED_LINK_RATES + i*2 + 1) << 8) | d8(DP_SUPPORTED_LINK_RATES + i*2);
						if (val) cp("%dkHz", val);
					}
				}
			}

			if (d8(DP_DPCD_REV) >= 0x20) {
				ob(DP_SINK_VIDEO_FALLBACK_FORMATS) { // 0x020   /* 2.0 */
					ofs(DP_FALLBACK_1024x768_60HZ_24BPP, "1024x768 60Hz 24bpp") // (1 << 0)
					ofs(DP_FALLBACK_1280x720_60HZ_24BPP, "1280x720 60Hz 24bpp") // (1 << 1)
					ofs(DP_FALLBACK_1920x1080_60HZ_24BPP, "1920x1080 60Hz 24bpp") // (1 << 2)
					ofd(0xf8)
				}
			} else {
				/* Multiple stream transport */
				ob(DP_FAUX_CAP) { // 0x020   /* 1.2 */
					of(DP_FAUX_CAP_1) // (1 << 0)
					ofd(0xfe)
				}
			}

			ob(DP_MSTM_CAP) { // 0x021   /* 1.2 */
				of(DP_MST_CAP) // (1 << 0)
				of(DP_SINGLE_STREAM_SIDEBAND_MSG) // (1 << 1) /* 2.0 */
				ofd(0xfc)
			}

			ob(DP_NUMBER_OF_AUDIO_ENDPOINTS) { // 0x022   /* 1.2 */
				oi()
			}

			#define DP_AV_SYNC_DATA_BLOCK DP_AV_GRANULARITY
			if (!iszero(DP_AV_SYNC_DATA_BLOCK, DP_RECEIVER_ALPM_CAP)) {
				/* AV_SYNC_DATA_BLOCK                                  1.2 */
				obl(DP_AV_SYNC_DATA_BLOCK,
					obx(DP_AV_GRANULARITY) { // 0x023
						lb("AG_FACTOR")
						sw(DP_AG_FACTOR_MASK) { // (0xf << 0)
							oc(DP_AG_FACTOR_3MS, "3ms") // (0 << 0)
							oc(DP_AG_FACTOR_2MS, "2ms") // (1 << 0)
							oc(DP_AG_FACTOR_1MS, "1ms") // (2 << 0)
							oc(DP_AG_FACTOR_500US, "500µs") // (3 << 0)
							oc(DP_AG_FACTOR_200US, "200µs") // (4 << 0)
							oc(DP_AG_FACTOR_100US, "100µs") // (5 << 0)
							oc(DP_AG_FACTOR_10US, "10µs") // (6 << 0)
							oc(DP_AG_FACTOR_1US, "1µs") // (7 << 0)
							od("?%d (reserved)", val & DP_AG_FACTOR_MASK)
						}
						lb("VG_FACTOR")
						sw(DP_VG_FACTOR_MASK) { // (0xf << 4)
							oc(DP_VG_FACTOR_3MS, "3ms") // (0 << 4)
							oc(DP_VG_FACTOR_2MS, "2ms") // (1 << 4)
							oc(DP_VG_FACTOR_1MS, "1ms") // (2 << 4)
							oc(DP_VG_FACTOR_500US, "500µs") // (3 << 4)
							oc(DP_VG_FACTOR_200US, "200µs") // (4 << 4)
							oc(DP_VG_FACTOR_100US, "100µs") // (5 << 4)
							od("?%d (reserved)", (val & DP_VG_FACTOR_MASK) >> 4)
						}
					}

					#define DP_AUD_DEC_LAT DP_AUD_DEC_LAT0
					ob16(DP_AUD_DEC_LAT) { // 0x024, 0x025
						cp("%d * AG_FACTOR", val);
					}

					#define DP_AUD_PP_LAT DP_AUD_PP_LAT0
					ob16(DP_AUD_PP_LAT) { // 0x026, 0x027
						cp("%d * AG_FACTOR", val);
					}

					ob(DP_VID_INTER_LAT) { // 0x028
						cp("%d * VG_FACTOR", val);
					}

					ob(DP_VID_PROG_LAT) { // 0x029
						cp("%d * VG_FACTOR", val);
					}

					ob(DP_REP_LAT) { // 0x02a
						cp("%dµs", val * 10);
					}

					#define DP_AUD_DEL_INS DP_AUD_DEL_INS0
					ob24(DP_AUD_DEL_INS0) { // 0x02b, 0x02c, 0x02d
						cp("%dµs", val);
					}
				)
				/* End of AV_SYNC_DATA_BLOCK */
			}

			ob(DP_RECEIVER_ALPM_CAP) { // 0x02e   /* eDP 1.4 */
				of(DP_ALPM_CAP) // (1 << 0)
				ofd(0xfe)
			}

			ob(DP_SINK_DEVICE_AUX_FRAME_SYNC_CAP) { // 0x02f   /* eDP 1.4 */
				of(DP_AUX_FRAME_SYNC_CAP) // (1 << 0)
				ofd(0xfe)
			}

			if (!iszero(DP_GUID, DP_GUID + 16)) {
				obx(DP_GUID) // 0x030   /* 1.2 */
				oguid(DP_GUID);
			}
			
			dumpnotzero(0x40, 0x054)

			#define DP_RX_GTC_VALUE 0x054
			ob32(DP_RX_GTC_VALUE) { // 0x054
				cp("0x%08x", val);
			}

			#define DP_RX_GTC 0x058
			# define DP_RX_GTC_MSTR_REQ (1 << 0)
			# define DP_TX_GTC_VALUE_PHASE_SKEW_EN (1 << 1)
			ob(DP_RX_GTC) { // 0x058
				of(DP_RX_GTC_MSTR_REQ)
				of(DP_TX_GTC_VALUE_PHASE_SKEW_EN)
				ofd(0xfc)
			}

			dumpnotzero(0x59, 0x060)
			
			if (!iszero(DP_DSC_SUPPORT, DP_DSC_BITS_PER_PIXEL_INC + 1) || !iszero(DP_DSC_ENABLE, DP_DSC_CONFIGURATION + 1)) {
				obx(DP_DSC_SUPPORT) { // 0x060   /* DP 1.4 */
					of(DP_DSC_DECOMPRESSION_IS_SUPPORTED) // (1 << 0)
					ofd(0xfe)
				}

				obx(DP_DSC_REV) { // 0x061
					cp("%d.%d", (val & DP_DSC_MAJOR_MASK) >> DP_DSC_MAJOR_SHIFT, (val & DP_DSC_MINOR_MASK) >> DP_DSC_MINOR_SHIFT);
				}

				obx(DP_DSC_RC_BUF_BLK_SIZE) { // 0x062
					switch (val & 3) {
						oc(DP_DSC_RC_BUF_BLK_SIZE_1,   "1kB") // 0x0
						oc(DP_DSC_RC_BUF_BLK_SIZE_4,   "4kB") // 0x1
						oc(DP_DSC_RC_BUF_BLK_SIZE_16, "16kB") // 0x2
						oc(DP_DSC_RC_BUF_BLK_SIZE_64, "64kB") // 0x3
					}
					ofd(0xfc)
				}

				obx(DP_DSC_RC_BUF_SIZE) { // 0x063
					cp("%d * DSC_RC_BUF_BLK_SIZE", val);
				}

				if (d8(DP_DSC_SLICE_CAP_1) || d8(DP_DSC_SLICE_CAP_2)) {
					obf(DP_DSC_SLICE_CAP_1, "DSC_SLICE_CAP_1 & 2")  // 0x064

					ofs(DP_DSC_1_PER_DP_DSC_SINK, "1") // (1 << 0)
					ofs(DP_DSC_2_PER_DP_DSC_SINK, "2") // (1 << 1)
					ofd(1 << 2)
					ofs(DP_DSC_4_PER_DP_DSC_SINK, "4") // (1 << 3)
					ofs(DP_DSC_6_PER_DP_DSC_SINK, "6") // (1 << 4)
					ofs(DP_DSC_8_PER_DP_DSC_SINK, "8") // (1 << 5)
					ofs(DP_DSC_10_PER_DP_DSC_SINK, "10") // (1 << 6)
					ofs(DP_DSC_12_PER_DP_DSC_SINK, "12") // (1 << 7)

					val = d8(DP_DSC_SLICE_CAP_2); // 0x06D
					ofs(DP_DSC_16_PER_DP_DSC_SINK, "16") // (1 << 0)
					ofs(DP_DSC_20_PER_DP_DSC_SINK, "20") // (1 << 1)
					ofs(DP_DSC_24_PER_DP_DSC_SINK, "24") // (1 << 2)
					ofd(0xf8)

					cprintf(" max slices per DisplayPort DSC sink");
				}

				obx(DP_DSC_LINE_BUF_BIT_DEPTH) { // 0x065
					sw(DP_DSC_LINE_BUF_BIT_DEPTH_MASK) { // (0xf << 0)
						oc(DP_DSC_LINE_BUF_BIT_DEPTH_9 ,  "9 bits") // 0x0
						oc(DP_DSC_LINE_BUF_BIT_DEPTH_10, "10 bits") // 0x1
						oc(DP_DSC_LINE_BUF_BIT_DEPTH_11, "11 bits") // 0x2
						oc(DP_DSC_LINE_BUF_BIT_DEPTH_12, "12 bits") // 0x3
						oc(DP_DSC_LINE_BUF_BIT_DEPTH_13, "13 bits") // 0x4
						oc(DP_DSC_LINE_BUF_BIT_DEPTH_14, "14 bits") // 0x5
						oc(DP_DSC_LINE_BUF_BIT_DEPTH_15, "15 bits") // 0x6
						oc(DP_DSC_LINE_BUF_BIT_DEPTH_16, "16 bits") // 0x7
						oc(DP_DSC_LINE_BUF_BIT_DEPTH_8 ,  "8 bits") // 0x8
						od("?%d (unknown bits)", val)
					}
					ofd(0xf0)
				}

				obx(DP_DSC_BLK_PREDICTION_SUPPORT) { // 0x066
					of(DP_DSC_BLK_PREDICTION_IS_SUPPORTED) // (1 << 0)
					ofd(0xfe)
				}

				#define DP_DSC_MAX_BITS_PER_PIXEL DP_DSC_MAX_BITS_PER_PIXEL_LOW
				obx16(DP_DSC_MAX_BITS_PER_PIXEL) { // 0x067, 0x068   /* eDP 1.4 */
					cp("%g bpp", (val & 0x3ff) / 16.0);
					ofd(0xfc00)
				}

				obx(DP_DSC_DEC_COLOR_FORMAT_CAP) { // 0x069
					ofs(DP_DSC_RGB            , "RGB"               ) // (1 << 0)
					ofs(DP_DSC_YCbCr444       , "YCbCr 4:4:4"       ) // (1 << 1)
					ofs(DP_DSC_YCbCr422_Simple, "YCbCr 4:2:2 Simple") // (1 << 2)
					ofs(DP_DSC_YCbCr422_Native, "YCbCr 4:2:2 Native") // (1 << 3)
					ofs(DP_DSC_YCbCr420_Native, "YCbCr 4:2:0 Native") // (1 << 4)
					ofd(0xe0)
				}

				#define DP_DSC_16_BPC (1 << 5) // I saw this in https://www.quantumdata.com/assets/displayport_dsc_protocols_webinar.pdf
				obx(DP_DSC_DEC_COLOR_DEPTH_CAP) { // 0x06A
					ofd(0x01)
					ofs(DP_DSC_8_BPC ,  "8") // (1 << 1)
					ofs(DP_DSC_10_BPC, "10") // (1 << 2)
					ofs(DP_DSC_12_BPC, "12") // (1 << 3)
					ofd(0x10)
					ofs(DP_DSC_16_BPC, "16") // (1 << 5)
					ofd(0xc0)
					cprintf(" bpc");
				}

				obx(DP_DSC_PEAK_THROUGHPUT) { // 0x06B
					lb("MODE_0")
					sw(DP_DSC_THROUGHPUT_MODE_0_MASK) { // (0xf << 0)
						oc(DP_DSC_THROUGHPUT_MODE_0_UNSUPPORTED, "Unsupported") // 0
						oc(DP_DSC_THROUGHPUT_MODE_0_340,   "340 Mp/s") // (1 << 0)
						oc(DP_DSC_THROUGHPUT_MODE_0_400,   "400 Mp/s") // (2 << 0)
						oc(DP_DSC_THROUGHPUT_MODE_0_450,   "450 Mp/s") // (3 << 0)
						oc(DP_DSC_THROUGHPUT_MODE_0_500,   "500 Mp/s") // (4 << 0)
						oc(DP_DSC_THROUGHPUT_MODE_0_550,   "550 Mp/s") // (5 << 0)
						oc(DP_DSC_THROUGHPUT_MODE_0_600,   "600 Mp/s") // (6 << 0)
						oc(DP_DSC_THROUGHPUT_MODE_0_650,   "650 Mp/s") // (7 << 0)
						oc(DP_DSC_THROUGHPUT_MODE_0_700,   "700 Mp/s") // (8 << 0)
						oc(DP_DSC_THROUGHPUT_MODE_0_750,   "750 Mp/s") // (9 << 0)
						oc(DP_DSC_THROUGHPUT_MODE_0_800,   "800 Mp/s") // (10 << 0)
						oc(DP_DSC_THROUGHPUT_MODE_0_850,   "850 Mp/s") // (11 << 0)
						oc(DP_DSC_THROUGHPUT_MODE_0_900,   "900 Mp/s") // (12 << 0)
						oc(DP_DSC_THROUGHPUT_MODE_0_950,   "950 Mp/s") // (13 << 0)
						oc(DP_DSC_THROUGHPUT_MODE_0_1000, "1000 Mp/s") // (14 << 0)
						oc(DP_DSC_THROUGHPUT_MODE_0_170,   "170 Mp/s") // (15 << 0) /* 1.4a */
					}
					lb("MODE_1")
					sw(DP_DSC_THROUGHPUT_MODE_1_MASK) { // (0xf << 4)
						oc(DP_DSC_THROUGHPUT_MODE_1_UNSUPPORTED, "Unsupported") // 0
						oc(DP_DSC_THROUGHPUT_MODE_1_340,   "340 Mp/s") // (1 << 4)
						oc(DP_DSC_THROUGHPUT_MODE_1_400,   "400 Mp/s") // (2 << 4)
						oc(DP_DSC_THROUGHPUT_MODE_1_450,   "450 Mp/s") // (3 << 4)
						oc(DP_DSC_THROUGHPUT_MODE_1_500,   "500 Mp/s") // (4 << 4)
						oc(DP_DSC_THROUGHPUT_MODE_1_550,   "550 Mp/s") // (5 << 4)
						oc(DP_DSC_THROUGHPUT_MODE_1_600,   "600 Mp/s") // (6 << 4)
						oc(DP_DSC_THROUGHPUT_MODE_1_650,   "650 Mp/s") // (7 << 4)
						oc(DP_DSC_THROUGHPUT_MODE_1_700,   "700 Mp/s") // (8 << 4)
						oc(DP_DSC_THROUGHPUT_MODE_1_750,   "750 Mp/s") // (9 << 4)
						oc(DP_DSC_THROUGHPUT_MODE_1_800,   "800 Mp/s") // (10 << 4)
						oc(DP_DSC_THROUGHPUT_MODE_1_850,   "850 Mp/s") // (11 << 4)
						oc(DP_DSC_THROUGHPUT_MODE_1_900,   "900 Mp/s") // (12 << 4)
						oc(DP_DSC_THROUGHPUT_MODE_1_950,   "950 Mp/s") // (13 << 4)
						oc(DP_DSC_THROUGHPUT_MODE_1_1000, "1000 Mp/s") // (14 << 4)
						oc(DP_DSC_THROUGHPUT_MODE_1_170,   "170 Mp/s") // (15 << 4)
					}
				}

				obx(DP_DSC_MAX_SLICE_WIDTH) { // 0x06C
					cp("%d pixels", val * DP_DSC_SLICE_WIDTH_MULTIPLIER);
				}

		#if 0
				// this might not be in the DisplayPort 1.4 spec? I saw this in https://www.quantumdata.com/assets/displayport_dsc_protocols_webinar.pdf
				#define DP_DSC_MIN_BITS_PER_PIXEL 0x06E
				ob16(DP_DSC_MIN_BITS_PER_PIXEL) { // 0x06E, 0x06F
					cp("%g bpp", (val & 0x3ff) / 16.0);
					ofd(0xfc00)
				}
		#else
				dumpnotzero(0x06e, 0x06f)
		#endif

				obx(DP_DSC_BITS_PER_PIXEL_INC) { // 0x06F // differs from https://www.quantumdata.com/assets/displayport_dsc_protocols_webinar.pdf
					switch (val & 7) {
						oc(DP_DSC_BITS_PER_PIXEL_1_16, "1/16 bpp") // 0
						oc(DP_DSC_BITS_PER_PIXEL_1_8 ,  "1/8 bpp") // 1
						oc(DP_DSC_BITS_PER_PIXEL_1_4 ,  "1/4 bpp") // 2
						oc(DP_DSC_BITS_PER_PIXEL_1_2 ,  "1/2 bpp") // 3
						oc(DP_DSC_BITS_PER_PIXEL_1   ,    "1 bpp") // 4
						od("?%d (unknown)", val & 7)
					}
					ofd(0xf8)
				}
			} // if DP_DSC_SUPPORT .. DP_DSC_BITS_PER_PIXEL_INC

			ob(DP_PSR_SUPPORT) { // 0x070   /* XXX 1.2? */
				switch (val) {
					oc(DP_PSR_IS_SUPPORTED, "PSR_IS_SUPPORTED") // 1
					oc(DP_PSR2_IS_SUPPORTED, "PSR2_IS_SUPPORTED") // 2	    /* eDP 1.4 */
					oc(DP_PSR2_WITH_Y_COORD_IS_SUPPORTED, "PSR2_WITH_Y_COORD_IS_SUPPORTED") // 3	    /* eDP 1.4a */
					oc(DP_PSR2_WITH_Y_COORD_ET_SUPPORTED, "PSR2_WITH_Y_COORD_ET_SUPPORTED") //  4	    /* eDP 1.5, adopted eDP 1.4b SCR */
					od("?%d (unknown)", val)
				}
			}

			ob(DP_PSR_CAPS) { // 0x071   /* XXX 1.2? */
				of(DP_PSR_NO_TRAIN_ON_EXIT)
				lb("PSR_SETUP_TIME")
				sw(DP_PSR_SETUP_TIME_MASK) {
					oc(DP_PSR_SETUP_TIME_330, "330µs") // (0 << 1)
					oc(DP_PSR_SETUP_TIME_275, "275µs") // (1 << 1)
					oc(DP_PSR_SETUP_TIME_220, "220µs") // (2 << 1)
					oc(DP_PSR_SETUP_TIME_165, "165µs") // (3 << 1)
					oc(DP_PSR_SETUP_TIME_110, "110µs") // (4 << 1)
					oc(DP_PSR_SETUP_TIME_55,   "55µs") // (5 << 1)
					oc(DP_PSR_SETUP_TIME_0,     "0µs") // (6 << 1)
					od("?%d (unknown)", val)
				}
				of(DP_PSR2_SU_Y_COORDINATE_REQUIRED)     // (1 << 4)  /* eDP 1.4a */
				of(DP_PSR2_SU_GRANULARITY_REQUIRED)      // (1 << 5)  /* eDP 1.4b */
				of(DP_PSR2_SU_AUX_FRAME_SYNC_NOT_NEEDED) // (1 << 6)  /* eDP 1.5, adopted eDP 1.4b SCR */
				ofd(0xc0)
			}

			ob16(DP_PSR2_SU_X_GRANULARITY) { // 0x072 /* eDP 1.4b */
				oi()
			}

			ob(DP_PSR2_SU_Y_GRANULARITY) { // 0x074 /* eDP 1.4b */
				oi()
			}
			
			dumpnotzero(0x075, 0x080)

			/*
			* 0x80-0x8f describe downstream port capabilities, but there are two layouts
			* based on whether DP_DETAILED_CAP_INFO_AVAILABLE was set.  If it was not,
			* each port's descriptor is one byte wide.  If it was set, each port's is
			* four bytes wide, starting with the one byte from the base info.  As of
			* DP interop v1.1a only VGA defines additional detail.
			*/

			for (int portreg = DP_DOWNSTREAM_PORT_0, portnumber = 0; portreg < 0x90; portreg += portsize, portnumber++) {
				/* offset 0 */
				if (!iszero(portreg, portreg + portsize)) {
					int porttype;
					obf(portreg, "DOWNSTREAM_PORT_%d", portnumber) { // 0x80
						porttype = val & DP_DS_PORT_TYPE_MASK; // (7 << 0)
						lb("PORT_TYPE")
						switch (porttype) {
							oc(DP_DS_PORT_TYPE_DP,          "DisplayPort") // 0
							oc(DP_DS_PORT_TYPE_VGA,         "VGA") // 1
							oc(DP_DS_PORT_TYPE_DVI,         "DVI") // 2
							oc(DP_DS_PORT_TYPE_HDMI,        "HDMI") // 3
							oc(DP_DS_PORT_TYPE_NON_EDID,    "non EDID") // 4
							oc(DP_DS_PORT_TYPE_DP_DUALMODE, "DisplayPort Dual Mode") // 5
							oc(DP_DS_PORT_TYPE_WIRELESS,    "Wireless") // 6
							od("?%d (reserved)", val & DP_DS_PORT_TYPE_MASK)
						}
						ofs(DP_DS_PORT_HPD, "HPD aware") // (1 << 3)
						if (porttype == DP_DS_PORT_TYPE_NON_EDID) {
							lb("NON_EDID")
							sw(DP_DS_NON_EDID_MASK) { // (0xf << 4)
								oc(DP_DS_NON_EDID_720x480i_60,   "720x480i 60Hz") // (1 << 4)
								oc(DP_DS_NON_EDID_720x480i_50,   "720x480i 50Hz") // (2 << 4)
								oc(DP_DS_NON_EDID_1920x1080i_60, "1920x1080i 60Hz") // (3 << 4)
								oc(DP_DS_NON_EDID_1920x1080i_50, "1920x1080i 50Hz") // (4 << 4)
								oc(DP_DS_NON_EDID_1280x720_60,   "1280x720 60Hz") // (5 << 4)
								oc(DP_DS_NON_EDID_1280x720_50,   "1280x720 50Hz") // (7 << 4)
								od("?%d (reserved)", (val & DP_DS_PORT_TYPE_MASK) >> 4)
							}
						}
						else {
							ofd(DP_DS_NON_EDID_MASK)
						}
					}
					
					if (portsize > 1) {
						val = d8(portreg + 1);
						if (porttype == DP_DS_PORT_TYPE_VGA) {
							/* offset 1 for VGA is maximum megapixels per second / 8 */
							cp("%d Mp/s max", val * 8);
						}
						else if (porttype == DP_DS_PORT_TYPE_DVI || porttype == DP_DS_PORT_TYPE_HDMI) {
							/* offset 1 for DVI/HDMI is maximum TMDS clock in Mbps / 2.5 */
							cp("%g MHz max TMDS clock", val * 2.5);
						}
						else {
							ofd(0xff)
						}

						val = d8(portreg + 2);
						if (porttype == DP_DS_PORT_TYPE_VGA || porttype == DP_DS_PORT_TYPE_DVI || porttype == DP_DS_PORT_TYPE_HDMI) {
							/* offset 2 for VGA/DVI/HDMI */
							lb("DS_MAX_BPC")
							sw(DP_DS_MAX_BPC_MASK) { // (3 << 0)
								oc(DP_DS_8BPC,   "8bpc") // 0
								oc(DP_DS_10BPC, "10bpc") // 1
								oc(DP_DS_12BPC, "12bpc") // 2
								oc(DP_DS_16BPC, "16bpc") // 3
							}
						}
						else {
							ofd(DP_DS_MAX_BPC_MASK)
						}
						if (porttype == DP_DS_PORT_TYPE_HDMI) {
							/* HDMI2.1 PCON FRL CONFIGURATION */
							lb("PCON_MAX_FRL_BW")
							sw (DP_PCON_MAX_FRL_BW) {
								oc(DP_PCON_MAX_0GBPS,   "0Gbps") // (0 << 2)
								oc(DP_PCON_MAX_9GBPS,   "9Gbps") // (1 << 2)
								oc(DP_PCON_MAX_18GBPS, "18Gbps") // (2 << 2)
								oc(DP_PCON_MAX_24GBPS, "24Gbps") // (3 << 2)
								oc(DP_PCON_MAX_32GBPS, "32Gbps") // (4 << 2)
								oc(DP_PCON_MAX_40GBPS, "40Gbps") // (5 << 2)
								oc(DP_PCON_MAX_48GBPS, "48Gbps") // (6 << 2)
								od("?%d (reserved)", (val & DP_PCON_MAX_FRL_BW) >> 2)
							}
							of(DP_PCON_SOURCE_CTL_MODE) // (1 << 5)
						}
						else {
							ofd(DP_PCON_MAX_FRL_BW | DP_PCON_SOURCE_CTL_MODE)
						}
						ofd(0xc0)
					
						val = d8(portreg + 3);
						if (porttype == DP_DS_PORT_TYPE_DVI) {
							/* offset 3 for DVI */
							ofs(DP_DS_DVI_DUAL_LINK, "DVI_DUAL_LINK") // (1 << 1)
							ofs(DP_DS_DVI_HIGH_COLOR_DEPTH, "HIGH_COLOR_DEPTH") // (1 << 2)
							ofd(0xf9)
						}
						else if (porttype == DP_DS_PORT_TYPE_HDMI) {
							/* offset 3 for HDMI */
							ofs(DP_DS_HDMI_FRAME_SEQ_TO_FRAME_PACK, "FRAME_SEQ_TO_FRAME_PACK") // (1 << 0)
							ofs(DP_DS_HDMI_YCBCR422_PASS_THROUGH  , "YCBCR422_PASS_THROUGH"  ) // (1 << 1)
							ofs(DP_DS_HDMI_YCBCR420_PASS_THROUGH  , "YCBCR420_PASS_THROUGH"  ) // (1 << 2)
							ofs(DP_DS_HDMI_YCBCR444_TO_422_CONV   , "YCBCR444_TO_422_CONV"   ) // (1 << 3)
							ofs(DP_DS_HDMI_YCBCR444_TO_420_CONV   , "YCBCR444_TO_420_CONV"   ) // (1 << 4)

							/*
							* VESA DP-to-HDMI PCON Specification adds caps for colorspace
							* conversion in DFP cap DPCD 83h. Sec6.1 Table-3.
							* Based on the available support the source can enable
							* color conversion by writing into PROTOCOL_COVERTER_CONTROL_2
							* DPCD 3052h.
							*/
							ofs(DP_DS_HDMI_BT601_RGB_YCBCR_CONV,  "BT601_RGB_YCBCR_CONV" ) // (1 << 5)
							ofs(DP_DS_HDMI_BT709_RGB_YCBCR_CONV,  "BT709_RGB_YCBCR_CONV" ) // (1 << 6)
							ofs(DP_DS_HDMI_BT2020_RGB_YCBCR_CONV, "BT2020_RGB_YCBCR_CONV") // (1 << 7)
						}
						else {
							ofd(0xff)
						}
					} // if portsize > 1
				} // if port
			} // for port

			# define DP_PARITY_BLOCK_ERROR_COUNT_CAP (1 << 4)
			# define DP_PARITY_ERROR_COUNT_CAP (1 << 5)
			# define DP_FEC_ERROR_REPORTING_POLICY_SUPPORTED (1 << 7)

			/* DP Forward error Correction Registers */
			ob(DP_FEC_CAPABILITY) { // 0x090    /* 1.4 */
				of(DP_FEC_CAPABLE) // (1 << 0)
				of(DP_FEC_UNCORR_BLK_ERROR_COUNT_CAP) // (1 << 1)
				of(DP_FEC_CORR_BLK_ERROR_COUNT_CAP) // (1 << 2)
				of(DP_FEC_BIT_ERROR_COUNT_CAP) // (1 << 3)
				of(DP_PARITY_BLOCK_ERROR_COUNT_CAP) // (1 << 4)
				of(DP_PARITY_ERROR_COUNT_CAP) // (1 << 5)
				of(DP_FEC_ERROR_REPORTING_POLICY_SUPPORTED) // (1 << 7)
				ofd(0x40)
			}

			ob(DP_FEC_CAPABILITY_1) { // 0x091   /* 2.0 */
				union dp_fec_capability1 *fec_cap1 = (union dp_fec_capability1 *)&dpcd[DP_FEC_CAPABILITY_1];
				if (fec_cap1->bits.AGGREGATED_ERROR_COUNTERS_CAPABLE) {
					cp("AGGREGATED_ERROR_COUNTERS_CAPABLE");
				}
				ofd(0xfe)
			}

			/* DP-HDMI2.1 PCON DSC ENCODER SUPPORT */
			// DP_PCON_DSC_ENCODER_CAP_SIZE        0xD	/* 0x92 through 0x9E */
			ob(DP_PCON_DSC_ENCODER) { // 0x092
				of(DP_PCON_DSC_ENCODER_SUPPORTED) // (1 << 0)
				of(DP_PCON_DSC_PPS_ENC_OVERRIDE) // (1 << 1)
				ofd(0xfc)
			}

			/* DP-HDMI2.1 PCON DSC Version */
			ob(DP_PCON_DSC_VERSION) { // 0x093
				cp("%d.%d", (val & DP_PCON_DSC_MAJOR_MASK) >> DP_PCON_DSC_MAJOR_SHIFT, (val & DP_PCON_DSC_MINOR_MASK) >> DP_PCON_DSC_MINOR_SHIFT);
			}

			/* DP-HDMI2.1 PCON DSC RC Buffer block size */
			if (d8(DP_PCON_DSC_RC_BUF_SIZE) || d8(DP_PCON_DSC_RC_BUF_BLK_INFO)) {
				obx(DP_PCON_DSC_RC_BUF_BLK_INFO) { // 0x094
					lb("PCON_DSC_RC_BUF_BLK_SIZE")
					sw(DP_PCON_DSC_RC_BUF_BLK_SIZE) { // (0x3 << 0)
						oc(DP_PCON_DSC_RC_BUF_BLK_1KB,   "1kB") // 0
						oc(DP_PCON_DSC_RC_BUF_BLK_4KB,   "4kB") // 1
						oc(DP_PCON_DSC_RC_BUF_BLK_16KB, "16kB") // 2
						oc(DP_PCON_DSC_RC_BUF_BLK_64KB, "64kB") // 3
					}
					ofd(0xfc)
				}
			}

			/* DP-HDMI2.1 PCON DSC RC Buffer size */
			ob(DP_PCON_DSC_RC_BUF_SIZE) { // 0x095
				cp("%d * PCON_DSC_RC_BUF_BLK_SIZE", val);
			}
					
			if (d8(DP_PCON_DSC_SLICE_CAP_1) || d8(DP_PCON_DSC_SLICE_CAP_2)) {
				/* DP-HDMI2.1 PCON DSC Slice capabilities-1 */
				obf(DP_PCON_DSC_SLICE_CAP_1, "PCON_DSC_SLICE_CAP_1 & 2")  // 0x096

				ofs(DP_PCON_DSC_1_PER_DSC_ENC,   "1") // (1 << 0)
				ofs(DP_PCON_DSC_2_PER_DSC_ENC,   "2") // (1 << 1)
				ofd(1 << 2)
				ofs(DP_PCON_DSC_4_PER_DSC_ENC,   "4") // (1 << 3)
				ofs(DP_PCON_DSC_6_PER_DSC_ENC,   "6") // (1 << 4)
				ofs(DP_PCON_DSC_8_PER_DSC_ENC,   "8") // (1 << 5)
				ofs(DP_PCON_DSC_10_PER_DSC_ENC, "10") // (1 << 6)
				ofs(DP_PCON_DSC_12_PER_DSC_ENC, "12") // (1 << 7)

				/* DP-HDMI2.1 PCON DSC Slice capabilities-2 */
				val = d8(DP_PCON_DSC_SLICE_CAP_2); // 0x09C
				ofs(DP_PCON_DSC_16_PER_DSC_ENC, "16") // (1 << 0)
				ofs(DP_PCON_DSC_20_PER_DSC_ENC, "20") // (1 << 1)
				ofs(DP_PCON_DSC_24_PER_DSC_ENC, "24") // (1 << 2)
				ofd(0xf8)

				cprintf(" max slices per DSC encoder");
			}

			ob(DP_PCON_DSC_BUF_BIT_DEPTH) { // 0x097
				sw(DP_PCON_DSC_BIT_DEPTH_MASK) { // (0xF << 0)
					oc(DP_PCON_DSC_DEPTH_9_BITS ,  "9 bits") // 0
					oc(DP_PCON_DSC_DEPTH_10_BITS, "10 bits") // 1
					oc(DP_PCON_DSC_DEPTH_11_BITS, "11 bits") // 2
					oc(DP_PCON_DSC_DEPTH_12_BITS, "12 bits") // 3
					oc(DP_PCON_DSC_DEPTH_13_BITS, "13 bits") // 4
					oc(DP_PCON_DSC_DEPTH_14_BITS, "14 bits") // 5
					oc(DP_PCON_DSC_DEPTH_15_BITS, "15 bits") // 6
					oc(DP_PCON_DSC_DEPTH_16_BITS, "16 bits") // 7
					oc(DP_PCON_DSC_DEPTH_8_BITS ,  "8 bits") // 8
					od("?%d (unknown bits)", val)
				}
			}

			ob(DP_PCON_DSC_BLOCK_PREDICTION) { // 0x098
				of(DP_PCON_DSC_BLOCK_PRED_SUPPORT) // (1 << 0)
				ofd(0xfe)
			}

			ob(DP_PCON_DSC_ENC_COLOR_FMT_CAP) { // 0x099
				ofs(DP_PCON_DSC_ENC_RGB     , "RGB"               ) // (1 << 0)
				ofs(DP_PCON_DSC_ENC_YUV444  , "YCbCr 4:4:4"       ) // (1 << 1)
				ofs(DP_PCON_DSC_ENC_YUV422_S, "YCbCr 4:2:2 Simple") // (1 << 2)
				ofs(DP_PCON_DSC_ENC_YUV422_N, "YCbCr 4:2:2 Native") // (1 << 3)
				ofs(DP_PCON_DSC_ENC_YUV420_N, "YCbCr 4:2:0 Native") // (1 << 4)
				ofd(0xe0)
			}

			#define DP_PCON_DSC_ENC_16BPC (1 << 5) // I made this one up like in https://www.quantumdata.com/assets/displayport_dsc_protocols_webinar.pdf
			ob(DP_PCON_DSC_ENC_COLOR_DEPTH_CAP) { // 0x09A
				ofd(0x01)
				ofs(DP_PCON_DSC_ENC_8BPC ,  "8") // (1 << 1)
				ofs(DP_PCON_DSC_ENC_10BPC, "10") // (1 << 2)
				ofs(DP_PCON_DSC_ENC_12BPC, "12") // (1 << 3)
				ofd(0x10)
				ofs(DP_PCON_DSC_ENC_16BPC, "16") // (1 << 5)
				ofd(0xc0)
				cprintf(" bpc");
			}

			ob(DP_PCON_DSC_MAX_SLICE_WIDTH) { // 0x09B
				cp("%d pixels", val * DP_DSC_SLICE_WIDTH_MULTIPLIER);
			}

			/* DP-HDMI2.1 PCON HDMI TX Encoder Bits/pixel increment */
			ob(DP_PCON_DSC_BPP_INCR) { // 0x09E
				sw(DP_PCON_DSC_BPP_INCR_MASK) { // (0x7 << 0)
					oc(DP_PCON_DSC_ONE_16TH_BPP, "1/16 bpp") // 0
					oc(DP_PCON_DSC_ONE_8TH_BPP ,  "1/8 bpp") // 1
					oc(DP_PCON_DSC_ONE_4TH_BPP ,  "1/4 bpp") // 2
					oc(DP_PCON_DSC_ONE_HALF_BPP,  "1/2 bpp") // 3
					oc(DP_PCON_DSC_ONE_BPP     ,    "1 bpp") // 4
					od("?%d (unknown)", val & DP_PCON_DSC_BPP_INCR_MASK)
				}
				ofd(0xf8)
			}
			
			dumpnotzero(0x09F, 0x0A0)

			/* DP Extended DSC Capabilities */
			ob(DP_DSC_BRANCH_OVERALL_THROUGHPUT_0) { // 0x0a0   /* DP 1.4a SCR */
				if (val == 1)
					cp("%d Mp/s", 680);
				else
					cp("%d Mp/s", val * 50 + 600);
			}
			ob(DP_DSC_BRANCH_OVERALL_THROUGHPUT_1) { // 0x0a1
				if (val == 1)
					cp("%d Mp/s", 680);
				else
					cp("%d Mp/s", val * 50 + 600);
			}
			ob(DP_DSC_BRANCH_MAX_LINE_WIDTH) { // 0x0a2
				cp("%d pixels", val * DP_DSC_SLICE_WIDTH_MULTIPLIER);
			}

			/* DFP Capability Extension */
			if (!iszero(DP_DFP_CAPABILITY_EXTENSION_SUPPORT, DP_DFP_CAPABILITY_EXTENSION_SUPPORT + sizeof(union dp_dfp_cap_ext))) {
				union dp_dfp_cap_ext *dp_dfp = (union dp_dfp_cap_ext *)&dpcd[DP_DFP_CAPABILITY_EXTENSION_SUPPORT];

				obf(DP_DFP_CAPABILITY_EXTENSION_SUPPORT + (int)offsetof(union dp_dfp_cap_ext, fields.supported), "DFP_CAPABILITY_EXTENSION_SUPPORT") { // 0x0a3	/* 2.0 */
					ofs(1, "true")
					ofd(0xfe)
				}
				
				obf16(DP_DFP_CAPABILITY_EXTENSION_SUPPORT + (int)offsetof(union dp_dfp_cap_ext, fields.max_pixel_rate_in_mps), "DFP_CAPABILITY_EXTENSION_MAX_PIXEL_RATE") { // 0x0a4, 0x0a5	/* 2.0 */
					cp("%d Mp/s", val);
				}
				obf16(DP_DFP_CAPABILITY_EXTENSION_SUPPORT + (int)offsetof(union dp_dfp_cap_ext, fields.max_video_h_active_width), "DFP_CAPABILITY_EXTENSION_MAX_VIDEO_H") { // 0x0a6, 0x0a7	/* 2.0 */
					cp("%d pixels", val);
				}
				obf16(DP_DFP_CAPABILITY_EXTENSION_SUPPORT + (int)offsetof(union dp_dfp_cap_ext, fields.max_video_v_active_height), "DFP_CAPABILITY_EXTENSION_MAX_VIDEO_V") { // 0x0a8, 0x0a9	/* 2.0 */
					cp("%d pixels", val);
				}

				obf(DP_DFP_CAPABILITY_EXTENSION_SUPPORT + (int)offsetof(union dp_dfp_cap_ext, fields.encoding_format_caps), "DFP_CAPABILITY_EXTENSION_ENCODING_FORMAT_CAPS") { // 0x0aa	/* 2.0 */
					if (dp_dfp->fields.encoding_format_caps.support_rgb) cp("RGB");
					if (dp_dfp->fields.encoding_format_caps.support_ycbcr444) cp("YCbCr 4:4:4");
					if (dp_dfp->fields.encoding_format_caps.support_ycbcr422) cp("YCbCr 4:2:2");
					if (dp_dfp->fields.encoding_format_caps.support_ycbcr420) cp("YCbCr 4:2:0");
					if (dp_dfp->fields.encoding_format_caps.RESERVED) cp("?%d", dp_dfp->fields.encoding_format_caps.RESERVED);
				}

				for (i = 0; i < 4; i++) {
					obf(DP_DFP_CAPABILITY_EXTENSION_SUPPORT + (int)offsetof(union dp_dfp_cap_ext, fields.rgb_color_depth_caps) + i,
						i == 0 ? "DFP_CAPABILITY_EXTENSION_RGB_COLOR_DEPTH_CAPS" : // 0x0ab	/* 2.0 */
						i == 1 ? "DFP_CAPABILITY_EXTENSION_YCBCR444_COLOR_DEPTH_CAPS" : // 0x0ac	/* 2.0 */
						i == 2 ? "DFP_CAPABILITY_EXTENSION_YCBCR422_COLOR_DEPTH_CAPS" : // 0x0ad	/* 2.0 */
						i == 3 ? "DFP_CAPABILITY_EXTENSION_YCBCR420_COLOR_DEPTH_CAPS" : // 0x0ae	/* 2.0 */
						""
					) {
						struct dp_color_depth_caps *color_depth_caps = (struct dp_color_depth_caps *)&val;
						if (color_depth_caps->support_6bpc ) cp( "6");
						if (color_depth_caps->support_8bpc ) cp( "8");
						if (color_depth_caps->support_10bpc) cp("10");
						if (color_depth_caps->support_12bpc) cp("12");
						if (color_depth_caps->support_16bpc) cp("16");
						if (color_depth_caps->RESERVED) cp("?%d", color_depth_caps->RESERVED);
						cprintf(" bpc");
					}
				}
			}

			dumpnotzero(DP_DFP_CAPABILITY_EXTENSION_SUPPORT + (uint32_t)sizeof(union dp_dfp_cap_ext), 0x100) // 0x0af

			olf; OUTDENT
		}

		if (!iszero(0x00100, 0x00200)) {
			iprintf("Link Configuration"); INDENT

			ob(DP_LINK_BW_SET) { // 0x100
				switch (val) {
					oc(DP_LINK_RATE_TABLE, "LINK_RATE_TABLE") // 0x00    /* eDP 1.4 */
					oc(DP_LINK_BW_1_62, "RBR") // 0x06
					oc(DP_LINK_BW_2_7, "HBR") // 0x0a
					oc(12, "3_24 (AppleVGA)") // 0x0c
					oc(DP_LINK_BW_5_4, "HBR2") // 0x14    /* 1.2 */
					oc(DP_LINK_BW_8_1, "HBR3") // 0x1e    /* 1.4 */
					oc(DP_LINK_BW_10, "UHBR 10") // 0x01    /* 2.0 128b/132b Link Layer */
					oc(DP_LINK_BW_13_5, "UHBR 13.5") // 0x04    /* 2.0 128b/132b Link Layer */
					oc(DP_LINK_BW_20, "UHBR 20") // 0x02    /* 2.0 128b/132b Link Layer */
					od("?%gGbps (unknown)", 0.27 * val)
				}
			}

			ob(DP_LANE_COUNT_SET) { // 0x101
				oim(DP_LANE_COUNT_MASK) // 0x0f
				ofd(0x70)
				ofs(DP_LANE_COUNT_ENHANCED_FRAME_EN, "ENHANCED_FRAME_EN") // (1 << 7)
			}

			ob(DP_TRAINING_PATTERN_SET) { // 0x102
				sw(DP_TRAINING_PATTERN_MASK_1_4) { // 0xf
					oc(DP_TRAINING_PATTERN_DISABLE, "TRAINING_PATTERN_DISABLE") // 0
					oc(DP_TRAINING_PATTERN_1      , "TRAINING_PATTERN_1"      ) // 1
					oc(DP_TRAINING_PATTERN_2      , "TRAINING_PATTERN_2"      ) // 2
//					oc(DP_TRAINING_PATTERN_2_CDS  , "TRAINING_PATTERN_2"      ) // 3	   /* 2.0 E11 */
					oc(DP_TRAINING_PATTERN_3      , "TRAINING_PATTERN_3"      ) // 3	   /* 1.2 */
					oc(DP_TRAINING_PATTERN_4      , "TRAINING_PATTERN_4"      ) // 7       /* 1.4 */
					od("?%d (unknown)", val & DP_TRAINING_PATTERN_MASK_1_4)
				}

				/* DPCD 1.1 only. For DPCD >= 1.2 see per-lane DP_LINK_QUAL_LANEn_SET */
				sw(DP_LINK_QUAL_PATTERN_11_MASK) { // (3 << 2)
					oc(DP_LINK_QUAL_PATTERN_11_DISABLE   , "LINK_QUAL_PATTERN_11_DISABLE"   ) // (0 << 2)
					oc(DP_LINK_QUAL_PATTERN_11_D10_2     , "LINK_QUAL_PATTERN_11_D10_2"     ) // (1 << 2)
					oc(DP_LINK_QUAL_PATTERN_11_ERROR_RATE, "LINK_QUAL_PATTERN_11_ERROR_RATE") // (2 << 2)
					oc(DP_LINK_QUAL_PATTERN_11_PRBS7     , "LINK_QUAL_PATTERN_11_PRBS7"     ) // (3 << 2)
				}

				of(DP_RECOVERED_CLOCK_OUT_EN) // (1 << 4)
				of(DP_LINK_SCRAMBLING_DISABLE) // (1 << 5)

				sw(DP_SYMBOL_ERROR_COUNT_MASK) { // (3 << 6)
					oc(DP_SYMBOL_ERROR_COUNT_BOTH     , "SYMBOL_ERROR_COUNT_BOTH"     ) // (0 << 6)
					oc(DP_SYMBOL_ERROR_COUNT_DISPARITY, "SYMBOL_ERROR_COUNT_DISPARITY") // (1 << 6)
					oc(DP_SYMBOL_ERROR_COUNT_SYMBOL   , "SYMBOL_ERROR_COUNT_SYMBOL"   ) // (2 << 6)
					od("?%d (unknown SYMBOL_ERROR_COUNT_SEL)", (val & DP_SYMBOL_ERROR_COUNT_MASK) >> 6)
				}
			}

			if (!iszero(DP_TRAINING_LANE0_SET, DP_TRAINING_LANE0_SET + 4)) {
				for (int lane = 0; lane < 4; lane++) {
					switch (lane) {
						case 0: obx(DP_TRAINING_LANE0_SET); break; // 0x103
						case 1: obx(DP_TRAINING_LANE1_SET); break; // 0x104
						case 2: obx(DP_TRAINING_LANE2_SET); break; // 0x105
						case 3: obx(DP_TRAINING_LANE3_SET); break; // 0x106
					}

					sw(DP_TRAIN_VOLTAGE_SWING_MASK) { // 0x3
						// DP_TRAIN_VOLTAGE_SWING_SHIFT	    0
						oc(DP_TRAIN_VOLTAGE_SWING_LEVEL_0, "TRAIN_VOLTAGE_SWING_LEVEL_0") // (0 << 0)
						oc(DP_TRAIN_VOLTAGE_SWING_LEVEL_1, "TRAIN_VOLTAGE_SWING_LEVEL_1") // (1 << 0)
						oc(DP_TRAIN_VOLTAGE_SWING_LEVEL_2, "TRAIN_VOLTAGE_SWING_LEVEL_2") // (2 << 0)
						oc(DP_TRAIN_VOLTAGE_SWING_LEVEL_3, "TRAIN_VOLTAGE_SWING_LEVEL_3") // (3 << 0)
					}
					of(DP_TRAIN_MAX_SWING_REACHED) // (1 << 2)

					sw(DP_TRAIN_PRE_EMPHASIS_MASK) { // (3 << 3)
						// DP_TRAIN_PRE_EMPHASIS_SHIFT	    3
						oc(DP_TRAIN_PRE_EMPH_LEVEL_0, "TRAIN_PRE_EMPH_LEVEL_0") // (0 << 3)
						oc(DP_TRAIN_PRE_EMPH_LEVEL_1, "TRAIN_PRE_EMPH_LEVEL_1") // (1 << 3)
						oc(DP_TRAIN_PRE_EMPH_LEVEL_2, "TRAIN_PRE_EMPH_LEVEL_2") // (2 << 3)
						oc(DP_TRAIN_PRE_EMPH_LEVEL_3, "TRAIN_PRE_EMPH_LEVEL_3") // (3 << 3)
					}
					of(DP_TRAIN_MAX_PRE_EMPHASIS_REACHED) // (1 << 5)

					ofd(0xc0)

					// if UHBR then
					//		DP_TX_FFE_PRESET_VALUE_MASK        (0xf << 0) /* 2.0 128b/132b Link Layer */

				} // for lane
			}

			ob(DP_DOWNSPREAD_CTRL) { // 0x107
				of(DP_SPREAD_AMP_0_5) // (1 << 4)
				of(DP_MSA_TIMING_PAR_IGNORE_EN) // (1 << 7) /* eDP */
				ofd(0x6f)
			}

			ob(DP_MAIN_LINK_CHANNEL_CODING_SET) { // 0x108
				of(DP_SET_ANSI_8B10B) // (1 << 0)
				of(DP_SET_ANSI_128B132B) // (1 << 1)
				ofd(0xfc)
			}

			ob(DP_I2C_SPEED_CONTROL_STATUS) { // 0x109   /* DPI */
				/* bitmask as for DP_I2C_SPEED_CAP */
				ofs(DP_I2C_SPEED_1K, "1kbps") // 0x01
				ofs(DP_I2C_SPEED_5K, "5kbps") // 0x02
				ofs(DP_I2C_SPEED_10K, "10kbps") // 0x04
				ofs(DP_I2C_SPEED_100K, "100kbps") // 0x08
				ofs(DP_I2C_SPEED_400K, "400kbps") // 0x10
				ofs(DP_I2C_SPEED_1M, "1Mbps") // 0x20
				ofd(0xc0)
			}

			ob(DP_EDP_CONFIGURATION_SET) { // 0x10a   /* XXX 1.2? */
				of(DP_ALTERNATE_SCRAMBLER_RESET_ENABLE) // (1 << 0)
				of(DP_FRAMING_CHANGE_ENABLE) // (1 << 1)
				of(DP_PANEL_SELF_TEST_ENABLE) // (1 << 7)
				ofd(0x7c)
			}

			if (!iszero(DP_LINK_QUAL_LANE0_SET, DP_LINK_QUAL_LANE0_SET + 4)) {
				for (int lane = 0; lane < 4; lane++) {
					switch (lane) {
						case 0: obx(DP_LINK_QUAL_LANE0_SET); break; // 0x10b /* DPCD >= 1.2 */
						case 1: obx(DP_LINK_QUAL_LANE1_SET); break; // 0x10c
						case 2: obx(DP_LINK_QUAL_LANE2_SET); break; // 0x10d
						case 3: obx(DP_LINK_QUAL_LANE3_SET); break; // 0x10e
					}
					switch (val) {
						oc(DP_LINK_QUAL_PATTERN_DISABLE      , "LINK_QUAL_PATTERN_DISABLE"     ) // 0
						oc(DP_LINK_QUAL_PATTERN_D10_2        , "LINK_QUAL_PATTERN_D10_2"       ) // 1
						oc(DP_LINK_QUAL_PATTERN_ERROR_RATE   , "LINK_QUAL_PATTERN_ERROR_RATE"  ) // 2
						oc(DP_LINK_QUAL_PATTERN_PRBS7        , "LINK_QUAL_PATTERN_PRBS7"       ) // 3
						oc(DP_LINK_QUAL_PATTERN_80BIT_CUSTOM , "LINK_QUAL_PATTERN_80BIT_CUSTOM") // 4
						oc(DP_LINK_QUAL_PATTERN_CP2520_PAT_1 , "LINK_QUAL_PATTERN_CP2520_PAT_1") // 5
						oc(DP_LINK_QUAL_PATTERN_CP2520_PAT_2 , "LINK_QUAL_PATTERN_CP2520_PAT_2") // 6
						oc(DP_LINK_QUAL_PATTERN_CP2520_PAT_3 , "LINK_QUAL_PATTERN_CP2520_PAT_3") // 7
						/* DP 2.0 UHBR10, UHBR13.5, UHBR20 */
						oc(DP_LINK_QUAL_PATTERN_128B132B_TPS1, "LINK_QUAL_PATTERN_128B132B_TPS1") // 0x08
						oc(DP_LINK_QUAL_PATTERN_128B132B_TPS2, "LINK_QUAL_PATTERN_128B132B_TPS2") // 0x10
						oc(DP_LINK_QUAL_PATTERN_PRSBS9       , "LINK_QUAL_PATTERN_PRSBS9"       ) // 0x18
						oc(DP_LINK_QUAL_PATTERN_PRSBS11      , "LINK_QUAL_PATTERN_PRSBS11"      ) // 0x20
						oc(DP_LINK_QUAL_PATTERN_PRSBS15      , "LINK_QUAL_PATTERN_PRSBS15"      ) // 0x28
						oc(DP_LINK_QUAL_PATTERN_PRSBS23      , "LINK_QUAL_PATTERN_PRSBS23"      ) // 0x30
						oc(DP_LINK_QUAL_PATTERN_PRSBS31      , "LINK_QUAL_PATTERN_PRSBS31"      ) // 0x38
						oc(DP_LINK_QUAL_PATTERN_CUSTOM       , "LINK_QUAL_PATTERN_CUSTOM"       ) // 0x40
						oc(DP_LINK_QUAL_PATTERN_SQUARE       , "LINK_QUAL_PATTERN_SQUARE"       ) // 0x48
						od("?%d (unknown)", val)
					}
				}
			}

			if (d8(DP_DPCD_REV) >= 0x20) {
				ob(DP_LINK_SQUARE_PATTERN) { // 0x10F
					ofd(0xff)
				}
				ob(DP_CABLE_ATTRIBUTES_UPDATED_BY_DPTX) { // 0x110
					ofd(0xff)
				}
			}
			else {
				if (!iszero(DP_TRAINING_LANE0_1_SET2, DP_TRAINING_LANE0_1_SET2 + 2)) {
					for (int lane = 0; lane < 4; lane++) {
						switch (lane & 2) {
							case 0: obx(DP_TRAINING_LANE0_1_SET2); break; // 0x10f
							case 2: obx(DP_TRAINING_LANE2_3_SET2); break; // 0x110
						}
						if (lane & 1) val >>= 4;
						lb("LANE%d", lane)
						cp("POST_CURSOR2_SET Training Pattern 2 or 3 with post cursor2 level %d", val & DP_LANE02_POST_CURSOR2_SET_MASK); // (3 << 0) or  (3 << 4)
						ofs(DP_LANE02_MAX_POST_CURSOR2_REACHED, "MAX_POST_CURSOR2_REACHED") // (1 << 2)
						ofd(0x08)
					}
				}
			}

			ob(DP_MSTM_CTRL) { // 0x111   /* 1.2 */
				of(DP_MST_EN) // (1 << 0)
				of(DP_UP_REQ_EN) // (1 << 1)
				of(DP_UPSTREAM_IS_SRC) // (1 << 2)
				ofd(0xf8)
			}

			#define DP_AUDIO_DELAY DP_AUDIO_DELAY0
			ob24(DP_AUDIO_DELAY) { // 0x112, 0x113, 0x114   /* 1.2 */
				cp("%dµs", val);
			}

			ob(DP_LINK_RATE_SET) { // 0x115   /* eDP 1.4 */
				cp("#%d = %dkHz", val & DP_LINK_RATE_SET_MASK, d16(DP_SUPPORTED_LINK_RATES + (val & DP_LINK_RATE_SET_MASK) * 2)); // (7 << 0)
				ofd(0xf8)
			}

			ob(DP_RECEIVER_ALPM_CONFIG) { // 0x116   /* eDP 1.4 */
				of(DP_ALPM_ENABLE) // (1 << 0)
				of(DP_ALPM_LOCK_ERROR_IRQ_HPD_ENABLE) // (1 << 1)
				ofd(0xfc)
			}

			ob(DP_SINK_DEVICE_AUX_FRAME_SYNC_CONF) { // 0x117   /* eDP 1.4 */
				of(DP_AUX_FRAME_SYNC_ENABLE) // (1 << 0)
				of(DP_IRQ_HPD_ENABLE) // (1 << 1)
				ofd(0xfc)
			}

			ob(DP_UPSTREAM_DEVICE_DP_PWR_NEED) { // 0x118   /* 1.2 */
				of(DP_PWR_NOT_NEEDED) // (1 << 0)
				ofd(0xfe)
			}

			dumpnotzero(0x119, 0x120)
			
			if (d8(DP_DPCD_REV) < 0x14) {
				#define DP_FAUX_MODE_CTRL 0x120 /* 1.2 */
				# define DP_FAUX_EN (1 << 0)
				# define DP_FAUX_FORWARD_CHANNEL_TRAINING_PATTERN_EN (1 << 1)
				# define DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_EN (1 << 2)
				# define DP_FAUX_SCRAMBLER_DIS (1 << 3)
				# define DP_FAUX_FORWARD_CHANNEL_SQUELCH_TRAINING_EN (1 << 4)
				# define DP_FAUX_FORWARD_CHANNEL_SYMBOL_ERROR_COUNT_BOTH (0 << 6)
				# define DP_FAUX_FORWARD_CHANNEL_SYMBOL_ERROR_COUNT_DISPARITY (1 << 6)
				# define DP_FAUX_FORWARD_CHANNEL_SYMBOL_ERROR_COUNT_SYMBOL (2 << 6)
				# define DP_FAUX_FORWARD_CHANNEL_SYMBOL_ERROR_COUNT_SEL_MASK (3 << 6)

				#define DP_FAUX_FORWARD_CHANNEL_DRIVE_SET 0x121 /* 1.2 */
				# define DP_FAUX_FORWARD_CHANNEL_VOLTAGE_SWING_SET_MASK (3 << 0)
				# define DP_FAUX_FORWARD_CHANNEL_VOLTAGE_SWING_LEVEL_0 (0 << 0)
				# define DP_FAUX_FORWARD_CHANNEL_VOLTAGE_SWING_LEVEL_1 (1 << 0)
				# define DP_FAUX_FORWARD_CHANNEL_VOLTAGE_SWING_LEVEL_2 (2 << 0)
				# define DP_FAUX_FORWARD_CHANNEL_VOLTAGE_SWING_LEVEL_3 (3 << 0)
				# define DP_FAUX_FORWARD_CHANNEL_MAX_SWING_REACHED (1 << 2)
				# define DP_FAUX_FORWARD_CHANNEL_PRE_EMPHASIS_SET_MASK (3 << 3)
				# define DP_FAUX_FORWARD_CHANNEL_PRE_EMPHASIS_LEVEL_0 (0 << 3)
				# define DP_FAUX_FORWARD_CHANNEL_PRE_EMPHASIS_LEVEL_1 (1 << 3)
				# define DP_FAUX_FORWARD_CHANNEL_PRE_EMPHASIS_LEVEL_2 (2 << 3)
				# define DP_FAUX_FORWARD_CHANNEL_PRE_EMPHASIS_LEVEL_3 (3 << 3)
				# define DP_FAUX_FORWARD_CHANNEL_MAX_PRE_EMPHASIS_REACHED (1 << 5)

				#define DP_FAUX_BACK_CHANNEL_STATUS 0x122
				# define DP_FAUX_BACK_CHANNEL_SYMBOL_LOCK_DONE (1 << 0)
				# define DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_ADJ_REQ_MASK (3 << 1)
				# define DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_ADJ_REQ_LEVEL_0 (0 << 1)
				# define DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_ADJ_REQ_LEVEL_1 (1 << 1)
				# define DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_ADJ_REQ_LEVEL_2 (2 << 1)
				# define DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_ADJ_REQ_LEVEL_3 (3 << 1)
				# define DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_ADJ_REQ_MASK (3 << 3)
				# define DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_ADJ_REQ_LEVEL_0 (0 << 3)
				# define DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_ADJ_REQ_LEVEL_1 (1 << 3)
				# define DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_ADJ_REQ_LEVEL_2 (2 << 3)
				# define DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_ADJ_REQ_LEVEL_3 (3 << 3)

				#define DP_FAUX_BACK_CHANNEL_SYMBOL_ERROR_COUNT 0x123 // & 0x124

				#define DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_TIME 0x125
				# define DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_TIME_MASK (15 << 0)
				# define DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_TIME_400US 0
				# define DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_TIME_4MS 1
				# define DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_TIME_8MS 2
				# define DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_TIME_12MS 3
				# define DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_TIME_16MS 4
				
				ob(DP_FAUX_MODE_CTRL) { //  0x120 /* 1.2 */
					of(DP_FAUX_EN) // (1 << 0)
					of(DP_FAUX_FORWARD_CHANNEL_TRAINING_PATTERN_EN) // (1 << 1)
					of(DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_EN) // (1 << 2)
					of(DP_FAUX_SCRAMBLER_DIS) // (1 << 3)
					of(DP_FAUX_FORWARD_CHANNEL_SQUELCH_TRAINING_EN) // (1 << 4)
					ofd(1 << 5)
					sw(DP_FAUX_FORWARD_CHANNEL_SYMBOL_ERROR_COUNT_SEL_MASK) { // (3 << 6)
						oc(DP_FAUX_FORWARD_CHANNEL_SYMBOL_ERROR_COUNT_BOTH     , "FAUX_FORWARD_CHANNEL_SYMBOL_ERROR_COUNT_BOTH"     ) // (0 << 6)
						oc(DP_FAUX_FORWARD_CHANNEL_SYMBOL_ERROR_COUNT_DISPARITY, "FAUX_FORWARD_CHANNEL_SYMBOL_ERROR_COUNT_DISPARITY") // (1 << 6)
						oc(DP_FAUX_FORWARD_CHANNEL_SYMBOL_ERROR_COUNT_SYMBOL   , "FAUX_FORWARD_CHANNEL_SYMBOL_ERROR_COUNT_SYMBOL"   ) // (2 << 6)
					   od("?%d (unknown SYMBOL_ERROR_COUNT_SEL)", (val & DP_FAUX_FORWARD_CHANNEL_SYMBOL_ERROR_COUNT_SEL_MASK) >> 6)
					}
				}
				
				ob(DP_FAUX_FORWARD_CHANNEL_DRIVE_SET) { // 0x121 /* 1.2 */
					sw(DP_FAUX_FORWARD_CHANNEL_VOLTAGE_SWING_SET_MASK) { // (3 << 0)
						oc(DP_FAUX_FORWARD_CHANNEL_VOLTAGE_SWING_LEVEL_0, "VOLTAGE_SWING_LEVEL_0") // (0 << 0)
						oc(DP_FAUX_FORWARD_CHANNEL_VOLTAGE_SWING_LEVEL_1, "VOLTAGE_SWING_LEVEL_1") // (1 << 0)
						oc(DP_FAUX_FORWARD_CHANNEL_VOLTAGE_SWING_LEVEL_2, "VOLTAGE_SWING_LEVEL_2") // (2 << 0)
						oc(DP_FAUX_FORWARD_CHANNEL_VOLTAGE_SWING_LEVEL_3, "VOLTAGE_SWING_LEVEL_3") // (3 << 0)
					}
					of(DP_FAUX_FORWARD_CHANNEL_MAX_SWING_REACHED) // (1 << 2)
					sw(DP_FAUX_FORWARD_CHANNEL_PRE_EMPHASIS_SET_MASK) { // (3 << 3)
						oc(DP_FAUX_FORWARD_CHANNEL_PRE_EMPHASIS_LEVEL_0, "PRE_EMPHASIS_LEVEL_0") // (0 << 3)
						oc(DP_FAUX_FORWARD_CHANNEL_PRE_EMPHASIS_LEVEL_1, "PRE_EMPHASIS_LEVEL_1") // (1 << 3)
						oc(DP_FAUX_FORWARD_CHANNEL_PRE_EMPHASIS_LEVEL_2, "PRE_EMPHASIS_LEVEL_2") // (2 << 3)
						oc(DP_FAUX_FORWARD_CHANNEL_PRE_EMPHASIS_LEVEL_3, "PRE_EMPHASIS_LEVEL_3") // (3 << 3)
					}
					of(DP_FAUX_FORWARD_CHANNEL_MAX_PRE_EMPHASIS_REACHED) // (1 << 5)
					ofd(0xc0)
				}
				
				ob(DP_FAUX_BACK_CHANNEL_STATUS) { // 0x122
					of(DP_FAUX_BACK_CHANNEL_SYMBOL_LOCK_DONE) // (1 << 0)
					sw(DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_ADJ_REQ_MASK) { // (3 << 1)
						oc(DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_ADJ_REQ_LEVEL_0, "VOLTAGE_SWING_ADJ_REQ_LEVEL_0") // (0 << 1)
						oc(DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_ADJ_REQ_LEVEL_1, "VOLTAGE_SWING_ADJ_REQ_LEVEL_1") // (1 << 1)
						oc(DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_ADJ_REQ_LEVEL_2, "VOLTAGE_SWING_ADJ_REQ_LEVEL_2") // (2 << 1)
						oc(DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_ADJ_REQ_LEVEL_3, "VOLTAGE_SWING_ADJ_REQ_LEVEL_3") // (3 << 1)
					}
					sw(DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_ADJ_REQ_MASK) { // (3 << 3)
						oc(DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_ADJ_REQ_LEVEL_0, "PRE_EMPHASIS_ADJ_REQ_LEVEL_0") // (0 << 3)
						oc(DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_ADJ_REQ_LEVEL_1, "PRE_EMPHASIS_ADJ_REQ_LEVEL_1") // (1 << 3)
						oc(DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_ADJ_REQ_LEVEL_2, "PRE_EMPHASIS_ADJ_REQ_LEVEL_2") // (2 << 3)
						oc(DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_ADJ_REQ_LEVEL_3, "PRE_EMPHASIS_ADJ_REQ_LEVEL_3") // (3 << 3)
					}
					ofd(0xe0)
				}
				
				ob16(DP_FAUX_BACK_CHANNEL_SYMBOL_ERROR_COUNT) { // 0x123 & 0x124
					oi()
				}

				ob(DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_TIME) { // 0x125
					sw(DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_TIME_MASK) { // (15 << 0)
						oc(DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_TIME_400US, "400µs") // 0
						oc(DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_TIME_4MS  ,   "4ms") // 1
						oc(DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_TIME_8MS  ,   "8ms") // 2
						oc(DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_TIME_12MS ,  "12ms") // 3
						oc(DP_FAUX_BACK_CHANNEL_TRAINING_PATTERN_TIME_16MS ,  "16ms") // 4
					}
					ofd(0xf0)
				}
				
				dumpnotzero(0x126, 0x130)
			}
			else {
				ob(DP_FEC_CONFIGURATION) { // 0x120    /* 1.4 */
					of(DP_FEC_READY) // (1 << 0)
					sw(DP_FEC_ERR_COUNT_SEL_MASK) { // (7 << 1)
						oc(DP_FEC_ERR_COUNT_DIS         , "FEC_ERR_COUNT_DIS"         ) // (0 << 1)
						oc(DP_FEC_UNCORR_BLK_ERROR_COUNT, "FEC_UNCORR_BLK_ERROR_COUNT") // (1 << 1)
						oc(DP_FEC_CORR_BLK_ERROR_COUNT  , "FEC_CORR_BLK_ERROR_COUNT"  ) // (2 << 1)
						oc(DP_FEC_BIT_ERROR_COUNT       , "FEC_BIT_ERROR_COUNT"       ) // (3 << 1)
					}
					sw(DP_FEC_LANE_SELECT_MASK) { // (3 << 4)
						oc(DP_FEC_LANE_0_SELECT, "FEC_LANE_0_SELECT") // (0 << 4)
						oc(DP_FEC_LANE_1_SELECT, "FEC_LANE_1_SELECT") // (1 << 4)
						oc(DP_FEC_LANE_2_SELECT, "FEC_LANE_2_SELECT") // (2 << 4)
						oc(DP_FEC_LANE_3_SELECT, "FEC_LANE_3_SELECT") // (3 << 4)
					}
					ofd(0xc0)
				}
				dumpnotzero(0x121, 0x130)
			}

			dumpnotzero(0x130, 0x154)

			#define DP_TX_GTC_VALUE 0x154
			ob32(DP_TX_GTC_VALUE) {
				cp("0x%08x", val);
			}
			
			#define DP_RX_GTC_VALUE_PHASE_SKEW_EN 0x158
			ob(DP_RX_GTC_VALUE_PHASE_SKEW_EN) { // 0x158
				ofs(1, "enabled")
				ofd(0xfe)
			}
			
			#define DP_TX_GTC_FREQ_LOCK_DONE 0x159
			ob(DP_TX_GTC_FREQ_LOCK_DONE) { // 0x159
				ofs(1, "enabled")
				ofd(0xfe)
			}
			
			dumpnotzero(0x15a, 0x15c)
			
			ob(DP_AUX_FRAME_SYNC_VALUE) { // 0x15c   /* eDP 1.4 */
				of(DP_AUX_FRAME_SYNC_VALID) // (1 << 0)
				ofd(0xfe)
			}

			dumpnotzero(0x15d, 0x160)
			
			if (!iszero(DP_DSC_SUPPORT, DP_DSC_BITS_PER_PIXEL_INC + 1) || !iszero(DP_DSC_ENABLE, DP_DSC_CONFIGURATION + 1)) {
				obx(DP_DSC_ENABLE) { // 0x160   /* DP 1.4 */
					of2(DP_DECOMPRESSION_EN, DP_disabled) // (1 << 0)
					ofd(0xfe)
				}

				ob(DP_DSC_CONFIGURATION) { // 0x161	/* DP 2.0 */
					oi()
				}
			}

			dumpnotzero(0x162, 0x170)

			ob(DP_PSR_EN_CFG) { // 0x170   /* XXX 1.2? */
				of(DP_PSR_ENABLE) // BIT(0)
				of(DP_PSR_MAIN_LINK_ACTIVE) // BIT(1)
				of(DP_PSR_CRC_VERIFICATION) // BIT(2)
				of(DP_PSR_FRAME_CAPTURE) // BIT(3)
				of(DP_PSR_SU_REGION_SCANLINE_CAPTURE) // BIT(4) /* eDP 1.4a */
				of(DP_PSR_IRQ_HPD_WITH_CRC_ERRORS) // BIT(5) /* eDP 1.4a */
				of(DP_PSR_ENABLE_PSR2) // BIT(6) /* eDP 1.4a */
				ofd(0x80)
			}

			dumpnotzero(0x171, 0x1a0)

			ob(DP_ADAPTER_CTRL) { // 0x1a0
				of(DP_ADAPTER_CTRL_FORCE_LOAD_SENSE) // (1 << 0)
				ofd(0xfe)
			}

			ob(DP_BRANCH_DEVICE_CTRL) { // 0x1a1
				of(DP_BRANCH_DEVICE_IRQ_HPD) // (1 << 0)
				ofd(0xfe)
			}

			dumpnotzero(0x1a2, 0x1c0)

			ob(DP_PAYLOAD_ALLOCATE_SET) { // 0x1c0
				cp("VC Payload Id to be allocated = %d", val & 0x7f);
				ofd(0x80)
			}
			ob(DP_PAYLOAD_ALLOCATE_START_TIME_SLOT) { // 0x1c1
				cp("Starting Time Slot of VC Payload Id in DPCD address 2C0h = %d", val & 0x3f);
				ofd(0xc0)
			}
			ob(DP_PAYLOAD_ALLOCATE_TIME_SLOT_COUNT) { // 0x1c2
				cp("Time Slot Count of VC Payload Id in DPCD address 2C0h = %d", val & 0x3f);
				ofd(0xc0)
			}

			dumpnotzero(0x1c3, 0x200)

			olf; OUTDENT
		}

		if (!iszero(0x00200, 0x00300)) {
			iprintf("Link/Sink Device Status"); INDENT

			ob(DP_SINK_COUNT) { // 0x200
				/* prior to 1.2 bit 7 was reserved mbz */
				cp("%d", DP_GET_SINK_COUNT(val));
				of(DP_SINK_CP_READY) // (1 << 6)
			}

			ob(DP_DEVICE_SERVICE_IRQ_VECTOR) { // 0x201
				of(DP_REMOTE_CONTROL_COMMAND_PENDING) // (1 << 0)
				of(DP_AUTOMATED_TEST_REQUEST) // (1 << 1)
				of(DP_CP_IRQ) // (1 << 2)
				of(DP_MCCS_IRQ) // (1 << 3)
				of(DP_DOWN_REP_MSG_RDY) // (1 << 4) /* 1.2 MST */
				of(DP_UP_REQ_MSG_RDY) // (1 << 5) /* 1.2 MST */
				of(DP_SINK_SPECIFIC_IRQ) // (1 << 6)
				ofd(0x80)
			}

			if (!iszero(DP_LANE0_1_STATUS, DP_LANE0_1_STATUS + 2)) {
				for (int lane = 0; lane < 4; lane++) {
					switch (lane & 2) {
						case 0: obx(DP_LANE0_1_STATUS); break; // 0x202
						case 2: obx(DP_LANE2_3_STATUS); break; // 0x203
					}
					if (lane & 1) val >>= 4;
					lb("LANE%d", lane)
					ofs(DP_LANE_CR_DONE         , "CR_DONE"        ) // (1 << 0)
					ofs(DP_LANE_CHANNEL_EQ_DONE , "CHANNEL_EQ_DONE") // (1 << 1)
					ofs(DP_LANE_SYMBOL_LOCKED   , "SYMBOL_LOCKED"  ) // (1 << 2)
					ofd(8                                          ) // (1 << 3)
				} // for lane
			}

			ob(DP_LANE_ALIGN_STATUS_UPDATED) { // 0x204
				of(DP_INTERLANE_ALIGN_DONE) // (1 << 0)
				ofd(0x02)
				of(DP_128B132B_DPRX_EQ_INTERLANE_ALIGN_DONE) // (1 << 2) /* 2.0 E11 */
				of(DP_128B132B_DPRX_CDS_INTERLANE_ALIGN_DONE) // (1 << 3) /* 2.0 E11 */
				of(DP_128B132B_LT_FAILED) // (1 << 4) /* 2.0 E11 */
				ofd(0x20)
				of(DP_DOWNSTREAM_PORT_STATUS_CHANGED) // (1 << 6)
				of(DP_LINK_STATUS_UPDATED) // (1 << 7)
			}

			ob(DP_SINK_STATUS) { // 0x205
				of(DP_RECEIVE_PORT_0_STATUS) // (1 << 0)
				of(DP_RECEIVE_PORT_1_STATUS) // (1 << 1)
				of(DP_STREAM_REGENERATION_STATUS) // (1 << 2) /* 2.0 */
				of(DP_INTRA_HOP_AUX_REPLY_INDICATION) // (1 << 3) /* 2.0 */
				ofd(0xf0)
			}

			if (!iszero(DP_ADJUST_REQUEST_LANE0_1, DP_ADJUST_REQUEST_LANE0_1 + 2)) {
				for (int lane = 0; lane < 4; lane++) {
					switch (lane & 2) {
						case 0: obx(DP_ADJUST_REQUEST_LANE0_1); break; // 0x206
						case 2: obx(DP_ADJUST_REQUEST_LANE2_3); break; // 0x207
					}
					if (lane & 1) val >>= 4;
					lb("LANE%d", lane)

					if (d8(DP_DPCD_REV) < 0x20) {
						cp("ADJUST_VOLTAGE_SWING_LEVEL_%d", (val & DP_ADJUST_VOLTAGE_SWING_LANE0_MASK) >> DP_ADJUST_VOLTAGE_SWING_LANE0_SHIFT);
						cp("ADJUST_PRE_EMPHASIS_LEVEL_%d" , (val & DP_ADJUST_PRE_EMPHASIS_LANE0_MASK ) >> DP_ADJUST_PRE_EMPHASIS_LANE0_SHIFT );
					}
					else {
						/* DP 2.0 128b/132b Link Layer */
						cp("ADJUST_TX_FFE_PRESET_%d", (val & DP_ADJUST_TX_FFE_PRESET_LANE0_MASK) >> DP_ADJUST_TX_FFE_PRESET_LANE0_SHIFT);
					}
				} // for lane
			}
			
			#define DP_TRAINING_SCORE_LANE0 0x208
			#define DP_TRAINING_SCORE_LANE1 0x209
			#define DP_TRAINING_SCORE_LANE2 0x20a
			#define DP_TRAINING_SCORE_LANE3 0x20b
			if (!iszero(DP_TRAINING_SCORE_LANE0, DP_TRAINING_SCORE_LANE0 + 4)) {
				for (int lane = 0; lane < 4; lane++) {
					switch (lane) {
						case 0: obx(DP_TRAINING_SCORE_LANE0); break; // 0x208
						case 1: obx(DP_TRAINING_SCORE_LANE1); break; // 0x209
						case 2: obx(DP_TRAINING_SCORE_LANE2); break; // 0x20a
						case 3: obx(DP_TRAINING_SCORE_LANE3); break; // 0x20b
					}
					cp("0x%02x", val);
				}
			}

			if (!iszero(DP_TRAINING_LANE0_1_SET2, DP_TRAINING_LANE0_1_SET2 + 2)) {
				for (int lane = 0; lane < 4; lane++) {
					switch (lane & 2) {
						case 0: obx(DP_TRAINING_LANE0_1_SET2); break; // 0x10f
						case 2: obx(DP_TRAINING_LANE2_3_SET2); break; // 0x110
					}
					if (lane & 1) val >>= 4;
					lb("LANE%d", lane)
					cp("POST_CURSOR2_SET Training Pattern 2 or 3 with post cursor2 level %d", val & DP_LANE02_POST_CURSOR2_SET_MASK); // (3 << 0)
					ofs(DP_LANE02_MAX_POST_CURSOR2_REACHED, "MAX_POST_CURSOR2_REACHED") // (1 << 2)
					ofd(0x08)
				}
			}

			if (d8(DP_ADJUST_REQUEST_POST_CURSOR2)) { // 0x20c
				for (int lane = 0; lane < 4; lane++) {
					obx(DP_ADJUST_REQUEST_POST_CURSOR2)
					val >>= (lane * 2);
					lb("LANE%d", lane)
					cp("ADJUST_POST_CURSOR2 level %d", val & DP_ADJUST_POST_CURSOR2_LANE0_MASK);
				}
			}
			
			#define DP_FAUX_FORWARD_CHANNEL_SYMBOL_ERROR_COUNT 0x20d
			ob16(DP_FAUX_FORWARD_CHANNEL_SYMBOL_ERROR_COUNT) { // 0x20d
				oim(0x7fff)
				ofs(0x8000, "valid")
			}
			
			dumpnotzero(0x20f, 0x210)

			#define DP_SYMBOL_ERROR_COUNT_LANE0 0x210
			#define DP_SYMBOL_ERROR_COUNT_LANE1 0x212
			#define DP_SYMBOL_ERROR_COUNT_LANE2 0x214
			#define DP_SYMBOL_ERROR_COUNT_LANE3 0x216
			if (!iszero(DP_SYMBOL_ERROR_COUNT_LANE0, DP_SYMBOL_ERROR_COUNT_LANE0 + 4 * 2)) {
				for (int lane = 0; lane < 4; lane++) {
					switch (lane) {
						case 0: obx(DP_SYMBOL_ERROR_COUNT_LANE0); break; // 0x210
						case 1: obx(DP_SYMBOL_ERROR_COUNT_LANE1); break; // 0x212
						case 2: obx(DP_SYMBOL_ERROR_COUNT_LANE2); break; // 0x214
						case 3: obx(DP_SYMBOL_ERROR_COUNT_LANE3); break; // 0x216
					}
					val = d16(DP_SYMBOL_ERROR_COUNT_LANE0 + 2 * lane);
					oim(0x7fff)
					ofs(0x8000, "valid")
				}
			}
			
			ob(DP_TEST_REQUEST) { // 0x218
				of(DP_TEST_LINK_TRAINING) // (1 << 0)
				of(DP_TEST_LINK_VIDEO_PATTERN) // (1 << 1)
				of(DP_TEST_LINK_EDID_READ) // (1 << 2)
				of(DP_TEST_LINK_PHY_TEST_PATTERN) // (1 << 3) /* DPCD >= 1.1 */
				of(DP_TEST_LINK_FAUX_PATTERN) // (1 << 4) /* DPCD >= 1.2 */
				of(DP_TEST_LINK_AUDIO_PATTERN) // (1 << 5) /* DPCD >= 1.2 */
				of(DP_TEST_LINK_AUDIO_DISABLED_VIDEO) // (1 << 6) /* DPCD >= 1.2 */
				ofd(0x80)
			}

			ob(DP_TEST_LINK_RATE) { // 0x219
				switch (val) {
					oc(DP_LINK_RATE_162, "RBR") // (0x6)
					oc(DP_LINK_RATE_27 , "HBR") // (0xa)
					oc(DP_LINK_BW_5_4  , "HBR2") // (0x14)
					oc(DP_LINK_BW_8_1  , "HBR3") // 0x1e    /* 1.4 */
					od("?%d (unknown)", val)
				}
			}
			
			dumpnotzero(0x21a, 0x220)

			ob(DP_TEST_LANE_COUNT) { // 0x220
				oim(0x1f)
				ofd(0xe0)
			}

			ob(DP_TEST_PATTERN) { // 0x221
				switch (val) {
					oc(DP_NO_TEST_PATTERN               , "NONE"                          ) // 0x0
					oc(DP_COLOR_RAMP                    , "COLOR_RAMP"                    ) // 0x1
					oc(DP_BLACK_AND_WHITE_VERTICAL_LINES, "BLACK_AND_WHITE_VERTICAL_LINES") // 0x2
					oc(DP_COLOR_SQUARE                  , "COLOR_SQUARES"                 ) // 0x3
					od("?%d (unknown TEST_PATTERN)", val)
				}
			}

	#define DP_TEST_H_TOTAL DP_TEST_H_TOTAL_HI
	#define DP_TEST_V_TOTAL DP_TEST_V_TOTAL_HI
	#define DP_TEST_H_START DP_TEST_H_START_HI
	#define DP_TEST_V_START DP_TEST_V_START_HI
			ob16be(DP_TEST_H_TOTAL) { // 0x222, 0x223
				oi()
			}
			ob16be(DP_TEST_V_TOTAL) { // 0x224, 0x225
				oi()
			}
			ob16be(DP_TEST_H_START) { // 0x226, 0x227
				oi()
			}
			ob16be(DP_TEST_V_START) { // 0x228, 0x229
				oi()
			}

	#define DP_TEST_HSYNC DP_TEST_HSYNC_HI
	#define DP_TEST_VSYNC DP_TEST_VSYNC_HI
	#define DP_TEST_H_WIDTH DP_TEST_H_WIDTH_HI
	#define DP_TEST_V_HEIGHT DP_TEST_V_HEIGHT_HI

			ob16be(DP_TEST_HSYNC) { // 0x22A, 0x22B
				lb("WIDTH")
				oim(0x7fff)
				lb("POLARITY")
				cp("%d", val >> 15);
			}

			ob16be(DP_TEST_VSYNC) { // 0x22C, 0x22D
				lb("WIDTH")
				oim(0x7fff)
				lb("POLARITY")
				cp("%d", val >> 15);
			}

			ob16be(DP_TEST_H_WIDTH) { // 0x22E, 0x22F
				oi()
			}

			ob16be(DP_TEST_V_HEIGHT) { // 0x230, 0x231
				oi()
			}

			ob(DP_TEST_MISC0) { // 0x232
				cp("%s", (val & DP_TEST_SYNC_CLOCK) ? "Link clock and stream clock synchronous" : "Link clock and stream clock asynchronous"); // (1 << 0) (0 << 0)
				sw(DP_TEST_COLOR_FORMAT_MASK) { // (3 << 1)
					oc(DP_COLOR_FORMAT_RGB     , "COLOR_FORMAT_RGB"     ) // (0 << 1)
					oc(DP_COLOR_FORMAT_YCbCr422, "COLOR_FORMAT_YCbCr422") // (1 << 1)
					oc(DP_COLOR_FORMAT_YCbCr444, "COLOR_FORMAT_YCbCr444") // (2 << 1)
					od("?%d (unknown COLOR_FORMAT)", (val & DP_TEST_COLOR_FORMAT_MASK) >> DP_TEST_COLOR_FORMAT_SHIFT)
				}
				of2(DP_TEST_DYNAMIC_RANGE_CEA, DP_TEST_DYNAMIC_RANGE_VESA) // (1 << 3) (0 << 3)
				of2(DP_YCBCR_COEFFICIENTS_ITU709, DP_YCBCR_COEFFICIENTS_ITU601) // (1 << 4) (0 << 4)
				sw(DP_TEST_BIT_DEPTH_MASK) { // (7 << 5)
					oc(DP_TEST_BIT_DEPTH_6 , "TEST_BIT_DEPTH_6") // (0 << 5)
					oc(DP_TEST_BIT_DEPTH_8 , "TEST_BIT_DEPTH_8") // (1 << 5)
					oc(DP_TEST_BIT_DEPTH_10, "TEST_BIT_DEPTH_10") // (2 << 5)
					oc(DP_TEST_BIT_DEPTH_12, "TEST_BIT_DEPTH_12") // (3 << 5)
					oc(DP_TEST_BIT_DEPTH_16, "TEST_BIT_DEPTH_16") // (4 << 5)
					od("?%d (unknown BIT_DEPTH)", (val & DP_TEST_BIT_DEPTH_MASK) >> DP_TEST_BIT_DEPTH_SHIFT)
				}
			}

			ob(DP_TEST_MISC1) { // 0x233
				lb("REFRESH_RATE_DENOMINATOR")
				cp("%s", (val & DP_TEST_REFRESH_DENOMINATOR) ? "1.001" : "1"); // (1 << 0) (0 << 0)
				cp("%s", (val & DP_TEST_INTERLACED) ? "interlaced" : "non-interlaced"); // (1 << 0)  (1 << 1) (0 << 1)
				ofd(0xfc)
			}

			ob(DP_TEST_REFRESH_RATE_NUMERATOR) { // 0x234
				cp("%d", val);
				lb("TEST_REFRESH_RATE")
				cp("%gHz", val / (((d8(DP_TEST_MISC1) & DP_TEST_REFRESH_DENOMINATOR)) ? 1.001 : 1));
			}

			dumpnotzero(0x235, 0x240);
			
			ob16(DP_TEST_CRC_R_CR) { // 0x240
				cp("0x%04x", val);
			}
			ob16(DP_TEST_CRC_G_Y) { // 0x242
				cp("0x%04x", val);
			}
			ob16(DP_TEST_CRC_B_CB) { // 0x244
				cp("0x%04x", val);
			}

			ob(DP_TEST_SINK_MISC) { // 0x246
				lb("TST_CRC_COUNT")
				oim(DP_TEST_COUNT_MASK)
				ofd(0x10)
				of(DP_TEST_CRC_SUPPORTED) // (1 << 5)
				ofd(0xc0)
			}

			dumpnotzero(0x247, 0x248)

			ob(DP_PHY_TEST_PATTERN) { // 0x248
				sw(DP_PHY_TEST_PATTERN_SEL_MASK) { // 0x7
					oc(DP_PHY_TEST_PATTERN_NONE        , "PHY_TEST_PATTERN_NONE"        ) // 0x0
					oc(DP_PHY_TEST_PATTERN_D10_2       , "PHY_TEST_PATTERN_D10_2"       ) // 0x1
					oc(DP_PHY_TEST_PATTERN_ERROR_COUNT , "PHY_TEST_PATTERN_ERROR_COUNT" ) // 0x2
					oc(DP_PHY_TEST_PATTERN_PRBS7       , "PHY_TEST_PATTERN_PRBS7"       ) // 0x3
					oc(DP_PHY_TEST_PATTERN_80BIT_CUSTOM, "PHY_TEST_PATTERN_80BIT_CUSTOM") // 0x4
					oc(DP_PHY_TEST_PATTERN_CP2520      , "PHY_TEST_PATTERN_CP2520"      ) // 0x5
					od("?%d (unknown PHY_TEST_PATTERN)", val & DP_PHY_TEST_PATTERN_SEL_MASK)
				}
				ofd(0xf8)
			}

			if (d8(DP_DPCD_REV) < 0x14) {
				#define DP_TEST_FAUX 0x249 /* 1.2 */
				# define DP_FAUX_FORWARD_CHANNEL_TEST_PATTERN_SEL_MASK (7 << 0)
				# define DP_FAUX_FORWARD_CHANNEL_TEST_PATTERN_NONE        0
				# define DP_FAUX_FORWARD_CHANNEL_TEST_PATTERN_D10_2       1
				# define DP_FAUX_FORWARD_CHANNEL_TEST_PATTERN_ERROR_COUNT 2
				# define DP_FAUX_FORWARD_CHANNEL_TEST_PATTERN_PRBS7       3
				# define DP_FAUX_BACK_CHANNEL_ERROR_COUNT_REQUEST (1 << 3)

				ob(DP_TEST_FAUX) { // 0x249 /* 1.2 */
					sw(DP_FAUX_FORWARD_CHANNEL_TEST_PATTERN_SEL_MASK) { // (7 << 0)
						oc(DP_FAUX_FORWARD_CHANNEL_TEST_PATTERN_NONE        , "FAUX_FORWARD_CHANNEL_TEST_PATTERN_NONE"        ) // 0x0
						oc(DP_FAUX_FORWARD_CHANNEL_TEST_PATTERN_D10_2       , "FAUX_FORWARD_CHANNEL_TEST_PATTERN_D10_2"       ) // 0x1
						oc(DP_FAUX_FORWARD_CHANNEL_TEST_PATTERN_ERROR_COUNT , "FAUX_FORWARD_CHANNEL_TEST_PATTERN_ERROR_COUNT" ) // 0x2
						oc(DP_FAUX_FORWARD_CHANNEL_TEST_PATTERN_PRBS7       , "FAUX_FORWARD_CHANNEL_TEST_PATTERN_PRBS7"       ) // 0x3
						od("?%d (unknown FAUX_FORWARD_CHANNEL_TEST_PATTERN)", val & DP_FAUX_FORWARD_CHANNEL_TEST_PATTERN_SEL_MASK)
					}
					of(DP_FAUX_BACK_CHANNEL_ERROR_COUNT_REQUEST)
					ofd(0xf0)
				}
			}
			else {
				ob(DP_PHY_SQUARE_PATTERN) { // 0x249
					cp("0x%02x", val);
				}
			}

			ob16(DP_TEST_HBR2_SCRAMBLER_RESET) { // 0x24A, 0x24B
				oi();
			}
			
			dumpnotzero(0x24c, 0x250)

			if (!iszero(DP_TEST_80BIT_CUSTOM_PATTERN_7_0, DP_TEST_80BIT_CUSTOM_PATTERN_7_0 + 10)) { // 0x250 - 0x259
				#define DP_TEST_80BIT_CUSTOM_PATTERN DP_TEST_80BIT_CUSTOM_PATTERN_7_0
				obx(DP_TEST_80BIT_CUSTOM_PATTERN)
				cp("0x");
				for (int i = 0; i < 10; i++) {
					cprintf("%02x", d8(DP_TEST_80BIT_CUSTOM_PATTERN_7_0 + 9 - i));
				}
			}
			
			dumpnotzero(0x25a, 0x260)

			ob(DP_TEST_RESPONSE) { // 0x260
				of(DP_TEST_ACK) // (1 << 0)
				of(DP_TEST_NAK) // (1 << 1)
				of(DP_TEST_EDID_CHECKSUM_WRITE) // (1 << 2)
				ofd(0xf8)
			}

			ob(DP_TEST_EDID_CHECKSUM) { // 0x261
				cp("0x%02x", val);
			}

			#define DP_TEST_FAUX_BACK_CHANNEL_TEST_PATTERN 0x262 /* 1.2 */
			# define DP_FAUX_BACK_CHANNEL_TEST_PATTERN_SEL_MASK (7 << 0)
			# define DP_FAUX_BACK_CHANNEL_TEST_PATTERN_NONE        0
			# define DP_FAUX_BACK_CHANNEL_TEST_PATTERN_D10_2       1
			# define DP_FAUX_BACK_CHANNEL_TEST_PATTERN_ERROR_COUNT 2
			# define DP_FAUX_BACK_CHANNEL_TEST_PATTERN_PRBS7       3

			ob(DP_TEST_FAUX_BACK_CHANNEL_TEST_PATTERN) { // 0x262 /* 1.2 */
				sw(DP_FAUX_BACK_CHANNEL_TEST_PATTERN_SEL_MASK) { // (7 << 0)
					oc(DP_FAUX_BACK_CHANNEL_TEST_PATTERN_NONE        , "FAUX_BACK_CHANNEL_TEST_PATTERN_NONE"        ) // 0x0
					oc(DP_FAUX_BACK_CHANNEL_TEST_PATTERN_D10_2       , "FAUX_BACK_CHANNEL_TEST_PATTERN_D10_2"       ) // 0x1
					oc(DP_FAUX_BACK_CHANNEL_TEST_PATTERN_ERROR_COUNT , "FAUX_BACK_CHANNEL_TEST_PATTERN_ERROR_COUNT" ) // 0x2
					oc(DP_FAUX_BACK_CHANNEL_TEST_PATTERN_PRBS7       , "FAUX_BACK_CHANNEL_TEST_PATTERN_PRBS7"       ) // 0x3
					od("?%d (unknown FAUX_BACK_CHANNEL_TEST_PATTERN)", val & DP_FAUX_BACK_CHANNEL_TEST_PATTERN_SEL_MASK)
				}
				ofd(0xf8)
			}
			
			dumpnotzero(0x263, 0x270)

			# define DP_PHY_SINK_TEST_LANE_SEL_MASK (3 << 4)
			# define DP_PHY_SINK_TEST_LANE_0 (0 << 4)
			# define DP_PHY_SINK_TEST_LANE_1 (1 << 4)
			# define DP_PHY_SINK_TEST_LANE_2 (2 << 4)
			# define DP_PHY_SINK_TEST_LANE_3 (3 << 4)
			# define DP_PHY_SINK_TEST_LANE_EN (1 << 7)
			ob(DP_TEST_SINK) { // 0x270
				of(DP_TEST_SINK_START) // (1 << 0)
				ofd(0x0e)
				sw(DP_PHY_SINK_TEST_LANE_SEL_MASK) { // (3 << 4)
					oc(DP_PHY_SINK_TEST_LANE_0, "PHY_SINK_TEST_LANE_0")
					oc(DP_PHY_SINK_TEST_LANE_1, "PHY_SINK_TEST_LANE_1")
					oc(DP_PHY_SINK_TEST_LANE_2, "PHY_SINK_TEST_LANE_2")
					oc(DP_PHY_SINK_TEST_LANE_3, "PHY_SINK_TEST_LANE_3")
				}
				ofd(0x40)
				of(DP_PHY_SINK_TEST_LANE_EN)
			}
			
			ob(DP_TEST_AUDIO_MODE) { // 0x271
				sw(val & 0x0f) {
					oc(AUDIO_SAMPLING_RATE_32KHZ   , "32kHz")
					oc(AUDIO_SAMPLING_RATE_44_1KHZ , "44.1kHz")
					oc(AUDIO_SAMPLING_RATE_48KHZ   , "48kHz")
					oc(AUDIO_SAMPLING_RATE_88_2KHZ , "88.2kHz")
					oc(AUDIO_SAMPLING_RATE_96KHZ   , "96kHz")
					oc(AUDIO_SAMPLING_RATE_176_4KHZ, "176.4kHz")
					oc(AUDIO_SAMPLING_RATE_192KHZ  , "192kHz")
					od("?%d (unknown AUDIO_SAMPLING_RATE)", val & 0x0f)
				}
				sw ((val & 0xf0) >> 4) {
					oc(AUDIO_CHANNELS_1, "AUDIO_CHANNELS_1")
					oc(AUDIO_CHANNELS_2, "AUDIO_CHANNELS_2")
					oc(AUDIO_CHANNELS_3, "AUDIO_CHANNELS_3")
					oc(AUDIO_CHANNELS_4, "AUDIO_CHANNELS_4")
					oc(AUDIO_CHANNELS_5, "AUDIO_CHANNELS_5")
					oc(AUDIO_CHANNELS_6, "AUDIO_CHANNELS_6")
					oc(AUDIO_CHANNELS_7, "AUDIO_CHANNELS_7")
					oc(AUDIO_CHANNELS_8, "AUDIO_CHANNELS_8")
					od("?%d (unknown AUDIO_CHANNELS)", ((val & 0xf0) >> 4) + 1)
				}
			}

			ob(DP_TEST_AUDIO_PATTERN_TYPE) { // 0x272
				switch (val) {
					oc(AUDIO_TEST_PATTERN_OPERATOR_DEFINED, "OPERATOR_DEFINED")
					oc(AUDIO_TEST_PATTERN_SAWTOOTH, "SAWTOOTH")
					od("?%d (unknown AUDIO_TEST_PATTERN)", val)
				}
			}
				
			ob(DP_TEST_AUDIO_PERIOD_CH1) { // 0x273
				oi()
			}
			ob(DP_TEST_AUDIO_PERIOD_CH2) { // 0x274
				oi()
			}
			ob(DP_TEST_AUDIO_PERIOD_CH3) { // 0x275
				oi()
			}
			ob(DP_TEST_AUDIO_PERIOD_CH4) { // 0x276
				oi()
			}
			ob(DP_TEST_AUDIO_PERIOD_CH5) { // 0x277
				oi()
			}
			ob(DP_TEST_AUDIO_PERIOD_CH6) { // 0x278
				oi()
			}
			ob(DP_TEST_AUDIO_PERIOD_CH7) { // 0x279
				oi()
			}
			ob(DP_TEST_AUDIO_PERIOD_CH8) { // 0x27A
				oi()
			}
			
			dumpnotzero(0x27b, 0x280)
			
			if (d8(DP_DPCD_REV) < 0x14) {
				
				#define DP_FAUX_BACK_CHANNEL_DRIVE_SET 0x281 /* 1.2 */
				# define DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_SET_MASK (3 << 0)
				# define DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_LEVEL_0 (0 << 0)
				# define DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_LEVEL_1 (1 << 0)
				# define DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_LEVEL_2 (2 << 0)
				# define DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_LEVEL_3 (3 << 0)
				# define DP_FAUX_BACK_CHANNEL_MAX_SWING_REACHED (1 << 2)
				# define DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_SET_MASK (3 << 3)
				# define DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_LEVEL_0 (0 << 3)
				# define DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_LEVEL_1 (1 << 3)
				# define DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_LEVEL_2 (2 << 3)
				# define DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_LEVEL_3 (3 << 3)
				# define DP_FAUX_BACK_CHANNEL_MAX_PRE_EMPHASIS_REACHED (1 << 5)
							
				#define DP_FAUX_BACK_CHANNEL_SYMBOL_ERROR_COUNT_CONTROL 0x282 /* 1.2 */
				# define DP_FAUX_BACK_CHANNEL_SYMBOL_ERROR_COUNT_SEL_MASK (3 << 6)
				# define DP_FAUX_BACK_CHANNEL_SYMBOL_ERROR_COUNT_BOTH (0 << 6)
				# define DP_FAUX_BACK_CHANNEL_SYMBOL_ERROR_COUNT_DISPARITY (1 << 6)
				# define DP_FAUX_BACK_CHANNEL_SYMBOL_ERROR_COUNT_SYMBOL (2 << 6)
				# define DP_FAUX_BACK_CHANNEL_SYMBOL_ERROR_COUNT_SEL_MASK (3 << 6)

				ob(DP_FAUX_BACK_CHANNEL_DRIVE_SET) { // 0x281 /* 1.2 */
					sw(DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_SET_MASK) { // (3 << 0)
						oc(DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_LEVEL_0, "VOLTAGE_SWING_LEVEL_0") // (0 << 0)
						oc(DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_LEVEL_1, "VOLTAGE_SWING_LEVEL_1") // (1 << 0)
						oc(DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_LEVEL_2, "VOLTAGE_SWING_LEVEL_2") // (2 << 0)
						oc(DP_FAUX_BACK_CHANNEL_VOLTAGE_SWING_LEVEL_3, "VOLTAGE_SWING_LEVEL_3") // (3 << 0)
					}
					of(DP_FAUX_BACK_CHANNEL_MAX_SWING_REACHED) // (1 << 2)
					sw(DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_SET_MASK) { // (3 << 3)
						oc(DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_LEVEL_0, "PRE_EMPHASIS_LEVEL_0") // (0 << 3)
						oc(DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_LEVEL_1, "PRE_EMPHASIS_LEVEL_1") // (1 << 3)
						oc(DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_LEVEL_2, "PRE_EMPHASIS_LEVEL_2") // (2 << 3)
						oc(DP_FAUX_BACK_CHANNEL_PRE_EMPHASIS_LEVEL_3, "PRE_EMPHASIS_LEVEL_3") // (3 << 3)
					}
					of(DP_FAUX_BACK_CHANNEL_MAX_PRE_EMPHASIS_REACHED) // (1 << 5)
					ofd(0xc0)
				}

				ob(DP_FAUX_BACK_CHANNEL_SYMBOL_ERROR_COUNT_CONTROL) { //  0x282 /* 1.2 */
					ofd(0x3f)
					sw(DP_FAUX_BACK_CHANNEL_SYMBOL_ERROR_COUNT_SEL_MASK) { // (3 << 6)
						oc(DP_FAUX_BACK_CHANNEL_SYMBOL_ERROR_COUNT_BOTH     , "COUNT_BOTH"     ) // (0 << 6)
						oc(DP_FAUX_BACK_CHANNEL_SYMBOL_ERROR_COUNT_DISPARITY, "COUNT_DISPARITY") // (1 << 6)
						oc(DP_FAUX_BACK_CHANNEL_SYMBOL_ERROR_COUNT_SYMBOL   , "COUNT_SYMBOL"   ) // (2 << 6)
					   od("?%d (unknown SYMBOL_ERROR_COUNT_SEL)", (val & DP_FAUX_BACK_CHANNEL_SYMBOL_ERROR_COUNT_SEL_MASK) >> 6)
					}
				}
				
			}
			else {
				ob(DP_FEC_STATUS) { // 0x280    /* 1.4 */
					of(DP_FEC_DECODE_EN_DETECTED) // (1 << 0)
					of(DP_FEC_DECODE_DIS_DETECTED) // (1 << 1)
					ofd(0xfc)
				}

				#define DP_FEC_ERROR_COUNT DP_FEC_ERROR_COUNT_LSB // 0x0281, 0x282 /* 1.4 */
				ob16(DP_FEC_ERROR_COUNT) { // 0x0281, 0x282 /* 1.4 */
					oim(0x7fff)
					ofs(0x8000, "valid")
				}
			}
			
			dumpnotzero(0x283, 0x2c0)

			ob(DP_PAYLOAD_TABLE_UPDATE_STATUS) { // 0x2c0   /* 1.2 MST */
				of(DP_PAYLOAD_TABLE_UPDATED) // (1 << 0)
				of(DP_PAYLOAD_ACT_HANDLED) // (1 << 1)
				ofd(0xfc)
			}

			if (!iszero(DP_VC_PAYLOAD_ID_SLOT_1, DP_VC_PAYLOAD_ID_SLOT_1 + 63)) { // 0x2c1   /* 1.2 MST */
				obf(DP_VC_PAYLOAD_ID_SLOT_1, "VC_PAYLOAD_ID")
				cp("");
				for (int i = 0; i < 63; i++) { /* up to ID_SLOT_63 at 0x2ff */
					cprintf("%02x", d8(DP_VC_PAYLOAD_ID_SLOT_1 + i));
				}
			}
			olf; OUTDENT
		}
		
		if (!iszero(0x00300, 0x00400)) {
			iprintf("Source Device-Specific"); INDENT

			int oui = 0;
			ob24be(DP_SOURCE_OUI) { // 0x300
				oui = val;
				lb("%02X-%02X-%02X", (oui >> 16) & 0xff, (oui >> 8) & 0xff, (oui >> 0) & 0xff);
				cp("%s", convertoui(oui));
			}

			#define DP_SOURCE_ID 0x303
			dumpnotzero(DP_SOURCE_ID, DP_SOURCE_ID + 6, "SOURCE_ID");
			
			#define DP_SOURCE_HW_REV 0x309
			ob(DP_SOURCE_HW_REV) { // 0x309
				cp("%d.%d", (val & 0xf0) >> 4, (val & 0x0f) >> 0);
			}
			
			#define DP_SOURCE_SW_REV 0x30a
			ob16(DP_SOURCE_SW_REV) { // 0x30a, 0x30b
				cp("%d.%d", (val >> 0) & 0xff, (val >> 8) & 0xff);
			}
			
			dumpnotzero(0x30c, 0x400)

			olf; OUTDENT
		}
			
		if (!iszero(0x00400, 0x00500)) {
			iprintf("Sink Device-Specific"); INDENT

			int oui = 0;
			ob24be(DP_SINK_OUI) { // 0x400
				oui = val;
				lb("%02X-%02X-%02X", (oui >> 16) & 0xff, (oui >> 8) & 0xff, (oui >> 0) & 0xff);
				cp("%s", convertoui(oui));
			}

			#define DP_SINK_ID 0x403
			dumpnotzero(DP_SINK_ID, DP_SINK_ID + 6, "SINK_ID");
			
			#define DP_SINK_HW_REV 0x409
			ob(DP_SINK_HW_REV) { // 0x409
				cp("%d.%d", (val & 0xf0) >> 4, (val & 0x0f) >> 0);
			}
			
			#define DP_SINK_SW_REV 0x40a
			ob16(DP_SINK_SW_REV) { // 0x40a, 0x40b
				cp("%d.%d", (val >> 0) & 0xff, (val >> 8) & 0xff);
			}
			
			dumpnotzero(0x40c, 0x500)

			olf; OUTDENT
		}

		if (!iszero(0x00500, 0x00600)) {
			iprintf("Branch Device-Specific"); INDENT

			int oui = 0;
			ob24be(DP_BRANCH_OUI) { // 0x500
				oui = val;
				lb("%02X-%02X-%02X", (oui >> 16) & 0xff, (oui >> 8) & 0xff, (oui >> 0) & 0xff);
				cp("%s", convertoui(oui));
			}

			dumpnotzero(DP_BRANCH_ID, DP_BRANCH_ID + 6, "BRANCH_ID");

			ob(DP_BRANCH_HW_REV) { // 0x509
				cp("%d.%d", (val & 0xf0) >> 4, (val & 0x0f) >> 0);
			}

			ob16(DP_BRANCH_SW_REV) { // 0x50A, 0x50B
				cp("%d.%d", (val >> 0) & 0xff, (val >> 8) & 0xff);
			}
			
			dumpnotzero(0x50c, 0x600)

			olf; OUTDENT
		}

		if (!iszero(0x00600, 0x00700)) {
			iprintf("Sink Control"); INDENT

			ob(DP_SET_POWER) { // 0x600
				sw(DP_SET_POWER_MASK) { // 0x3
					oc(DP_SET_POWER_D0       , "SET_POWER_D0") // 0x1
					oc(DP_SET_POWER_D3       , "SET_POWER_D3") // 0x2
					oc(DP_SET_POWER_D3_AUX_ON, "SET_POWER_D3_AUX_ON") // 0x5
					od("?%d (unknown)", (val & DP_SET_POWER_MASK) >> 0)
				}
			}
			
			dumpnotzero(0x601, 0x700)

			olf; OUTDENT
		}

		if (!iszero(0x00700, 0x00800)) {
			iprintf("eDP-Specific"); INDENT

			ob(DP_EDP_DPCD_REV) { // 0x700    /* eDP 1.2 */
				switch (val) {
					oc(DP_EDP_11 , "eDP 1.1" ) // 0x00
					oc(DP_EDP_12 , "eDP 1.2" ) // 0x01
					oc(DP_EDP_13 , "eDP 1.3" ) // 0x02
					oc(DP_EDP_14 , "eDP 1.4" ) // 0x03
					oc(DP_EDP_14a, "eDP 1.4a") // 0x04    /* eDP 1.4a */
					oc(DP_EDP_14b, "eDP 1.4b") // 0x05    /* eDP 1.4b */
					od("?%d (unknown)", val)
				}
			}

			ob(DP_EDP_GENERAL_CAP_1) { // 0x701
				of(DP_EDP_TCON_BACKLIGHT_ADJUSTMENT_CAP) // (1 << 0)
				of(DP_EDP_BACKLIGHT_PIN_ENABLE_CAP) // (1 << 1)
				of(DP_EDP_BACKLIGHT_AUX_ENABLE_CAP) // (1 << 2)
				of(DP_EDP_PANEL_SELF_TEST_PIN_ENABLE_CAP) // (1 << 3)
				of(DP_EDP_PANEL_SELF_TEST_AUX_ENABLE_CAP) // (1 << 4)
				of(DP_EDP_FRC_ENABLE_CAP) // (1 << 5)
				of(DP_EDP_COLOR_ENGINE_CAP) // (1 << 6)
				of(DP_EDP_SET_POWER_CAP) // (1 << 7)
			}

			ob(DP_EDP_BACKLIGHT_ADJUSTMENT_CAP) { // 0x702
				of(DP_EDP_BACKLIGHT_BRIGHTNESS_PWM_PIN_CAP) // (1 << 0)
				of(DP_EDP_BACKLIGHT_BRIGHTNESS_AUX_SET_CAP) // (1 << 1)
				of(DP_EDP_BACKLIGHT_BRIGHTNESS_BYTE_COUNT) // (1 << 2)
				of(DP_EDP_BACKLIGHT_AUX_PWM_PRODUCT_CAP) // (1 << 3)
				of(DP_EDP_BACKLIGHT_FREQ_PWM_PIN_PASSTHRU_CAP) // (1 << 4)
				of(DP_EDP_BACKLIGHT_FREQ_AUX_SET_CAP) // (1 << 5)
				of(DP_EDP_DYNAMIC_BACKLIGHT_CAP) // (1 << 6)
				of(DP_EDP_VBLANK_BACKLIGHT_UPDATE_CAP) // (1 << 7)
			}

			ob(DP_EDP_GENERAL_CAP_2) { // 0x703
				of(DP_EDP_OVERDRIVE_ENGINE_ENABLED) // (1 << 0)
				ofd(0xfe)
			}

			ob(DP_EDP_GENERAL_CAP_3) { // 0x704    /* eDP 1.4 */
				lb("EDP_X_REGION_CAP")
				cp("%d", (val & DP_EDP_X_REGION_CAP_MASK) >> DP_EDP_X_REGION_CAP_SHIFT);
				lb("EDP_Y_REGION_CAP")
				cp("%d", (val & DP_EDP_Y_REGION_CAP_MASK) >> DP_EDP_Y_REGION_CAP_SHIFT);
			}
			
			dumpnotzero(0x705, 0x720)

			ob(DP_EDP_DISPLAY_CONTROL_REGISTER) { // 0x720
				of(DP_EDP_BACKLIGHT_ENABLE) // (1 << 0)
				of(DP_EDP_BLACK_VIDEO_ENABLE) // (1 << 1)
				of(DP_EDP_FRC_ENABLE) // (1 << 2)
				of(DP_EDP_COLOR_ENGINE_ENABLE) // (1 << 3)
				of(DP_EDP_VBLANK_BACKLIGHT_UPDATE_ENABLE) // (1 << 7)
				ofd(0x70)
			}

			ob(DP_EDP_BACKLIGHT_MODE_SET_REGISTER) { // 0x721
				sw(DP_EDP_BACKLIGHT_CONTROL_MODE_MASK) { // (3 << 0)
					oc(DP_EDP_BACKLIGHT_CONTROL_MODE_PWM    , "PWM"    ) // (0 << 0)
					oc(DP_EDP_BACKLIGHT_CONTROL_MODE_PRESET , "PRESET" ) // (1 << 0)
					oc(DP_EDP_BACKLIGHT_CONTROL_MODE_DPCD   , "DPCD"   ) // (2 << 0)
					oc(DP_EDP_BACKLIGHT_CONTROL_MODE_PRODUCT, "PRODUCT") // (3 << 0)
				}
				of(DP_EDP_BACKLIGHT_FREQ_PWM_PIN_PASSTHRU_ENABLE) // (1 << 2)
				of(DP_EDP_BACKLIGHT_FREQ_AUX_SET_ENABLE) // (1 << 3)
				of(DP_EDP_DYNAMIC_BACKLIGHT_ENABLE) // (1 << 4)
				of(DP_EDP_REGIONAL_BACKLIGHT_ENABLE) // (1 << 5)
				of(DP_EDP_UPDATE_REGION_BRIGHTNESS) // (1 << 6) /* eDP 1.4 */
				ofd(0x80)
			}

			#define DP_EDP_BACKLIGHT_BRIGHTNESS DP_EDP_BACKLIGHT_BRIGHTNESS_MSB
			ob16be(DP_EDP_BACKLIGHT_BRIGHTNESS_MSB) { // 0x722, 0x723
				oi()
			}

			ob(DP_EDP_PWMGEN_BIT_COUNT) { // 0x724
				oim(DP_EDP_PWMGEN_BIT_COUNT_MASK)
				ofd(0xe0)
			}
			ob(DP_EDP_PWMGEN_BIT_COUNT_CAP_MIN) { // 0x725
				oim(DP_EDP_PWMGEN_BIT_COUNT_MASK)
				ofd(0xe0)
			}
			ob(DP_EDP_PWMGEN_BIT_COUNT_CAP_MAX) { // 0x726
				oim(DP_EDP_PWMGEN_BIT_COUNT_MASK)
				ofd(0xe0)
			}

			ob(DP_EDP_BACKLIGHT_CONTROL_STATUS) { // 0x727
				oi()
			}

			ob(DP_EDP_BACKLIGHT_FREQ_SET) { // 0x728
				oi()
			}
			
			dumpnotzero(0x729, 0x72a)

			#define DP_EDP_BACKLIGHT_FREQ_CAP_MIN DP_EDP_BACKLIGHT_FREQ_CAP_MIN_MSB
			ob24be(DP_EDP_BACKLIGHT_FREQ_CAP_MIN_MSB) { // 0x72a, 0x72b, 0x72c
				oi();
			}

			#define DP_EDP_BACKLIGHT_FREQ_CAP_MAX DP_EDP_BACKLIGHT_FREQ_CAP_MAX_MSB
			ob(DP_EDP_BACKLIGHT_FREQ_CAP_MAX) { // 0x72d, 0x72e, 0x72f
				oi();
			}

			dumpnotzero(0x730, 0x732)

			ob(DP_EDP_DBC_MINIMUM_BRIGHTNESS_SET) { // 0x732
				oi();
			}
			ob(DP_EDP_DBC_MAXIMUM_BRIGHTNESS_SET) { // 0x733
				oi();
			}

			dumpnotzero(0x734, 0x740)

			ob(DP_EDP_REGIONAL_BACKLIGHT_BASE) { // 0x740    /* eDP 1.4 */
				oi();
			}
			ob(DP_EDP_REGIONAL_BACKLIGHT_0) { // 0x741    /* eDP 1.4 */
				oi();
			}

			dumpnotzero(0x742, 0x7a4)

			ob(DP_EDP_MSO_LINK_CAPABILITIES) { // 0x7a4    /* eDP 1.4 */
				lb("EDP_MSO_NUMBER_OF_LINKS")
				oim(DP_EDP_MSO_NUMBER_OF_LINKS_MASK) // (7 << 0)
				of(DP_EDP_MSO_INDEPENDENT_LINK_BIT) // (1 << 3)
				ofd(0xf0)
			}

			dumpnotzero(0x7a5, 0x800)

			olf; OUTDENT
		}

		if (!iszero(0x0800, 0x01000)) {
			iprintf("Undefined"); INDENT
			dumpnotzero(0x0800, 0x01000)
			olf; OUTDENT
		}

		if (!iszero(0x01000, 0x01800)) {
			iprintf("Sideband MSG Buffers"); INDENT
			
			#define doonesideband(x) \
				if(!iszero(DP_ ## x ## _BASE, DP_ ## x ## _BASE + 0x200)) { \
					dumpnotzero(DP_ ## x ## _BASE, DP_ ## x ## _BASE + 0x200, #x) \
					olf; INDENT \
					DumpOneDisplayPortMessage(&dpcd[DP_ ## x ## _BASE], 0x200, DP_ ## x ## _BASE); OUTDENT \
				}

			doonesideband(SIDEBAND_MSG_DOWN_REQ) // 0x1000   /* 1.2 MST */
			doonesideband(SIDEBAND_MSG_UP_REP) // 0x1200   /* 1.2 MST */
			doonesideband(SIDEBAND_MSG_DOWN_REP) // 0x1400   /* 1.2 MST */
			doonesideband(SIDEBAND_MSG_UP_REQ) // 0x1600   /* 1.2 MST */
			#undef doonesideband

			olf; OUTDENT
		}

		if (!iszero(0x1800, 0x02000)) {
			iprintf("Undefined"); INDENT
			dumpnotzero(0x1800, 0x02000)
			olf; OUTDENT
		}

		if (!iszero(0x02000, 0x02200)) {
			iprintf("DPRX ESI (Event Status Indicator)"); INDENT

			dumpnotzero(0x02000, 0x02002)
			
			ob(DP_SINK_COUNT_ESI) { // 0x2002   /* same as 0x200 */
				cp("%d", DP_GET_SINK_COUNT(val));
				of(DP_SINK_CP_READY) // (1 << 6)
			}

			ob(DP_DEVICE_SERVICE_IRQ_VECTOR_ESI0) { // 0x2003   /* same as 0x201 */
				of(DP_REMOTE_CONTROL_COMMAND_PENDING) // (1 << 0)
				of(DP_AUTOMATED_TEST_REQUEST) // (1 << 1)
				of(DP_CP_IRQ) // (1 << 2)
				of(DP_MCCS_IRQ) // (1 << 3)
				of(DP_DOWN_REP_MSG_RDY) // (1 << 4)
				of(DP_UP_REQ_MSG_RDY) // (1 << 5)
				of(DP_SINK_SPECIFIC_IRQ) // (1 << 6)
				ofd(0x80)
			}

			ob(DP_DEVICE_SERVICE_IRQ_VECTOR_ESI1) { // 0x2004   /* 1.2 */
				of(DP_RX_GTC_MSTR_REQ_STATUS_CHANGE) // (1 << 0)
				of(DP_LOCK_ACQUISITION_REQUEST) // (1 << 1)
				of(DP_CEC_IRQ) // (1 << 2)
				ofd(0xf8)
			}

			ob(DP_LINK_SERVICE_IRQ_VECTOR_ESI0) { // 0x2005   /* 1.2 */
				of(RX_CAP_CHANGED) // (1 << 0)
				of(LINK_STATUS_CHANGED) // (1 << 1)
				of(STREAM_STATUS_CHANGED) // (1 << 2)
				of(HDMI_LINK_STATUS_CHANGED) // (1 << 3)
				of(CONNECTED_OFF_ENTRY_REQUESTED) // (1 << 4)
				ofd(0xe0)
			}

			ob(DP_PSR_ERROR_STATUS) { // 0x2006  /* XXX 1.2? */
				of(DP_PSR_LINK_CRC_ERROR) // (1 << 0)
				of(DP_PSR_RFB_STORAGE_ERROR) // (1 << 1)
				of(DP_PSR_VSC_SDP_UNCORRECTABLE_ERROR) // (1 << 2) /* eDP 1.4 */
				ofd(0xf8)
			}

			ob(DP_PSR_ESI) { // 0x2007  /* XXX 1.2? */
				of(DP_PSR_CAPS_CHANGE) // (1 << 0)
				ofd(0xfe)
			}

			ob(DP_PSR_STATUS) { // 0x2008  /* XXX 1.2? */
				sw(DP_PSR_SINK_STATE_MASK) { // 0x07
					oc(DP_PSR_SINK_INACTIVE          , "PSR_SINK_INACTIVE"          ) // 0
					oc(DP_PSR_SINK_ACTIVE_SRC_SYNCED , "PSR_SINK_ACTIVE_SRC_SYNCED" ) // 1
					oc(DP_PSR_SINK_ACTIVE_RFB        , "PSR_SINK_ACTIVE_RFB"        ) // 2
					oc(DP_PSR_SINK_ACTIVE_SINK_SYNCED, "PSR_SINK_ACTIVE_SINK_SYNCED") // 3
					oc(DP_PSR_SINK_ACTIVE_RESYNC     , "PSR_SINK_ACTIVE_RESYNC"     ) // 4
					oc(DP_PSR_SINK_INTERNAL_ERROR    , "PSR_SINK_INTERNAL_ERROR"    ) // 7
					od("?%d (unknown)", val & DP_PSR_SINK_STATE_MASK)
				}
				ofd(0xf8)
			}

			ob(DP_SYNCHRONIZATION_LATENCY_IN_SINK) { // 0x2009 /* edp 1.4 */
				lb("MAX_RESYNC_FRAME_COUNT")
				cp("%d", (val & DP_MAX_RESYNC_FRAME_COUNT_MASK) >> DP_MAX_RESYNC_FRAME_COUNT_SHIFT);
				lb("LAST_ACTUAL_SYNCHRONIZATION_LATENCY")
				cp("%d", (val & DP_LAST_ACTUAL_SYNCHRONIZATION_LATENCY_MASK) >> DP_LAST_ACTUAL_SYNCHRONIZATION_LATENCY_SHIFT);
			}

			ob(DP_LAST_RECEIVED_PSR_SDP) { // 0x200a /* eDP 1.2 */
				of(DP_PSR_STATE_BIT) // (1 << 0) /* eDP 1.2 */
				of(DP_UPDATE_RFB_BIT) // (1 << 1) /* eDP 1.2 */
				of(DP_CRC_VALID_BIT) // (1 << 2) /* eDP 1.2 */
				of(DP_SU_VALID) // (1 << 3) /* eDP 1.4 */
				of(DP_FIRST_SCAN_LINE_SU_REGION) // (1 << 4) /* eDP 1.4 */
				of(DP_LAST_SCAN_LINE_SU_REGION) // (1 << 5) /* eDP 1.4 */
				of(DP_Y_COORDINATE_VALID) // (1 << 6) /* eDP 1.4a */
				ofd(0x80)
			}

			ob(DP_RECEIVER_ALPM_STATUS) { // 0x200b  /* eDP 1.4 */
				of(DP_ALPM_LOCK_TIMEOUT_ERROR) // (1 << 0)
				ofd(0xfe)
			}

			if (!iszero(DP_LANE0_1_STATUS, DP_LANE0_1_STATUS + 2)) {
				for (int lane = 0; lane < 4; lane++) {
					switch (lane & 2) {
						case 0: obx(DP_LANE0_1_STATUS_ESI); break; // 0x200c
						case 2: obx(DP_LANE2_3_STATUS_ESI); break; // 0x200d
					}
					if (lane & 1) val >>= 4;
					lb("LANE%d", lane)
					ofs(DP_LANE_CR_DONE         , "CR_DONE"        ) // (1 << 0)
					ofs(DP_LANE_CHANNEL_EQ_DONE , "CHANNEL_EQ_DONE") // (1 << 1)
					ofs(DP_LANE_SYMBOL_LOCKED   , "SYMBOL_LOCKED"  ) // (1 << 2)
					ofd(8                                          ) // (1 << 3)
				} // for lane
			}

			ob(DP_LANE_ALIGN_STATUS_UPDATED_ESI) { // 0x200e /* status same as 0x204 */
				of(DP_INTERLANE_ALIGN_DONE) // (1 << 0)
				ofd(0x3e)
				of(DP_DOWNSTREAM_PORT_STATUS_CHANGED) // (1 << 6)
				of(DP_LINK_STATUS_UPDATED) // (1 << 7)
			}

			ob(DP_SINK_STATUS_ESI) { // 0x200f /* status same as 0x205 */
				of(DP_RECEIVE_PORT_0_STATUS) // (1 << 0)
				of(DP_RECEIVE_PORT_1_STATUS) // (1 << 1)
				of(DP_STREAM_REGENERATION_STATUS) // (1 << 2) /* 2.0 */
				of(DP_INTRA_HOP_AUX_REPLY_INDICATION) // (1 << 3) /* 2.0 */
				ofd(0xf0)
			}

			dumpnotzero(0x02010, 0x02200)

			olf; OUTDENT
		}

		if (!iszero(0x02200, 0x02300)) {
			iprintf("Extended Receiver Capability"); INDENT

			ob(DP_DP13_DPCD_REV) { // 0x2200
				cp("%d.%d", (val >> 4) & 0xf, val & 0xf);
			}

			#define D3_MAX_LINK_RATE 0x2201
			ob(D3_MAX_LINK_RATE) { // 0x001
				switch (val) {
					oc( 6, "RBR")
					oc(10, "HBR")
					oc(12, "3_24 (AppleVGA)")
					oc(20, "HBR2")
					oc(30, "HBR3")
					od("?%gGbps (unknown)", 0.27 * val)
				}
			}

			#define D3_MAX_LANE_COUNT 0x2202
			ob(D3_MAX_LANE_COUNT) { // 0x002
				oim(DP_MAX_LANE_COUNT_MASK)
				of(DP_ENHANCED_FRAME_CAP)
				of(DP_TPS3_SUPPORTED)
				ofd(1 << 5)
			}
			
			#define D3_MAX_DOWNSPREAD 0x2203
			ob(D3_MAX_DOWNSPREAD) { // 0x003
				of(DP_MAX_DOWNSPREAD_0_5)
				of(DP_STREAM_REGENERATION_STATUS_CAP)
				ofd(0x3c)
				of(DP_NO_AUX_HANDSHAKE_LINK_TRAINING)
				of(DP_TPS4_SUPPORTED)
			}
			
			#define D3_NORP 0x2204
			ob(D3_NORP) { // 0x004
				cp("%d", (val & 1) + 1);
				ofd(0xfe)
			}
			
			#define D3_DOWNSTREAMPORT_PRESENT 0x2205
			ob(D3_DOWNSTREAMPORT_PRESENT) { // 0x005
				of(DP_DWN_STRM_PORT_PRESENT)
				lb("PORT_TYPE")
				switch (val & DP_DWN_STRM_PORT_TYPE_MASK) {
					oc(DP_DWN_STRM_PORT_TYPE_DP, "DisplayPort")
					oc(DP_DWN_STRM_PORT_TYPE_ANALOG, "Analog VGA")
					oc(DP_DWN_STRM_PORT_TYPE_TMDS, "DVI or HDMI")
					oc(DP_DWN_STRM_PORT_TYPE_OTHER, "Other (no EDID)")
				}
				of(DP_FORMAT_CONVERSION)
				of(DP_DETAILED_CAP_INFO_AVAILABLE)
				ofd(0xe0)
			}
			//int portsize = (val & DP_DETAILED_CAP_INFO_AVAILABLE) ? 4 : 1;

			#define D3_MAIN_LINK_CHANNEL_CODING 0x2206
			ob(D3_MAIN_LINK_CHANNEL_CODING) { // 0x006
				of(DP_CAP_ANSI_8B10B)
				of(DP_CAP_ANSI_128B132B)
				ofd(0xfc)
			}
			
			#define D3_DOWN_STREAM_PORT_COUNT 0x2207
			ob(D3_DOWN_STREAM_PORT_COUNT) { // 0x007
				oim(DP_PORT_COUNT_MASK)
				ofd(0x30)
				of(DP_MSA_TIMING_PAR_IGNORED)
				of(DP_OUI_SUPPORT)
			}

			#define D3_RECEIVE_PORT_0_CAP_0 0x2208
			ob(D3_RECEIVE_PORT_0_CAP_0) { // 0x008
				ofd(0x01)
				of(DP_LOCAL_EDID_PRESENT) // (1 << 1)
				of(DP_ASSOCIATED_TO_PRECEDING_PORT) // (1 << 2)
				ofd(0xf8)
			}

			#define D3_RECEIVE_PORT_0_BUFFER_SIZE 0x2209
			ob(D3_RECEIVE_PORT_0_BUFFER_SIZE) { // 0x009
				cp("%d bytes per lane", (val + 1) * 32 );
			}

			#define D3_RECEIVE_PORT_1_CAP_0 0x220a
			ob(D3_RECEIVE_PORT_1_CAP_0) { // 0x00a
				ofd(0xf9)
				of(DP_LOCAL_EDID_PRESENT) // (1 << 1)
				of(DP_ASSOCIATED_TO_PRECEDING_PORT) // (1 << 2)
			}

			#define D3_RECEIVE_PORT_1_BUFFER_SIZE 0x220b
			ob(D3_RECEIVE_PORT_1_BUFFER_SIZE) { // 0x00b
				cp("%d bytes per lane", (val + 1) * 32 );
			}

			#define D3_I2C_SPEED_CAP 0x220c
			ob(D3_I2C_SPEED_CAP) { // 0x00c    /* DPI */
				ofs(DP_I2C_SPEED_1K, "1kbps") // 0x01
				ofs(DP_I2C_SPEED_5K, "5kbps") // 0x02
				ofs(DP_I2C_SPEED_10K, "10kbps") // 0x04
				ofs(DP_I2C_SPEED_100K, "100kbps") // 0x08
				ofs(DP_I2C_SPEED_400K, "400kbps") // 0x10
				ofs(DP_I2C_SPEED_1M, "1Mbps") // 0x20
				ofd(0xc0)
			}

			#define D3_EDP_CONFIGURATION_CAP 0x220d
			ob(D3_EDP_CONFIGURATION_CAP) { // 0x00d   /* XXX 1.2? */
				of(DP_ALTERNATE_SCRAMBLER_RESET_CAP) // (1 << 0)
				of(DP_FRAMING_CHANGE_CAP) // (1 << 1)
				of(DP_DPCD_DISPLAY_CONTROL_CAPABLE) // (1 << 3) /* edp v1.2 or higher */
				ofd(0xf8)
			}

				#define D3_TRAINING_AUX_RD_INTERVAL 0x220e
				ob(D3_TRAINING_AUX_RD_INTERVAL) { // 0x00e   /* XXX 1.2? */
				sw(DP_TRAINING_AUX_RD_MASK) { // 0x7F    /* DP 1.3 */
					oc(0, "100µs for Main Link Clock Recovery phase, 400µs for Main Link Channel Equalization phase and FAUX training")
					oc(1, "4ms all")
					oc(2, "8ms all")
					oc(3, "12ms all")
					oc(4, "16ms all")
					od("?%dms all (reserved)", (val & DP_TRAINING_AUX_RD_MASK) * 4)
				}
				of(DP_EXTENDED_RECEIVER_CAP_FIELD_PRESENT) // (1 << 7) /* DP 1.3 */
			}

			#define D3_ADAPTER_CAP 0x220f
			ob(D3_ADAPTER_CAP) { // 0x00f   /* 1.2 */
				of(DP_FORCE_LOAD_SENSE_CAP) // (1 << 0)
				of(DP_ALTERNATE_I2C_PATTERN_CAP) // (1 << 1)
				ofd(0xfc)
			}

			ob(DP_DPRX_FEATURE_ENUMERATION_LIST) { // 0x2210  /* DP 1.3 */
				of(DP_GTC_CAP) // (1 << 0)  /* DP 1.3 */
				of(DP_SST_SPLIT_SDP_CAP) // (1 << 1)  /* DP 1.4 */
				of(DP_AV_SYNC_CAP) // (1 << 2)  /* DP 1.3 */
				of(DP_VSC_SDP_EXT_FOR_COLORIMETRY_SUPPORTED) // (1 << 3)  /* DP 1.3 */
				of(DP_VSC_EXT_VESA_SDP_SUPPORTED) // (1 << 4)  /* DP 1.4 */
				of(DP_VSC_EXT_VESA_SDP_CHAINING_SUPPORTED) // (1 << 5)  /* DP 1.4 */
				of(DP_VSC_EXT_CEA_SDP_SUPPORTED) // (1 << 6)  /* DP 1.4 */
				of(DP_VSC_EXT_CEA_SDP_CHAINING_SUPPORTED) // (1 << 7)  /* DP 1.4 */
			}

			dumpnotzero(0x02211, 0x02215)

			ob(DP_128B132B_SUPPORTED_LINK_RATES) { // 0x2215 /* 2.0 */
				ofs(DP_UHBR10, "UHBR 10") // (1 << 0)
				ofs(DP_UHBR20, "UHBR 20") // (1 << 1)
				ofs(DP_UHBR13_5, "UHBR 13.5") // (1 << 2)
				ofd(0xf8)
			}

			ob(DP_128B132B_TRAINING_AUX_RD_INTERVAL) { // 0x2216 /* 2.0 */
				sw(DP_128B132B_TRAINING_AUX_RD_INTERVAL_MASK) { // 0x7f
					ofs(DP_128B132B_TRAINING_AUX_RD_INTERVAL_400_US ,"400µs") // 0x00
					ofs(DP_128B132B_TRAINING_AUX_RD_INTERVAL_4_MS   ,  "4ms") // 0x01
					ofs(DP_128B132B_TRAINING_AUX_RD_INTERVAL_8_MS   ,  "8ms") // 0x02
					ofs(DP_128B132B_TRAINING_AUX_RD_INTERVAL_12_MS  , "12ms") // 0x03
					ofs(DP_128B132B_TRAINING_AUX_RD_INTERVAL_16_MS  , "16ms") // 0x04
					ofs(DP_128B132B_TRAINING_AUX_RD_INTERVAL_32_MS  , "32ms") // 0x05
					ofs(DP_128B132B_TRAINING_AUX_RD_INTERVAL_64_MS  , "64ms") // 0x06
					ofs(DP_128B132B_TRAINING_AUX_RD_INTERVAL_1MS_UNIT, "1ms unit") // (1 << 7)
				}
				ofd(0x80)
			}
			
			ob(DP_CABLE_ATTRIBUTES_UPDATED_BY_DPRX) { // 0x2217
				ofd(0xff)
			}

			dumpnotzero(0x02218, 0x02230)
			
			dumpnotzero(DP_TEST_264BIT_CUSTOM_PATTERN_7_0, DP_TEST_264BIT_CUSTOM_PATTERN_263_256 + 1, "DP_TEST_264BIT_CUSTOM_PATTERN") // 0x2230 - 0x2250

			dumpnotzero(0x02251, 0x02260)

			/* DSC Extended Capability Branch Total DSC Resources */
			ob(DP_DSC_SUPPORT_AND_DSC_DECODER_COUNT) { // 0x2260	/* 2.0 */
				cp("%d", (val & DP_DSC_DECODER_COUNT_MASK) >> DP_DSC_DECODER_COUNT_SHIFT); // (0b111 << 5)
				ofd(0x1f)
			}
			ob(DP_DSC_MAX_SLICE_COUNT_AND_AGGREGATION_0) { // 0x2270	/* 2.0 */
				lb("DSC_DECODER_0_MAXIMUM_SLICE_COUNT")
				oim(DP_DSC_DECODER_0_MAXIMUM_SLICE_COUNT_MASK) // (1 << 0)
				lb("DSC_DECODER_0_AGGREGATION_SUPPORT")
				cp("%d", (val & DP_DSC_DECODER_0_AGGREGATION_SUPPORT_MASK) >> DP_DSC_DECODER_0_AGGREGATION_SUPPORT_SHIFT); // (0b111 << 1)
				ofd(0xf0)
			}
			
			dumpnotzero(0x02271, 0x02300)

			olf; OUTDENT
		}

		if (!iszero(0x02300, 0x03000)) {
			iprintf("Undefined"); INDENT
			dumpnotzero(0x02300, 0x03000)
			olf; OUTDENT
		}

		if (!iszero(0x03000, 0x03100)) {
			iprintf("PCON HDMI CONFIG PPS Override Buffer"); INDENT

			/* HDMI CEC tunneling over AUX DP 1.3 section 5.3.3.3.1 DPCD 1.4+ */
			ob(DP_CEC_TUNNELING_CAPABILITY) { // 0x3000
				of(DP_CEC_TUNNELING_CAPABLE) // (1 << 0)
				of(DP_CEC_SNOOPING_CAPABLE) // (1 << 1)
				of(DP_CEC_MULTIPLE_LA_CAPABLE) // (1 << 2)
				ofd(0xf8)
			}

			ob(DP_CEC_TUNNELING_CONTROL) { // 0x3001
				of(DP_CEC_TUNNELING_ENABLE) // (1 << 0)
				of(DP_CEC_SNOOPING_ENABLE) // (1 << 1)
				ofd(0xfc)
			}

			ob(DP_CEC_RX_MESSAGE_INFO) { // 0x3002
				lb("CEC_RX_MESSAGE_LEN")
				cp("%d", (val & DP_CEC_RX_MESSAGE_LEN_MASK) >> DP_CEC_RX_MESSAGE_LEN_SHIFT); // (0xf << 0)
				of(DP_CEC_RX_MESSAGE_HPD_STATE) // (1 << 4)
				of(DP_CEC_RX_MESSAGE_HPD_LOST) // (1 << 5)
				of(DP_CEC_RX_MESSAGE_ACKED) // (1 << 6)
				of(DP_CEC_RX_MESSAGE_ENDED) // (1 << 7)
			}

			ob(DP_CEC_TX_MESSAGE_INFO) { // 0x3003
				lb("CEC_TX_MESSAGE_LEN")
				cp("%d", (val & DP_CEC_TX_MESSAGE_LEN_MASK) >> DP_CEC_TX_MESSAGE_LEN_SHIFT); // (0xf << 0)
				lb("CEC_TX_RETRY_COUNT")
				cp("%d", (val & DP_CEC_TX_RETRY_COUNT_MASK) >> DP_CEC_TX_RETRY_COUNT_SHIFT); // (0x7 << 4)
				of(DP_CEC_TX_MESSAGE_SEND) // (1 << 7)
			}

			ob(DP_CEC_TUNNELING_IRQ_FLAGS) { // 0x3004
				of(DP_CEC_RX_MESSAGE_INFO_VALID) // (1 << 0)
				of(DP_CEC_RX_MESSAGE_OVERFLOW) // (1 << 1)
				ofd(0x0c)
				of(DP_CEC_TX_MESSAGE_SENT) // (1 << 4)
				of(DP_CEC_TX_LINE_ERROR) // (1 << 5)
				of(DP_CEC_TX_ADDRESS_NACK_ERROR) // (1 << 6)
				of(DP_CEC_TX_DATA_NACK_ERROR) // (1 << 7)
			}
			
			dumpnotzero(0x03005, 0x0300e)

			ob(DP_CEC_LOGICAL_ADDRESS_MASK) { // 0x300E /* 0x300F word */
				of(DP_CEC_LOGICAL_ADDRESS_0) // (1 << 0)
				of(DP_CEC_LOGICAL_ADDRESS_1) // (1 << 1)
				of(DP_CEC_LOGICAL_ADDRESS_2) // (1 << 2)
				of(DP_CEC_LOGICAL_ADDRESS_3) // (1 << 3)
				of(DP_CEC_LOGICAL_ADDRESS_4) // (1 << 4)
				of(DP_CEC_LOGICAL_ADDRESS_5) // (1 << 5)
				of(DP_CEC_LOGICAL_ADDRESS_6) // (1 << 6)
				of(DP_CEC_LOGICAL_ADDRESS_7) // (1 << 7)
			}
			ob(DP_CEC_LOGICAL_ADDRESS_MASK_2) { // 0x300F /* 0x300E word */
				of(DP_CEC_LOGICAL_ADDRESS_8) // (1 << 0)
				of(DP_CEC_LOGICAL_ADDRESS_9) // (1 << 1)
				of(DP_CEC_LOGICAL_ADDRESS_10) // (1 << 2)
				of(DP_CEC_LOGICAL_ADDRESS_11) // (1 << 3)
				of(DP_CEC_LOGICAL_ADDRESS_12) // (1 << 4)
				of(DP_CEC_LOGICAL_ADDRESS_13) // (1 << 5)
				of(DP_CEC_LOGICAL_ADDRESS_14) // (1 << 6)
				of(DP_CEC_LOGICAL_ADDRESS_15) // (1 << 7)
			}

			dumpnotzero(DP_CEC_RX_MESSAGE_BUFFER, 0x3020, "CEC_RX_MESSAGE_BUFFER") // 0x3010
			dumpnotzero(DP_CEC_TX_MESSAGE_BUFFER, 0x3030, "CEC_RX_MESSAGE_BUFFER") // 0x3020
			
			dumpnotzero(0x03030, 0x03036)

			ob(DP_PCON_HDMI_POST_FRL_STATUS) { // 0x3036
				/* PCON HDMI POST FRL STATUS */
				lb("PCON_HDMI_LINK_MODE") // (1 << 0)
				of2(DP_PCON_HDMI_MODE_FRL, DP_PCON_HDMI_MODE_TMDS) // 1 0
				lb("PCON_HDMI_FRL_TRAINED_BW") // (0x3F << 1)
				ofs(DP_PCON_FRL_TRAINED_BW_9GBPS ,  "9Gbps") // (1 << 1)
				ofs(DP_PCON_FRL_TRAINED_BW_18GBPS, "18Gbps") // (1 << 2)
				ofs(DP_PCON_FRL_TRAINED_BW_24GBPS, "24Gbps") // (1 << 3)
				ofs(DP_PCON_FRL_TRAINED_BW_32GBPS, "32Gbps") // (1 << 4)
				ofs(DP_PCON_FRL_TRAINED_BW_40GBPS, "40Gbps") // (1 << 5)
				ofs(DP_PCON_FRL_TRAINED_BW_48GBPS, "48Gbps") // (1 << 6)
				ofd(0x80)
			}

			/* PCON Downstream HDMI ERROR Status per Lane */
			ob(DP_PCON_HDMI_ERROR_STATUS_LN0) { // 0x3037
				// DP_PCON_HDMI_ERROR_COUNT_MASK         (0x7 << 0)
				of(DP_PCON_HDMI_ERROR_COUNT_THREE_PLUS) // (1 << 0)
				of(DP_PCON_HDMI_ERROR_COUNT_TEN_PLUS) // (1 << 1)
				of(DP_PCON_HDMI_ERROR_COUNT_HUNDRED_PLUS) // (1 << 2)
				ofd(0xf8)
			}
			ob(DP_PCON_HDMI_ERROR_STATUS_LN1) { // 0x3038
				// DP_PCON_HDMI_ERROR_COUNT_MASK         (0x7 << 0)
				of(DP_PCON_HDMI_ERROR_COUNT_THREE_PLUS) // (1 << 0)
				of(DP_PCON_HDMI_ERROR_COUNT_TEN_PLUS) // (1 << 1)
				of(DP_PCON_HDMI_ERROR_COUNT_HUNDRED_PLUS) // (1 << 2)
				ofd(0xf8)
			}
			ob(DP_PCON_HDMI_ERROR_STATUS_LN2) { // 0x3039
				// DP_PCON_HDMI_ERROR_COUNT_MASK         (0x7 << 0)
				of(DP_PCON_HDMI_ERROR_COUNT_THREE_PLUS) // (1 << 0)
				of(DP_PCON_HDMI_ERROR_COUNT_TEN_PLUS) // (1 << 1)
				of(DP_PCON_HDMI_ERROR_COUNT_HUNDRED_PLUS) // (1 << 2)
				ofd(0xf8)
			}
			ob(DP_PCON_HDMI_ERROR_STATUS_LN3) { // 0x303A
				// DP_PCON_HDMI_ERROR_COUNT_MASK         (0x7 << 0)
				of(DP_PCON_HDMI_ERROR_COUNT_THREE_PLUS) // (1 << 0)
				of(DP_PCON_HDMI_ERROR_COUNT_TEN_PLUS) // (1 << 1)
				of(DP_PCON_HDMI_ERROR_COUNT_HUNDRED_PLUS) // (1 << 2)
				ofd(0xf8)
			}

			ob(DP_PCON_HDMI_TX_LINK_STATUS) { // 0x303B
				/* PCON HDMI LINK STATUS */
				of(DP_PCON_HDMI_TX_LINK_ACTIVE) // (1 << 0)
				of(DP_PCON_FRL_READY) // (1 << 1)
				ofd(0xfc)
			}

			dumpnotzero(0x0303c, 0x03050)

			ob(DP_PROTOCOL_CONVERTER_CONTROL_0) { // 0x3050 /* DP 1.3 */
				of(DP_HDMI_DVI_OUTPUT_CONFIG) // (1 << 0) /* DP 1.3 */
				ofd(0xfe)
			}
			
			ob(DP_PROTOCOL_CONVERTER_CONTROL_1) { // 0x3051 /* DP 1.3 */
				of(DP_HDMI_FORCE_SCRAMBLING) // (1 << 0) /* DP 1.3 */
				of(DP_HDMI_EDID_PROCESSING_DISABLE) // (1 << 1) /* DP 1.4 */
				of(DP_HDMI_AUTONOMOUS_SCRAMBLING_DISABLE) // (1 << 2) /* DP 1.4 */
				of(DP_HDMI_FORCE_SCRAMBLING) // (1 << 3) /* DP 1.4 */
				ofd(0xf0)
			}

			ob(DP_PROTOCOL_CONVERTER_CONTROL_2) { // 0x3052 /* DP 1.3 */
				of(DP_CONVERSION_TO_YCBCR422_ENABLE) // (1 << 0) /* DP 1.3 */
				of(DP_PCON_ENABLE_DSC_ENCODER) // (1 << 1)
				sw(DP_PCON_ENCODER_PPS_OVERRIDE_MASK) { // (3 << 2)
					oc(DP_PCON_ENC_PPS_OVERRIDE_DISABLED , "PCON_ENC_PPS_OVERRIDE_DISABLED" ) // 0
					oc(DP_PCON_ENC_PPS_OVERRIDE_EN_PARAMS, "PCON_ENC_PPS_OVERRIDE_EN_PARAMS") // 1
					oc(DP_PCON_ENC_PPS_OVERRIDE_EN_BUFFER, "PCON_ENC_PPS_OVERRIDE_EN_BUFFER") // 2
					od("?%d (unknown PCON_ENCODER_PPS_OVERRIDE)", (val & DP_PCON_ENCODER_PPS_OVERRIDE_MASK) >> 2)
				}
				// DP_CONVERSION_RGB_YCBCR_MASK (7 << 4)
				of(DP_CONVERSION_BT601_RGB_YCBCR_ENABLE) // (1 << 4)
				of(DP_CONVERSION_BT709_RGB_YCBCR_ENABLE) // (1 << 5)
				of(DP_CONVERSION_BT2020_RGB_YCBCR_ENABLE) // (1 << 6)
				ofd(0x80)
			}

			dumpnotzero(0x03053, 0x0305a)

			ob(DP_PCON_HDMI_LINK_CONFIG_1) { // 0x305A
				/* PCON CONFIGURE-1 FRL FOR HDMI SINK */
				lb("PCON_ENABLE_MAX_FRL_BW")
				sw(DP_PCON_ENABLE_MAX_FRL_BW) { // (7 << 0)
					oc(DP_PCON_ENABLE_MAX_BW_0GBPS ,  "0Gbps") // 0
					oc(DP_PCON_ENABLE_MAX_BW_9GBPS ,  "9Gbps") // 1
					oc(DP_PCON_ENABLE_MAX_BW_18GBPS, "18Gbps") // 2
					oc(DP_PCON_ENABLE_MAX_BW_24GBPS, "24Gbps") // 3
					oc(DP_PCON_ENABLE_MAX_BW_32GBPS, "32Gbps") // 4
					oc(DP_PCON_ENABLE_MAX_BW_40GBPS, "40Gbps") // 5
					oc(DP_PCON_ENABLE_MAX_BW_48GBPS, "48Gbps") // 6
					od("?%d (reserved)", (val & DP_PCON_ENABLE_MAX_FRL_BW) >> 0)
				}
				of(DP_PCON_ENABLE_SOURCE_CTL_MODE) // (1 << 3)
				of2(DP_PCON_ENABLE_CONCURRENT_LINK, DP_PCON_ENABLE_SEQUENTIAL_LINK) // (1 << 4) (0 << 4)
				of(DP_PCON_ENABLE_LINK_FRL_MODE) // (1 << 5)
				of(DP_PCON_ENABLE_HPD_READY) // (1 << 6)
				of(DP_PCON_ENABLE_HDMI_LINK) // (1 << 7)
			}

			ob(DP_PCON_HDMI_LINK_CONFIG_2) { // 0x305B
				/* PCON CONFIGURE-2 FRL FOR HDMI SINK */
				lb("PCON_MAX_LINK_BW") // (0x3F << 0)
				ofs(DP_PCON_FRL_BW_MASK_9GBPS ,  "9Gbps") // (1 << 0)
				ofs(DP_PCON_FRL_BW_MASK_18GBPS, "18Gbps") // (1 << 1)
				ofs(DP_PCON_FRL_BW_MASK_24GBPS, "24Gbps") // (1 << 2)
				ofs(DP_PCON_FRL_BW_MASK_32GBPS, "32Gbps") // (1 << 3)
				ofs(DP_PCON_FRL_BW_MASK_40GBPS, "40Gbps") // (1 << 4)
				ofs(DP_PCON_FRL_BW_MASK_48GBPS, "48Gbps") // (1 << 5)
				of2(DP_PCON_FRL_LINK_TRAIN_EXTENDED, DP_PCON_FRL_LINK_TRAIN_NORMAL) // (1 << 6) (0 << 6)
				ofd(0x80)
			}

			dumpnotzero(0x0305c, 0x03100)

			olf; OUTDENT
		}

		if (!iszero(0x03100, 0x03200)) {
			iprintf("Protocol Converter Extension"); INDENT

			/* PCON HDMI CONFIG PPS Override Buffer
			 * Valid Offsets to be added to Base : 0-127
			 */
			dumpnotzero(DP_PCON_HDMI_PPS_OVERRIDE_BASE, 0x3180, "PCON_HDMI_PPS_OVERRIDE_BUFFER") // 0x3100

			/* PCON HDMI CONFIG PPS Override Parameter: Slice height
			 * Offset-0 8LSBs of the Slice height.
			 * Offset-1 8MSBs of the Slice height.
			 */
			ob16(DP_PCON_HDMI_PPS_OVRD_SLICE_HEIGHT) { // 0x3180, 0x3181
				oi()
			}

			/* PCON HDMI CONFIG PPS Override Parameter: Slice width
			 * Offset-0 8LSBs of the Slice width.
			 * Offset-1 8MSBs of the Slice width.
			 */
			ob16(DP_PCON_HDMI_PPS_OVRD_SLICE_WIDTH) { // 0x3182, 0x3183
				oi()
			}

			/* PCON HDMI CONFIG PPS Override Parameter: bits_per_pixel
			 * Offset-0 8LSBs of the bits_per_pixel.
			 * Offset-1 2MSBs of the bits_per_pixel.
			 */
			ob16(DP_PCON_HDMI_PPS_OVRD_BPP) { // 0x3184, 0x3185
				oim(0x3ff)
				ofd(0xfc00)
			}

			dumpnotzero(0x03186, 0x03200)

			olf; OUTDENT
		}

		if (!iszero(0x03200, 0x68000)) {
			iprintf("Undefined"); INDENT
			dumpnotzero(0x03200, 0x68000)
			olf; OUTDENT
		}

		if (!iszero(0x68000, 0x69000)) {
			iprintf("HDCP 1.3 and HDCP 2.2"); INDENT

			dumpnotzero(DP_AUX_HDCP_BKSV, DP_AUX_HDCP_BKSV + DRM_HDCP_KSV_LEN, "AUX_HDCP_BKSV") // 0x68000, 5
			dumpnotzero(DP_AUX_HDCP_RI_PRIME, DP_AUX_HDCP_RI_PRIME + DRM_HDCP_RI_LEN, "AUX_HDCP_RI_PRIME") // 0x68005, 2
			dumpnotzero(DP_AUX_HDCP_AKSV, DP_AUX_HDCP_AKSV + DRM_HDCP_KSV_LEN, "AUX_HDCP_AKSV") // 0x68007, 5
			dumpnotzero(DP_AUX_HDCP_AN, DP_AUX_HDCP_AN + DRM_HDCP_AN_LEN, "AUX_HDCP_AN") // 0x6800C, 8

			for (i = 0; i < DRM_HDCP_V_PRIME_NUM_PARTS; i++) {
				dumpnotzero(DP_AUX_HDCP_V_PRIME(i), DP_AUX_HDCP_V_PRIME((i+1)), "AUX_HDCP_V_PRIME[%d]", i) // 0x68014, 4 bytes each * 5
			}
			
			ob(DP_AUX_HDCP_BCAPS) { // 0x68028
				of(DP_BCAPS_HDCP_CAPABLE) // BIT(0)
				of(DP_BCAPS_REPEATER_PRESENT) // BIT(1)
				ofd(0xfc)
			}
			
			ob(DP_AUX_HDCP_BSTATUS) { // 0x68029
				of(DP_BSTATUS_READY) // BIT(0)
				of(DP_BSTATUS_R0_PRIME_READY) // BIT(1)
				of(DP_BSTATUS_LINK_FAILURE) // BIT(2)
				of(DP_BSTATUS_REAUTH_REQ) // BIT(3)
				ofd(0xf0)
			}

			ob16(DP_AUX_HDCP_BINFO) { // 0x6802A, 2
				lb("DEVICE_COUNT")
				oim(0x7f)
				ofs(0x80, "MAX_DEVS_EXCEEDED")
				lb("DEPTH")
				cp("%d", (val & 0x700) >> 8);
				ofs(0x800, "MAX_CASCADE_EXCEEDED")
				ofd(0xf000)
			}
			
			for (i = 0; i < 3; i++) {
				dumpnotzero(DP_AUX_HDCP_KSV_FIFO + i * DRM_HDCP_KSV_LEN, DP_AUX_HDCP_KSV_FIFO + (i+1) * DRM_HDCP_KSV_LEN, "AUX_HDCP_KSV_FIFO[%d]", i) // 0x6802C, 5 bytes each * 3
			}

			ob(DP_AUX_HDCP_AINFO) { // 0x6803B
				ofs(1, "REAUTHENTICATION_ENABLE_IRQ_HPD")
				ofd(0xfe)
			}
			
			dumpnotzero(0x6803c, 0x680c0)

			dumpnotzero(0x680c0, 0x68100, "AUX_HDCP_DBG")
			
			dumpnotzero(0x68100, 0x69000)

			olf; OUTDENT
		}

		if (!iszero(0x69000, 0x6a000)) {
			iprintf("DP HDCP 2.2 Parameters"); INDENT

#define DP_HDCP_2_2_REG_RTX         	DP_HDCP_2_2_REG_RTX_OFFSET
#define DP_HDCP_2_2_REG_TXCAPS			DP_HDCP_2_2_REG_TXCAPS_OFFSET
#define DP_HDCP_2_2_REG_CERT_RX			DP_HDCP_2_2_REG_CERT_RX_OFFSET
#define DP_HDCP_2_2_REG_RRX		    	DP_HDCP_2_2_REG_RRX_OFFSET
#define DP_HDCP_2_2_REG_RX_CAPS			DP_HDCP_2_2_REG_RX_CAPS_OFFSET
#define DP_HDCP_2_2_REG_EKPUB_KM		DP_HDCP_2_2_REG_EKPUB_KM_OFFSET
#define DP_HDCP_2_2_REG_EKH_KM_WR		DP_HDCP_2_2_REG_EKH_KM_WR_OFFSET
#define DP_HDCP_2_2_REG_M				DP_HDCP_2_2_REG_M_OFFSET
#define DP_HDCP_2_2_REG_HPRIME			DP_HDCP_2_2_REG_HPRIME_OFFSET
#define DP_HDCP_2_2_REG_EKH_KM_RD		DP_HDCP_2_2_REG_EKH_KM_RD_OFFSET
#define DP_HDCP_2_2_REG_RN				DP_HDCP_2_2_REG_RN_OFFSET
#define DP_HDCP_2_2_REG_LPRIME			DP_HDCP_2_2_REG_LPRIME_OFFSET
#define DP_HDCP_2_2_REG_EDKEY_KS		DP_HDCP_2_2_REG_EDKEY_KS_OFFSET
#define	DP_HDCP_2_2_REG_RIV				DP_HDCP_2_2_REG_RIV_OFFSET
#define DP_HDCP_2_2_REG_RXINFO			DP_HDCP_2_2_REG_RXINFO_OFFSET
#define DP_HDCP_2_2_REG_SEQ_NUM_V		DP_HDCP_2_2_REG_SEQ_NUM_V_OFFSET
#define DP_HDCP_2_2_REG_VPRIME			DP_HDCP_2_2_REG_VPRIME_OFFSET
#define DP_HDCP_2_2_REG_RECV_ID_LIST	DP_HDCP_2_2_REG_RECV_ID_LIST_OFFSET
#define DP_HDCP_2_2_REG_V				DP_HDCP_2_2_REG_V_OFFSET
#define DP_HDCP_2_2_REG_SEQ_NUM_M		DP_HDCP_2_2_REG_SEQ_NUM_M_OFFSET
#define DP_HDCP_2_2_REG_K				DP_HDCP_2_2_REG_K_OFFSET
#define DP_HDCP_2_2_REG_STREAM_ID_TYPE	DP_HDCP_2_2_REG_STREAM_ID_TYPE_OFFSET
#define DP_HDCP_2_2_REG_MPRIME			DP_HDCP_2_2_REG_MPRIME_OFFSET
#define DP_HDCP_2_2_REG_RXSTATUS		DP_HDCP_2_2_REG_RXSTATUS_OFFSET
#define DP_HDCP_2_2_REG_STREAM_TYPE		DP_HDCP_2_2_REG_STREAM_TYPE_OFFSET
#define DP_HDCP_2_2_REG_DBG				DP_HDCP_2_2_REG_DBG_OFFSET

			dumpnotzero(DP_HDCP_2_2_REG_RTX, DP_HDCP_2_2_REG_RTX + 8, "HDCP_2_2_REG_RTX") // 0x69000

			ob24be(DP_HDCP_2_2_REG_TXCAPS) { // 0x69008 w
				lb("VERSION")
				cp("%d", val >> 16);
				lb("TRANSMITTER_CAPABILITY_MASK")
				cp("%d", val & 0xffff);
			}
			
			dumpnotzero(DP_HDCP_2_2_REG_CERT_RX, DP_HDCP_2_2_REG_CERT_RX + 522, "HDCP_2_2_REG_CERT_RX") // 0x6900B

			dumpnotzero(DP_HDCP_2_2_REG_RRX, DP_HDCP_2_2_REG_RRX + 8, "HDCP_2_2_REG_RRX") // 0x69215

			ob24be(DP_HDCP_2_2_REG_RX_CAPS) { // 0x6921D
				lb("VERSION")
				cp("%d", val >> 16);
				lb("RECEIVER_CAPABILITY_MASK")
				cp("%d", (val & 0xfffc) >> 2);
				ofs(2, "HDCP_CAPABLE")
				ofs(1, "REPEATER")
			}

			dumpnotzero(DP_HDCP_2_2_REG_EKPUB_KM, DP_HDCP_2_2_REG_EKPUB_KM + 128, "HDCP_2_2_REG_EKPUB_KM") // 0x69220
			
			dumpnotzero(DP_HDCP_2_2_REG_EKH_KM_WR, DP_HDCP_2_2_REG_EKH_KM_WR + 16, "HDCP_2_2_REG_EKH_KM_WR") // 0x692A0

			dumpnotzero(DP_HDCP_2_2_REG_M, DP_HDCP_2_2_REG_M + 16, "HDCP_2_2_REG_M") // 0x692B0

			dumpnotzero(DP_HDCP_2_2_REG_HPRIME, DP_HDCP_2_2_REG_HPRIME + 32, "HDCP_2_2_REG_HPRIME") // 0x692C0

			dumpnotzero(DP_HDCP_2_2_REG_EKH_KM_RD, DP_HDCP_2_2_REG_EKH_KM_RD + 16, "HDCP_2_2_REG_EKH_KM_RD") // 0x692E0

			dumpnotzero(DP_HDCP_2_2_REG_RN, DP_HDCP_2_2_REG_RN + 8, "HDCP_2_2_REG_RN") // 0x692F0

			dumpnotzero(DP_HDCP_2_2_REG_LPRIME, DP_HDCP_2_2_REG_LPRIME + 32, "HDCP_2_2_REG_LPRIME") // 0x692F8

			dumpnotzero(DP_HDCP_2_2_REG_EDKEY_KS, DP_HDCP_2_2_REG_EDKEY_KS + 16, "HDCP_2_2_REG_EDKEY_KS") // 0x69318

			dumpnotzero(DP_HDCP_2_2_REG_RIV, DP_HDCP_2_2_REG_RIV + 8, "HDCP_2_2_REG_RIV") // 0x69328

			ob16be(DP_HDCP_2_2_REG_RXINFO) { // 0x69330
				ofd(0xf000)
				lb("DEPTH")
				cp("%d", (val & 0xe00) >> 9);
				lb("DEVICE_COUNT")
				cp("%d", (val & 0x1f0) >> 4);
				ofs(8, "MAX_DEVS_EXCEEDED")
				ofs(4, "MAX_CASCADE_EXCEEDED")
				ofs(2, "HDCP2_LEGACY_DEVICE_DOWNSTREAM")
				ofs(1, "HDCP1_DEVICE_DOWNSTREAM")
			}
			
			ob24be(DP_HDCP_2_2_REG_SEQ_NUM_V) { // 0x69332
				oi()
			}
			
			dumpnotzero(DP_HDCP_2_2_REG_VPRIME, DP_HDCP_2_2_REG_VPRIME + 16, "HDCP_2_2_REG_VPRIME") // 0x69335

			dumpnotzero(DP_HDCP_2_2_REG_VPRIME, DP_HDCP_2_2_REG_VPRIME + 16, "HDCP_2_2_REG_VPRIME") // 0x69335

			dumpnotzero(DP_HDCP_2_2_REG_RECV_ID_LIST, DP_HDCP_2_2_REG_RECV_ID_LIST + 155, "HDCP_2_2_REG_RECV_ID_LIST") // 0x69345

			dumpnotzero(DP_HDCP_2_2_REG_V, DP_HDCP_2_2_REG_V + 16, "HDCP_2_2_REG_RECV_ID_LIST") // 0x693E0

			ob24be(DP_HDCP_2_2_REG_SEQ_NUM_M) { // 0x693F0
				oi()
			}
			
			ob16be(DP_HDCP_2_2_REG_K) { // 0x693F3
				oi()
			}
			
			dumpnotzero(DP_HDCP_2_2_REG_STREAM_ID_TYPE, DP_HDCP_2_2_REG_STREAM_ID_TYPE + 126, "HDCP_2_2_REG_STREAM_ID_TYPE") // 0x693F5

			dumpnotzero(DP_HDCP_2_2_REG_MPRIME, DP_HDCP_2_2_REG_MPRIME + 32, "HDCP_2_2_REG_MPRIME") // 0x69473

			ob(DP_HDCP_2_2_REG_RXSTATUS) { // 0x69493
				ofd(0xe0)
				ofs(0x10, "LINK_INTEGRITY_FAILURE")
				ofs(0x08, "REAUTH_REQ")
				ofs(0x04, "PAIRING_AVAILABLE")
				ofs(0x02, "HPRIME_AVAILABLE")
				ofs(0x01, "READY")
			}

			ob(DP_HDCP_2_2_REG_STREAM_TYPE) { // 0x69494
				oi()
			}

			dumpnotzero(0x69495, 0x69495 + 131) // 0x69495

			dumpnotzero(DP_HDCP_2_2_REG_DBG, DP_HDCP_2_2_REG_DBG + 64, "HDCP_2_2_REG_DBG") // 0x69518

			dumpnotzero(0x69558, 0x6a000) // 0x69558

			olf; OUTDENT
		}

		if (!iszero(0x6a000, 0xf0000)) {
			iprintf("Undefined"); INDENT
			dumpnotzero(0x6a000, 0xf0000)
			olf; OUTDENT
		}

		if (!iszero(0xf0000, 0xf0300)) {
			iprintf("Link Training (LT)-tunable PHY Repeaters (LTTPR)"); INDENT

			ob(DP_LT_TUNABLE_PHY_REPEATER_FIELD_DATA_STRUCTURE_REV) { // 0xf0000 /* 1.3 */
				cp("%d.%d", (val >> 4) & 0xf, val & 0xf);
			}

			ob(DP_MAX_LINK_RATE_PHY_REPEATER) { // 0xf0001 /* 1.4a */
				switch (val) {
					oc(DP_LINK_RATE_TABLE, "LINK_RATE_TABLE") // 0x00    /* eDP 1.4 */
					oc(DP_LINK_BW_1_62, "RBR") // 0x06
					oc(DP_LINK_BW_2_7, "HBR") // 0x0a
					oc(12, "3_24 (AppleVGA)") // 0x0c
					oc(DP_LINK_BW_5_4, "HBR2") // 0x14    /* 1.2 */
					oc(DP_LINK_BW_8_1, "HBR3") // 0x1e    /* 1.4 */
					oc(DP_LINK_BW_10, "UHBR 10") // 0x01    /* 2.0 128b/132b Link Layer */
					oc(DP_LINK_BW_13_5, "UHBR 13.5") // 0x04    /* 2.0 128b/132b Link Layer */
					oc(DP_LINK_BW_20, "UHBR 20") // 0x02    /* 2.0 128b/132b Link Layer */
					od("?%gGbps (unknown)", 0.27 * val)
				}
			}

			ob(DP_PHY_REPEATER_CNT) { // 0xf0002 /* 1.3 */
				oi()
			}

			ob(DP_PHY_REPEATER_MODE) { // 0xf0003 /* 1.3 */
				switch (val) {
					oc(DP_PHY_REPEATER_MODE_TRANSPARENT, "TRANSPARENT")
					oc(DP_PHY_REPEATER_MODE_NON_TRANSPARENT, "NON_TRANSPARENT")
					od("?%x (unknown)", val)
				}
			}

			ob(DP_MAX_LANE_COUNT_PHY_REPEATER) { // 0xf0004 /* 1.4a */
				oim(DP_MAX_LANE_COUNT_MASK)
				ofd(0xe0)
			}

			#if 0
			ob(DP_Repeater_FEC_CAPABILITY) { // 0xf0004 /* 1.4 */
				oi()
			}
			#endif

			ob(DP_PHY_REPEATER_EXTENDED_WAIT_TIMEOUT) { // 0xf0005 /* 1.4a */
				oim(0x7f)
				ofd(0x80)
			}

			ob(DP_MAIN_LINK_CHANNEL_CODING_PHY_REPEATER) { // 0xf0006 /* 2.0 */
				of(DP_PHY_REPEATER_128B132B_SUPPORTED) // (1 << 0)
				ofd(0xfe)
			}

			ob (DP_PHY_REPEATER_128B132B_RATES) { // 0xf0007 /* 2.0 */
			   /* See DP_128B132B_SUPPORTED_LINK_RATES for values */
			   ofs(DP_UHBR10, "UHBR 10") // (1 << 0)
			   ofs(DP_UHBR20, "UHBR 20") // (1 << 1)
			   ofs(DP_UHBR13_5, "UHBR 13.5") // (1 << 2)
			   ofd(0xf8)
		   }

			ob (DP_PHY_REPEATER_EQ_DONE) { // 0xf0008 /* 2.0 E11 */
				oi()
			}

			dumpnotzero(0xf0009, 0xf0010)

			for (i = 0; i < DP_MAX_LTTPR_COUNT; i++) { // 0xf0010, 0xf0060, 0xf00b0, 0xf0100,   0xf0150, 0xf01a0, 0xf01f0, 0xf0240 /* 1.3 */
				enum drm_dp_phy dp_phy = (enum drm_dp_phy)DP_PHY_LTTPR(i);
				if (!iszero(DP_LTTPR_BASE(dp_phy), DP_LTTPR_BASE(DP_PHY_LTTPR(i+1)))) {

					obf(DP_TRAINING_PATTERN_SET_PHY_REPEATER(dp_phy), "TRAINING_PATTERN_SET_PHY_REPEATER%d", i+1) { // 0xf0010 /* 1.3 */
						oi();
					}

					for (int lane = 0; lane < 4; lane++) {
						obf(DP_TRAINING_LANE0_SET_PHY_REPEATER(dp_phy) + lane, "TRAINING_LANE%d_SET_PHY_REPEATER%d", lane, i+1) { // 0xf0011, 0xf0012, 0xf0013, 0xf0014 /* 1.3 */
							oi();
						}
					}

					dumpnotzero(DP_LTTPR_REG(dp_phy, 0xf0015), DP_LTTPR_REG(dp_phy, 0xf0020))

					obf(DP_TRAINING_AUX_RD_INTERVAL_PHY_REPEATER(dp_phy), "TRAINING_AUX_RD_INTERVAL_PHY_REPEATER%d", i+1) { // 0xf0020 /* 1.4a */
						oi();
					}

					obf(DP_LTTPR_REG(dp_phy, DP_TRANSMITTER_CAPABILITY_PHY_REPEATER1), "TRANSMITTER_CAPABILITY_PHY_REPEATER%d", i+1) { // 0xf0021 /* 1.4a */
						of(DP_VOLTAGE_SWING_LEVEL_3_SUPPORTED) // BIT(0)
						of(DP_PRE_EMPHASIS_LEVEL_3_SUPPORTED) // BIT(1)
						ofd(0xfc)
					}

					obf(DP_128B132B_TRAINING_AUX_RD_INTERVAL_PHY_REPEATER(dp_phy), "128B132B_TRAINING_AUX_RD_INTERVAL_PHY_REPEATER%d", i+1) { // 0xf0022 /* 2.0 */
						/* see DP_128B132B_TRAINING_AUX_RD_INTERVAL for values */
						sw(DP_128B132B_TRAINING_AUX_RD_INTERVAL_MASK) { // 0x7f
							ofs(DP_128B132B_TRAINING_AUX_RD_INTERVAL_400_US ,"400µs") // 0x00
							ofs(DP_128B132B_TRAINING_AUX_RD_INTERVAL_4_MS   ,  "4ms") // 0x01
							ofs(DP_128B132B_TRAINING_AUX_RD_INTERVAL_8_MS   ,  "8ms") // 0x02
							ofs(DP_128B132B_TRAINING_AUX_RD_INTERVAL_12_MS  , "12ms") // 0x03
							ofs(DP_128B132B_TRAINING_AUX_RD_INTERVAL_16_MS  , "16ms") // 0x04
							ofs(DP_128B132B_TRAINING_AUX_RD_INTERVAL_32_MS  , "32ms") // 0x05
							ofs(DP_128B132B_TRAINING_AUX_RD_INTERVAL_64_MS  , "64ms") // 0x06
						}
						ofd(0x80)
					}
					
					dumpnotzero(DP_LTTPR_REG(dp_phy, 0xf0023), DP_LTTPR_REG(dp_phy, 0xf0030))

					for (int lane = 0; lane < 4; lane++) {
						obf(DP_LANE0_1_STATUS_PHY_REPEATER(dp_phy) + ((lane & 2) ? 1 : 0), "LANE%d_%d_STATUS_PHY_REPEATER%d", lane & 2, (lane & 2) + 1, i+1) { // 0xf0030, 0xf0031 /* 1.3 */
							if (lane & 1) val >>= 4;
							lb("LANE%d", lane)
							oim(0xf)
						}
					} // for lane

					obf(DP_LTTPR_REG(dp_phy, DP_LANE_ALIGN_STATUS_UPDATED_PHY_REPEATER1), "LANE_ALIGN_STATUS_UPDATED_PHY_REPEATER%d", i+1) { // 0xf0032 /* 1.3 */
						oi();
					}

					for (int lane = 0; lane < 4; lane++) {
						obf(DP_LTTPR_REG(dp_phy, DP_ADJUST_REQUEST_LANE0_1_PHY_REPEATER1) + ((lane & 2) ? 1 : 0), "ADJUST_REQUEST_LANE%d_%d_PHY_REPEATER%d", lane & 2, (lane & 2) + 1, i+1) { // 0xf0033, 0xf0034 /* 1.3 */
							if (lane & 1) val >>= 4;
							lb("LANE%d", lane)
							oim(0xf)
						}
					} // for lane

					for (int lane = 0; lane < 4; lane++) {
						obf16(DP_LTTPR_REG(dp_phy, DP_SYMBOL_ERROR_COUNT_LANE0_PHY_REPEATER1) + lane, "SYMBOL_ERROR_COUNT_LANE%d_PHY_REPEATER%d", lane, i+1) { // 0xf0035, 0xf0037, 0xf0039, 0xf003b /* 1.3 */
							oi();
						}
					}

					dumpnotzero(DP_LTTPR_REG(dp_phy, 0xf003c), DP_LTTPR_REG(dp_phy, 0xf0060))
				} // if !iszero dp_phy
			} // for dp_phy

			for (i = 0; i < DP_MAX_LTTPR_COUNT; i++) { // 0xf0290, 0xf0298, 0xf02a0, 0xf02a8, 0xf02b0, 0xf02b8, 0xf02c0, 0xf02c8 /* 1.4 */
				enum drm_dp_phy dp_phy = (enum drm_dp_phy)DP_PHY_LTTPR(i);
				if (!iszero(DP_LTTPR_BASE(dp_phy), DP_LTTPR_BASE(DP_PHY_LTTPR(i+1)))) {

					obf(DP_FEC_STATUS_PHY_REPEATER(dp_phy), "FEC_STATUS_PHY_REPEATER%d", i+1) { // 0xf0290 /* 1.4 */
						oi();
					}

					obf(DP_FEC_REG(dp_phy, DP_FEC_ERROR_COUNT_PHY_REPEATER1), "FEC_ERROR_COUNT_PHY_REPEATER%d", i+1) { // 0xf0291 /* 1.4 */
						oi();
					}

					obf(DP_FEC_REG(dp_phy, DP_FEC_CAPABILITY_PHY_REPEATER1), "FEC_CAPABILITY_PHY_REPEATER%d", i+1) { // 0xf0294 /* 1.4a */
						oi();
					}

					dumpnotzero(DP_FEC_REG(dp_phy, 0xf0295), DP_FEC_REG(dp_phy, 0xf0298))
				} // if !iszero dp_phy
			} // for dp_phy

			dumpnotzero(0xf02d0, 0xf0300)

			olf; OUTDENT
		} // if (!iszero(0xf0000, 0xf0300))

		if (!iszero(0xf0300, 0x100000)) {
			iprintf("Undefined"); INDENT
			dumpnotzero(0xf0300, 0x100000)
			olf; OUTDENT
		}

	} // if looks like a valid dpcd

	OUTDENT
	iprintf("}; // dpcd\n");

} // parsedpcd
