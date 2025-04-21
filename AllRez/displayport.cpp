//
//  mst.m
//  AllRez
//
//  Created by joevt on 2022-05-20.
//
#include "AppleMisc.h"
#include "displayport.h"
#include "dpcd.h"
#include "iofbdebuguser.h"
#include "printf.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <CoreFoundation/CFByteOrder.h>

//=================================================================================================================================
// Linux includes

#include <drm/display/drm_dp_helper.h>
#include <drm/display/drm_hdcp.h>
#include "dpcd_defs.h"

//=================================================================================================================================
// Defines

// These are in micoseconds

#define kDelayDisplayPortReply 0
#define kDelayDisplayPortSidebandReply   10000 // works at 10000
//                                     1000000

//=================================================================================================================================

uint8_t crc4(const uint8_t * data, size_t NumberOfNibbles)
{
	uint8_t BitMask      = 0x80;
	uint8_t BitShift     = 7;
	uint8_t ArrayIndex   = 0;
	int     NumberOfBits = (int)NumberOfNibbles * 4;
	uint8_t Remainder    = 0;

	while (NumberOfBits != 0)
	{
		NumberOfBits--;
		   Remainder <<= 1;
		   Remainder |= (data[ArrayIndex] & BitMask) >> BitShift;
		   BitMask >>= 1;
		   BitShift--;
		   if (BitMask == 0)
		   {
			   BitMask  = 0x80;
			   BitShift = 7;
			   ArrayIndex++;
		   }
		   if ((Remainder & 0x10) == 0x10)
		   {
			   Remainder ^= 0x13;
		   }
	}
	NumberOfBits = 4;
	while (NumberOfBits != 0)
	{
		NumberOfBits--;
		Remainder <<= 1;
		if ((Remainder & 0x10) != 0)
		{
			Remainder ^= 0x13;
		}
	}

   return Remainder;
} // crc4

uint8_t crc8(const uint8_t * data, uint8_t NumberOfBytes)
{
	uint8_t  BitMask      = 0x80;
	uint8_t  BitShift     = 7;
	uint8_t  ArrayIndex   = 0;
	uint16_t NumberOfBits = NumberOfBytes * 8;
	uint16_t Remainder    = 0;

	while (NumberOfBits != 0)
	{
		NumberOfBits--;
		Remainder <<= 1;
		Remainder |= (data[ArrayIndex] & BitMask) >> BitShift;
		BitMask >>= 1;
		BitShift--;
		if (BitMask == 0)
		{
			BitMask  = 0x80;
			BitShift = 7;
			ArrayIndex++;
		}
		if ((Remainder & 0x100) == 0x100)
		{
			Remainder ^= 0xD5;
		}
	}
	NumberOfBits = 8;
	while (NumberOfBits != 0)
	{
		NumberOfBits--;
		Remainder <<= 1;
		if ((Remainder & 0x100) != 0)
		{
			Remainder ^= 0xD5;
		}
	}
   return Remainder & 0xFF;
} // crc8



char * DumpOneDisplayPortGuid(char *buf, size_t bufSize, uint8_t *guid) {
	scnprintf(buf, bufSize, "%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x",
		CFSwapInt32BigToHost(*(UInt32*)(guid + 0)),
		CFSwapInt16BigToHost(*(UInt16*)(guid + 4)),
		CFSwapInt16BigToHost(*(UInt16*)(guid + 6)),
		CFSwapInt16BigToHost(*(UInt16*)(guid + 8)),
		*(UInt8 *)(guid + 10),
		*(UInt8 *)(guid + 11),
		*(UInt8 *)(guid + 12),
		*(UInt8 *)(guid + 13),
		*(UInt8 *)(guid + 14),
		*(UInt8 *)(guid + 15)
	);
	return buf;
}



int gDumpSidebandMessage = 0;
int gMessageSequenceNumber = 1;

struct WholeMessage {
	uint32_t dpcdType;
	size_t bodyLength;
	size_t partLength;
	UInt8 body[0x1000];
	UInt8 part[0x200];
} WholeMessage = { 0, 0, 0 };

void ClearWholeMessage(UInt32 dpcdType)
{
	WholeMessage.dpcdType = dpcdType;
	WholeMessage.partLength = 0;
	bzero(WholeMessage.part, sizeof(WholeMessage.part));
	WholeMessage.bodyLength = 0;
	bzero(WholeMessage.body, sizeof(WholeMessage.body));
}

void DumpOneDisplayPortMessage(UInt8 *msgbytes, int msglen, uint32_t dpcdAddress)
{
	int i;
	UInt32 offset = dpcdAddress & 0x01ff;
	int dpcdType = dpcdAddress & ~0x1ff;

	iprintf("%s0x%05x:", offset ? "        " : "message ", dpcdAddress);
	for (i = 0; i < msglen; i++) {
		cprintf(" %02x", ((UInt8*)msgbytes)[i]);
	}

	if (dpcdAddress < DP_SIDEBAND_MSG_DOWN_REQ_BASE || dpcdAddress > DP_SIDEBAND_MSG_UP_REQ_BASE + 0x200) {
		cprintf(" *** message 0x%05x is out of range", dpcdAddress);
		return;
	}
	if (offset + msglen > 0x200) {
        cprintf(" *** message 0x%05x has out of range end %0x05x", dpcdAddress, dpcdAddress + msglen);
		return;
	}

	if (offset > 0x30) {
        cprintf(" *** message 0x%05x probably has out of range offset", dpcdAddress);
	}
	
	if (dpcdType != WholeMessage.dpcdType) {
		if (WholeMessage.dpcdType) {
            cprintf(" *** new message 0x%05x but expecting more from 0x%05x", dpcdAddress, WholeMessage.dpcdType);
		}
		ClearWholeMessage(dpcdType);
	}

	if (offset != WholeMessage.partLength) {
        cprintf(" *** message 0x%05x but expected offset 0x%lx", dpcdAddress, WholeMessage.partLength);
	}
	
	memcpy(&WholeMessage.part[offset], msgbytes, msglen);
	WholeMessage.partLength = offset + msglen;

	Sideband_MSG1 *msg1 = (Sideband_MSG1 *)WholeMessage.part;
	Sideband_MSG2 *msg2 = (Sideband_MSG2 *)((uint8_t*)&msg1->header.Relative_Address + msg1->header.Link_Count_Total / 2);
	Sideband_MSG3 *msg3 = (Sideband_MSG3 *)((uint8_t*)&msg2->body + msg2->header.Sideband_MSG_Body_Length - 1);

	if (WholeMessage.partLength <= (uint8_t*)msg3 - (uint8_t*)msg1) {
		return;
	}
	
	if (msg2->header.Start_Of_Message_Transaction) {
		if (WholeMessage.bodyLength) {
            cprintf(" *** got a new message while other message (%ld bytes) in progress", WholeMessage.bodyLength);
		}
		WholeMessage.bodyLength = 0;
		bzero(WholeMessage.body, sizeof(WholeMessage.body));
	}

	memcpy(&WholeMessage.body[WholeMessage.bodyLength], &msg2->body, msg2->header.Sideband_MSG_Body_Length - 1);
	WholeMessage.bodyLength = WholeMessage.bodyLength + msg2->header.Sideband_MSG_Body_Length - 1;

    cprintf("\n");
	iprintf("lct=%d lcr=%d , rad=",
		msg1->header.Link_Count_Total,
		msg1->header.Link_Count_Remaining
	);
	for (i = 0; i < msg1->header.Link_Count_Total - 1; i++) {
		cprintf("%s%d", i > 0 ? "." : "", (msg1->header.Relative_Address[i / 2] >> ((i & 1) ? 0 : 4)) & 15);
	}
	UInt8 crc = crc8((uint8_t*)&msg2->body, msg2->header.Sideband_MSG_Body_Length - 1);
	cprintf(" , broadcast=%d path=%d len=%d , somt=%d eomt=%d zero=%d seq=%d crc=0x%x:%s ... ; crc=0x%02x:%s",
		msg2->header.Broadcast_Message,
		msg2->header.Path_Message,
		msg2->header.Sideband_MSG_Body_Length,

		msg2->header.Start_Of_Message_Transaction,
		msg2->header.End_Of_Message_Transaction,
		msg2->header.zero,
		msg2->header.Message_Sequence_No,
		msg2->header.Sideband_MSG_Header_CRC,
		msg2->header.Sideband_MSG_Header_CRC == crc4((uint8_t*)msg1, ((uint8_t*)&msg2->body - (uint8_t*)&msg1->header) * 2 - 1) ? "ok" : "err",

		msg3->body.Sideband_MSG_Body_CRC,
		msg3->body.Sideband_MSG_Body_CRC == crc ? "ok" : "err"
	);

	if (!msg2->header.End_Of_Message_Transaction) {
		goto done;
	}

    cprintf("\n");
	DumpOneDisplayPortMessageBody((void*)WholeMessage.body, (int)WholeMessage.bodyLength, (dpcdAddress >= DP_SIDEBAND_MSG_DOWN_REP_BASE && dpcdAddress < DP_SIDEBAND_MSG_UP_REQ_BASE));
	
done:
	if (msg2->header.End_Of_Message_Transaction) {
		ClearWholeMessage(0);
	}
	else {
		WholeMessage.partLength = 0;
		bzero(WholeMessage.part, sizeof(WholeMessage.part));
	}
} // DumpOneDisplayPortMessage



const char* GetOneRequestIdentifierStr(char *buf, size_t bufSize, int rid) {
    scnprintf(buf, bufSize, "type=0x%02x:%s",
        rid,
        rid == DP_GET_MSG_TRANSACTION_VERSION ? "GET_MSG_TRANSACTION_VERSION" :
        rid == DP_LINK_ADDRESS                ? "LINK_ADDRESS" :
        rid == DP_CONNECTION_STATUS_NOTIFY    ? "CONNECTION_STATUS_NOTIFY" :
        rid == DP_ENUM_PATH_RESOURCES         ? "ENUM_PATH_RESOURCES" :
        rid == DP_ALLOCATE_PAYLOAD            ? "ALLOCATE_PAYLOAD" :
        rid == DP_QUERY_PAYLOAD               ? "QUERY_PAYLOAD" :
        rid == DP_RESOURCE_STATUS_NOTIFY      ? "RESOURCE_STATUS_NOTIFY" :
        rid == DP_CLEAR_PAYLOAD_ID_TABLE      ? "CLEAR_PAYLOAD_ID_TABLE" :
        rid == DP_REMOTE_DPCD_READ            ? "REMOTE_DPCD_READ" :
        rid == DP_REMOTE_DPCD_WRITE           ? "REMOTE_DPCD_WRITE" :
        rid == DP_REMOTE_I2C_READ             ? "REMOTE_I2C_READ" :
        rid == DP_REMOTE_I2C_WRITE            ? "REMOTE_I2C_WRITE" :
        rid == DP_POWER_UP_PHY                ? "POWER_UP_PHY" :
        rid == DP_POWER_DOWN_PHY              ? "POWER_DOWN_PHY" :
        rid == DP_SINK_EVENT_NOTIFY           ? "SINK_EVENT_NOTIFY" :
        rid == DP_QUERY_STREAM_ENC_STATUS     ? "QUERY_STREAM_ENC_STATUS" : "?"
    );
    return buf;
} // GetOneRequestIdentifierStr



void DumpOneDisplayPortMessageBody(void *bodyData, int bodyLength, bool isReply) {
	Sideband_MSG_Body *body = (Sideband_MSG_Body *)bodyData;
	Sideband_MSG3 *msg3 = (Sideband_MSG3 *)((uint8_t*)body + bodyLength);
	
	Sideband_MSG3 *msg3parseEnd;
	Sideband_MSG_Body body2c;
	char guid[40];
    char idStr[40];
	int i;

    iprintf("%s",
        GetOneRequestIdentifierStr(idStr, sizeof(idStr), body->request.Request_Identifier)
    );

    if (!isReply) {
		msg3parseEnd = (Sideband_MSG3 *)&body->request.Default.end;
		
		switch (body->request.Request_Identifier) {
			case DP_GET_MSG_TRANSACTION_VERSION: {
				// unknown
				break;
			}
			case DP_LINK_ADDRESS: {
				// empty
				break;
			}
			case DP_CONNECTION_STATUS_NOTIFY: {
				cprintf(" port:%d zeros:%d , guid:%s , zero:%d Legacy_Device_Plug_status:%d DisplayPort_Device_Plug_Status:%d Messaging_Capability_Status:%d input:%d Peer_Device_Type=%d%s",
					body->request.Connection_Status_Notify.Port_Number,
					body->request.Connection_Status_Notify.zeros,
					DumpOneDisplayPortGuid(guid, sizeof(guid), (uint8_t *)&body->request.Connection_Status_Notify.Global_Unique_Identifier),
					body->request.Connection_Status_Notify.zero,
					body->request.Connection_Status_Notify.Legacy_Device_Plug_status,
					body->request.Connection_Status_Notify.DisplayPort_Device_Plug_Status,
					body->request.Connection_Status_Notify.Messaging_Capability_Status,
					body->request.Connection_Status_Notify.Input_Port,
					body->request.Connection_Status_Notify.Peer_Device_Type,
					body->request.Connection_Status_Notify.Peer_Device_Type == DP_PEER_DEVICE_NONE           ? ":\"No device connected\"" :
					body->request.Connection_Status_Notify.Peer_Device_Type == DP_PEER_DEVICE_SOURCE_OR_SST  ? ":\"Source device or SST Branch device connected to an upstream port\"" :
					body->request.Connection_Status_Notify.Peer_Device_Type == DP_PEER_DEVICE_MST_BRANCHING  ? ":\"Device with MST Branching Unit or SST Branch device connected to a downstream port\"" :
					body->request.Connection_Status_Notify.Peer_Device_Type == DP_PEER_DEVICE_SST_SINK       ? ":\"SST Sink device or Stream Sink in an MST Sink/Composite device\"" :
					body->request.Connection_Status_Notify.Peer_Device_Type == DP_PEER_DEVICE_DP_LEGACY_CONV ? ":\"DP-to-Legacy converter (DP to VGA, DVI or HDMI)\"" :
					":unknown"
				);
				msg3parseEnd = (Sideband_MSG3 *)&body->request.Connection_Status_Notify.end;
				break;
			}
			case DP_ENUM_PATH_RESOURCES: {
				cprintf(" port:%d zeros:%d",
					body->request.Enum_Path_Resources.Port_Number,
					body->request.Enum_Path_Resources.zeros
				);
				msg3parseEnd = (Sideband_MSG3 *)&body->request.Enum_Path_Resources.end;
				break;
			}
			case DP_ALLOCATE_PAYLOAD: {
				cprintf(" port=%d Number_SDP_Streams=%d , zero=%d Virtual_Channel_Payload_Identifier=%d , PBN=0x%04x , SDP_Stream_Sink=",
					body->request.Allocate_Payload.Port_Number,
					body->request.Allocate_Payload.Number_SDP_Streams,
					body->request.Allocate_Payload.zero,
					body->request.Allocate_Payload.Virtual_Channel_Payload_Identifier,
					CFSwapInt16BigToHost(body->request.Allocate_Payload.Payload_Bandwidth_Number)
				);
				for (i = 0; i < body->request.Allocate_Payload.Number_SDP_Streams; i++) {
					cprintf("%s%d", i > 0 ? "." : "", (body->request.Allocate_Payload.SDP_Stream_Sink[i / 2] >> ((i & 1) ? 0 : 4)) & 15);
				}
				msg3parseEnd = (Sideband_MSG3 *)((uint8_t*)&body->request.Allocate_Payload.SDP_Stream_Sink + (body->request.Allocate_Payload.Number_SDP_Streams + 1) / 2);
				break;
			}
			case DP_QUERY_PAYLOAD: {
				cprintf(" port=%d zeros=%d , zero=%d Virtual_Channel_Payload_Identifier=%d",
					body->request.Query_Payload.Port_Number,
					body->request.Query_Payload.zeros,
					body->request.Query_Payload.zero,
					body->request.Query_Payload.Virtual_Channel_Payload_Identifier
				);
				msg3parseEnd = (Sideband_MSG3 *)&body->request.Query_Payload.end;
				break;
			}
			case DP_RESOURCE_STATUS_NOTIFY: {
				cprintf(" port=%d zeros=%d , guid=%s Available_PBN=%d",
					body->request.Resource_Status_Notify.Port_Number,
					body->request.Resource_Status_Notify.zeros,
					DumpOneDisplayPortGuid(guid, sizeof(guid), (uint8_t *)&body->request.Resource_Status_Notify.Global_Unique_Identifier),
					CFSwapInt16BigToHost(body->request.Resource_Status_Notify.Available_PBN)
				);
				msg3parseEnd = (Sideband_MSG3 *)&body->request.Resource_Status_Notify.end;
				break;
			}
			case DP_CLEAR_PAYLOAD_ID_TABLE: {
				// empty
				break;
			}
			case DP_REMOTE_DPCD_READ: {
				body2c.request.Remote_DPCD_Read.raw = CFSwapInt32BigToHost(body->request.Remote_DPCD_Read.raw);
				cprintf(" port=%d dpcd=%05x , bytes=%d",
					body2c.request.Remote_DPCD_Read.Port_Number,
					body2c.request.Remote_DPCD_Read.DPCD_Address,
					body2c.request.Remote_DPCD_Read.Number_Of_Bytes_To_Read
				);
				msg3parseEnd = (Sideband_MSG3 *)&body->request.Remote_DPCD_Read.end;
				break;
			}
			case DP_REMOTE_DPCD_WRITE: {
				body2c.request.Remote_DPCD_Write.raw = CFSwapInt32BigToHost(body->request.Remote_DPCD_Write.raw);
				cprintf(" port=%d dpcd=%05x , bytes=%d , Write_Data=",
					body2c.request.Remote_DPCD_Write.Port_Number,
					body2c.request.Remote_DPCD_Write.DPCD_Address,
					body2c.request.Remote_DPCD_Write.Number_Of_Bytes_To_Write
				);
				for (i = 0; i < body2c.request.Remote_DPCD_Write.Number_Of_Bytes_To_Write && i < (uint8_t*)msg3 - (uint8_t*)body->request.Remote_DPCD_Write.Write_Data; i++) {
					cprintf("%02x", body->request.Remote_DPCD_Write.Write_Data[i]);
				}
				if (i < body2c.request.Remote_DPCD_Write.Number_Of_Bytes_To_Write) {
					cprintf(" (not enough bytes in message body)");
				}
				msg3parseEnd = (Sideband_MSG3 *)&body->request.Remote_DPCD_Write.Write_Data[i];
				break;
			}
			case DP_REMOTE_I2C_READ: {
				cprintf(" port=%d zeros=%d Number_Of_I2C_Write_Transactions=%d , {\n",
					body->request.Remote_I2C_Read.Port_Number,
					body->request.Remote_I2C_Read.zeros,
					body->request.Remote_I2C_Read.Number_Of_I2C_Write_Transactions
				);
				INDENT
				
				I2C_Transaction *transaction = body->request.Remote_I2C_Read.Transactions;
				
				for (i = 0; i < body->request.Remote_I2C_Read.Number_Of_I2C_Write_Transactions; i++) {
					iprintf("zero=%d Write_I2C_Device_Identifier=0x%02x , Number_Of_Bytes_To_Write=%d , I2C_Data_To_Write=",
						transaction->zero,
						transaction->Write_I2C_Device_Identifier,
						transaction->Number_Of_Bytes_To_Write
					);
					for (int j = 0; j < transaction->Number_Of_Bytes_To_Write; j++) {
						cprintf("%02x", transaction->I2C_Data_To_Write[j]);
					}

					I2C_Transaction2 *transaction2 = (I2C_Transaction2 *)((uint8_t *)&transaction->I2C_Data_To_Write + transaction->Number_Of_Bytes_To_Write);
					cprintf(" , zeros=%d No_Stop_Bit=%d I2C_Transaction_Delay=%d\n",
						transaction2->zeros,
						transaction2->No_Stop_Bit,
						transaction2->I2C_Transaction_Delay
					);

					transaction = (I2C_Transaction *)&transaction2->end;
				}
				OUTDENT

				Remote_I2C_Read2 *read2 = (Remote_I2C_Read2 *)transaction;
				iprintf("} zero:%d Read_I2C_Device_Identifier:0x%02x , Number_Of_Bytes_To_Read:%d",
					read2->zero,
					read2->Read_I2C_Device_Identifier,
					read2->Number_Of_Bytes_To_Read
				);
				msg3parseEnd = (Sideband_MSG3 *)&read2->end;
				break;
			}
			case DP_REMOTE_I2C_WRITE: {
				cprintf(" port=%d zeros=%d , zero=%d Write_I2C_Device_Identifier:0x%02x , Number_Of_Bytes_To_Write=%d , I2C_Data_To_Write=",
					body->request.Remote_I2C_Write.Port_Number,
					body->request.Remote_I2C_Write.zeros,
					body->request.Remote_I2C_Write.zero,
					body->request.Remote_I2C_Write.Write_I2C_Device_Identifier,
					body->request.Remote_I2C_Write.Number_Of_Bytes_To_Write
				);
				for (i = 0; i < body->request.Remote_I2C_Write.Number_Of_Bytes_To_Write; i++) {
					cprintf("%02x", body->request.Remote_I2C_Write.I2C_Data_To_Write[i]);
				}
				msg3parseEnd = (Sideband_MSG3 *)((uint8_t*)&body->request.Remote_I2C_Write.I2C_Data_To_Write + (body->request.Remote_I2C_Write.Number_Of_Bytes_To_Write + 1) / 2);
				break;
			}
			case DP_POWER_UP_PHY: {
				cprintf("POWER_UP_PHY");
				cprintf(" port=%d zeros=%d",
					body->request.Power_Up_PHY.zeros,
					body->request.Power_Up_PHY.Port_Number
				);
				msg3parseEnd = (Sideband_MSG3 *)&body->request.Power_Up_PHY.end;
				break;
			}
			case DP_POWER_DOWN_PHY: {
				cprintf(" port=%d zeros=%d",
					body->request.Power_Down_PHY.zeros,
					body->request.Power_Down_PHY.Port_Number
				);
				msg3parseEnd = (Sideband_MSG3 *)&body->request.Power_Down_PHY.end;
				break;
			}
			case DP_SINK_EVENT_NOTIFY: {
				cprintf(" zeros=%d Link_Count=%d , Relative_Address=",
					body->request.Sink_Event_Notify.zeros,
					body->request.Sink_Event_Notify.Link_Count
				);
				for (i = 0; i < body->request.Sink_Event_Notify.Link_Count; i++) {
					cprintf("%s%d", i > 0 ? "." : "", (body->request.Sink_Event_Notify.Relative_Address[i / 2] >> ((i & 1) ? 0 : 4)) & 15);
				}
				
				Sink_Event_Notify2 *sinkevent2 = (Sink_Event_Notify2 *)((uint8_t*)&body->request.Sink_Event_Notify.Relative_Address + body->request.Sink_Event_Notify.Link_Count);
				cprintf(" , Sink_Event=0x%02x", sinkevent2->Sink_Event);

				msg3parseEnd = (Sideband_MSG3 *)&sinkevent2->end;
				break;
			}
			case DP_QUERY_STREAM_ENC_STATUS: {
				// see Appendix I of DisplayPort 1.2 spec
				break;
			}
			default: {
				cprintf("?0x%x", body->request.Request_Identifier);
				break;
			}
		} // switch body->request.Request_Identifier
	} // request
	else
	{ // reply
        cprintf(" reply_type=%s", body->reply.Reply_Type == DP_SIDEBAND_REPLY_NAK ? "NAK" : "ACK");
		switch (body->reply.Reply_Type) {
			case DP_SIDEBAND_REPLY_NAK:
			{
				msg3parseEnd = (Sideband_MSG3 *)&body->reply.NAK.end;
				cprintf(" guid=%s reason=",
					DumpOneDisplayPortGuid(guid, sizeof(guid), (uint8_t *)&body->reply.NAK.Global_Unique_Identifier)
				);
				switch (body->reply.NAK.Reason_For_NAK) {
					case DP_NAK_WRITE_FAILURE: cprintf("WRITE_FAILURE"); break;
					case DP_NAK_INVALID_READ: cprintf("INVALID_READ"); break;
					case DP_NAK_CRC_FAILURE: cprintf("CRC_FAILURE"); break;
					case DP_NAK_BAD_PARAM: cprintf("BAD_PARAM"); break;
					case DP_NAK_DEFER: cprintf("DEFER"); break;
					case DP_NAK_LINK_FAILURE: cprintf("LINK_FAILURE"); break;
					case DP_NAK_NO_RESOURCES: cprintf("NO_RESOURCES"); break;
					case DP_NAK_DPCD_FAIL: cprintf("DPCD_FAIL"); break;
					case DP_NAK_I2C_NAK: cprintf("I2C_NAK"); break;
					case DP_NAK_ALLOCATE_FAIL: cprintf("ALLOCATE_FAIL"); break;
					default: cprintf("ALLOCATE_FAIL"); break;
				}
				break;
			} // DP_SIDEBAND_REPLY_NAK
			case DP_SIDEBAND_REPLY_ACK:
			{
				msg3parseEnd = (Sideband_MSG3 *)&body->reply.ACK.Default.end;
				switch (body->reply.Request_Identifier) {
					case DP_GET_MSG_TRANSACTION_VERSION: {
						cprintf("GET_MSG_TRANSACTION_VERSION");
						// unknown
						break;
					}
					case DP_LINK_ADDRESS: {
						cprintf(" guid=%s , zeros=%d ports=%d , {\n",
							DumpOneDisplayPortGuid(guid, sizeof(guid), (uint8_t *)&body->reply.ACK.Link_Address.Global_Unique_Identifier),
								body->reply.ACK.Link_Address.zeros,
								body->reply.ACK.Link_Address.Number_Of_Ports
						);
						INDENT
						Link_Address_Port *port = body->reply.ACK.Link_Address.Ports;
						for (i = 0; i < body->reply.ACK.Link_Address.Number_Of_Ports; i++) {
							iprintf("input=%d Peer_Device_Type=%d%s port=%d , ",
								port->Input_Port,
								port->Peer_Device_Type,
								port->Peer_Device_Type == DP_PEER_DEVICE_NONE           ? ":\"No device connected\"" :
								port->Peer_Device_Type == DP_PEER_DEVICE_SOURCE_OR_SST  ? ":\"Source device or SST Branch device connected to an upstream port\"" :
								port->Peer_Device_Type == DP_PEER_DEVICE_MST_BRANCHING  ? ":\"Device with MST Branching Unit or SST Branch device connected to a downstream port\"" :
								port->Peer_Device_Type == DP_PEER_DEVICE_SST_SINK       ? ":\"SST Sink device or Stream Sink in an MST Sink/Composite device\"" :
								port->Peer_Device_Type == DP_PEER_DEVICE_DP_LEGACY_CONV ? ":\"DP-to-Legacy converter (DP to VGA, DVI or HDMI)\"" :
								":unknown",
								port->Port_Number
							);
							if (port->Input_Port) {
								cprintf("Messaging_Capability_Status=%d DisplayPort_Device_Plug_Status=%d zeros=%d\n",
									port->Input.Messaging_Capability_Status,
									port->Input.DisplayPort_Device_Plug_Status,
									port->Input.zeros
								);
								port = (Link_Address_Port *)&port->Input.end;
							}
							else
							{
								cprintf("Messaging_Capability_Status=%d DisplayPort_Device_Plug_Status=%d Legacy_Device_Plug_Status=%d zeros=%d , Dpcd_Revision=%d.%d , Peer_Guid=%s , Number_SDP_Streams=%d Number_SDP_Stream_Sinks=%d\n",
									port->Output.Messaging_Capability_Status,
									port->Output.DisplayPort_Device_Plug_Status,
									port->Output.Legacy_Device_Plug_Status,
									port->Output.zeros,
									
									port->Output.Dpcd_Revision_Major,
									port->Output.Dpcd_Revision_Minor,

									DumpOneDisplayPortGuid(guid, sizeof(guid), (uint8_t *)&port->Output.Peer_Guid),

									port->Output.Number_SDP_Streams,
									port->Output.Number_SDP_Stream_Sinks
								);
								port = (Link_Address_Port *)&port->Output.end;
							}
						} // for port
						OUTDENT
						iprintf("}");
						msg3parseEnd = (Sideband_MSG3 *)port;
						break;
					}
					case DP_CONNECTION_STATUS_NOTIFY: {
						// empty
						break;
					}
					case DP_ENUM_PATH_RESOURCES: {
						cprintf(" port=%d zeros=%d FEC_Capable=%d, Full_PBN=0x%04x , Available_PBN=0x%04x",
							body->reply.ACK.Enum_Path_Resources.Port_Number,
							body->reply.ACK.Enum_Path_Resources.zeros,
							body->reply.ACK.Enum_Path_Resources.FEC_Capable,
							
							CFSwapInt16BigToHost(body->reply.ACK.Enum_Path_Resources.Payload_Bandwidth_Number),
							CFSwapInt16BigToHost(body->reply.ACK.Enum_Path_Resources.Available_PBN)
						);
						msg3parseEnd = (Sideband_MSG3 *)&body->reply.ACK.Enum_Path_Resources.end;
						break;
					}
					case DP_ALLOCATE_PAYLOAD: {
						cprintf(" port=%d zeros=%d , zero=%d Virtual_Channel_Payload_Identifier=%d , Allocated_PBN=0x%04x",
							body->reply.ACK.Allocate_Payload.Port_Number,
							body->reply.ACK.Allocate_Payload.zeros,
							body->reply.ACK.Allocate_Payload.zero,
							body->reply.ACK.Allocate_Payload.Virtual_Channel_Payload_Identifier,
							CFSwapInt16BigToHost(body->reply.ACK.Allocate_Payload.Payload_Bandwidth_Number)
						);
						msg3parseEnd = (Sideband_MSG3 *)&body->reply.ACK.Allocate_Payload.end;
						break;
					}
					case DP_QUERY_PAYLOAD: {
						cprintf(" port=%d zeros=%d , Allocated_PBN=0x%04x",
							body->reply.ACK.Query_Payload.Port_Number,
							body->reply.ACK.Query_Payload.zeros,
							CFSwapInt16BigToHost(body->reply.ACK.Query_Payload.Allocated_PBN)
						);
						msg3parseEnd = (Sideband_MSG3 *)&body->reply.ACK.Query_Payload.end;
						break;
					}
					case DP_RESOURCE_STATUS_NOTIFY: {
						// empty
						break;
					}
					case DP_CLEAR_PAYLOAD_ID_TABLE: {
						// empty
						break;
					}
					case DP_REMOTE_DPCD_READ: {
						cprintf(" zeros=%d port=%d , Number_Of_Bytes_Read=%d , Data_Read=",
							body->reply.ACK.Remote_DPCD_Read.zeros,
							body->reply.ACK.Remote_DPCD_Read.Port_Number,
							body->reply.ACK.Remote_DPCD_Read.Number_Of_Bytes_Read
						);
						for (i = 0; i < body->reply.ACK.Remote_DPCD_Read.Number_Of_Bytes_Read && i < (uint8_t*)msg3 - (uint8_t*)body->reply.ACK.Remote_DPCD_Read.Data_Read; i++) {
							cprintf("%02x", body->reply.ACK.Remote_DPCD_Read.Data_Read[i]);
						}
						if (i < body->reply.ACK.Remote_DPCD_Read.Number_Of_Bytes_Read) {
							cprintf(" (not enough bytes in message body)");
						}
						msg3parseEnd = (Sideband_MSG3 *)&body->reply.ACK.Remote_DPCD_Read.Data_Read[i];
						break;
					}
					case DP_REMOTE_DPCD_WRITE: {
						cprintf(" zeros=%d port=%d",
							body->reply.ACK.Remote_DPCD_Write.zeros,
							body->reply.ACK.Remote_DPCD_Write.Port_Number
						);
						msg3parseEnd = (Sideband_MSG3 *)&body->reply.ACK.Remote_DPCD_Write.end;
						break;
					}
					case DP_REMOTE_I2C_READ: {
						cprintf(" zeros=%d port=%d , Number_Of_Bytes_Read=%d , Data_Read=",
							body->reply.ACK.Remote_I2C_Read.zeros,
							body->reply.ACK.Remote_I2C_Read.Downstream_Port_Number,
							body->reply.ACK.Remote_I2C_Read.Number_Of_Bytes_Read
						);
						for (i = 0; i < body->reply.ACK.Remote_I2C_Read.Number_Of_Bytes_Read && i < (uint8_t*)msg3 - (uint8_t*)body->reply.ACK.Remote_I2C_Read.Data_Read ; i++) {
							cprintf("%02x", body->reply.ACK.Remote_I2C_Read.Data_Read[i]);
						}
						if (i < body->reply.ACK.Remote_I2C_Read.Number_Of_Bytes_Read) {
							cprintf(" (not enough bytes in message body)");
						}
						msg3parseEnd = (Sideband_MSG3 *)&body->reply.ACK.Remote_I2C_Read.Data_Read[i];
						break;
					}
					case DP_REMOTE_I2C_WRITE: {
						cprintf(" zeros=%d port=%d",
							body->reply.ACK.Remote_I2C_Write.zeros,
							body->reply.ACK.Remote_I2C_Write.Port_Number
						);
						msg3parseEnd = (Sideband_MSG3 *)&body->reply.ACK.Remote_I2C_Write.end;
						break;
					}
					case DP_POWER_UP_PHY: {
						cprintf(" port=%d zeros=%d",
							body->reply.ACK.Power_Up_PHY.Port_Number,
							body->reply.ACK.Power_Up_PHY.zeros
						);
						msg3parseEnd = (Sideband_MSG3 *)&body->reply.ACK.Power_Up_PHY.end;
						break;
					}
					case DP_POWER_DOWN_PHY: {
						cprintf(" port=%d zeros=%d",
							body->reply.ACK.Power_Down_PHY.Port_Number,
							body->reply.ACK.Power_Down_PHY.zeros
						);
						msg3parseEnd = (Sideband_MSG3 *)&body->reply.ACK.Power_Down_PHY.end;
						break;
					}
					case DP_SINK_EVENT_NOTIFY: {
						// empty
						break;
					}
					case DP_QUERY_STREAM_ENC_STATUS: {
						cprintf(" State=%d:%s Repeater_Present=%d Encryption_Enabled=%d Auth_Completed=%d zeros=%d ,",
							body->reply.ACK.Query_Stream_Enc_Status.State,
							body->reply.ACK.Query_Stream_Enc_Status.State == 0 ? "\"Stream does not exist\"" :
							body->reply.ACK.Query_Stream_Enc_Status.State == 1 ? "\"Stream not active\"" :
							body->reply.ACK.Query_Stream_Enc_Status.State == 2 ? "\"Stream active\"" :
							body->reply.ACK.Query_Stream_Enc_Status.State == 3 ? "\"Stream error\"" : "",
							body->reply.ACK.Query_Stream_Enc_Status.Repeater_Present,
							body->reply.ACK.Query_Stream_Enc_Status.Encryption_Enabled,
							body->reply.ACK.Query_Stream_Enc_Status.Auth_Completed,
							body->reply.ACK.Query_Stream_Enc_Status.zeros
						);
						
						iprintf("Unauthorizable_Device_Present=%d Legacy_Device_Present=%d Query_Capable_Device_Present=%d hdcp_1x_device_present=%d Hdcp_2x_Device_Present=%d zeros=%d Reply_Signed=%d , Stream_Id=%d",
							body->reply.ACK.Query_Stream_Enc_Status.Unauthorizable_Device_Present,
							body->reply.ACK.Query_Stream_Enc_Status.Legacy_Device_Present,
							body->reply.ACK.Query_Stream_Enc_Status.Query_Capable_Device_Present,
							body->reply.ACK.Query_Stream_Enc_Status.hdcp_1x_device_present,
							body->reply.ACK.Query_Stream_Enc_Status.Hdcp_2x_Device_Present,
							body->reply.ACK.Query_Stream_Enc_Status.zeros2,
							body->reply.ACK.Query_Stream_Enc_Status.Reply_Signed,
							body->reply.ACK.Query_Stream_Enc_Status.Stream_Id
						);
						msg3parseEnd = (Sideband_MSG3 *)&body->reply.ACK.Query_Stream_Enc_Status.end;
						
						// see Appendix I of DisplayPort 1.2 spec
						break;
					}
					default: {
						break;
					}
				} // switch body->reply.Request_Identifier
				break;
			} // case DP_SIDEBAND_REPLY_ACK
			default:
				msg3parseEnd = NULL;
		} // switch body->reply.Reply_Type
	} // reply
	
	if (msg3parseEnd < msg3) {
		cprintf(" (stopped parsing %d bytes before the expected end)", (int)((size_t)msg3 - (size_t)msg3parseEnd));
	}
	else if (msg3parseEnd > msg3) {
		cprintf(" (stopped parsing %d bytes after the expected end)", (int)((size_t)msg3parseEnd - (size_t)msg3));
	}
} // DumpOneDisplayPortMessageBody



int mst_get_message_sotm(Sideband_MSG1 *msg1) {
	Sideband_MSG2 *msg2 = (Sideband_MSG2 *)((uint8_t*)&msg1->header.Relative_Address + msg1->header.Link_Count_Total / 2);
	return msg2->header.Start_Of_Message_Transaction;
}



int mst_get_message_eotm(Sideband_MSG1 *msg1) {
	Sideband_MSG2 *msg2 = (Sideband_MSG2 *)((uint8_t*)&msg1->header.Relative_Address + msg1->header.Link_Count_Total / 2);
	return msg2->header.End_Of_Message_Transaction;
}



int mst_get_message_length(Sideband_MSG1 *msg1) {
	Sideband_MSG2 *msg2 = (Sideband_MSG2 *)((uint8_t*)&msg1->header.Relative_Address + msg1->header.Link_Count_Total / 2);
	return (int)((uint8_t*)&msg2->body - (uint8_t*)msg1) + msg2->header.Sideband_MSG_Body_Length;
}



void mst_set_message_sequence(Sideband_MSG1 *msg1) {
	Sideband_MSG2 *msg2 = (Sideband_MSG2 *)((uint8_t*)&msg1->header.Relative_Address + msg1->header.Link_Count_Total / 2);
	msg2->header.Message_Sequence_No = gMessageSequenceNumber;
	msg2->header.Sideband_MSG_Header_CRC = crc4((uint8_t*)msg1, ((uint8_t*)&msg2->body - (uint8_t*)&msg1->header) * 2 - 1);
}



void mst_set_message_path_message(Sideband_MSG1 *msg1) {
	Sideband_MSG2 *msg2 = (Sideband_MSG2 *)((uint8_t*)&msg1->header.Relative_Address + msg1->header.Link_Count_Total / 2);
	msg2->header.Path_Message = 1;
	msg2->header.Sideband_MSG_Header_CRC = crc4((uint8_t*)msg1, ((uint8_t*)&msg2->body - (uint8_t*)&msg1->header) * 2 - 1);
}



UInt8* mst_get_message_body(Sideband_MSG1 *msg1) {
	Sideband_MSG2 *msg2 = (Sideband_MSG2 *)((uint8_t*)&msg1->header.Relative_Address + msg1->header.Link_Count_Total / 2);
	return (UInt8*)&msg2->body;
}



UInt8 mst_get_message_request_identifier(Sideband_MSG1 *msg1) {
	Sideband_MSG2 *msg2 = (Sideband_MSG2 *)((uint8_t*)&msg1->header.Relative_Address + msg1->header.Link_Count_Total / 2);
	return msg2->body.request.Request_Identifier;
}



bool mst_get_use_sideband_property(io_service_t ioFramebufferService)
{
	static UInt32 useSidebandProperty = 0;
	static bool gotUseSidebandProperty = false;
	if (!gotUseSidebandProperty) {
		IofbSetAttributeForService(ioFramebufferService, 'iofb', 'sbnd', 0, 0, &useSidebandProperty);
		gotUseSidebandProperty = true;
	}
	return useSidebandProperty;
}



IOReturn dp_dpcd_read(IOI2CConnectRef i2cconnect, int dpcdAddr, int dpcdLength, void *dpcdDest) {
	IOReturn result;
	IOI2CRequest_10_6 sendrequest;
	bzero(&sendrequest, sizeof(sendrequest));
	bzero(dpcdDest, dpcdLength);
	sendrequest.replyTransactionType = kIOI2CDisplayPortNativeTransactionType;
	sendrequest.replyAddress = dpcdAddr;
	sendrequest.replyBuffer = (vm_address_t) dpcdDest;
	sendrequest.replyBytes = dpcdLength;
	result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &sendrequest);
	if (!result) result = sendrequest.result;
	return result;
}



IOReturn dp_dpcd_write(IOI2CConnectRef i2cconnect, int dpcdAddr, int dpcdLength, void *dpcdSource) {
	IOReturn result;
	IOI2CRequest_10_6 sendrequest;
	bzero(&sendrequest, sizeof(sendrequest));
	sendrequest.sendTransactionType = kIOI2CDisplayPortNativeTransactionType;
	sendrequest.sendAddress = dpcdAddr;
	sendrequest.sendBuffer = (vm_address_t) dpcdSource;
	sendrequest.sendBytes = dpcdLength;
	result = UniversalI2CSendRequest(i2cconnect, kNilOptions, &sendrequest);
	if (!result) result = sendrequest.result;
	return result;
}

IOReturn dp_messages_write(IOI2CConnectRef i2cconnect, UInt8 *reqMsgs, int reqMsgsLength) {
	IOReturn result = kIOReturnSuccess;
	int reqMsgLength;
	int dpcdLength;
    int dpcdAddr;
    int intMsgRemaining;
	for ( ; !result && reqMsgsLength && (reqMsgLength = mst_get_message_length((Sideband_MSG1 *)reqMsgs)) <= reqMsgsLength; reqMsgsLength -= reqMsgLength) {
		mst_set_message_sequence((Sideband_MSG1 *)reqMsgs);
		for (
			dpcdAddr = DP_SIDEBAND_MSG_DOWN_REQ_BASE, intMsgRemaining = reqMsgLength;
			!result && (dpcdLength = intMsgRemaining > 16 ? 16 : intMsgRemaining) > 0;
			dpcdAddr += dpcdLength, intMsgRemaining -= dpcdLength, reqMsgs += dpcdLength
		) {
			result = dp_dpcd_write(i2cconnect, dpcdAddr, dpcdLength, reqMsgs);
			if (gDumpSidebandMessage & kReq) {
                char resultStr[40];
				DumpOneDisplayPortMessage(reqMsgs, dpcdLength, dpcdAddr);
                cprintf("%s\n", DumpOneReturn(resultStr, sizeof(resultStr), result));
			}
		}
	}
	return result;
} // dp_messages_write



void mst_encode_header(
	UInt8 *inpath,
	int pathLength,
	UInt32 msg2size,
	UInt32 *lenResult,
	Sideband_MSG1 **msg1Result,
	Sideband_MSG2 **msg2Result,
	Sideband_MSG3 **msg3Result
) {
    // If pathLength is 0, then do normal DisplayPort (not mst sideband messages - there is no port or RAD).
    // If pathLength is 1, The first number in the path is the port number. The RAD is empty.
    // If pathLength is 2 or more, then the RAD lists 1 or more ports.
    
    if (msg1Result) *msg1Result = NULL;
	if (msg2Result) *msg2Result = NULL;
	if (msg3Result) *msg3Result = NULL;
	if (lenResult) *lenResult = 0;
	
	if (pathLength > 15 || (pathLength && !inpath))
		return;
	
	UInt32 len = (UInt32)offsetof(Sideband_MSG1, header.Relative_Address) + msg2size + pathLength / 2 + (UInt32)offsetof(Sideband_MSG3, end);
	Sideband_MSG1 *msg1 = (Sideband_MSG1 *)malloc(len);
	if (!msg1)
		return;
	Sideband_MSG2 *msg2 = (Sideband_MSG2 *)((uint8_t*)&msg1->header.Relative_Address + pathLength / 2); // pathlength 1 & 2 = 1 byte, pathlength 3 & 4 = 2 bytes, ... (since path[0] is port number)
	Sideband_MSG3 *msg3 = (Sideband_MSG3 *)((uint8_t*)&msg2->header + msg2size);
    
    //iprintf("len:%d pathlength:%d msg2size:%d msg2:%ld msg2body:%ld msg3:%ld\n", len, pathLength, msg2size, (void*)msg2 - (void*)msg1, (void*)&msg2->body - (void*)msg1, (void*)msg3 - (void*)msg1);

	msg1->header.Link_Count_Total = pathLength;
	msg1->header.Link_Count_Remaining = pathLength - 1;
	bzero(&msg1->header.Relative_Address, pathLength / 2);
	for (int i = 0; i < pathLength - 1; i++) {
		msg1->header.Relative_Address[i / 2] |= inpath[i + 1] << ((i & 1) ? 0 : 4);
	}
	msg2->header.Broadcast_Message = 0;
	msg2->header.Path_Message = 0;
	msg2->header.Sideband_MSG_Body_Length = (uint8_t*)&msg3->end - (uint8_t*)&msg2->body; // the CRC is part of the MSG_Body_Length
	
	msg2->header.Start_Of_Message_Transaction = 1;
	msg2->header.End_Of_Message_Transaction = 1;
	msg2->header.zero = 0;
	msg2->header.Message_Sequence_No = 0;
	msg2->header.Sideband_MSG_Header_CRC = crc4((uint8_t*)msg1, ((uint8_t*)&msg2->body - (uint8_t*)&msg1->header) * 2 - 1);
	
	msg2->body.request.zero = 0;
	msg2->body.request.Request_Identifier = 0;

	if (msg1Result) *msg1Result = msg1;
	if (msg2Result) *msg2Result = msg2;
	if (msg3Result) *msg3Result = msg3;
	if (lenResult) *lenResult = len;
} // mst_encode_header



UInt8 *mst_encode_link_address(UInt8 *inpath, int pathLength, UInt32 *msgLength)
{
	UInt32 len;
	Sideband_MSG1 *msg1;
	Sideband_MSG2 *msg2;
	Sideband_MSG3 *msg3;
	mst_encode_header(inpath, pathLength, (UInt32)offsetof(Sideband_MSG2, body.request.Default.end), &len, &msg1, &msg2, &msg3);
	if (msg1) {
		msg2->body.request.Request_Identifier = DP_LINK_ADDRESS;
		msg3->body.Sideband_MSG_Body_CRC = crc8((uint8_t*)&msg2->body, msg2->header.Sideband_MSG_Body_Length - 1);
		if (msgLength) *msgLength = len;
	}
	return (UInt8*)msg1;
}



UInt8 *mst_encode_dpcd_read(UInt8 *inpath, int pathLength, UInt32 *msgLength, int dpcdAddr, int numBytes)
{
	if (numBytes > 16)
		return NULL;
	UInt32 len;
	Sideband_MSG1 *msg1;
	Sideband_MSG2 *msg2;
	Sideband_MSG3 *msg3;
	mst_encode_header(inpath, pathLength, (UInt32)offsetof(Sideband_MSG2, body.request.Remote_DPCD_Read.end), &len, &msg1, &msg2, &msg3);
	if (msg1) {
		msg2->body.request.Request_Identifier = DP_REMOTE_DPCD_READ;
		msg2->body.request.Remote_DPCD_Read.Port_Number = inpath[0];
		msg2->body.request.Remote_DPCD_Read.DPCD_Address = dpcdAddr;
		msg2->body.request.Remote_DPCD_Read.Number_Of_Bytes_To_Read = numBytes;
		msg2->body.request.Remote_DPCD_Read.raw = CFSwapInt32HostToBig(msg2->body.request.Remote_DPCD_Read.raw);
		msg3->body.Sideband_MSG_Body_CRC = crc8((uint8_t*)&msg2->body, msg2->header.Sideband_MSG_Body_Length - 1);
		if (msgLength) *msgLength = len;
	}
	return (UInt8*)msg1;
} // mst_encode_dpcd_read



UInt8 *mst_encode_enum_path_resources(UInt8 *inpath, int pathLength, UInt32 *msgLength)
{
	UInt32 len;
	Sideband_MSG1 *msg1;
	Sideband_MSG2 *msg2;
	Sideband_MSG3 *msg3;
	mst_encode_header(inpath, pathLength, (UInt32)offsetof(Sideband_MSG2, body.request.Enum_Path_Resources.end), &len, &msg1, &msg2, &msg3);
	if (msg1) {
		mst_set_message_path_message(msg1);
		msg2->body.request.Request_Identifier = DP_ENUM_PATH_RESOURCES;
		msg2->body.request.Enum_Path_Resources.Port_Number = inpath[0];
		msg2->body.request.Enum_Path_Resources.zeros = 0;
		msg3->body.Sideband_MSG_Body_CRC = crc8((uint8_t*)&msg2->body, msg2->header.Sideband_MSG_Body_Length - 1);
		if (msgLength) *msgLength = len;
	}
	return (UInt8*)msg1;
} // mst_encode_enum_path_resources



IOReturn mst_message_read(
	IOI2CConnectRef i2cconnect,
	UInt8 *part,
	DpError *dperr
) {
	IOReturn result;
	if (dperr) *dperr = dpNoError;

	int partlen = 0;
	int partremain = 16;

	do {
		int replysize = partremain;
		if (replysize > 16) replysize = 16;

		result = dp_dpcd_read(i2cconnect, DP_SIDEBAND_MSG_DOWN_REP_BASE + partlen, replysize, &part[partlen]);
		if (result) {
			if (dperr) *dperr = dpErrReadMsgDownRep;
			break;
		}

		if (partlen == 0) {
			partremain = mst_get_message_length((Sideband_MSG1 *)part);
			if (replysize > partremain) {
				replysize = partremain;
			}
		}

        if (gDumpSidebandMessage & kRep) {
            char resultStr[40];
            DumpOneDisplayPortMessage(&part[partlen], replysize, DP_SIDEBAND_MSG_DOWN_REP_BASE + partlen);
            cprintf("%s\n", DumpOneReturn(resultStr, sizeof(resultStr), result));
        }

		partlen += replysize;
		partremain -= replysize;
        //usleep(10000);

	} while (partremain);

	return result;
} // mst_message_read



IOReturn mst_messages_read(
	io_service_t ioFramebufferService,
	IOI2CConnectRef i2cconnect,
	UInt8 expected_request_identifier,
	Sideband_MSG_Body **bodyResult,
    int *bodyLength,
	DpError *dperr
) {
	IOReturn result = kIOReturnSuccess;
	if (dperr) *dperr = dpNoError;
	int wholelen = 0; // length of all the reply sideband messages (usually 48 bytes each except the last one)
	Sideband_MSG1 *repmsg1First = NULL;
    if (bodyResult) *bodyResult = NULL;
    if (bodyLength) *bodyLength = 0;
    int replylength = 0;

	UInt8 *replydata = (UInt8 *)malloc(300);
	UInt8 *whole = (UInt8 *)malloc(300);
	if (!whole) {
		result = kIOReturnNoMemory;
		goto done;
	}
	if (!replydata) {
		result = kIOReturnNoMemory;
		goto done;
	}
	
    // 4 seconds is needed for LINK_ADDRESS NAK (with bad path)
	for (int poll = 0; !result && !repmsg1First && (poll < 4 * 1000000 / kDelayDisplayPortSidebandReply); poll++) {
		if (poll) {
			usleep(kDelayDisplayPortSidebandReply);
		}
		
		if (mst_get_use_sideband_property(ioFramebufferService)) {
			// get all the sideband messages
			CFDataRef data = (CFDataRef)IORegistryEntryCreateCFProperty(ioFramebufferService, CFSTR("sideband"), kCFAllocatorDefault, kNilOptions);
			if (!data) {
				continue;
			}
			wholelen = (int)CFDataGetLength(data);
			memcpy(whole, CFDataGetBytePtr(data), wholelen);
			repmsg1First = (Sideband_MSG1 *)whole;
			if (data) {
				CFRelease(data);
				data = NULL;
			}
			break;
		}
		
		// read interrupt status
		UInt8 interruptsStatus = 0;
		result = dp_dpcd_read(i2cconnect, kDPRegisterServiceIRQ, 1, &interruptsStatus);
		if (result) {
			if (dperr) *dperr = dpErrReadServiceIrq;
			break;
		}
		
		if (!(interruptsStatus & DP_DOWN_REP_MSG_RDY)) {
			continue;
		}
		
		UInt8 part[80];
		result = mst_message_read(i2cconnect, part, dperr);

		if (!result) {
			if (mst_get_message_sotm((Sideband_MSG1 *)part)) {
				wholelen = 0;
			}

			int partlen = mst_get_message_length((Sideband_MSG1 *)part);
			memcpy(&whole[wholelen], part, partlen);
			wholelen += partlen;

			if (mst_get_message_eotm((Sideband_MSG1 *)part)) {
				repmsg1First = (Sideband_MSG1 *)whole;
			}
		}
		
		interruptsStatus = DP_DOWN_REP_MSG_RDY;
		IOReturn interruptResult = dp_dpcd_write(i2cconnect, kDPRegisterServiceIRQ, 1, &interruptsStatus);
		if (interruptResult) {
			if (dperr) *dperr = dpErrResetServiceIrq;
		}

		if (repmsg1First) {
			result = kIOReturnSuccess;
		}
	} // for poll
		
	if (repmsg1First) {
		Sideband_MSG1 *repmsg1 = NULL;
		Sideband_MSG2 *repmsg2 = NULL;
		Sideband_MSG3 *repmsg3 = NULL;
		repmsg1 = repmsg1First;

		while (wholelen) {
			repmsg2 = (Sideband_MSG2 *)((uint8_t*)&repmsg1->header.Relative_Address + repmsg1->header.Link_Count_Total / 2);
			repmsg3 = (Sideband_MSG3 *)((uint8_t*)&repmsg2->body + repmsg2->header.Sideband_MSG_Body_Length - 1);
			int msglength = (int)((uint8_t*)repmsg3 + 1 - (uint8_t*)repmsg1);
            UInt8 crc = crc8((uint8_t*)&repmsg2->body, repmsg2->header.Sideband_MSG_Body_Length - 1);

            if ((gDumpSidebandMessage & kRep) && mst_get_use_sideband_property(ioFramebufferService)) {
                char resultStr[40];
                DumpOneDisplayPortMessage((UInt8 *)repmsg1, msglength, DP_SIDEBAND_MSG_DOWN_REP_BASE);
                cprintf("%s\n", DumpOneReturn(resultStr, sizeof(resultStr), result));
            }

			DpError reperr = dpNoError;
			if ((replylength == 0) && (repmsg2->body.reply.Reply_Type == DP_SIDEBAND_REPLY_NAK))
				reperr = dpErrNak;
			if ((replylength == 0) && (repmsg2->body.reply.Request_Identifier != expected_request_identifier))
				reperr = dpErrIdentifierMismatch;
			if ((replylength == 0) && (repmsg2->header.Message_Sequence_No != gMessageSequenceNumber))
				reperr = dpErrSequenceMismatch;
			if ((replylength == 0) && (repmsg2->header.Start_Of_Message_Transaction == 0))
				reperr = dpErrFirstMsgIsNotStart;
			if ((replylength >  0) && (repmsg2->header.Start_Of_Message_Transaction == 1))
				reperr = dpErrStartIsNotFirstMsg;
			if ((msglength == wholelen) && (repmsg2->header.End_Of_Message_Transaction == 0))
				reperr = dpErrLastMsgIsNotEnd;
			if ((msglength <  wholelen) && (repmsg2->header.End_Of_Message_Transaction == 1))
				reperr = dpErrEndIsNotLastMsg;
			if ((replylength >  0) && memcmp(repmsg1, repmsg1First, (uint8_t*)repmsg2 - (uint8_t*)repmsg1))
				reperr = dpErrPathMismatch;
			if ((crc != repmsg3->body.Sideband_MSG_Body_CRC))
				reperr = dpErrCrc;
			
			if (reperr)
			{
				if (gDumpSidebandMessage & kRep) {
					iprintf("Problem with sideband reply message%s.\n", DpErrorStr(reperr));
				}
				if (dperr) *dperr = reperr;
				result = kIOReturnError;
				break;
			}
			
			memcpy(&replydata[replylength], (void*)&repmsg2->body, repmsg2->header.Sideband_MSG_Body_Length - 1);
			replylength += repmsg2->header.Sideband_MSG_Body_Length - 1;
			wholelen -= msglength;
			repmsg1 = (Sideband_MSG1 *)((uint8_t*)repmsg3 + 1);
		} // while message
		
	} // if repmsg1First

	if (!result && !repmsg1First) {
		if (dperr) *dperr = dpErrReplyTimeout;
		result = kIOReturnError;
	}

done:
	if (whole) {
		free(whole);
	}
	if (result && replydata) {
		free(replydata);
	}
	if (!result) {
		if (bodyResult) *bodyResult = (Sideband_MSG_Body *)replydata;
        if (bodyLength) *bodyLength = replylength;
	}
	return result;
} // mst_messages_read



IOReturn mst_transaction(
	io_service_t ioFramebufferService,
	IOI2CConnectRef i2cconnect,
	UInt8 *reqMsgs,
	int reqMsgsLength,
	Sideband_MSG_Body **bodyResult,
	int *bodyLength,
	DpError *dperr
) {
    DpError dperrLocal = dpNoError;
	if (bodyResult) *bodyResult = NULL;
    IOReturn result = kIOReturnSuccess;
    for (int attempt = 0; attempt < 3; attempt++) {
        gMessageSequenceNumber ^= 1;
        result = dp_messages_write(i2cconnect, reqMsgs, reqMsgsLength);
        if (result) {
            if (dperr) dperrLocal = dpErrWriteMsgDownReq;
        }
        else {
            result = mst_messages_read(ioFramebufferService, i2cconnect, mst_get_message_request_identifier((Sideband_MSG1 *)reqMsgs), bodyResult, bodyLength, &dperrLocal);
        }
        if (!result || dperrLocal != dpErrCrc) {
            break;
        }
    }
    if (dperr) *dperr = dperrLocal;
	return result;
} // mst_transaction



IOReturn mst_req_dpcd_read(
	io_service_t ioFramebufferService,
	IOI2CConnectRef i2cconnect,
	UInt8 *inpath,
	int pathLength,
	int dpcdAddr,
	int dpcdLength,
	void *dpcdDest,
	DpError *dperr
) {
	IOReturn result;
	if (dperr) *dperr = dpNoError;
	if (!pathLength) {
		result = dp_dpcd_read(i2cconnect, dpcdAddr, dpcdLength, dpcdDest);
		if (result) {
			if (dperr) *dperr = dpErrReadDpcd;
		}
		return result;
	}
	Sideband_MSG_Body *body;
	UInt32 reqMsgLength;
	UInt8 *reqMsg = mst_encode_dpcd_read(inpath, pathLength, &reqMsgLength, dpcdAddr, dpcdLength);
	if (!reqMsg) {
		return kIOReturnBadArgument;
	}
	result = mst_transaction(ioFramebufferService, i2cconnect, reqMsg, reqMsgLength, &body, NULL, dperr);
	if (result == kIOReturnSuccess) {
		memcpy(dpcdDest, &body->reply.ACK.Remote_DPCD_Read.Data_Read, body->reply.ACK.Remote_DPCD_Read.Number_Of_Bytes_Read);
		free(body);
	}
	free(reqMsg);
	return result;
} // mst_req_dpcd_read



IOReturn mst_req_enum_path_resources(
	io_service_t ioFramebufferService,
	IOI2CConnectRef i2cconnect,
	UInt8 *inpath,
	int pathLength,
	Sideband_MSG_Body **bodyResult,
	int *bodyLength,
	DpError *dperr
) {
	IOReturn result;
	if (bodyResult) *bodyResult = NULL;
	if (bodyLength) *bodyLength = 0;
	if (dperr) *dperr = dpNoError;
	UInt32 reqMsgLength;
	UInt8 *reqMsg = mst_encode_enum_path_resources(inpath, pathLength, &reqMsgLength);
	if (!reqMsg) {
		return kIOReturnBadArgument;
	}
	result = mst_transaction(ioFramebufferService, i2cconnect, reqMsg, reqMsgLength, bodyResult, bodyLength, dperr);
	free(reqMsg);
	return result;
} // mst_req_enum_path_resources



IOReturn mst_req_link_address(
	io_service_t ioFramebufferService,
	IOI2CConnectRef i2cconnect,
	UInt8 *inpath,
	int pathLength,
	Sideband_MSG_Body **bodyResult,
	int *bodyLength,
	DpError *dperr
) {
	IOReturn result;
	if (bodyResult) *bodyResult = NULL;
	if (bodyLength) *bodyLength = 0;
	if (dperr) *dperr = dpNoError;
	UInt32 reqMsgLength;
	UInt8 *reqMsg = mst_encode_link_address(inpath, pathLength, &reqMsgLength);
	if (!reqMsg) {
		return kIOReturnBadArgument;
	}
	result = mst_transaction(ioFramebufferService, i2cconnect, reqMsg, reqMsgLength, bodyResult, bodyLength, dperr);
	free(reqMsg);
	return result;
} // mst_req_link_address



const char *DpErrorStr(DpError dperr) {
	switch (dperr) {
		case dpNoError: return "";
		case dpErrReadMsgDownRep: return " dpErrReadMsgDownRep";
		case dpErrReadServiceIrq: return " dpErrReadServiceIrq";
		case dpErrResetServiceIrq: return " dpErrResetServiceIrq";
		case dpErrNak: return " dpErrNak";
		case dpErrIdentifierMismatch: return " dpErrIdentifierMismatch";
		case dpErrSequenceMismatch: return " dpErrSequenceMismatch";
		case dpErrFirstMsgIsNotStart: return " dpErrFirstMsgIsNotStart";
		case dpErrStartIsNotFirstMsg: return " dpErrStartIsNotFirstMsg";
		case dpErrLastMsgIsNotEnd: return " dpErrLastMsgIsNotEnd";
		case dpErrEndIsNotLastMsg: return " dpErrEndIsNotLastMsg";
		case dpErrPathMismatch: return " dpErrPathMismatch";
		case dpErrCrc: return " dpErrCrc";
		case dpErrReplyTimeout: return " dpErrReplyTimeout";
		case dpErrWriteMsgDownReq: return " dpErrWriteMsgDownReq";
		case dpErrReadDpcd: return " dpErrReadDpcd";
		default: return " ?dpErrUnknown";
	}
}
