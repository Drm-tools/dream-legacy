/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Andrea Russo, Doyle Richard
 *
 * Description:
 *	DAB MOT interface implementation
 *
 * 12/22/2003 Doyle Richard
 *	- Header extension decoding
 * 10/13/2005 Andrea Russo
 *	- Broadcast WebSite application
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "DABMOT.h"


/* Implementation *************************************************************/
/******************************************************************************\
* Encoder                                                                      *
\******************************************************************************/
void CMOTDABEnc::SetMOTObject(CMOTObject& NewMOTObject)
{
	int				i;
	CMOTObjectRaw	MOTObjectRaw;

	/* Get some necessary parameters of object */
	const int iPicSizeBits = NewMOTObject.vecbRawData.Size();
	const int iPicSizeBytes = iPicSizeBits / SIZEOF__BYTE;
	const string strFileName = NewMOTObject.strName;

	/* File name size is restricted (in this implementation) to 128 (2^7) bytes.
	   If file name is longer, cut it. TODO: better solution: set Ext flag in
	   "ContentName" header extension to allow larger file names */
	int iFileNameSize = strFileName.size();
	if (iFileNameSize > 128)
		iFileNameSize = 128;

	/* Copy actual raw data of object */
	MOTObjectRaw.Body.vecbiData.Init(iPicSizeBits);
	MOTObjectRaw.Body.vecbiData = NewMOTObject.vecbRawData;

	/* Get content type and content sub type of object. We use the format string
	   to get these informations about the object */
	int iContentType = 0; /* Set default value (general data) */
	int iContentSubType = 0; /* Set default value (gif) */

	/* Get ending string which declares the type of the file. Make lowercase */

// The following line is not working for Linux!!!! TODO!
#ifdef _WIN32
	const string strFormat = _strlwr(_strdup(NewMOTObject.strFormat.c_str()));
#else
	const string strFormat = NewMOTObject.strFormat;
#endif



	/* gif: 0, image: 2 */
	if (strFormat.compare("gif") == 0)
	{
		iContentType = 2;
		iContentSubType = 0;
	}

	/* jfif: 1, image: 2. Possible endings: jpg, jpeg, jfif */
	if ((strFormat.compare("jpg") == 0) ||
		(strFormat.compare("jpeg") == 0) ||
		(strFormat.compare("jfif") == 0))
	{
		iContentType = 2;
		iContentSubType = 1;
	}

	/* bmp: 2, image: 2 */
	if (strFormat.compare("bmp") == 0)
	{
		iContentType = 2;
		iContentSubType = 2;
	}

	/* png: 3, image: 2 */
	if (strFormat.compare("png") == 0)
	{
		iContentType = 2;
		iContentSubType = 3;
	}


	/* Header --------------------------------------------------------------- */
	/* Header size (including header extension) */
	const int iHeaderSize = 7 /* Header core  */ +
		5 /* TriggerTime */ +
		3 + iFileNameSize /* ContentName (header + actual name) */ +
		2 /* VersionNumber */;

	/* Allocate memory and reset bit access */
	MOTObjectRaw.Header.vecbiData.Init(iHeaderSize * SIZEOF__BYTE);
	MOTObjectRaw.Header.vecbiData.ResetBitAccess();

	/* BodySize: This 28-bit field, coded as an unsigned binary number,
	   indicates the total size of the body in bytes */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) iPicSizeBytes, 28);

	/* HeaderSize: This 13-bit field, coded as an unsigned binary number,
	   indicates the total size of the header in bytes */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) iHeaderSize, 13);

	/* ContentType: This 6-bit field indicates the main category of the body's
	   content */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) iContentType, 6);

	/* ContentSubType: This 9-bit field indicates the exact type of the body's
	   content depending on the value of the field ContentType */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) iContentSubType, 9);


	/* Header extension ----------------------------------------------------- */
	/* MOT Slideshow application: Only the MOT parameter ContentName is
	   mandatory and must be used for each slide object that will be handled by
	   the MOT decoder and the memory management of the Slide Show terminal */

	/* TriggerTime: This parameter specifies the time for when the presentation
	   takes place. The TriggerTime activates the object according to its
	   ContentType. The value of the parameter field is coded in the UTC
	   format */

	/* PLI (Parameter Length Indicator): This 2-bit field describes the total
	   length of the associated parameter. In this case:
	   1 0 total parameter length = 5 bytes; length of DataField is 4 bytes */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 2, 2);

	/* ParamId (Parameter Identifier): This 6-bit field identifies the
	   parameter. 1 0 1 (dec: 5) -> TriggerTime */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 5, 6);

	/* Validity flag = 0: "Now", MJD and UTC shall be ignored and be set to 0.
	   Set MJD and UTC to zero. UTC flag is also zero -> short form */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 0, 32);



	/* VersionNumber: If several versions of an object are transferred, this
	   parameter indicates its VersionNumber. The parameter value is coded as an
	   unsigned binary number, starting at 0 and being incremented by 1 modulo
	   256 each time the version changes. If the VersionNumber differs, the
	   content of the body was modified */
	/* PLI
	   0 1 total parameter length = 2 bytes, length of DataField is 1 byte */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 1, 2);

	/* ParamId (Parameter Identifier): This 6-bit field identifies the
	   parameter. 1 1 0 (dec: 6) -> VersionNumber */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 6, 6);

	/* Version number data field */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 0, 8);



	/* ContentName: The DataField of this parameter starts with a one byte
	   field, comprising a 4-bit character set indicator (see table 3) and a
	   4-bit Rfa field. The following character field contains a unique name or
	   identifier for the object. The total number of characters is determined
	   by the DataFieldLength indicator minus one byte */

	/* PLI
	   1 1 total parameter length depends on the DataFieldLength indicator */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 3, 2);

	/* ParamId (Parameter Identifier): This 6-bit field identifies the
	   parameter. 1 1 0 0 (dec: 12) -> ContentName */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 12, 6);

	/* Ext (ExtensionFlag): This 1-bit field specifies the length of the
	   DataFieldLength Indicator.
	   0: the total parameter length is derived from the next 7 bits */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 0, 1);

	/* DataFieldLength Indicator: This field specifies as an unsigned binary
	   number the length of the parameter's DataField in bytes. The length of
	   this field is either 7 or 15 bits, depending on the setting of the
	   ExtensionFlag */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) (1 /* header */ +
		iFileNameSize /* actual data */), 7);

	/* Character set indicator (0 0 0 0 complete EBU Latin based repertoire) */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 0, 4);

	/* Rfa 4 bits */
	MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) 0, 4);

	/* Character field */
	for (i = 0; i < iFileNameSize; i++)
		MOTObjectRaw.Header.vecbiData.Enqueue((uint32_t) strFileName[i], 8);


	/* Generate segments ---------------------------------------------------- */
	/* Header (header should not be partitioned! TODO) */
const int iPartiSizeHeader = 100; /* Bytes */ // TEST

	PartitionUnits(MOTObjectRaw.Header.vecbiData, MOTObjSegments.vvbiHeader,
		iPartiSizeHeader);

	/* Body */
const int iPartiSizeBody = 100; /* Bytes */ // TEST

	PartitionUnits(MOTObjectRaw.Body.vecbiData, MOTObjSegments.vvbiBody,
		iPartiSizeBody);
}

void CMOTDABEnc::PartitionUnits(CVector<_BINARY>& vecbiSource,
								CVector<CVector<_BINARY> >& vecbiDest,
								const int iPartiSize)
{
	int i, j;
	int iActSegSize;

	/* Divide the generated units in partitions */
	const int iSourceSize = vecbiSource.Size() / SIZEOF__BYTE;
	const int iNumSeg =
		(int) ceil((_REAL) iSourceSize / iPartiSize); /* Bytes */
	const int iSizeLastSeg = iSourceSize -
		(int) floor((_REAL) iSourceSize / iPartiSize) * iPartiSize;

	/* Init memory for destination vector, reset bit access of source */
	vecbiDest.Init(iNumSeg);
	vecbiSource.ResetBitAccess();

	for (i = 0; i < iNumSeg; i++)
	{
		/* All segments except the last one must have the size
		   "iPartSizeHeader". If "iSizeLastSeg" is =, the source data size is
		   a multiple of the partitions size. In this case, all units have
		   the same size (-> "|| (iSizeLastSeg == 0)") */
		if ((i < iNumSeg - 1) || (iSizeLastSeg == 0))
			iActSegSize = iPartiSize;
		else
			iActSegSize = iSizeLastSeg;


		/* Add segment data ------------------------------------------------- */
		/* Header */
		/* Allocate memory for body data and segment header bits (16) */
		vecbiDest[i].Init(iActSegSize * SIZEOF__BYTE + 16);
		vecbiDest[i].ResetBitAccess();

		/* Segment header */
		/* RepetitionCount: This 3-bit field indicates, as an unsigned
		   binary number, the remaining transmission repetitions for the
		   current object.
		   In our current implementation, no repetitions used. TODO */
		vecbiDest[i].Enqueue((uint32_t) 0, 3);

		/* SegmentSize: This 13-bit field, coded as an unsigned binary
		   number, indicates the size of the segment data field in bytes */
		vecbiDest[i].Enqueue((uint32_t) iActSegSize, 13);


		/* Body */
		for (j = 0; j < iActSegSize * SIZEOF__BYTE; j++)
			vecbiDest[i].Enqueue(vecbiSource.Separate(1), 1);
	}
}

void CMOTDABEnc::GenMOTObj(CVector<_BINARY>& vecbiData,
						   CVector<_BINARY>& vecbiSeg, const _BOOLEAN bHeader,
						   const int iSegNum, const int iTranspID,
						   const _BOOLEAN bLastSeg)
{
	int		i;
	CCRC	CRCObject;
	
	/* Standard settings for this implementation */
	const _BOOLEAN bCRCUsed = TRUE; /* CRC */
	const _BOOLEAN bSegFieldUsed = TRUE; /* segment field */
	const _BOOLEAN bUsAccFieldUsed = TRUE; /* user access field */
	const _BOOLEAN bTransIDFieldUsed = TRUE; /* transport ID field */


// TODO: Better solution!
/* Total length of object in bits */
int iTotLenMOTObj = 16 /* group header */;
if (bSegFieldUsed == TRUE)
	iTotLenMOTObj += 16;
if (bUsAccFieldUsed == TRUE)
{
	iTotLenMOTObj += 8;
	if (bTransIDFieldUsed == TRUE)
		iTotLenMOTObj += 16;
}
iTotLenMOTObj += vecbiSeg.Size();
if (bCRCUsed == TRUE)
	iTotLenMOTObj += 16;


	/* Init data vector */
	vecbiData.Init(iTotLenMOTObj);
	vecbiData.ResetBitAccess();


	/* MSC data group header ------------------------------------------------ */
	/* Extension flag: this 1-bit flag shall indicate whether the extension
	   field is present, or not. Not used right now -> 0 */
	vecbiData.Enqueue((uint32_t) 0, 1);

	/* CRC flag: this 1-bit flag shall indicate whether there is a CRC at the
	   end of the MSC data group */
	if (bCRCUsed == TRUE)
		vecbiData.Enqueue((uint32_t) 1, 1);
	else
		vecbiData.Enqueue((uint32_t) 0, 1);

	/* Segment flag: this 1-bit flag shall indicate whether the segment field is
	   present, or not */
	if (bSegFieldUsed == TRUE)
		vecbiData.Enqueue((uint32_t) 1, 1);
	else
		vecbiData.Enqueue((uint32_t) 0, 1);

	/* User access flag: this 1-bit flag shall indicate whether the user access
	   field is present, or not. We always use this field -> 1 */
	if (bUsAccFieldUsed == TRUE)
		vecbiData.Enqueue((uint32_t) 1, 1);
	else
		vecbiData.Enqueue((uint32_t) 0, 1);

	/* Data group type: this 4-bit field shall define the type of data carried
	   in the data group data field. Data group types:
	   3: MOT header information
	   4: MOT data */
	if (bHeader == TRUE)
		vecbiData.Enqueue((uint32_t) 3, 4);
	else
		vecbiData.Enqueue((uint32_t) 4, 4);

	/* Continuity index: the binary value of this 4-bit field shall be
	   incremented each time a MSC data group of a particular type, with a
	   content different from that of the immediately preceding data group of
	   the same type, is transmitted */
	if (bHeader == TRUE)
	{
		vecbiData.Enqueue((uint32_t) iContIndexHeader, 4);

		/* Increment modulo 16 */
		iContIndexHeader++;
		if (iContIndexHeader == 16)
			iContIndexHeader = 0;
	}
	else
	{
		vecbiData.Enqueue((uint32_t) iContIndexBody, 4);

		/* Increment modulo 16 */
		iContIndexBody++;
		if (iContIndexBody == 16)
			iContIndexBody = 0;
	}

	/* Repetition index: the binary value of this 4-bit field shall signal the
	   remaining number of repetitions of a MSC data group with the same data
	   content, occurring in successive MSC data groups of the same type.
	   No repetition used in this implementation right now -> 0, TODO */
	vecbiData.Enqueue((uint32_t) 0, 4);

	/* Extension field: this 16-bit field shall be used to carry the Data Group
	  Conditional Access (DGCA) when general data or MOT data uses conditional
	  access (Data group types 0010 and 0101, respectively). The DGCA contains
	  the Initialization Modifier (IM) and additional Conditional Access (CA)
	  information. For other Data group types, the Extension field is reserved
	  for future additions to the Data group header.
	  Extension field is not used in this implementation! */


	/* Session header ------------------------------------------------------- */
	/* Segment field */
	if (bSegFieldUsed == TRUE)
	{
		/* Last: this 1-bit flag shall indicate whether the segment number field
		   is the last or whether there are more to be transmitted */
		if (bLastSeg == TRUE)
			vecbiData.Enqueue((uint32_t) 1, 1);
		else
			vecbiData.Enqueue((uint32_t) 0, 1);

		/* Segment number: this 15-bit field, coded as an unsigned binary number
		   (in the range 0 to 32767), shall indicate the segment number.
		   NOTE: The first segment is numbered 0 and the segment number is
		   incremented by one at each new segment */
		vecbiData.Enqueue((uint32_t) iSegNum, 15);
	}

	/* User access field */
	if (bUsAccFieldUsed == TRUE)
	{
		/* Rfa (Reserved for future addition): this 3-bit field shall be
		   reserved for future additions */
		vecbiData.Enqueue((uint32_t) 0, 3);

		/* Transport Id flag: this 1-bit flag shall indicate whether the
		   Transport Id field is present, or not */
		if (bTransIDFieldUsed == TRUE)
			vecbiData.Enqueue((uint32_t) 1, 1);
		else
			vecbiData.Enqueue((uint32_t) 0, 1);

		/* Length indicator: this 4-bit field, coded as an unsigned binary
		   number (in the range 0 to 15), shall indicate the length n in bytes
		   of the Transport Id and End user address fields.
		   We do not use end user address field, only transport ID -> 2 */
		if (bTransIDFieldUsed == TRUE)
			vecbiData.Enqueue((uint32_t) 2, 4);
		else
			vecbiData.Enqueue((uint32_t) 0, 4);

		/* Transport Id (Identifier): this 16-bit field shall uniquely identify
		   one data object (file and header information) from a stream of such
		   objects, It may be used to indicate the object to which the
		   information carried in the data group belongs or relates */
		if (bTransIDFieldUsed == TRUE)
			vecbiData.Enqueue((uint32_t) iTranspID, 16);
	}


	/* MSC data group data field -------------------------------------------- */
	vecbiSeg.ResetBitAccess();

	for (i = 0; i < vecbiSeg.Size(); i++)
		vecbiData.Enqueue(vecbiSeg.Separate(1), 1);


	/* MSC data group CRC --------------------------------------------------- */
	/* The data group CRC shall be a 16-bit CRC word calculated on the data
	   group header, the session header and the data group data field. The
	   generation shall be based on the ITU-T Recommendation X.25.
	   At the beginning of each CRC word calculation, all shift register stage
	   contents shall be initialized to "1". The CRC word shall be complemented
	   (1's complement) prior to transmission */
	if (bCRCUsed == TRUE)
	{
		/* Reset bit access */
		vecbiData.ResetBitAccess();

		/* Calculate the CRC and put it at the end of the segment */
		CRCObject.Reset(16);

		/* "byLengthBody" was defined in the header */
		for (i = 0; i < iTotLenMOTObj / SIZEOF__BYTE - 2 /* CRC */; i++)
			CRCObject.AddByte((_BYTE) vecbiData.Separate(SIZEOF__BYTE));

		/* Now, pointer in "enqueue"-function is back at the same place, 
		   add CRC */
		vecbiData.Enqueue(CRCObject.GetCRC(), 16);
	}
}

_BOOLEAN CMOTDABEnc::GetDataGroup(CVector<_BINARY>& vecbiNewData)
{
	_BOOLEAN bLastSegment;

	/* Init return value. Is overwritten in case the object is ready */
	_BOOLEAN bObjectDone = FALSE;

	if (bCurSegHeader == TRUE)
	{
		/* Check if this is last segment */
		if (iSegmCntHeader == MOTObjSegments.vvbiHeader.Size() - 1)
			bLastSegment = TRUE;
		else
			bLastSegment = FALSE;

		/* Generate MOT object for header */
		GenMOTObj(vecbiNewData, MOTObjSegments.vvbiHeader[iSegmCntHeader],
			TRUE, iSegmCntHeader, iTransportID, bLastSegment);

		iSegmCntHeader++;
		if (iSegmCntHeader == MOTObjSegments.vvbiHeader.Size())
		{
			/* Reset counter for body */
			iSegmCntBody = 0;

			/* Header is ready, transmit body now */
			bCurSegHeader = FALSE;
		}
	}
	else
	{
		/* Check that body size is not zero */
		if (iSegmCntBody < MOTObjSegments.vvbiBody.Size())
		{
			/* Check if this is last segment */
			if (iSegmCntBody == MOTObjSegments.vvbiBody.Size() - 1)
				bLastSegment = TRUE;
			else
				bLastSegment = FALSE;

			/* Generate MOT object for Body */
			GenMOTObj(vecbiNewData, MOTObjSegments.vvbiBody[iSegmCntBody],
				FALSE, iSegmCntBody, iTransportID, bLastSegment);

			iSegmCntBody++;
		}

		if (iSegmCntBody == MOTObjSegments.vvbiBody.Size())
		{
			/* Reset counter for header */
			iSegmCntHeader = 0;

			/* Body is ready, transmit header from next object */
			bCurSegHeader = TRUE;
			iTransportID++;

			/* Signal that old object is done */
			bObjectDone = TRUE;
		}
	}

	/* Return status of object transmission */
	return bObjectDone;
}

_REAL CMOTDABEnc::GetProgPerc() const
{
/*
	Get percentage of processed data of current object.
*/
	const int iTotNumSeg =
		MOTObjSegments.vvbiHeader.Size() + MOTObjSegments.vvbiBody.Size();

	return ((_REAL) iSegmCntBody + (_REAL) iSegmCntHeader) / iTotNumSeg;
}

void CMOTDABEnc::Reset()
{
	/* Reset continuity indices */
	iContIndexHeader = 0;
	iContIndexBody = 0;
	iTransportID = 0;

	/* Init counter for segments */
	iSegmCntHeader = 0;
	iSegmCntBody = 0;

	/* Init flag which shows what is currently generated, header or body */
	bCurSegHeader = TRUE; /* Start with header */

	/* Add a "null object" so that at least one valid object can be processed */
	CMOTObject NullObject;
	SetMOTObject(NullObject);
}


/******************************************************************************\
* Decoder                                                                      *
\******************************************************************************/
_BOOLEAN CMOTDABDec::AddDataGroup(CVector<_BINARY>& vecbiNewData)
{
	int					i, j;
	int					iSegmentNum;
	int					iLenIndicat;
	int					iLenGroupDataField;
	int					iSegmentSize;
	int					iTransportID;

	int					iIndex;
	int					iNumberOfObjects;
	int					iDirectoryExtensionLength;
	CMOTObjectRaw		MOTObjectRawTemp;

	CVector<_BINARY>	vecbiDirectoryExtension; 
	int					iTranspostIdNew;
	CVector<_BINARY>	vecbiHeaderPart;
	int					iHeaderSize;
	int					iBodySize;
	
	_BINARY				biLastFlag;
	_BINARY				biTransportIDFlag;
	CCRC				CRCObject;
	FILE*				pFiBody;
	_BOOLEAN			bCRCOk;

	/* Init return value with "not ready". If not MOT object is ready after
	   adding this new data group, this flag is overwritten with "TRUE" */
	_BOOLEAN bMOTObjectReady = FALSE;

	/* Get length of data unit */
	iLenGroupDataField = vecbiNewData.Size();


	/* CRC check ------------------------------------------------------------ */
	/* We do the CRC check at the beginning no matter if it is used or not
	   since we have to reset bit access for that */
	/* Reset bit extraction access */
	vecbiNewData.ResetBitAccess();

	/* Check the CRC of this packet */
	CRCObject.Reset(16);

	/* "- 2": 16 bits for CRC at the end */
	for (i = 0; i < iLenGroupDataField / SIZEOF__BYTE - 2; i++)
		CRCObject.AddByte((_BYTE) vecbiNewData.Separate(SIZEOF__BYTE));

	bCRCOk = CRCObject.CheckCRC(vecbiNewData.Separate(16));


	/* MSC data group header ------------------------------------------------ */
	/* Reset bit extraction access */
	vecbiNewData.ResetBitAccess();

	/* Extension flag */
	const _BINARY biExtensionFlag = (_BINARY) vecbiNewData.Separate(1);

	/* CRC flag */
	const _BINARY biCRCFlag = (_BINARY) vecbiNewData.Separate(1);

	/* Segment flag */
	const _BINARY biSegmentFlag = (_BINARY) vecbiNewData.Separate(1);

	/* User access flag */
	const _BINARY biUserAccFlag = (_BINARY) vecbiNewData.Separate(1);
	
	/* Data group type */
	const int iDataGroupType = (int) vecbiNewData.Separate(4);

	/* Continuity index (not yet used) */
	vecbiNewData.Separate(4);

	/* Repetition index (not yet used) */
	vecbiNewData.Separate(4);

	/* Extension field (not used) */
	if (biExtensionFlag == TRUE)
		vecbiNewData.Separate(16);


	/* Session header ------------------------------------------------------- */
	/* Segment field */
	if (biSegmentFlag == TRUE)
	{
		/* Last */
		biLastFlag = (_BINARY) vecbiNewData.Separate(1);

		/* Segment number */
		iSegmentNum = (int) vecbiNewData.Separate(15);
	}

	/* User access field */
	if (biUserAccFlag == TRUE)
	{
		/* Rfa (Reserved for future addition) */
		vecbiNewData.Separate(3);

		/* Transport Id flag */
		biTransportIDFlag = (_BINARY) vecbiNewData.Separate(1);

		/* Length indicator */
		iLenIndicat = (int) vecbiNewData.Separate(4);

		/* Transport Id */
		if (biTransportIDFlag == 1)
			iTransportID = (int) vecbiNewData.Separate(16);
			
		/* End user address field (not used) */
		int iLenEndUserAddress;

		if (biTransportIDFlag == 1)
			iLenEndUserAddress = (iLenIndicat - 2) * SIZEOF__BYTE;
		else
			iLenEndUserAddress = iLenIndicat * SIZEOF__BYTE;

		vecbiNewData.Separate(iLenEndUserAddress);
	}


	/* MSC data group data field -------------------------------------------- */
	/* If CRC is not used enter if-block, if CRC flag is used, it must be ok to
	   enter the if-block */
	if ((biCRCFlag == FALSE) || ((biCRCFlag == TRUE) && (bCRCOk == TRUE)))
	{
		/* Segmentation header ---------------------------------------------- */
		/* Repetition count (not used) */
		vecbiNewData.Separate(3);

		/* Segment size */
		iSegmentSize = (int) vecbiNewData.Separate(13);

		/* Get MOT data ----------------------------------------------------- */
		/* Segment number and user access data is needed */
		if ((biSegmentFlag == TRUE) && (biUserAccFlag == TRUE) &&
			(biTransportIDFlag == 1))
		{
			/* Distinguish between header and body */
			/* Header information, i.e. the header core and the header
			   extension, are transferred in Data Group type 3 */
			if (iDataGroupType == 3)
			{
				/* Header --------------------------------------------------- */
				if (iSegmentNum == 0)
				{
					/* Header */
					/* The first segment was received, reset header */
					MOTObjectRaw.Header.Reset();

					/* The first occurrence of a Data Group type 3 containing
					   header information is referred as the beginning of the
					   object transmission. Set new transport ID */
					MOTObjectRaw.iTransportID = iTransportID;

					/* Init flag for header ok */
					MOTObjectRaw.Header.bOK = TRUE;

					/* Add new segment data */
					MOTObjectRaw.Header.Add(vecbiNewData, iSegmentSize,
						iSegmentNum);
				}
				else
				{
					/* Check transport ID */
					if (MOTObjectRaw.iTransportID != iTransportID)
						MOTObjectRaw.Header.Reset();
					else
					{
						/* Sometimes a packet is transmitted more than once,
						   only use packets which are not already received
						   correctly */
						if (MOTObjectRaw.Header.iDataSegNum != iSegmentNum)
						{
							/* Check segment number */
							if (MOTObjectRaw.Header.iDataSegNum + 1 !=
								iSegmentNum)
							{
								/* A packet is missing, reset Header */
								MOTObjectRaw.Header.Reset();
							}
							else
							{
								/* Add data */
								MOTObjectRaw.Header.Add(vecbiNewData,
									iSegmentSize, iSegmentNum);
							}
						}
					}
				}

				/* Test if last flag is active */
				if (biLastFlag == TRUE)
				{
					if (MOTObjectRaw.Header.bOK == TRUE)
					{
						/* This was the last segment and all segments were ok.
						   Mark header as ready */
						MOTObjectRaw.Header.bReady = TRUE;
					}
				}
			}
			else if (iDataGroupType == 4)
			{
				/* Body data segments are transferred in Data Group type 4 */
				/* Body ----------------------------------------------------- */
				MOTObjectRawTemp = MOTObjectRaw;

				/* Find if this TransportID is already in vecMOTCarouselRaw */
				iIndex = FindObjectIntoCarousel(iTransportID);
				if (iIndex < 0)
				{
					/* Object not found in carousel. Check if header was already
					   received and stored in the MOT directory. If it is
					   available, copy it */
					if (GetObjectHeader(iTransportID, MOTObjectRaw))
					{
						MOTObjectRaw.Body.Reset();
						iIndex = AddCarouselObject(MOTObjectRaw);
					}
				}
				else
				{
					/* If ready do not decode it because this part of the
					   directory is ready */
					if (vecMOTCarouselRaw[iIndex].Body.bReady == TRUE)
						iIndex = INVALID_DATA_SEG_NUM;
				}

				if (iIndex >= 0)
					MOTObjectRaw = vecMOTCarouselRaw[iIndex];

				if (iSegmentNum == 0)
				{
					/* The first segment was received, reset body */
					MOTObjectRaw.Body.Reset();

					/* Check transport ID */
					if (MOTObjectRaw.iTransportID == iTransportID)
					{
						/* Init flag for body ok */
						MOTObjectRaw.Body.bOK = TRUE;

						/* Add data */
						MOTObjectRaw.Body.Add(vecbiNewData, iSegmentSize,
							iSegmentNum);
					}
				}
				else
				{
					/* Check transport ID */
					if (MOTObjectRaw.iTransportID != iTransportID)
						MOTObjectRaw.Body.Reset();
					else
					{
						/* Sometimes a packet is transmitted more than once,
						   only use packets which are not already received
						   correctly */
						if (MOTObjectRaw.Body.iDataSegNum != iSegmentNum)
						{
							/* Check segment number */
							if (MOTObjectRaw.Body.iDataSegNum + 1 !=
								iSegmentNum)
							{
								/* A packet is missing, reset Header */
								MOTObjectRaw.Body.Reset();
							}
							else
							{
								/* Add data */
								MOTObjectRaw.Body.Add(vecbiNewData,
									iSegmentSize, iSegmentNum);
							}
						}
					}
				}

				/* Test if last flag is active */
				if (biLastFlag == TRUE)
				{
					if (MOTObjectRaw.Body.bOK == TRUE)
					{
						/* This was the last segment and all segments were ok.
						   Mark body as ready */
						MOTObjectRaw.Body.bReady = TRUE;
					}
				}

				if (iIndex >= 0)
				{
					/* Use MOT object --------------------------------------- */
					/* Test if MOT object is ready */
					if ((MOTObjectRaw.Header.bReady == TRUE) &&
						(MOTObjectRaw.Body.bReady == TRUE))
					{
						DecodeObject(MOTObjectRaw);

						/* Set flag that new object was successfully decoded */
						bMOTObjectReady = TRUE;

						/* Reset raw MOT object and dispose logically */
						vecMOTCarouselRaw[iIndex].Reset();
						vecMOTCarouselRaw[iIndex].bDispose = TRUE;
					}
					else
						vecMOTCarouselRaw[iIndex] = MOTObjectRaw;

					MOTObjectRaw = MOTObjectRawTemp;
					MOTObjectRawTemp.Reset();
				}
			}
			else if (iDataGroupType == 6) /* MOT directory */
			{
				if (iSegmentNum == 0)
				{
					/* The first segment was received, reset body */
					MOTDirectoryRaw.Body.Reset();

					/* The first occurrence of a Data Group type 6 containing
					   header information is referred as the beginning of the
					   object transmission. Set new transport ID */
					MOTDirectoryRaw.iTransportID = iTransportID;

					/* Init flag for body ok */
					MOTDirectoryRaw.Body.bOK = TRUE;

					/* Add new segment data */
					MOTDirectoryRaw.Body.Add(vecbiNewData, iSegmentSize,
						iSegmentNum);
				}
				else
				{
					/* Check transport ID */
					if (MOTDirectoryRaw.iTransportID != iTransportID)
						MOTDirectoryRaw.Body.Reset();
					else
					{
						/* Sometimes a packet is transmitted more than once,
						   only use packets which are not already received
						   correctly */
						if (MOTDirectoryRaw.Body.iDataSegNum != iSegmentNum)
						{
							/* Check segment number */
							if (MOTDirectoryRaw.Body.iDataSegNum + 1 !=
								iSegmentNum)
							{
								/* A packet is missing, reset body */
								MOTDirectoryRaw.Body.Reset();
							}
							else
							{
								/* Add data */
								MOTDirectoryRaw.Body.Add(vecbiNewData,
									iSegmentSize, iSegmentNum);
							}
						}
					}
				}

				/* Test if last flag is active */
				if (biLastFlag == TRUE)
				{
					if (MOTDirectoryRaw.Body.bOK == TRUE)
					{
						/* This was the last segment and all segments were ok.
						   Mark body as ready */
						MOTDirectoryRaw.Body.bReady = TRUE;
					}
				}

				if (MOTDirectoryRaw.Body.bReady == TRUE)
				{
					/* Decode the directory --------------------------------- */
					MOTDirectoryRaw.Body.vecbiData.ResetBitAccess();

					/* Not used */
					MOTDirectoryRaw.Body.vecbiData.Separate(2);

					/* Directory size: not used */
					MOTDirectoryRaw.Body.vecbiData.Separate(30);

					/* Number of objects */
					iNumberOfObjects =
						(int) MOTDirectoryRaw.Body.vecbiData.Separate(16);

					/* Carousel period: not used */
					MOTDirectoryRaw.Body.vecbiData.Separate(24);

					/* Not used */
					MOTDirectoryRaw.Body.vecbiData.Separate(1);

					/* Rfa: not used */
					MOTDirectoryRaw.Body.vecbiData.Separate(2);

					/* Segment size: not used */
					iSegmentSize =
						(int) MOTDirectoryRaw.Body.vecbiData.Separate(13);

					/* Directory extension length */
					iDirectoryExtensionLength =
						(int) MOTDirectoryRaw.Body.vecbiData.Separate(16);

					/* Copy directory extension in separate vector */
					vecbiDirectoryExtension.
						Init(iDirectoryExtensionLength * SIZEOF__BYTE);
					for (i = 0; i < vecbiDirectoryExtension.Size(); i++)
					{
						vecbiDirectoryExtension[i] = (_BINARY) MOTDirectoryRaw.
							Body.vecbiData.Separate(1);
					}

					MOTDirectoryRaw.Header.Add(vecbiDirectoryExtension,
						vecbiDirectoryExtension.Size(), 1);

					vecMOTDirectoryHeaders.Init(0);

					int iEntryNumber = 1;
					while (iEntryNumber <= iNumberOfObjects)
					{
						iTranspostIdNew =
							(int) MOTDirectoryRaw.Body.vecbiData.Separate(16);

						/* Header core */
						const int iHeaderPartSize = 7 * SIZEOF__BYTE;
						vecbiHeaderPart.Init(iHeaderPartSize);
						for (i = 0; i < iHeaderPartSize; i++)
						{
							vecbiHeaderPart[i] = (_BINARY) MOTDirectoryRaw.Body.
								vecbiData.Separate(1);
						}

						vecbiHeaderPart.ResetBitAccess();

						/* Body size is not used */
						iBodySize = (int) vecbiHeaderPart.Separate(28);
						iHeaderSize = (int) vecbiHeaderPart.Separate(13);

						/* Add remaining part */
						const int iTotHeaderSize = iHeaderSize * SIZEOF__BYTE;
						const int iRemPartSize =
							iTotHeaderSize - iHeaderPartSize;

						if (iRemPartSize > 0)
						{
							/* Enlarge vector size first */
							vecbiHeaderPart.Enlarge(iRemPartSize);

							/* Actual copying */
							for (i = iHeaderPartSize; i < iTotHeaderSize; i++)
							{
								vecbiHeaderPart[i] = (_BINARY) MOTDirectoryRaw.
									Body.vecbiData.Separate(1);
							}
						}

						/* Add only transport ID and header data vector */
						CMOTDirectoryHeader	MOTDirectoryHeaderEmpty;
						vecMOTDirectoryHeaders.Add(MOTDirectoryHeaderEmpty);

						vecMOTDirectoryHeaders[iEntryNumber - 1].iTransportID =
							iTranspostIdNew;
						vecMOTDirectoryHeaders[iEntryNumber - 1].Header.
							ResizeAndCopyVector(vecbiHeaderPart);

						iEntryNumber = iEntryNumber + 1;
					}

					iDirectoryTransportId = iTransportID;

					/* Delete in the Carousel old objects not completed */
					_BOOLEAN bFound;
					for (i = 0; i < vecMOTCarouselRaw.Size(); i++)
					{
						bFound = FALSE;
						for (j = 0; j < vecMOTDirectoryHeaders.Size(); j++)
						{
							if (vecMOTDirectoryHeaders[j].iTransportID ==
								vecMOTCarouselRaw[i].iTransportID)
							{
								bFound = TRUE;
							}
						}

						if (bFound == FALSE)
						{
							vecMOTCarouselRaw[i].Reset();
							vecMOTCarouselRaw[i].bDispose = TRUE;
						}
					}
				}
			}


			/* Use MOT object ----------------------------------------------- */
			/* Test if MOT object is ready */
			if ((MOTObjectRaw.Header.bReady == TRUE) &&
				(MOTObjectRaw.Body.bReady == TRUE))
			{
				DecodeObject(MOTObjectRaw);

				/* Set flag that new object was successfully decoded */
				bMOTObjectReady = TRUE;

				/* Reset raw MOT object */
				MOTObjectRaw.Reset();
			}
		}
	}

	/* Return status of MOT object decoding */
	return bMOTObjectReady;
}

void CMOTDABDec::DecodeObject(CMOTObjectRaw& MOTObjectRaw)
{
	int				iCharacterSet;
	int				i;
	int				iDaSiBytes;
	unsigned char	ucParamId;
	unsigned char	ucDatafield;


	/* MOT object must be reset before decoding, because if the contenttype
	   is not specified it remain set the content of the previous file */
	MOTObject.Reset();


	/* Header --------------------------------------------------------------- */
	/* ETSI EN 301 234 */
	MOTObjectRaw.Header.vecbiData.ResetBitAccess();

	/* HeaderSize and BodySize */
	const int iBodySize = (int) MOTObjectRaw.Header.vecbiData.Separate(28);
	const int iHeaderSize = (int) MOTObjectRaw.Header.vecbiData.Separate(13);

	/* 7 bytes for header core */
	int iSizeRec = iHeaderSize - 7;

	/* Content type and content sup-type */
	const int iContentType = (int) MOTObjectRaw.Header.vecbiData.Separate(6);
	const int iContentSubType =
		(int) MOTObjectRaw.Header.vecbiData.Separate(9);

	/* Use all header extension data blocks */
	while (iSizeRec > 0)
	{
		/* PLI (Parameter Length Indicator) */
		int iPLI = (int) MOTObjectRaw.Header.vecbiData.Separate(2);

		ucParamId = (unsigned char)
			MOTObjectRaw.Header.vecbiData.Separate(6);

		switch(iPLI)
		{
		case 0:
			/* Total parameter length = 1 byte; no DataField
			   available */

			/* TODO: Use "ucParamId" */

			iSizeRec -= 1; /* 6 + 2 (PLI) bits */
			break;

		case 1:
			/* Total parameter length = 2 bytes, length of DataField
			   is 1 byte */
			ucDatafield = (unsigned char)
				MOTObjectRaw.Header.vecbiData.Separate(8);

			/* Version */
			if (ucParamId == 6)
				MOTObject.iVersion = (int) ucDatafield;

			/* Compression Type */
			if (ucParamId == 17)
				MOTObject.iCompressionType = (int) ucDatafield;

			iSizeRec -= 2; /* 6 + 8 + 2 (PLI) bits */
			break;

		case 2:
			/* Total parameter length = 5 bytes; length of DataField
			   is 4 bytes */
			for (i = 0; i < 4; i++)
			{
				ucDatafield = (unsigned char)
					MOTObjectRaw.Header.vecbiData.Separate(8);

				/* TODO: Use information in data field */
			}
			iSizeRec -= 5; /* 6 + 4 * 8 + 2 (PLI) bits */
			break;

		case 3:
			/* Total parameter length depends on the DataFieldLength
			   indicator (the maximum parameter length is
			   32770 bytes) */
			iSizeRec -= 1; /* 2 (PLI) + 6 bits */

			/* Ext (ExtensionFlag): This 1-bit field specifies the
			   length of the DataFieldLength Indicator and is coded
			   as follows:
			   - 0: the total parameter length is derived from the
					next 7 bits;
			   - 1: the total parameter length is derived from the
					next 15 bits */
			unsigned char ucExt = (unsigned char)
				MOTObjectRaw.Header.vecbiData.Separate(1);

			int iDataFieldLen = 0;

			/* Get data field length */
			if (ucExt == 0)
			{
				iDataFieldLen = (int)
					MOTObjectRaw.Header.vecbiData.Separate(7);

				iSizeRec -= 1;
			}
			else
			{
				iDataFieldLen = (int)
					MOTObjectRaw.Header.vecbiData.Separate(15);
				
				iSizeRec -= 2;
			}

			if (iDataFieldLen > 0)
			{
				if (ucParamId == 6) /* Version */
				{
					MOTObject.iVersion = (int) MOTObjectRaw.Header.vecbiData.
						Separate(SIZEOF__BYTE);
				}

				if (ucParamId == 12) /* Content Name */
				{
					/* Only use "0 0 0 0 complete EBU Latin based repertoire"
					   character set */
					iCharacterSet = MOTObjectRaw.Header.vecbiData.Separate(4);

					/* rfa not used */
					MOTObjectRaw.Header.vecbiData.Separate(4);

					if (iCharacterSet == 0)
					{
						MOTObject.strName = ExtractString(
							MOTObjectRaw.Header.vecbiData, iDataFieldLen - 1);
					}
				}

				if (ucParamId == 15) /* Content Description */
				{
					/* Only use "0 0 0 0 complete EBU Latin based repertoire"
					   character set */
					iCharacterSet = MOTObjectRaw.Header.vecbiData.Separate(4);

					/* rfa not used */
					MOTObjectRaw.Header.vecbiData.Separate(4);

					if (iCharacterSet == 0)
					{
						MOTObject.strContentDescription = ExtractString(
							MOTObjectRaw.Header.vecbiData, iDataFieldLen - 1);
					}
				}

				if (ucParamId == 16) /* Mime Type */
				{
					MOTObject.strMimeType = ExtractString(
						MOTObjectRaw.Header.vecbiData, iDataFieldLen);
				}

				if (ucParamId == 17) /* Compression Type */
				{
					MOTObject.iCompressionType = (int) MOTObjectRaw.Header.
						vecbiData.Separate(iDataFieldLen * SIZEOF__BYTE);
				}
			}
			break;
		}
	}

	BinaryVecToByteVec(MOTObjectRaw.Body.vecbiData, MOTObject.vecbRawData);

	/* Take care of zip compression */
	if (IsZipped(MOTObjectRaw.Body.vecbiData) /* ||
		(MOTObject.iCompressionType == 1) */ )
	{
#ifdef HAVE_LIBFREEIMAGE
		CVector<_BYTE> vecbRawDataOut;
		CVector<_BYTE> vecbRawDataIn;

		BinaryVecToByteVec(MOTObjectRaw.Body.vecbiData, vecbRawDataIn);

		/* Extract the original file size */
		const unsigned long dest_len = gzGetOriginalSize(vecbRawDataIn);

		if (dest_len < MAX_DEC_NUM_BYTES_ZIP_DATA)
		{
			vecbRawDataOut.Init(dest_len);

			/* Actual decompression call */
			const int zerr = FreeImage_ZLibGUnzip(&vecbRawDataOut[0],
				dest_len, &vecbRawDataIn[0], vecbRawDataIn.Size());

			if (zerr > 0)
			{
				/* Copy data */
				MOTObject.vecbRawData.Init(zerr);
				MOTObject.vecbRawData = vecbRawDataOut;
			}
			else
				MOTObject.vecbRawData.Init(0);
		}
		else
			MOTObject.vecbRawData.Init(0);
#else
		/* Can't unzip so change the filename */
		MOTObject.strName = MOTObject.strName + ".gz";
#endif
	}


	/* Body ----------------------------------------------------------------- */
	switch(iContentType)
	{
		case 1: /* text */
			switch (iContentSubType)
			{
				case 2: /* html */
					MOTObject.strFormat = "html";
					break;

				default:
					MOTObject.strFormat = "text";
					break;
			}
			break;

		case 2: /* image */
			/* Check range of content sub type */
			if (iContentSubType < 4)
			{
				/* Set up MOT picture --------------------------------------- */
				/* Set format */
				switch (iContentSubType)
				{
				case 0: /* gif */
					MOTObject.strFormat = "gif";
					break;

				case 1: /* jfif */
					MOTObject.strFormat = "jpeg";
					break;

				case 2: /* bmp */
					MOTObject.strFormat = "bmp";
					break;

				case 3: /* png */
					MOTObject.strFormat = "png";
					break;

				default:
					MOTObject.strFormat = "";
					break;
				}
			}
			else
				MOTObject.vecbRawData.Init(0);
		break;
	}
}

string CMOTDABDec::ExtractString(CVector<_BINARY>& vecbiData,
								 const int iDataLen)
{
	string strVar = "";
	for (int i = 0; i < iDataLen; i++)
	{
		/* Get character and append it to string */
		char cNewChar = (char) vecbiData.Separate(SIZEOF__BYTE);
		strVar.append(&cNewChar, 1);
	}

	return strVar;
}

void CMOTDABDec::BinaryVecToByteVec(CVector<_BINARY>& vecbiIn,
									CVector<_BYTE>& vecbReturn)
{
	/* Data size in bytes */
	const int iSizeOfBytes = vecbiIn.Size() / SIZEOF__BYTE;
	
	/* Copy data */
	vecbReturn.Init(iSizeOfBytes);
	vecbiIn.ResetBitAccess();
	for (int i = 0; i < iSizeOfBytes; i++)
		vecbReturn[i] = (_BYTE) vecbiIn.Separate(SIZEOF__BYTE);
}

_BOOLEAN CMOTDABDec::IsZipped(CVector<_BINARY>& vecbiData)
{
/*
	Check if the header file is a gzip header

	see GZIP file format specification
	http://www.ietf.org/rfc/rfc1952.txt
*/
	CVector<_BYTE> byHeader(3);

	vecbiData.ResetBitAccess();
	for (int i = 0; i < 3; i++)
		byHeader[i] = (_BYTE) vecbiData.Separate(SIZEOF__BYTE);

	/* Check for gzip header [31, 139, 8] */
	if ((byHeader[0] == 31) && (byHeader[1] == 139) && (byHeader[2] == 8))
		return TRUE;
	else
		return FALSE;
}

unsigned int CMOTDABDec::gzGetOriginalSize(const CVector<_BYTE>& vecbData)
{
/*
	Get the original size from a gzip file
	last 4 bytes contains the original file size (ISIZE)

	see GZIP file format specification
	http://www.ietf.org/rfc/rfc1952.txt
*/
	CVector<_BYTE> byHeader(4);

	const int iLastByte = vecbData.Size() - 1;

	byHeader[0] = vecbData[iLastByte - 3];
	byHeader[1] = vecbData[iLastByte - 2];
	byHeader[2] = vecbData[iLastByte - 1];
	byHeader[3] = vecbData[iLastByte];

	return byHeader[0] + (byHeader[1] << 8) + (byHeader[2] << 16) +
		(byHeader[3] << 24);
}

_BOOLEAN CMOTDABDec::GetObjectHeader(const int iTransportID,
									 CMOTObjectRaw& MOTObjectRaw)
{
	_BOOLEAN bFound = FALSE;
	int i = 0;

	while ((i < vecMOTDirectoryHeaders.Size()) && (bFound == FALSE))
	{
		if (vecMOTDirectoryHeaders[i].iTransportID == iTransportID)
		{
			MOTObjectRaw.iTransportID = iTransportID;
			MOTObjectRaw.Header.Reset();
			MOTObjectRaw.Header.ResizeAndCopyVector(
				vecMOTDirectoryHeaders[i].Header.vecbiData);

			MOTObjectRaw.Header.bOK = TRUE;
			MOTObjectRaw.Header.bReady = TRUE;

			bFound = TRUE; 
		}
		else
			i++;
	}

	return bFound;
}

int CMOTDABDec::FindObjectIntoCarousel(const int iTransportID)
{
	int iResult = INVALID_DATA_SEG_NUM;
	int i = 0;

	while ((i < vecMOTCarouselRaw.Size()) && (iResult < 0))
	{
		if (vecMOTCarouselRaw[i].iTransportID == iTransportID)
			iResult = i;
		else
			i++;
	}

	return iResult;
}

int CMOTDABDec::AddCarouselObject(CMOTObjectRaw& MOTObjectRaw)
{
	int iIndex = INVALID_DATA_SEG_NUM;
	int i = 0;

	/* First search for object marked as "dispose" */
	while ((i < vecMOTCarouselRaw.Size()) && (iIndex < 0))
	{
		if (vecMOTCarouselRaw[i].bDispose == TRUE)
			iIndex = i;
		else
			i++;
	}

	/* If now object for disposal found, create new one */
	if (iIndex < 0)
	{
		vecMOTCarouselRaw.Add(MOTObjectRaw);
		iIndex = vecMOTCarouselRaw.Size() - 1;
	}
	else
		vecMOTCarouselRaw[iIndex] = MOTObjectRaw;

	return iIndex;
}

void CDataUnit::Add(CVector<_BINARY>& vecbiNewData,
					const int iSegmentSize,
					const int iSegNum)
{
	/* Add new data (bit-wise copying) */
	const int iOldDataSize = vecbiData.Size();
	const int iNewEnlSize = iSegmentSize * SIZEOF__BYTE;

	vecbiData.Enlarge(iNewEnlSize);

	/* Read useful bits. We have to use the "Separate()" function since we have
	   to start adding at the current bit position of "vecbiNewData" */
	for (int i = 0; i < iNewEnlSize; i++)
	{
		vecbiData[iOldDataSize + i] =
			(_BINARY) vecbiNewData.Separate(1);
	}

	/* Set new segment number */
	iDataSegNum = iSegNum;
}

void CDataUnit::Reset()
{
	vecbiData.Init(0);
	bOK = FALSE;
	bReady = FALSE;
	iDataSegNum = INVALID_DATA_SEG_NUM;
}

void CDataUnit::ResizeAndCopyVector(const CVector<_BINARY>& vecbiNewData)
{
	vecbiData.Init(vecbiNewData.Size());
	vecbiData = vecbiNewData;
}

void CMOTObjectRaw::Reset()
{
	iTransportID = 0;
	bDispose = FALSE;
	Header.Reset();
	Body.Reset();
}
