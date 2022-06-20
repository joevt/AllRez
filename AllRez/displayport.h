//
//  displayport.h
//  AllRez
//
//  Created by joevt on 2022-05-20.
//

#ifndef mst_h
#define mst_h

#include <CoreFoundation/CFBase.h>
#include <sys/_types/_guid_t.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <IOKit/i2c/IOI2CInterface.h>

typedef enum {
	dpNoError,
	dpErrReadMsgDownRep,
	dpErrReadServiceIrq,
	dpErrResetServiceIrq,
	dpErrNak,
	dpErrIdentifierMismatch,
	dpErrSequenceMismatch,
	dpErrFirstMsgIsNotStart,
	dpErrStartIsNotFirstMsg,
	dpErrLastMsgIsNotEnd,
	dpErrEndIsNotLastMsg,
	dpErrPathMismatch,
	dpErrCrc,
	dpErrReplyTimeout,
	dpErrWriteMsgDownReq,
	dpErrReadDpcd
} DpError;

const char *DpErrorStr(DpError dperr);

/*
All fields are defined from most-significant bit and most significant byte first.
*/

#define PACKED __attribute__((packed)) __attribute__((aligned(1)))

//typedef uint8_t guid_t[16];


typedef struct I2C_Transaction {
	UInt8 Write_I2C_Device_Identifier : 7; // LSB
	UInt8 zero : 1; // MSB
	UInt8 Number_Of_Bytes_To_Write;
	UInt8 I2C_Data_To_Write[];
	// I2C_Transaction2
} I2C_Transaction;

typedef struct I2C_Transaction2 {
	UInt8 I2C_Transaction_Delay : 4; // LSB
	UInt8 No_Stop_Bit : 1;
	UInt8 zeros : 3; // MSB
	UInt8 end[];
} I2C_Transaction2;

typedef struct Remote_I2C_Read2 {
	UInt8 Read_I2C_Device_Identifier : 7; // LSB
	UInt8 zero : 1; // MSB
	UInt8 Number_Of_Bytes_To_Read;
	UInt8 end[];
} Remote_I2C_Read2;

typedef struct Sink_Event_Notify2 {
	// byte alligned after Relative_Address
	UInt8 Sink_Event;
	UInt8 end[];
} Sink_Event_Notify2;

typedef struct Link_Address_Port {
	UInt8 Port_Number : 4; // LSB
	UInt8 Peer_Device_Type : 3;
	UInt8 Input_Port : 1; // MSB
	union {
		struct {
			UInt8 zeros : 5; // LSB;
			UInt8 Legacy_Device_Plug_Status : 1;
			UInt8 DisplayPort_Device_Plug_Status : 1;
			UInt8 Messaging_Capability_Status : 1; // MSB
			
			UInt8 Dpcd_Revision_Minor : 4; // LSB
			UInt8 Dpcd_Revision_Major : 4; // MSB

			PACKED guid_t Peer_Guid;

			UInt8 Number_SDP_Stream_Sinks : 4; // LSB
			UInt8 Number_SDP_Streams : 4; // MSB
			UInt8 end[];
		} Output;
		struct {
			UInt8 zeros : 6; // LSB
			UInt8 DisplayPort_Device_Plug_Status : 1;
			UInt8 Messaging_Capability_Status : 1; // MSB
			UInt8 end[];
		} Input;
	};
} Link_Address_Port;

typedef struct Message_Transaction_Request {
	UInt8 Request_Identifier : 7; // LSB
	UInt8 zero : 1; // MSB
	PACKED union { // Request_Data
		struct {
			UInt8 zeros : 4; // LSB
			UInt8 Port_Number : 4; // MSB
			
			PACKED guid_t Global_Unique_Identifier;

			UInt8 Peer_Device_Type : 3; // LSB
			UInt8 Input_Port : 1;
			UInt8 Messaging_Capability_Status : 1;
			UInt8 DisplayPort_Device_Plug_Status : 1;
			UInt8 Legacy_Device_Plug_status : 1;
			UInt8 zero : 1; // MSB
			UInt8 end[];
		} Connection_Status_Notify;
		struct {
			UInt8 zeros : 4; // LSB
			UInt8 Port_Number : 4; // MSB
			UInt8 end[];
		} Enum_Path_Resources;
		struct {
			UInt8 Number_SDP_Streams : 4; // LSB
			UInt8 Port_Number : 4; // MSB

			UInt8 Virtual_Channel_Payload_Identifier : 7; // LSB
			UInt8 zero : 1; // MSB

			PACKED UInt16 Payload_Bandwidth_Number; // big-endian

			UInt8 SDP_Stream_Sink[8]; // Number_SDP_Streams nibbles
		} Allocate_Payload;
		struct {
			UInt8 zeros : 4; // LSB
			UInt8 Port_Number : 4; // MSB
			UInt8 Virtual_Channel_Payload_Identifier : 7; // LSB
			UInt8 zero : 1; // MSB
			UInt8 end[];
		} Query_Payload;
		struct {
			UInt8 zeros : 4; // LSB
			UInt8 Port_Number : 4; // MSB

			PACKED guid_t Global_Unique_Identifier;

			PACKED UInt16 Available_PBN; // big-endian
			UInt8 end[];
		} Resource_Status_Notify;
		union {
			struct {
				UInt32 Number_Of_Bytes_To_Read : 8; // LSB of 32 bits
				UInt32 DPCD_Address : 20;
				UInt32 Port_Number : 4; // MSB of 32 bits
				UInt8 end[];
			}; // big-endian
			UInt32 raw; // big-endian
		} Remote_DPCD_Read;
		union {
			struct {
				UInt32 Number_Of_Bytes_To_Write : 8; // LSB of 32 bits
				UInt32 DPCD_Address : 20;
				UInt32 Port_Number : 4; // MSB of 32 bits
				UInt8 Write_Data[];
			}; // big-endian
			UInt32 raw; // big-endian
		} Remote_DPCD_Write;
		struct {
			// DisplayPort 1.2 spec page 263 has correct order but incorrect sizes.
			// DisplayPort 1.2 spec page 293 has incorrect order but correct sizes.
			UInt8 Number_Of_I2C_Write_Transactions : 2; // LSB
			UInt8 zeros : 2;
			UInt8 Port_Number : 4; // MSB
			I2C_Transaction Transactions[];
			// Remote_I2C_Read2;
		} Remote_I2C_Read;
		struct {
			UInt8 zeros : 4; // LSB
			UInt8 Port_Number : 4; // MSB
			UInt8 Write_I2C_Device_Identifier : 7; // LSB
			UInt8 zero : 1; // MSB
			UInt8 Number_Of_Bytes_To_Write;
			UInt8 I2C_Data_To_Write[];
		} Remote_I2C_Write;
		struct {
			UInt8 Port_Number : 4; // LSB
			UInt8 zeros : 4; // MSB
			UInt8 end[];
		} Power_Up_PHY;
		struct {
			UInt8 Port_Number : 4; // LSB
			UInt8 zeros : 4; // MSB
			UInt8 end[];
		} Power_Down_PHY;
		struct {
			UInt8 Link_Count : 4; // LSB
			UInt8 zeros : 4; // MSB
			UInt8 Relative_Address[8];
			// Sink_Event_Notify2;
		} Sink_Event_Notify;
		struct {
			UInt8 end;
		} Default;
	};
} Message_Transaction_Request;

typedef struct Message_Transaction_Reply {
	UInt8 Request_Identifier : 7; // LSB copy of same in Request Message Transaction
	UInt8 Reply_Type : 1; // MSB DP_SIDEBAND_REPLY_ACK=0 or DP_SIDEBAND_REPLY_NAK=1
	PACKED union { // Reply_Data
		struct {
			guid_t Global_Unique_Identifier;
			UInt8 Reason_For_NAK; // DP_NAK_INVALID_READ should actually be renamed DP_NAK_INVALID_RAD
			UInt8 NAK_Data;
#if 0
			REMOTE_DPCD__WRITE_Nak_Reply() {
				UInt8 Request_Type : 7; // LSB
				UInt8 Reply_Type : 1; // MSB
				UInt8 Port_Number : 4; // LSB
				UInt8 zeros : 4; // MSB
				UInt8 Reason_For_Nak;
				UInt8 Number_Of_Bytes_Written_Before_Failure;
			}
 
			REMOTE_I2C_READ_Nak_Reply() {
				UInt8 Request_Type : 7; // LSB
				UInt8 Reply_Type : 1; // MSB
				UInt8 Size_Of_RAD : 4; // LSB
				UInt8 Downstream Port Number : 4; // MSB
				UInt8 Remaining_RAD_To_Target[8];
			REMOTE_I2C_READ_Nak_Reply2()
				UInt8 Reason_For_Nak;
				UInt8 I2C_NAK_Transaction;
			}
#endif
			UInt8 end[];
		} NAK; // 1
		union {
			struct {
				guid_t Global_Unique_Identifier;
				UInt8 Number_Of_Ports : 4; // LSB
				UInt8 zeros : 4; // MSB
				Link_Address_Port Ports[]; // variable size
			} Link_Address;
			struct {
				UInt8 FEC_Capable : 1; // LSB
				UInt8 zeros : 3;
				UInt8 Port_Number : 4; // MSB

				PACKED UInt16 Payload_Bandwidth_Number; // big-endian
				PACKED UInt16 Available_PBN; // big-endian
				UInt8 end[];
			} Enum_Path_Resources;
			struct {
				UInt8 zeros : 4; // LSB
				UInt8 Port_Number : 4; // MSB

				UInt8 Virtual_Channel_Payload_Identifier : 7; // LSB
				UInt8 zero : 1; // MSB
				
				PACKED UInt16 Payload_Bandwidth_Number; // big-endian
				UInt8 end[];
			} Allocate_Payload;
			struct {
				UInt8 zeros : 4; // LSB
				UInt8 Port_Number : 4; // MSB
				PACKED UInt16 Allocated_PBN; // big-endian
				UInt8 end[];
			} Query_Payload;
			struct {
				UInt8 Port_Number : 4; // LSB
				UInt8 zeros : 4; // MSB

				UInt8 Number_Of_Bytes_Read;
				UInt8 Data_Read[];
			} Remote_DPCD_Read;
			struct {
				UInt8 Port_Number : 4; // LSB
				UInt8 zeros : 4; // MSB
				UInt8 end[];
			} Remote_DPCD_Write;
			struct {
				UInt8 Downstream_Port_Number : 4; // LSB
				UInt8 zeros : 4; // MSB
				UInt8 Number_Of_Bytes_Read;
				UInt8 Data_Read[];
			} Remote_I2C_Read;
			struct {
				UInt8 Port_Number : 4; // LSB
				UInt8 zeros : 4; // MSB
				UInt8 end[];
			} Remote_I2C_Write;
			struct {
				UInt8 zeros : 4; // LSB
				UInt8 Port_Number : 4; // MSB
				UInt8 end[];
			} Power_Up_PHY;
			struct {
				UInt8 zeros : 4; // LSB
				UInt8 Port_Number : 4; // MSB
				UInt8 end[];
			} Power_Down_PHY;
			struct {
				// taken from linux drm_dp_mst_topology.c which doesn't match DisplayPort 1.2 spec.
				UInt8 zeros : 3; // LSB
				UInt8 Auth_Completed : 1;
				UInt8 Encryption_Enabled : 1;
				UInt8 Repeater_Present : 1;
				UInt8 State : 2; // MSB

				UInt8 Reply_Signed : 1; // LSB
				UInt8 zeros2 : 2;
				UInt8 Hdcp_2x_Device_Present : 1;
				UInt8 hdcp_1x_device_present : 1;
				UInt8 Query_Capable_Device_Present : 1;
				UInt8 Legacy_Device_Present : 1;
				UInt8 Unauthorizable_Device_Present : 1; // MSB

				UInt8 Stream_Id;
				UInt8 end[];
			} Query_Stream_Enc_Status;
			struct {
				UInt8 end;
			} Default;
		} ACK; // 0
	};
} Message_Transaction_Reply;

typedef struct Sideband_MSG1 {
	struct Sideband_MSG_Header {
		UInt8 Link_Count_Remaining : 4; // LSB init to Link_Count_Total
		UInt8 Link_Count_Total : 4; // MSB

		UInt8 Relative_Address[8]; // 16 nibbles nibble[0] is MSB of byte[0] : nibbles = Link_Count_Total
	} header;
	// Sideband_MSG2
}  Sideband_MSG1;

typedef union {
	Message_Transaction_Request request;
	Message_Transaction_Reply reply;
} Sideband_MSG_Body;

typedef struct Sideband_MSG2 {
	struct Sideband_MSG_Header2 {
		// byte alligned after Relative_Address
		UInt8 Sideband_MSG_Body_Length : 6; // LSB
		UInt8 Path_Message : 1;
		UInt8 Broadcast_Message : 1; // MSB

		UInt8 Sideband_MSG_Header_CRC : 4; // LSB
		UInt8 Message_Sequence_No : 1; // bit(4)
		UInt8 zero : 1; // bit(5)
		UInt8 End_Of_Message_Transaction : 1; // bit(6)
		UInt8 Start_Of_Message_Transaction : 1; // MSB
	} header;

	Sideband_MSG_Body body;
	// Sideband_MSG3
} Sideband_MSG2;

typedef struct Sideband_MSG3 {
	struct Sideband_MSG_Body_2 {
		UInt8 Sideband_MSG_Body_CRC;
	} body;
	UInt8 end[];
} Sideband_MSG3;


enum {
	kReq = (1 << 0),
	kRep = (1 << 1),
};
extern int gDumpSidebandMessage;

void DumpOneDisplayPortMessage(UInt8 *msgbytes, int msglen, UInt32 dpcdAddress);
void DumpOneDisplayPortMessageBody(void *bodyData, int bodyLength, bool isReply);

UInt8* mst_get_message_body(Sideband_MSG1 *msg1);
UInt8 mst_get_message_request_identifier(Sideband_MSG1 *msg1);

UInt8* mst_encode_link_address       (UInt8 *inpath, int pathLength, UInt32 *msgLength);
UInt8 *mst_encode_enum_path_resources(UInt8 *inpath, int pathLength, UInt32 *msgLength);
UInt8* mst_encode_dpcd_read          (UInt8 *inpath, int pathLength, UInt32 *msgLength, int dpcdAddr, int numBytes);

IOReturn mst_req_link_address       (io_service_t ioFramebufferService, IOI2CConnectRef i2cconnect, UInt8 *inpath, int pathLength,     Sideband_MSG_Body **bodyResult, int *bodyLength, DpError *dperr);
IOReturn mst_req_enum_path_resources(io_service_t ioFramebufferService, IOI2CConnectRef i2cconnect, UInt8 *inpath, int pathLength,     Sideband_MSG_Body **bodyResult, int *bodyLength, DpError *dperr);
IOReturn mst_req_dpcd_read          (io_service_t ioFramebufferService, IOI2CConnectRef i2cconnect, UInt8 *inpath, int pathLength, int dpcdAddr, int dpcdLength, void *dpcdDest       , DpError *dperr);
IOReturn mst_transaction            (io_service_t ioFramebufferService, IOI2CConnectRef i2cconnect, UInt8 *reqMsgs, int reqMsgsLength, Sideband_MSG_Body **bodyResult, int *bodyLength, DpError *dperr);

IOReturn dp_dpcd_read (IOI2CConnectRef i2cconnect, int dpcdAddr, int dpcdLength, void *dpcdDest);
IOReturn dp_dpcd_write(IOI2CConnectRef i2cconnect, int dpcdAddr, int dpcdLength, void *dpcdSource);

#ifdef __cplusplus
}
#endif

#endif /* mst_h */
