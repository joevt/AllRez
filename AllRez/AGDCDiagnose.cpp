//
//  AGDCDiagnose.c
//  AllRez
//
//  Created by joevt on 2022-06-04.
//

#include "AGDCDiagnose.h"
#include <sys/socket.h>
#ifdef __cplusplus
#include <cerrno>
#endif
#include "printf.h"
#include "utilities.h"
#include <time.h>
#include <CoreFoundation/CFByteOrder.h>

#define kDisplayPolicyStateSocketName "/var/run/displaypolicyd/state"


void GetDisplayPolicyState(void **buf, size_t *bufSize)
{
	int err;
	char * errstr;
	int result;
	int sock;

	if (buf) *buf = NULL;
	if (bufSize) *bufSize = 0;

	sock = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (sock < 0) {
		err = errno;
		errstr = strerror(err);
		iprintf("socket failed; errno %d: %s\n", err, errstr);
		return;
	}
	
	//struct sockaddr_in address2;
	struct {
		__uint8_t       len;
		sa_family_t     family;
		char            path[68];
	} address;
	address.family = AF_UNIX;
	address.len = sizeof(address);
	strlcpy(address.path, "/var/run/displaypolicyd/state", sizeof(address.path));
	result = connect(sock, (struct sockaddr *)&address, sizeof(address));
	if (result) {
		err = errno;
		errstr = strerror(err);
		iprintf("connect failed; errno %d: %s\n", err, errstr);
		close(sock);
		return;
	}

	void *lbuf = NULL;
	size_t alloc = 0;
	size_t offset = 0;
	size_t bytesread = 1;
	while (bytesread) {
		if (alloc - offset == 0) {
			alloc = alloc ? alloc * 2 : 1000;
			lbuf = realloc(lbuf, alloc);
			if (!lbuf) {
				err = errno;
				errstr = strerror(err);
				iprintf("realloc failed; errno %d: %s\n", err, errstr);
				close(sock);
				return;
			}
		}
		bytesread = read(sock, (uint8_t*)lbuf + offset, alloc - offset);
		if (bytesread < 0) {
			err = errno;
			errstr = strerror(err);
			iprintf("read failed; errno %d: %s\n", err, errstr);
			close(sock);
			return;
		}
		offset = offset + bytesread;
	}
	
	if (buf) *buf = lbuf;
	if (bufSize) *bufSize = offset;
 
	close(sock);
}



CFStringRef GetOneRegPath(reginfo reg) {
	CFMutableDictionaryRef match = IORegistryEntryIDMatching(reg);
	io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, match);
	CFStringRef cfpath = NULL;
	if (service) {
		cfpath = IORegistryEntryCopyPath(service, "IOService");
		IOObjectRelease(service);
	}
	return cfpath;
}



void DoOneReg(uint16_t gpuid, reginfo reg, const char * name, const char * parent) {

	CFStringRef cfpath = GetOneRegPath(reg);
	const char *path = cfpath ? CFStringGetCStringPtr(cfpath, kCFStringEncodingUTF8) : "";
	
	const char * spaces = "";
	if (parent && strstr(path, parent) == path) {
		spaces = "    ";
		path += strlen(parent);
	}

	iprintf("gpu 0x%04x %10s 0x%09llx %s%s\n", gpuid, name, reg, spaces, path);
	
	if (cfpath) CFRelease(cfpath);
}



int entrycompare(const void *entry1, const void *entry2) {
	if (((gtraceentry*)entry1)->timestamp < ((gtraceentry*)entry2)->timestamp)
		return -1;
	else if (((gtraceentry*)entry1)->timestamp > ((gtraceentry*)entry2)->timestamp)
		return 1;
	return 0;
}


const char* GetTagStr(int tag) {
	switch (tag) {
		case  0: return "NULL";
		case  1: return "ARMTIMER";
		case  2: return "AUXACCESS";
		case  3: return "BLACKLISTED";
		case  4: return "CANCELTIMER";
		case  5: return "CHANGEEVENT";
		case  6: return "CHANGEEVENTADD";
		case  7: return "CHANGEEVENTREMOVE";
		case  8: return "DUMPDEVICEMAP";
		case  9: return "DUMP_DISPLAYMAP";
		case 10: return "DUMP_DISPLAYMAP2";
		case 11: return "DUMP_PORTINFO";
		case 12: return "GETHWDPDATA";
		case 13: return "IMPORTBOOTCONFIG";
		case 14: return "IOKIT_NOTIFY";
		case 15: return "MAINTHREAD";
		case 16: return "ML_STATEDONE";
		case 17: return "ML_STATEUPDATE";
		case 18: return "ML_TIMEDOUT";
		case 19: return "MUXEVENT";
		case 20: return "MUXEVENTSWITCH";
		case 21: return "MUXLOCKDOWN_AGC_DONE";
		case 22: return "MUXLOCKDOWN_AGC_INIT";
		case 23: return "MUXLOCKDOWN_RELEASE";
		case 24: return "MUXLOCKDOWN_START";
		case 25: return "NOMEDIAMODE";
		case 26: return "PE_APPROVAL";
		case 27: return "PE_IDLETIMEOUT";
		case 28: return "PE_INIT_DONE";
		case 29: return "PE_INIT_FAILED";
		case 30: return "PE_MUX_REGISTER";
		case 31: return "PE_START";
		case 32: return "PE_WAITIDLE";
		case 33: return "PE_WS_START";
		case 34: return "PE_WS_STARTED";
		case 35: return "PORTTRANSPORT";
		case 36: return "SCHEDULEPORTINFO";
		case 37: return "SETPORTENABLE";
		case 38: return "START";
		case 39: return "TIMERTICK";
		case 40: return "TRYPORTENABLE";
		case 41: return "VDDPOWEREVENT";
		case 42: return "VDDQUEUE";
		case 43: return "TIMEOUT";
		case 44: return "XFER";
		case 45: return "RELOAD";
		case 46: return "ADDRESS";
		case 47: return "FBOFFLINEMASK";
		case 48: return "PORTDATA";
		case 49: return "PORT";
		case 50: return "CONFIG";
		case 51: return "COUNT";
		case 52: return "FLAGS";
		case 53: return "TYPE";
		case 54: return "MODE";
		case 55: return "ML_NODE";
		case 56: return "STATE";
		case 57: return "GPU";
		case 58: return "CURRENT";
		case 59: return "QDATA";
		case 60: return "ENABLE";
		case 61: return "BOOL";
		case 62: return "STATUS";
		case 63: return "ERRORS";
		case 64: return "NOMEDIA";
		case 65: return "TRAINED";
		case 66: return "LINKS";
		case 67: return "ML_STATEVAL";
		case 68: return "POWER";
		case 69: return "PROBE";
		case 70: return "LINK";
		case 71: return "GPUSTATE";
		case 72: return "GPUEVENT";
		case 73: return "GPUID";
		case 74: return "GPUDRVSTART";
		case 75: return "SYSPWR";
		case 76: return "PORTSTATUS";
		case 77: return "GPUDISPSTATE";
		case 78: return "AGDC_PUBLISHED";
		case 79: return "AGDCATTACHSTATE";
		case 80: return "START_DISP_PLCY_MGR";
		case 81: return "FB_SET_ONLINE";
		case 82: return "FB_INJECT";
		case 83: return "FB_INDEX";
		case 84: return "USERACTIVITY";
		case 85: return "PMASSERT_PREVENTSYSSLEEP_CREATE";
		case 86: return "PMASSERT_PREVENTDISPSLEEP_CREATE";
		case 87: return "PMASSERT_RELEASE";
		case 88: return "DISPPWR";
		default: return UNKNOWN_VALUE(tag);
	}
}

static void HandleTag_ioreturn(uint16_t tag, uint64_t data, const char *name) {
	cprintf(":%s", name);
	char resultStr[40];
	const char *str = GetIOReturnStr(resultStr, sizeof(resultStr), data);
	if (str && *str && *str != '?') {
		cprintf("=%s", str);
	}
	else {
		cprintf("=%lld", data & 0x0ffff);
		if (data & 0xffffffffffff0000)
			cprintf(",?0x%llx", data & 0xffffffffffff0000);
	}
}

static void HandleTag_empty(uint16_t tag, uint64_t data) {
	if (data) cprintf(":empty:?%lld", data);
}

static void HandleTag_minus1(uint16_t tag, uint64_t data) { // 10000cac4
	cprintf(":-1=%lld", data);
}

static void HandleTag_0(uint16_t tag, uint64_t data) { // 10000c806
	cprintf(":0=%lld", data & 0x0ffff);
	if (data & 0xffffffffffff0000) cprintf(",?0x%llx", data & 0xffffffffffff0000);
}

static void HandleTag_1(uint16_t tag, uint64_t data) { // 10000cd92
	cprintf(":1=%lld", data & 0x0ffff);
	if (data & 0xffffffffffff0000) cprintf(",?0x%llx", data & 0xffffffffffff0000);
}

static void HandleTag_addr_bytes(uint16_t tag, uint64_t data) { // 10000c0c0
	cprintf(":addr=%lld,bytes=%lld", data & 0xffffff, (data >> 0x20) & 0xff);
	if (data & 0xffffff00ff000000) cprintf(",?0x%llx", data & 0xffffff00ff000000);
}

static void HandleTag_agdpRegId(uint16_t tag, uint64_t data) { // 10000c7e2
	cprintf(":agdpRegId=0x%llx", data);
}

static void HandleTag_assertId(uint16_t tag, uint64_t data) { // 10000c89e
	cprintf(":assertId=%lld", data);
}

static void HandleTag_attachCtx_registryID(uint16_t tag, uint64_t data) { // 10000c7a5
	cprintf(":attachCtx.registryID=0x%llx", data);
}

static void HandleTag_attachInfo_registryID(uint16_t tag, uint64_t data) { // 10000c2bf
	cprintf(":attachInfo->registryID=%lld", data);
}

static void HandleTag_attempts_tryHarderForHDMI_state(uint16_t tag, uint64_t data) { // 10000c6ee
	cprintf(":attempts=%lld,tryHarderForHDMI=%lld,state=%lld", data >> 0x18, (data >> 0x10) & 0xff, data & 0xff);
	if (data & 0x00000ff00) cprintf(",?0x%llx", data & 0x00000ff00);
}

static void HandleTag_count(uint16_t tag, uint64_t data) { // 10000c594, 10000caa6
	cprintf(":count=%lld", data);
}

static void HandleTag_doAPI_pport_enabled_pport_linkTrained(uint16_t tag, uint64_t data) { // 10000ca07
	cprintf(":doAPI=%lld,pport->enabled=%lld,pport->linkTrained=%lld", (data >> 0x20) & 0xff, (data >> 0x10) & 0xff, data & 0xff);
	if (data & 0xffffff00ffff0000) cprintf(",?0x%llx", data & 0xffffff00ffff0000);
}

static void HandleTag_element_data_address_port_element_data_address_stream(uint16_t tag, uint64_t data) { // 10000cdef
	cprintf(":element->data.address.port=%lld,element->data.address.stream=%lld", (data >> 0x20) & 0xff, data & 0xff);
	if (data & 0xffffff00ffffff00) cprintf(",?0x%llx", data & 0xffffff00ffffff00);
}

static void HandleTag_element_errorCountAll_errorCount_kDeviceHistory_OnlineErrorLinkTrain_errorCount_kDeviceHistory_OnlineErrorBandWidth_errorCount_kDeviceHistory_OnlineErrorMultiLink_errorCount_kDeviceHistory_OnlineErrorSimple(uint16_t tag, uint64_t data) { // 10000c3f1
	cprintf(":element->errorCountAll=%lld", data >> 0x38);
	cprintf(",element->errorCount[kDeviceHistory_OnlineErrorLinkTrain]=%lld", data & 0xff);
	cprintf(",element->errorCount[kDeviceHistory_OnlineErrorBandWidth]=%lld", (data >> 8) & 0xff);
	cprintf(",element->errorCount[kDeviceHistory_OnlineErrorMultiLink]=%lld", (data >> 0x10) & 0xff);
	cprintf(",element->errorCount[kDeviceHistory_OnlineErrorSimple]=%lld", (data >> 0x18) & 0xff);
	if (data & 0x00ffffff00000000) cprintf(",?0x%llx", data & 0x00ffffff00000000);
}

static void HandleTag_enable(uint16_t tag, uint64_t data) { // 10000c395
	cprintf(":enable=%lld", data);
}

static void HandleTag_enter(uint16_t tag, uint64_t data) { // 10000bef4
	cprintf(":enter=%lld", data);
}

static void HandleTag_err(uint16_t tag, uint64_t data) { // 10000cb5f
	cprintf(":err=%lld", data & 0x0ffff);
	if (data & 0xffffffffffff0000) cprintf(",?0x%llx", data & 0xffffffffffff0000);
}

static void HandleTag_error(uint16_t tag, uint64_t data) { // 10000bd30
	cprintf(":error=%lld", data);
}

static void HandleTag_event_message_eMsg_kAGDPMessageSystemDidPowerOn(uint16_t tag, uint64_t data) { // 10000c96d
	cprintf(":event->message.eMsg==kAGDPMessageSystemDidPowerOn=%lld", data);
}

static void HandleTag_event_message_eMsg_kAGDPMessageSystemWillPowerOff(uint16_t tag, uint64_t data) { // 10000c94f
	cprintf(":event->message.eMsg==kAGDPMessageSystemWillPowerOff=%lld", data);
}

static void HandleTag_event_mux_message(uint16_t tag, uint64_t data) { // 10000bdc9
	cprintf(":event->mux.message=%lld", data);
}

static void HandleTag_event_mux_status_active(uint16_t tag, uint64_t data) { // 10000cbd8
	cprintf(":event->mux.status.active=%lld", data);
}

static void HandleTag_event_mux_status_powered(uint16_t tag, uint64_t data) { // 10000c5d0
	cprintf(":event->mux.status.powered=%lld", data);
}

static void HandleTag_event_queueSize(uint16_t tag, uint64_t data) { // 10000c066, 10000c084
	cprintf(":event->queueSize=%lld", data);
}

static void HandleTag_event_type(uint16_t tag, uint64_t data) { // 10000c931
	cprintf(":event->type=%lld", data);
}

static void HandleTag_event_dot_type(uint16_t tag, uint64_t data) { // 10000c674
	cprintf(":event.type=%lld", data);
}

static void HandleTag_fb(uint16_t tag, uint64_t data) { // 10000cdd0
	cprintf(":fb=%lld", data & 0x0ffff);
	if (data & 0xffffffffffff0000) cprintf(",?0x%llx", data & 0xffffffffffff0000);
}

static void HandleTag_fbid(uint16_t tag, uint64_t data) { // 10000c825
	cprintf(":fbid=%lld", data & 0x0ffff);
	if (data & 0xffffffffffff0000) cprintf(",?0x%llx", data & 0xffffffffffff0000);
}

static void HandleTag_fbOfflineMask(uint16_t tag, uint64_t data) { // 10000c18e
	cprintf(":fbOfflineMask=0x%llx", data);
}

static void HandleTag_fbOnlineMask(uint16_t tag, uint64_t data) { // 10000bc53
	cprintf(":fbOnlineMask=0x%llx", data);
}

static void HandleTag_fbslot_id(uint16_t tag, uint64_t data) { // 10000cdb1
	cprintf(":fbslot->id=%lld", data & 0x0ffff);
	if (data & 0xffffffffffff0000) cprintf(",?0x%llx", data & 0xffffffffffff0000);
}

static void HandleTag_fDisplaySleeping(uint16_t tag, uint64_t data) { // 10000c8bc
	cprintf(":fDisplaySleeping=%lld", data);
}

static void HandleTag_fGPUActive(uint16_t tag, uint64_t data) { // 10000c2fb
	cprintf(":fGPUActive=%lld", data);
}

static void HandleTag_flags(uint16_t tag, uint64_t data) { // 10000c6b1
	cprintf(":flags=0x%llx", data);
}

static void HandleTag_fLaunchCount(uint16_t tag, uint64_t data) { // 10000bd85
	cprintf(":fLaunchCount=%lld", data);
}

static void HandleTag_fMultiLinkDisplayCfgIndex(uint16_t tag, uint64_t data) { // 10000c228
	cprintf(":fMultiLinkDisplayCfgIndex=%lld", data);
}

static void HandleTag_fNoMediaMode(uint16_t tag, uint64_t data) { // 10000c2a1, 10000c319, 10000c4b0
	cprintf(":fNoMediaMode=%lld", data);
}

static void HandleTag_fStaticPortWorkAround(uint16_t tag, uint64_t data) { // 10000bd4e
	cprintf(":fStaticPortWorkAround=%lld", data);
}

static void HandleTag_gpu_dispPolicyState_nextState(uint16_t tag, uint64_t data) { // 10000c75b
	cprintf(":gpu->dispPolicyState=%lld,nextState=%lld", data & 0xffff, data >> 0x10);
}

static void HandleTag_gpu_kernGPU_gpuId(uint16_t tag, uint64_t data) { // 10000c692
	cprintf(":gpu->kernGPU.gpuId=0x%llx", data & 0x0ffff);
	if (data & 0xffffffffffff0000) cprintf(",?0x%llx", data & 0xffffffffffff0000);
}

static void HandleTag_gpu_pmAssertionId(uint16_t tag, uint64_t data) { // 10000c862
	cprintf(":gpu->pmAssertionId=%lld", data);
}

static void HandleTag_gpu_pmInitAssertionId(uint16_t tag, uint64_t data) { // 10000c880
	cprintf(":gpu->pmInitAssertionId=%lld", data);
}

static void HandleTag_gpu_state_nextState(uint16_t tag, uint64_t data) { // 10000c62a
	cprintf(":gpu->state=%lld,nextState=%lld", data & 0xffff, data >> 0x10);
}

static void HandleTag_gpuId(uint16_t tag, uint64_t data) { // 10000cb40
	cprintf(":gpuId=0x%llx", data & 0x0ffff);
	if (data & 0xffffffffffff0000) cprintf(",?0x%llx", data & 0xffffffffffff0000);
}

static void HandleTag_gState_active(uint16_t tag, uint64_t data) { // 10000cb9c
	cprintf(":gState.active=%lld", data);
}

static void HandleTag_gState_powered(uint16_t tag, uint64_t data) { // 10000cbba
	cprintf(":gState.powered=%lld", data);
}

static void HandleTag_i(uint16_t tag, uint64_t data) { // 10000bd12, 10000c02a, 10000cc14
	cprintf(":i=%lld", data);
}

static void HandleTag_idx_element_isWhiteListDisplay_element_isMultiStreamDisplay(uint16_t tag, uint64_t data) { // 10000bbe9
	cprintf(":idx=%lld,element->isWhiteListDisplay=%lld,element->isMultiStreamDisplay=%lld", data >> 0x38, (data >> 8) & 0xff, data & 0xff);
	if (data & 0x00ffffffffff0000) cprintf(",?0x%llx", data & 0x00ffffffffff0000);
}

static void HandleTag_info_bitRate_laneCount_lane_0_1_status_lane_2_3_status(uint16_t tag, uint64_t data) { // 10000ccd2
	cprintf(":info->bitRate=%lld,info->laneCount=%lld,info->lane_0_1_status=%lld,info->lane_2_3_status=%lld", (data >> 0x30) & 0xff, (data >> 0x20) & 0xff, (data >> 8) & 0xff, data & 0xff);
	if (data & 0xff00ff00ffff0000) cprintf(",?0x%llx", data & 0xff00ff00ffff0000);
}

static void HandleTag_kernResult16(uint16_t tag, uint64_t data) { // 10000c3d2
	cprintf(":kernResult=%lld", data & 0x0ffff);
	if (data & 0xffffffffffff0000) cprintf(",?0x%llx", data & 0xffffffffffff0000);
}

static void HandleTag_kernResult(uint16_t tag, uint64_t data) { // 10000cb7e
	cprintf(":kernResult=%lld", data);
}

static void HandleTag_link_address_port_link_address_stream(uint16_t tag, uint64_t data) { // 10000cc32
	cprintf(":link->address.port=%lld,link->address.stream=%lld", (data >> 0x20) & 0xff, data & 0xff);
	if (data & 0xffffff00ffffff00) cprintf(",?0x%llx", data & 0xffffff00ffffff00);
}

static void HandleTag_link_flags(uint16_t tag, uint64_t data) { // 10000c264
	cprintf(":link->flags=0x%llx", data);
}

static void HandleTag_lockdownActive_fReason(uint16_t tag, uint64_t data) { // 10000be0d, 10000be5a, 10000bea7
	cprintf(":lockdownActive=%lld,fReason=%lld", data & 1, (data >> 8) & 0xff);
	if (data & 0xffffffffffff00fe) cprintf(",?0x%llx", data & 0xffffffffffff00fe);
}

static void HandleTag_map_id_state_linkedState_NumberOfMembers(uint16_t tag, uint64_t data) { // 10000bc71
	cprintf(":map->id=%lld,map->state=%lld,map->linkedState=%lld,map->NumberOfMembers=%lld", data >> 0x38, (data >> 0x30) & 0xff, (data >> 0x20) & 0xff, data & 0xff);
	if (data & 0x0000ff00ffffff00) cprintf(",?0x%llx", data & 0x0000ff00ffffff00);
}

static void HandleTag_mfg_id_product_id(uint16_t tag, uint64_t data) { // 10000bb13
	cprintf(":mfg_id=%lld,product_id=%lld", (data >> 0x20) & 0xffff, data & 0xffff);
	if (data & 0xffff0000ffff0000) cprintf(",?0x%llx", data & 0xffff0000ffff0000);
}

static void HandleTag_mostSig(uint16_t tag, uint64_t data) { // 10000c98b
	cprintf(":mostSig=%lld", data);
}

static void HandleTag_move2errorq_move2idleq(uint16_t tag, uint64_t data) { // 10000c337
	cprintf(":move2errorq=%lld,move2idleq=%lld", (data >> 0x20) & 0xff, data & 0xff);
	if (data & 0xffffff00ffffff00) cprintf(",?0x%llx", data & 0xffffff00ffffff00);
}

static void HandleTag_ms(uint16_t tag, uint64_t data) { // 10000c0a2
	cprintf(":ms=%lld", data);
}

static void HandleTag_msg(uint16_t tag, uint64_t data) { // 10000cbf6
	cprintf(":msg=%lld", data);
}

static void HandleTag_msgType(uint16_t tag, uint64_t data) { // 10000c6cf
	HandleTag_ioreturn(tag, data, "msgType");
}

static void HandleTag_n(uint16_t tag, uint64_t data) { // 10000ba97, 10000bb71
	cprintf(":n=%lld", data);
}

static void HandleTag_nothing(uint16_t tag, uint64_t data) { // 10000bd7f
	HandleTag_empty(tag, data);
}

static void HandleTag_ok(uint16_t tag, uint64_t data) { // 10000c3b3
	cprintf(":ok=%lld", data & 0xff);
	if (data & 0xffffffffffffff00) cprintf(",?0x%llx", data & 0xffffffffffffff00);
}

static void HandleTag_online(uint16_t tag, uint64_t data) { // 10000cd73
	cprintf(":online=%lld", data & 0x0ffff);
	if (data & 0xffffffffffff0000) cprintf(",?0x%llx", data & 0xffffffffffff0000);
}

static void HandleTag_params_stateNumber(uint16_t tag, uint64_t data) { // 10000c2dd
	cprintf(":params->stateNumber=%lld", data);
}

static void HandleTag_pinfo_linkTrained(uint16_t tag, uint64_t data) { // 10000c60c
	cprintf(":pinfo->linkTrained=%lld", data);
}

static void HandleTag_pinfo_port(uint16_t tag, uint64_t data) { // 10000bfdb
	cprintf(":pinfo->port=%lld", data);
}

static void HandleTag_port(uint16_t tag, uint64_t data) { // 10000bfbd, 10000bff9, 10000c048, 10000c20a
	cprintf(":port=%lld", data);
}

static void HandleTag_portbit32(uint16_t tag, uint64_t data) { // 10000ccaf
	cprintf(":port=%lld", (data >> 0x20) & 0xff);
	if (data & 0xffffff00ffffffff) cprintf(",?0x%llx", data & 0xffffff00ffffffff);
}

static void HandleTag_port_stream(uint16_t tag, uint64_t data) { // 10000bab5, 10000c130
	cprintf(":port=%lld,stream=%lld", (data >> 0x20) & 0xff, data & 0xff);
	if (data & 0xffffff00ffffff00) cprintf(",?0x%llx", data & 0xffffff00ffffff00);
}

static void HandleTag_porttype_transport(uint16_t tag, uint64_t data) { // 10000cae2
	cprintf(":porttype=%lld,transport=%lld", (data >> 0x20) & 0xff, data & 0xff);
	if (data & 0xffffff00ffffff00) cprintf(",?0x%llx", data & 0xffffff00ffffff00);
}

static void HandleTag_pport_enabled_pport_linkTrained(uint16_t tag, uint64_t data) { // 10000c9a9
	cprintf(":pport->enabled=%lld,pport->linkTrained=%lld", (data >> 0x20) & 0xff, data & 0xff);
	if (data & 0xffffff00ffffff00) cprintf(",?0x%llx", data & 0xffffff00ffffff00);
}

static void HandleTag_pport_fNodeCount(uint16_t tag, uint64_t data) { // 10000bb8f, 10000bbcb
	cprintf(":pport->fNodeCount=%lld", data);
}

static void HandleTag_pport_port_enabled(uint16_t tag, uint64_t data) { // 10000c1ac
	cprintf(":pport->port=%lld,pport->enabled=%lld", (data >> 0x20) & 0xff, data & 0xff);
	if (data & 0xffffff00ffffff00) cprintf(",?0x%llx", data & 0xffffff00ffffff00);
}

static void HandleTag_pport_porttype_pport_transport_pport_linkTrained_pport_linkConfig_bitRate_pport_linkConfig_laneCount(uint16_t tag, uint64_t data) { // 10000c4ce
	cprintf(":pport->porttype=%lld,pport->transport=%lld,pport->linkTrained=%lld,pport->linkConfig.bitRate=%lld,pport->linkConfig.laneCount=%lld", data >> 0x38, (data >> 0x28) & 0xff, (data >> 0x30) & 0xff, data & 0xff, (data >> 8) & 0xff);
	if (data & 0x000000ffffff0000) cprintf(",?0x%llx", data & 0x000000ffffff0000);
}

static void HandleTag_pportscratch_fNodeCount(uint16_t tag, uint64_t data) { // 10000bbad
	cprintf(":pportscratch->fNodeCount=%lld", data);
}

static void HandleTag_prcfg_count(uint16_t tag, uint64_t data) { // 10000c246
	cprintf(":prcfg->count=%lld", data);
}

static void HandleTag_probed(uint16_t tag, uint64_t data) { // 10000c5ee
	cprintf(":probed=%lld", data);
}

static void HandleTag_r(uint16_t tag, uint64_t data) { // 10000c5b2
	cprintf(":r=%lld", data);
}

static void HandleTag_reload(uint16_t tag, uint64_t data) { // 10000c112
	cprintf(":reload=%lld", data);
}

static void HandleTag_state(uint16_t tag, uint64_t data) { // 10000c7c3
	cprintf(":state=%lld", data & 0x0ffff);
	if (data & 0xffffffffffff0000) cprintf(",?0x%llx", data & 0xffffffffffff0000);
}

static void HandleTag_status(uint16_t tag, uint64_t data) { // 10000ca88
	cprintf(":status=%lld", data);
}

static void HandleTag_status_auxp_aux_status(uint16_t tag, uint64_t data) { // 10000ce6b
	cprintf(":status=%lld,auxp->aux.status=%lld", data & 0xff, (data >> 0x20) & 0xff);
	if (data & 0xffffff00ffffff00) cprintf(",?0x%llx", data & 0xffffff00ffffff00);
}

static void HandleTag_status16(uint16_t tag, uint64_t data) { // 10000cc90
	cprintf(":status=%lld", data & 0x0ffff);
	if (data & 0xffffffffffff0000) cprintf(",?0x%llx", data & 0xffffffffffff0000);
}

static void HandleTag_type(uint16_t tag, uint64_t data) { // 10000c282
	HandleTag_ioreturn(tag, data, "type");
}

static void HandleTag_userActive(uint16_t tag, uint64_t data) { // 10000c844
	cprintf(":userActive=%lld", data);
}

static void HandleTag_value(uint16_t tag, uint64_t data) { // 10000ce4d
	cprintf(":value=%lld", data);
}

static void HandleTag_Default(uint16_t tag, uint64_t data) {
	cprintf("=%llx", data);
}

void DoOneTag(uint16_t tag, uint64_t data, void (* handleTag)(uint16_t tag, uint64_t data)) {
	cprintf(" %s", GetTagStr(tag));
	handleTag(tag, data);
}



typedef struct {
	int line;
	const char* file;
	uint16_t tag0;
	uint16_t tag1;
	uint16_t tag2;
	void (* handleTag0)(uint16_t tag, uint64_t data);
	void (* handleTag1)(uint16_t tag, uint64_t data);
	void (* handleTag2)(uint16_t tag, uint64_t data);
} knowngtraceentry;

knowngtraceentry knowngtrace[] = {
	{
		155, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/queue.cpp",
		42, 53, 0,
		HandleTag_event_queueSize, // _10000c084
		HandleTag_event_type, // _10000c931
		NULL,
	},
	{
		190, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/queue.cpp",
		41, 61, 61,
		HandleTag_event_queueSize, // _10000c066
		HandleTag_event_message_eMsg_kAGDPMessageSystemWillPowerOff, // _10000c94f
		HandleTag_event_message_eMsg_kAGDPMessageSystemDidPowerOn, // _10000c96d
	},
	{
		662, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/GPUWrangler/launchwrangler.cpp",
		84, 84, 0,
		HandleTag_userActive, // _10000c844
		HandleTag_mostSig, // _10000c98b
		NULL,
	},
	{
		500, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/portMgr.cpp",
		40, 60, 49,
		HandleTag_port, // _10000c048
		HandleTag_enable, // _10000c395
		HandleTag_pport_enabled_pport_linkTrained, // _10000c9a9
	},
	{
		504, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/portMgr.cpp",
		40, 60, 49,
		HandleTag_port, // _10000c048
		HandleTag_enable, // _10000c395
		HandleTag_doAPI_pport_enabled_pport_linkTrained, // _10000ca07
	},
	{
		46, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/timer.cpp",
		39, 0, 0,
		HandleTag_i, // _10000c02a
		NULL,
		NULL,
	},
	{
		646, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/GPUWrangler/launchwrangler.cpp",
		75, 62, 0,
		HandleTag_msgType, // _10000c6cf
		HandleTag_status, // _10000ca88
		NULL,
	},
	{
		146, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/mainLauncher/mainmisc.cpp",
		80, 0, 0,
		HandleTag_agdpRegId, // _10000c7e2
		NULL,
		NULL,
	},
	{
		140, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/mainLauncher/mainmisc.cpp",
		80, 0, 0,
		HandleTag_agdpRegId, // _10000c7e2
		NULL,
		NULL,
	},
	{
		151, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/mainLauncher/mainmisc.cpp",
		80, 61, 0,
		HandleTag_agdpRegId, // _10000c7e2
		HandleTag_ok, // _10000c3b3
		NULL,
	},
	{
		392, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/mainLauncher/mainmisc.cpp",
		38, 0, 0,
		HandleTag_empty, // _10000c017
		NULL,
		NULL,
	},
	{
		481, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/portMgr.cpp",
		37, 60, 49,
		HandleTag_port, // _10000bff9
		HandleTag_enable, // _10000c395
		HandleTag_pport_enabled_pport_linkTrained, // _10000c9a9
	},
	{
		613, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/portMgr.cpp",
		36, 59, 70,
		HandleTag_pinfo_port, // _10000bfdb
		HandleTag_move2errorq_move2idleq, // _10000c337
		HandleTag_pinfo_linkTrained, // _10000c60c
	},
	{
		644, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/portMgr.cpp",
		36, 51, 0,
		HandleTag_pinfo_port, // _10000bfdb
		HandleTag_count, // _10000caa6
		NULL,
	},
	{
		1971, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		35, 50, 0,
		HandleTag_port, // _10000bfbd
		HandleTag_minus1, // _10000cac4
		NULL,
	},
	{
		1988, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		35, 50, 69,
		HandleTag_port, // _10000bfbd
		HandleTag_porttype_transport, // _10000cae2
		HandleTag_probed, // _10000c5ee
	},
	{
		5270, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		76, 46, 0,
		HandleTag_attempts_tryHarderForHDMI_state, // _10000c6ee
		HandleTag_port_stream, // _10000c130
		NULL,
	},
	{
		238, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/GPUWrangler/launchwrangler.cpp",
		87, 73, 62,
		HandleTag_assertId, // _10000c89e
		HandleTag_gpuId, // _10000cb40
		HandleTag_err, // _10000cb5f
	},
	{
		175, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/GPUWrangler/launchwrangler.cpp",
		85, 73, 62,
		HandleTag_gpu_pmAssertionId, // _10000c862
		HandleTag_gpu_kernGPU_gpuId, // _10000c692
		HandleTag_err, // _10000cb5f
	},
	{
		214, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/GPUWrangler/launchwrangler.cpp",
		86, 73, 62,
		HandleTag_gpu_pmInitAssertionId, // _10000c880
		HandleTag_gpu_kernGPU_gpuId, // _10000c692
		HandleTag_err, // _10000cb5f
	},
	{
		1004, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		34, 0, 0,
		HandleTag_empty, // _10000bfaa
		NULL,
		NULL,
	},
	{
		998, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		33, 0, 0,
		HandleTag_empty, // _10000bf97
		NULL,
		NULL,
	},
	{
		1325, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		32, 0, 0,
		HandleTag_empty, // _10000bf84
		NULL,
		NULL,
	},
	{
		763, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		31, 0, 0,
		HandleTag_empty, // _10000bf71
		NULL,
		NULL,
	},
	{
		1196, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		30, 0, 0,
		HandleTag_empty, // _10000bf5e
		NULL,
		NULL,
	},
	{
		1357, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		29, 0, 0,
		HandleTag_empty, // _10000bf4b
		NULL,
		NULL,
	},
	{
		1338, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		28, 0, 0,
		HandleTag_empty, // _10000bf38
		NULL,
		NULL,
	},
	{
		1336, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		27, 0, 0,
		HandleTag_empty, // _10000bf25
		NULL,
		NULL,
	},
	{
		989, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		26, 0, 0,
		HandleTag_empty, // _10000bf12
		NULL,
		NULL,
	},
	{
		1655, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		25, 58, 0,
		HandleTag_enter, // _10000bef4
		HandleTag_fNoMediaMode, // _10000c319
		NULL,
	},
	{
		173, "src/AGDCsupport/libraries/agdcObjects/muxlockdown.cpp",
		24, 62, 0,
		HandleTag_lockdownActive_fReason, // _10000bea7
		HandleTag_kernResult, // _10000cb7e
		NULL,
	},
	{
		194, "src/AGDCsupport/libraries/agdcObjects/muxlockdown.cpp",
		23, 62, 0,
		HandleTag_lockdownActive_fReason, // _10000be5a
		HandleTag_kernResult, // _10000cb7e
		NULL,
	},
	{
		143, "src/AGDCsupport/libraries/agdcObjects/muxlockdown.cpp",
		22, 0, 0,
		HandleTag_lockdownActive_fReason, // _10000be0d
		NULL,
		NULL,
	},
	{
		154, "src/AGDCsupport/libraries/agdcObjects/muxlockdown.cpp",
		21, 0, 0,
		HandleTag_empty, // _10000bdfa
		NULL,
		NULL,
	},
	{
		225, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/queue.cpp",
		20, 0, 0,
		HandleTag_empty, // _10000bde7
		NULL,
		NULL,
	},
	{
		250, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/queue.cpp",
		20, 57, 0,
		HandleTag_empty, // _10000bde7
		HandleTag_fGPUActive, // _10000c2fb
		NULL,
	},
	{
		212, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/queue.cpp",
		19, 56, 68,
		HandleTag_event_mux_message, // _10000bdc9
		HandleTag_gState_active, // _10000cb9c
		HandleTag_gState_powered, // _10000cbba
	},
	{
		239, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/queue.cpp",
		19, 56, 68,
		HandleTag_event_mux_message, // _10000bdc9
		HandleTag_event_mux_status_active, // _10000cbd8
		HandleTag_event_mux_status_powered, // _10000c5d0
	},
	{
		191, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/mainLauncher/mainmisc.cpp",
		18, 0, 0,
		HandleTag_empty, // _10000bdb6
		NULL,
		NULL,
	},
	{
		270, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/mainLauncher/mainmisc.cpp",
		17, 55, 0,
		HandleTag_empty, // _10000bda3
		HandleTag_attachInfo_registryID, // _10000c2bf
		NULL,
	},
	{
		282, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/mainLauncher/mainmisc.cpp",
		17, 55, 67,
		HandleTag_empty, // _10000bda3
		HandleTag_attachInfo_registryID, // _10000c2bf
		HandleTag_r, // _10000c5b2
	},
	{
		328, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/mainLauncher/mainmisc.cpp",
		16, 0, 0,
		HandleTag_fLaunchCount, // _10000bd85
		NULL,
		NULL,
	},
	{
		337, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		15, 54, 0,
		HandleTag_nothing, // _10000bd7f
		HandleTag_fNoMediaMode, // _10000c2a1
		NULL,
	},
	{
		1623, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		14, 53, 0,
		HandleTag_empty, // _10000bd6c
		HandleTag_msg, // _10000cbf6
		NULL,
	},
	{
		6617, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		13, 0, 0,
		HandleTag_i, // _10000cc14
		NULL,
		NULL,
	},
	{
		6332, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		13, 52, 46,
		HandleTag_i, // _10000cc14
		HandleTag_link_flags, // _10000c264
		HandleTag_link_address_port_link_address_stream, // _10000cc32
	},
	{
		6507, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		13, 51, 0,
		HandleTag_i, // _10000cc14
		HandleTag_prcfg_count, // _10000c246
		NULL,
	},
	{
		6610, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		13, 50, 66,
		HandleTag_i, // _10000cc14
		HandleTag_fMultiLinkDisplayCfgIndex, // _10000c228
		HandleTag_count, // _10000c594
	},
	{
		6500, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		13, 46, 0,
		HandleTag_i, // _10000cc14
		HandleTag_port_stream, // _10000c130
		NULL,
	},
	{
		6436, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		13, 49, 0,
		HandleTag_fStaticPortWorkAround, // _10000bd4e
		HandleTag_port, // _10000c20a
		NULL,
	},
	{
		70, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/GPUWrangler/launchwrangler.cpp",
		71, 73, 0,
		HandleTag_gpu_state_nextState, // _10000c62a
		HandleTag_gpu_kernGPU_gpuId, // _10000c692
		NULL,
	},
	{
		521, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/GPUWrangler/launchwrangler.cpp",
		72, 73, 0,
		HandleTag_event_dot_type, // _10000c674
		HandleTag_gpuId, // _10000cb40
		NULL,
	},
	{
		356, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/GPUWrangler/launchwrangler.cpp",
		74, 73, 62,
		HandleTag_flags, // _10000c6b1
		HandleTag_gpu_kernGPU_gpuId, // _10000c692
		HandleTag_status16, // _10000cc90
	},
	{
		81, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/GPUWrangler/launchwrangler.cpp",
		77, 73, 0,
		HandleTag_gpu_dispPolicyState_nextState, // _10000c75b
		HandleTag_gpu_kernGPU_gpuId, // _10000c692
		NULL,
	},
	{
		2259, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		12, 46, 56,
		HandleTag_error, // _10000bd30
		HandleTag_portbit32, // _10000ccaf
		HandleTag_info_bitRate_laneCount_lane_0_1_status_lane_2_3_status, // _10000ccd2
	},
	{
		3656, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		81, 83, 0,
		HandleTag_online, // _10000cd73
		HandleTag_fbid, // _10000c825
		NULL,
	},
	{
		358, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/config.cpp",
		81, 83, 0,
		HandleTag_1, // _10000cd92
		HandleTag_fbslot_id, // _10000cdb1
		NULL,
	},
	{
		188, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/singleLink.cpp",
		81, 83, 0,
		HandleTag_empty, // _10000c800
		HandleTag_fbslot_id, // _10000cdb1
		NULL,
	},
	{
		4022, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		81, 83, 0,
		HandleTag_empty, // _10000c800
		HandleTag_fbslot_id, // _10000cdb1
		NULL,
	},
	{
		1116, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/multiLink.cpp",
		81, 83, 0,
		HandleTag_empty, // _10000c800
		HandleTag_fb, // _10000cdd0
		NULL,
	},
	{
		3680, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		82, 83, 0,
		HandleTag_0, // _10000c806
		HandleTag_fbid, // _10000c825
		NULL,
	},
	{
		429, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/dataDumper.cpp",
		11, 48, 65,
		HandleTag_i, // _10000bd12
		HandleTag_pport_port_enabled, // _10000c1ac
		HandleTag_pport_porttype_pport_transport_pport_linkTrained_pport_linkConfig_bitRate_pport_linkConfig_laneCount, // _10000c4ce
	},
	{
		410, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/dataDumper.cpp",
		10, 0, 0,
		HandleTag_map_id_state_linkedState_NumberOfMembers, // _10000bc71
		NULL,
		NULL,
	},
	{
		401, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/dataDumper.cpp",
		9, 47, 64,
		HandleTag_fbOnlineMask, // _10000bc53
		HandleTag_fbOfflineMask, // _10000c18e
		HandleTag_fNoMediaMode, // _10000c4b0
	},
	{
		307, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/dataDumper.cpp",
		8, 46, 63,
		HandleTag_idx_element_isWhiteListDisplay_element_isMultiStreamDisplay, // _10000bbe9
		HandleTag_element_data_address_port_element_data_address_stream, // _10000cdef
		HandleTag_element_errorCountAll_errorCount_kDeviceHistory_OnlineErrorLinkTrain_errorCount_kDeviceHistory_OnlineErrorBandWidth_errorCount_kDeviceHistory_OnlineErrorMultiLink_errorCount_kDeviceHistory_OnlineErrorSimple, // _10000c3f1
	},
	{
		743, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/GPUWrangler/launchwrangler.cpp",
		88, 56, 0,
		HandleTag_fDisplaySleeping, // _10000c8bc
		HandleTag_value, // _10000ce4d
		NULL,
	},
	{
		685, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/GPUWrangler/launchwrangler.cpp",
		88, 56, 53,
		HandleTag_fDisplaySleeping, // _10000c8bc
		HandleTag_params_stateNumber, // _10000c2dd
		HandleTag_type, // _10000c282
	},
	{
		5514, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		7, 46, 0,
		HandleTag_pport_fNodeCount, // _10000bbcb
		HandleTag_port_stream, // _10000c130
		NULL,
	},
	{
		5507, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		6, 46, 0,
		HandleTag_pportscratch_fNodeCount, // _10000bbad
		HandleTag_port_stream, // _10000c130
		NULL,
	},
	{
		5428, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		5, 46, 0,
		HandleTag_pport_fNodeCount, // _10000bb8f
		HandleTag_port_stream, // _10000c130
		NULL,
	},
	{
		247, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/timer.cpp",
		4, 45, 0,
		HandleTag_n, // _10000bb71
		HandleTag_reload, // _10000c112
		NULL,
	},
	{
		230, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/whitelistData.cpp",
		3, 0, 0,
		HandleTag_mfg_id_product_id, // _10000bb13
		NULL,
		NULL,
	},
	{
		2028, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/policyEngine.cpp",
		2, 44, 62,
		HandleTag_port_stream, // _10000bab5
		HandleTag_addr_bytes, // _10000c0c0
		HandleTag_status_auxp_aux_status, // _10000ce6b
	},
	{
		228, "src/AppleGraphicsDevicePolicy/displaypolicyd/src/PolicyEngine/timer.cpp",
		1, 43, 0,
		HandleTag_n, // _10000ba97
		HandleTag_ms, // _10000c0a2
		NULL,
	},
	{
		303, "src/AGDCsupport/libraries/agdcObjects/agdcLaunchConfig.cpp",
		78, 62, 0,
		HandleTag_attachCtx_registryID, // _10000c7a5
		HandleTag_kernResult16, // _10000c3d2
		NULL,
	},
	{
		293, "src/AGDCsupport/libraries/agdcObjects/agdcLaunchConfig.cpp",
		78, 62, 0,
		HandleTag_attachCtx_registryID, // _10000c7a5
		HandleTag_kernResult, // _10000c3d2
		NULL,
	},
	{
		268, "src/AGDCsupport/libraries/agdcObjects/agdcLaunchConfig.cpp",
		78, 0, 0,
		HandleTag_attachCtx_registryID, // _10000c7a5
		NULL,
		NULL,
	},
	{
		316, "src/AGDCsupport/libraries/agdcObjects/agdcLaunchConfig.cpp",
		78, 79, 0,
		HandleTag_attachCtx_registryID, // _10000c7a5
		HandleTag_state, // _10000c7c3
		NULL,
	},
	{
		273, "src/AGDCsupport/libraries/agdcObjects/agdcLaunchConfig.cpp",
		78, 79, 0,
		HandleTag_attachCtx_registryID, // _10000c7a5
		HandleTag_state, // _10000c7c3
		NULL,
	},
};


void DoDisplayPolicyState(void) {
	void *buf;
	size_t bufSize;
	int i;
	GetDisplayPolicyState(&buf, &bufSize);
	iprintf("size: 0x%lx bytes.\n", bufSize);
	
	DisplayPolicyState *state = (DisplayPolicyState *)buf;
	
	iprintf("boardID: %s\n", state->boardID);
	iprintf("featureMask: %#llx\n", state->featureMask);
	iprintf("platformFlags: %#llx\n", state->platformFlags);
	iprintf("extraSupportFlags: %#llx\n", state->extraSupportFlags);
	iprintf("wranglerFlags: %#llx\n", state->wranglerFlags);
	for (i = 0; i < 8; i++) {
		if (state->launcher[i].state) {
			iprintf("launcher[%d]: name '%s', state %d, managerState %d, vendor class/id/version %#x %#x %#x\n",
				i,
				state->launcher[i].name,
				state->launcher[i].state,
				state->launcher[i].managerState,
				state->launcher[i].vendorclass,
				state->launcher[i].vendorid,
				state->launcher[i].vendorversion
			);
		} // if launcher state
	} // for launcher
	
	for (i = 0; i < 8; i++) {
		gpuinfo *gpu = &state->gpu[i];
		if (gpu->state) {
			iprintf("gpu[%d]: index=%d state=%#x (%s) events=(%s) dispPolicyState=%#x (%s) dispPolicyLaunchIndex=%d\n",
				i,
				gpu->index,
				gpu->state,

				gpu->state == 1 ? "Discovered" :
				gpu->state == 2 ? "GPUCDetect" :
				gpu->state == 3 ? "GPUCStartDrivers" :
				gpu->state == 4 ? "WaitForDrivers" :
				gpu->state == 5 ? "Published" :
				gpu->state == 6 ? "Unpublished" :
				gpu->state == 7 ? "Gone" :
				gpu->state == 8 ? "Eject" :
				gpu->state == 0x80 ? "GPUCDriverStartFailed" :
				UNKNOWN_VALUE(gpu->state),

				gpu->events == 0 ? "" :
				gpu->events == 1 ? "terminated" :
				UNKNOWN_VALUE(gpu->events),

				gpu->dispPolicyState,

				gpu->dispPolicyState == 0 ? "Pending" :
				gpu->dispPolicyState == 1 ? "Managed" :
				gpu->dispPolicyState == 2 ? "Unmanaged" :
				gpu->dispPolicyState == 3 ? "Stopped" :
				gpu->dispPolicyState == 4 ? "InitFailed" :
				gpu->dispPolicyState == 5 ? "InitAborted" :
				gpu->dispPolicyState == 6 ? "WaitingForAGDP" :
				gpu->dispPolicyState == 7 ? "StartingManager" :
				UNKNOWN_VALUE(gpu->dispPolicyState),

				gpu->dispPolicyLaunchIndex
			);

			char flagsStr[1000];
			snprintf(flagsStr, sizeof(flagsStr), "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
				gpu->subgpu.flags &          2 ? ",XG" : "",
				gpu->subgpu.flags &       0x10 ? ",IG" : "",
				gpu->subgpu.flags &       0x20 ? ",DG" : "",
				gpu->subgpu.flags &       0x40 ? ",NG" : "",
				gpu->subgpu.flags &          1 ? ",eject" : "",
				gpu->subgpu.flags &          4 ? ",ejectFinalizing" : "",
				gpu->subgpu.flags &          8 ? ",ejectFinalized" : "",
				gpu->subgpu.flags &  0x1000000 ? ",unsupported" : "",
				gpu->subgpu.flags & 0x80000000 ? ",published" : "",
				gpu->subgpu.flags &  0x8000000 ? ",driversStarted" : "",
				gpu->subgpu.flags &  0x4000000 ? ",hasGPUC" : "",
				gpu->subgpu.flags & 0x40000000 ? ",terminated" : "",
				gpu->subgpu.flags & 0x20000000 ? ",quiet" : "",
				gpu->subgpu.flags & 0x10000000 ? ",pubSched" : "",
				gpu->subgpu.flags &  0x2000000 ? ",pubArmed" : "",
				gpu->subgpu.flags & 0x00ffff80 ? "," : "",
				gpu->subgpu.flags & 0x00ffff80 ? UNKNOWN_VALUE(gpu->subgpu.flags & 0x00ffff80) : ""
			);
			
			iprintf("gpu %#04x flags %#x (%s)\n", gpu->subgpu.gpuid, gpu->subgpu.flags, flagsStr[0] ? flagsStr + 1 : flagsStr);

			CFStringRef cfpath = GetOneRegPath(gpu->subgpu.pci);
			const char * path = cfpath ? CFStringGetCStringPtr(cfpath, kCFStringEncodingUTF8) : "";
			
			DoOneReg(gpu->subgpu.gpuid, gpu->subgpu.pci, "pci", NULL);
			DoOneReg(gpu->subgpu.gpuid, gpu->subgpu.agdc, "agdc", path);
			DoOneReg(gpu->subgpu.gpuid, gpu->subgpu.gpuc, "gpuc", path);
			DoOneReg(gpu->subgpu.gpuid, gpu->subgpu.agdpclient, "agdpclient", path);
			DoOneReg(gpu->subgpu.gpuid, gpu->subgpu.accel, "accel", path);

			for (int j = 0; j < gpu->subgpu.fbcount; j++) {
				char fbname[10];
				snprintf(fbname, sizeof(fbname), "fb%d:%lld", j, gpu->subgpu.fb[j].fbindex);
				DoOneReg(gpu->subgpu.gpuid, gpu->subgpu.fb[j].fb, fbname, path);
			}
			
			if (cfpath) CFRelease(cfpath);
		} // if gpu state
	} // for gpu
	
	gtraceheader *header = &state->gtrace.header;

	char build[3];
	if (header->version[3])
		snprintf(build, sizeof(build), ".%d", header->version[3]);
	else
		build[0] = '\0';
	
	iprintf("Version: %d.%d.%d%s, Max: %d, counter: %d, calendar sync: 0x%llx/0x%llx\n",
		header->version[0],
		header->version[1],
		header->version[2],
		build,
		header->maxEntries,
		header->counter,
		header->ussue,
		header->usct
	);
	
	iprintf("GTRACEDATASTREAM traceData = {\n"); INDENT
	iprintf("/* Gtrace Stream Marker */\n");
	iprintf("header = {\n"); INDENT
	uint32_t swapped[4] = { CFSwapInt32BigToHost(header->gtraceid[0]), CFSwapInt32BigToHost(header->gtraceid[1]), CFSwapInt32BigToHost(header->gtraceid[2]), CFSwapInt32BigToHost(header->gtraceid[3]) };
	
	iprintf("id:\"%s\" ", (char*)swapped);
	if (header->gtraceid[0] != 0x67547261 || header->gtraceid[1] != 0x63654461 || header->gtraceid[2] != 0x74614475 || header->gtraceid[3] != 0x6d700000) cprintf("id0:0x%x id1:0x%x id2:0x%x id3:0x%x ", header->gtraceid[0], header->gtraceid[1], header->gtraceid[2], header->gtraceid[3]);
	cprintf("dataVersion:0x%x,\n", header->dataVersion);

	char thenstr[100];
	int64_t thenns = header->ussue * 1000;
	time_t thentime = thenns / 1000000000;
	struct tm thentm;
	localtime_r(&thentime, &thentm);
	size_t len = strftime(thenstr, sizeof(thenstr), "%F %T", &thentm);
	snprintf(thenstr + len, sizeof(thenstr) - len, ".%09lld", thenns % 1000000000);

	char thenstrct[100];
	strftimestamp(thenstrct, sizeof(thenstrct), "%F %T", 0, header->usct * 1000);
	
	iprintf("hdrSize:%llu ussue:%llu=%s usct:%llu=%s\n", header->hdrSize, header->ussue, thenstr, header->usct, thenstrct);
#if 0
	for (int decimals = 9; decimals >= 0; decimals--) {
		strftimestamp(thenstrct, sizeof(thenstrct), "%F %T", decimals, header->usct * 1000);
		iprintf("%s\n", thenstrct);
	}
#endif

	OUTDENT iprintf("};\n");
	
	iprintf("entries = {\n"); INDENT
	
	gtraceentry *entry = state->gtrace.entry;
	int numEntries = header->counter < header->maxEntries ? header->counter : header->maxEntries;
	qsort(entry, numEntries, sizeof(*entry), entrycompare);
	for (i = 0; i < numEntries; i++) {
		
		knowngtraceentry defaultgtraceentry = { 0, NULL, 0,0,0, HandleTag_Default, HandleTag_Default, HandleTag_Default };
		knowngtraceentry *currentgtrace = &defaultgtraceentry;
		for (int j = 0; j < sizeof(knowngtrace) / sizeof(knowngtrace[0]); j++) {
			if (knowngtrace[j].line == entry->line && knowngtrace[j].tag0 == entry->tag0 && knowngtrace[j].tag1 == entry->tag1 && knowngtrace[j].tag2 == entry->tag2) {
				currentgtrace = &knowngtrace[j];
				break;
			}
		}

		char thenstr[100];
		strftimestamp(thenstr, sizeof(thenstr), "%F %T", 0, entry->timestamp);

		iprintf("{ timestamp:%lld=%s", entry->timestamp, thenstr);
		if (entry->object) cprintf(" object:0x%llx", entry->object);
		if (entry->tag0 || entry->data0 || entry->tag1 || entry->data1 || entry->tag2 || entry->data2) DoOneTag(entry->tag0, entry->data0, currentgtrace->handleTag0);
		if (                               entry->tag1 || entry->data1 || entry->tag2 || entry->data2) DoOneTag(entry->tag1, entry->data1, currentgtrace->handleTag1);
		if (                                                              entry->tag2 || entry->data2) DoOneTag(entry->tag2, entry->data2, currentgtrace->handleTag2);
		cprintf(" },");

		if ((currentgtrace && currentgtrace->file) && entry->line) cprintf(" // %s:%d", currentgtrace->file, entry->line);
		else if (currentgtrace && currentgtrace->file) cprintf(" // %s", currentgtrace->file);
		else if (entry->line) cprintf(" // line:%d", entry->line);
		
		cprintf("\n");
		
		entry++;
	} // for entry
	
	OUTDENT iprintf("};\n");
	OUTDENT iprintf("};\n");
	
	free (buf);
}
