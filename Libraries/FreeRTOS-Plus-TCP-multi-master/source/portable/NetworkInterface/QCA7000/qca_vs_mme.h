/*====================================================================*
 *
 *   Copyright (c) 2011, 2012, Qualcomm Atheros Communications Inc.
 *
 *   Permission to use, copy, modify, and/or distribute this software
 *   for any purpose with or without fee is hereby granted, provided
 *   that the above copyright notice and this permission notice appear
 *   in all copies.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *   WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 *   THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 *   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 *   LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 *   NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 *   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *--------------------------------------------------------------------*/

#ifndef __QCA_VS_MME_H__
#define __QCA_VS_MME_H__

#define FIRMWARE_1_1_0_11 "MAC-QCA7000-1.1.0.11"
#define FIRMWARE_1_1_0_01 "MAC-QCA7000-1.1.0.32-01"

// MME EtherType
enum { eEtherTypeMME = 0x88e1 };

// MME Version
enum { eMMVersion0 = 0, eMMVersion1 };

// MME Type - 2 LSB's
enum { eReq = 0x0000, eCnf = 0x0001, eInd = 0x0002, eRsp = 0x0003 };

// MME Type - 3 MSB's
enum { eCC = 0x0000, eCP = 0x2000, eNN = 0x4000, eCM = 0x6000, eMS = 0x8000, eVS = 0xA000 };

enum { 	eMaxMmeData = 1495 } ;  //1518 - 23(Max MME header)
enum { eMMEHeaderSz = 17 };


//#define MME_TYPE_VS_SW_VER_REQ        0xA000
//#define MME_TYPE_VS_SW_VER_CNF        0xA001
//#define MME_TYPE_VS_HST_ACTION_IND    0xA062
//#define MME_TYPE_VS_GET_PROPERTY_REQ  0xA0F8
//#define MME_TYPE_VS_GET_PROPERTY_CNF  0xA0F9


//0xA000    VS_SW_VER (REQ, CNF)
typedef struct VS_SW_VER
{
	enum {
		eSwVerMMType = 0x0000,                 // VS_SW_VER
		eSwVerMMTypeReq = eVS | eSwVerMMType | eReq,  // .Req
		eSwVerMMTypeCnf = eVS | eSwVerMMType | eCnf,  // .Cnf
		eSwVerVerSz = 253,                     // Firmware Version String Size, determined by version.inc
		eSwVerVerSzBootRom = 64                // Bootrom Version String Size
	} tSwVerMME;

	struct SwVerReq
	{
		unsigned char        mOUI[3];        // OUI
		unsigned int         mCookie;        // message ID
	}__attribute__((packed)) SwVerReq;

	struct SwVerCnf
	{
		unsigned char        mOUI[3];        // OUI
		unsigned char        mStatus;        // Success = 0
		unsigned char        mDeviceID;      // DeviceID
		unsigned char        mVersionLen;    // Version String length
		char                 mVersion[eSwVerVerSz];  // Firmware Version String
		unsigned char        res;
		unsigned int         mIdent;         // Identification register value. See ProductInfo.cpp/h
		unsigned int         mSteppingNum;   // Stepping num register value
		unsigned int         mChipSequenceNumber;
		unsigned int         mChipPackage;
		unsigned int         mChipOptions;
	}__attribute__((packed)) SwVerCnf;

	struct SwVerCnfBootRom
	{
		unsigned char        mOUI[3];        // OUI
		unsigned char        mStatus;        // Success = 0
		unsigned char        mDeviceID;      // DeviceID
		unsigned char        mVersionLen;    // Version String length
		char                 mVersion[eSwVerVerSzBootRom];  // Bootrom Version String
		unsigned char        mRSVD;
		unsigned int         mIdent;         // Identification register value
		unsigned int         mSteppingNum;   // Stepping num register value
		unsigned int         mCookie;        // message ID
	}__attribute__((packed)) SwVerCnfBootRom;

	union {
		struct SwVerReq        REQ;
		struct SwVerCnf        CNF;
		struct SwVerCnfBootRom CNFBOOTROM;
	}__attribute__((packed));
}__attribute__((packed)) VS_SW_VER;


//0xA0F8    VS_GET_PROPERTY (REQ, CNF)
typedef struct VS_GET_PROPERTY
{
	enum {
		ePropertyMMType = 0x00F8,            // VS_OP_ATTRIBUTES
		ePropertyMMTypeReq = eVS | ePropertyMMType | eReq,  // .Req
		ePropertyMMTypeCnf = eVS | ePropertyMMType | eCnf   // .Cnf
	} tGetPropertyMMType;

	enum {
		ePropertyFormat_String = 0,
		ePropertyFormat_4byte_Id
	}tPropertyFormat;

	enum
	{
		//Consider them as binary bits. 1st bit  VLAN, 2nd bit TOS, 3rd bit TC
		//it can be combination of these. for example, 3 = TOS and VLAN, 5 = VLAN and TC.
		eOverride_DisableOverride = 0,
		eOverride_VLANPriority = 1,
		eOverride_TOS = 2,
		eOverride_TrafficClass = 4
	}tOverrride;

	enum {
		ePropertyId_Min = 100,               // IDs below 100 are reserved for other pib fields
		ePropertyId_DefaultCap = ePropertyId_Min,
		ePropertyId_TTL,
		ePropertyId_IGMPMLDSetting,
		ePropertyId_OverridePriority,
		ePropertyId_GpioInfo,
		ePropertyId_HostQueueInfo,
		ePropertyId_SLACParms=113,
		ePropertyId_Max
	}tPropertyId;

	enum
	{
		ePropertySuccess = 0x00,
		ePropertyFail = 0x01,
		ePropertyNotSupported = 0x02,
		ePropertyVersionNotSupported = 0x3
	} tStatut;

	enum { ePropertyMaxSize = 1024 } tPropertyMaxSize;

	struct PropertyReq
	{
		unsigned char        mOUI[3];        // OUI
		unsigned int         mCookie;        // message ID
		unsigned char        mOutputFormat;  // desired output format
		unsigned char        mPropertyFormat;  // Property format
		unsigned char        mReserved[2];   // reserved field for proper alignment
		unsigned int         mPropertyVersion;  // property version
		unsigned int         mPropertyStringLength;  // property string length
		char                 mFirstCharacterOfPropertyString;  // first character of property string
	}__attribute__((packed)) PropertyReq;

	struct PropertyCnf
	{
		unsigned char        mOUI[3];        // OUI
		unsigned int         mStatus;        // Success = 0
		unsigned int         mCookie;        // message ID
		unsigned char        mOutputFormat;  // output format
		unsigned char        mReserved[3];   // reserved field for proper alignment
		unsigned int         mPropertyDataLength;  // data formate returned
		unsigned char        mFirstByteOfPropertyData;  // Data Field
	}__attribute__((packed)) PropertyCnf;

	struct GpioInfo
	{
		unsigned int         mGpioState;
		unsigned int         mGpioEnable;
		unsigned int         mStraps;
	}__attribute__((packed)) GpioInfo;

	struct HostQueueInfo
	{
		unsigned char        mMaxQueueSize[4];
		unsigned char        mCurrentQueueStatus[4];
	}__attribute__((packed)) HostQueueInfo;

	union {
		struct PropertyReq   REQ;
		struct PropertyCnf   CNF;
	}__attribute__((packed));

}__attribute__((packed)) VS_GET_PROPERTY;


//0xA100    VS_SET_PROPERTY (REQ, CNF)
typedef struct VS_SET_PROPERTY
{
	enum
	{
		eApplyNow_Bit = 0x01,
		ePersist_Bit = 0x02,
		eReset_Bit = 0x04
	} tBit;

	enum
	{
		eOption_ApplyNow_DoNotPersist = (eApplyNow_Bit),
		eOption_ApplyNow_Persist = (eApplyNow_Bit | ePersist_Bit),
		eOption_DoNotApply_Persist_DoNotReset = (ePersist_Bit),
		eOption_DoNotApply_Persist_Reset = (ePersist_Bit | eReset_Bit)
	}tOption;

	union {
	    struct PropertyReq   REQ;
	    struct PropertyCnf   CNF;
	}__attribute__((packed));

}__attribute__((packed)) VS_SET_PROPERTY;


struct CCMMENTRY
{

	union {                     // MME Payloads
		struct VS_GET_PROPERTY  mVS_GET_PROPERTY;  // Get Property
		struct VS_SET_PROPERTY  mVS_SET_PROPERTY;  // Set Property
		unsigned char           mMaxSizeBuffer[eMaxMmeData];
	}__attribute__((packed));
}__attribute__((packed));


struct CCMMESubFrame_V0
{
	unsigned short           mEtherType;     // IEEE-assigned Ethertype
	unsigned char            mMMV;           // MME version
	unsigned short           mMMTYPE;        // MME type
	struct CCMMENTRY         mMMEntry;       // MME Payloads

}__attribute__((packed));

struct CCMMEFrame
{
	unsigned char            mODA[6];        // Original destination address
	unsigned char            mOSA[6];        // Original source address
	union {
		struct CCMMESubFrame_V0 mRegular_V0;
	}__attribute__((packed));
};


#endif  // end !(__QCA_VS_MME_H___)
