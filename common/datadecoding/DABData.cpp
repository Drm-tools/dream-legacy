/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	DAB data decoding (MOT)
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

#include "DABData.h"


/* Implementation *************************************************************/
void CDABData::AddDataUnit(CVector<_BINARY>& vecbiNewData)
{
	_BINARY		biExtensionFlag;
	_BINARY		biCRCFlag;
	_BINARY		biSegmentFlag;
	_BINARY		biUserAccFlag;
	_BINARY		biLastFlag;
	_BINARY		biTransportIDFlag;
	int			iDataGroupType;
	int			iNewContInd;
	int			iRepitiIndex;
	int			iSegmentNum;
	int			iLenIndicat;
	int			iLenGroupDataField;
	int			iRepetitionCount;
	int			iSegmentSize;
	int			iTransportID;
	int			iOldDataSize;
	int			iContentType;
	int			iContentSubType;
	int			iDaSiBytes;
	CCRC		CRCObject;
	_BOOLEAN	bCRCOk;
	_UINT32BIT	CRC;
	FILE*		pFiBody;

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
	for (int i = 0; i < (iLenGroupDataField / SIZEOF__BYTE) - 2; i++)
		CRCObject.AddByte((_BYTE) vecbiNewData.Separate(SIZEOF__BYTE));

	CRC = CRCObject.GetCRC();

	if (CRC == (_UINT32BIT) vecbiNewData.Separate(16))
		bCRCOk = TRUE;
	else
		bCRCOk = FALSE;


	/* MSC data group header ------------------------------------------------ */
	/* Reset bit extraction access */
	vecbiNewData.ResetBitAccess();

	/* Extension flag */
	biExtensionFlag = (_BINARY) vecbiNewData.Separate(1);

	/* CRC flag */
	biCRCFlag = (_BINARY) vecbiNewData.Separate(1);

	/* Segment flag */
	biSegmentFlag = (_BINARY) vecbiNewData.Separate(1);

	/* User access flag */
	biUserAccFlag = (_BINARY) vecbiNewData.Separate(1);
	
	/* Data group type */
	iDataGroupType = (int) vecbiNewData.Separate(4);

	/* Continuity index */
	iNewContInd = (int) vecbiNewData.Separate(4);

	/* Repetition index */
	iRepitiIndex = (int) vecbiNewData.Separate(4);

	/* Extension field (NOT USED) */
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
		iTransportID = (int) vecbiNewData.Separate(16);
			
		/* End user address field NOT USED */
		vecbiNewData.Separate((iLenIndicat - 2) * SIZEOF__BYTE);
	}


	/* MSC data group data field -------------------------------------------- */
	/* If CRC is not used enter if-block, if CRC flag is used, it must be ok to
	   enter the if-block */
	if ((biCRCFlag == FALSE) || ((biCRCFlag == TRUE) && (bCRCOk == TRUE)))
	{
		/* Segmentation header ---------------------------------------------- */
		/* Repetition count */
		iRepetitionCount = (int) vecbiNewData.Separate(3);

		/* Segment size */
		iSegmentSize = (int) vecbiNewData.Separate(13);


		/* Get MOT data ----------------------------------------------------- */
		/* Segment number and user access data is needed */
		if ((biSegmentFlag == TRUE) && (biUserAccFlag == TRUE) &&
			(biTransportIDFlag == TRUE))
		{

/*
// TEST
static FILE* pFile = fopen("test/v.dat", "w");

fprintf(pFile, "%d %d %d %d %d H:%d B:%d", iDataGroupType, MOTObject.Body.iDataSegNum,
		iSegmentNum, MOTObject.iTransportID, iTransportID,
		MOTObject.Header.vecbiData.Size()/8, MOTObject.Body.vecbiData.Size()/8);

if (MOTObject.Body.bOK == TRUE)
	fprintf(pFile, " bOK:+");
else
	fprintf(pFile, " bOK:-");

if (MOTObject.Body.bReady == TRUE)
	fprintf(pFile, " bReady:+");
else
	fprintf(pFile, " bReady:-");

fprintf(pFile, "\n");

fflush(pFile);
*/

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
					MOTObject.Header.Reset();

					/* The first occurrence of a Data Group type 3 containing
					   header information is referred as the beginning of the
					   object transmission. Set new transport ID */
					MOTObject.iTransportID = iTransportID;

					/* Init flag for header ok */
					MOTObject.Header.bOK = TRUE;

					/* Add new segment data */
					MOTObject.Header.Add(vecbiNewData, iSegmentSize,
						iSegmentNum);
				}
				else
				{
					/* Check segment number and transport ID */
					if ((MOTObject.Header.iDataSegNum + 1 != iSegmentNum) ||
						(MOTObject.iTransportID != iTransportID))
					{
						/* A packet is missing or wrong ID, reset Header */
						MOTObject.Header.Reset();
					}
					else
					{
						/* Add data */
						MOTObject.Header.Add(vecbiNewData, iSegmentSize,
							iSegmentNum);
					}
				}

				/* Test if last flag is active */
				if (biLastFlag == TRUE)
				{
					/* Header */
					if (MOTObject.Header.bOK == TRUE)
						MOTObject.Header.bReady = TRUE;
				}
			}
			else if (iDataGroupType == 4)
			{
				/* Body data segments are transferred in Data Group type 4 */
				/* Body ----------------------------------------------------- */
				if (iSegmentNum == 0)
				{
					/* The first segment was received, reset body */
					MOTObject.Body.Reset();

					/* Check transport ID */
					if (MOTObject.iTransportID == iTransportID)
					{
						/* Init flag for body ok */
						MOTObject.Body.bOK = TRUE;

						/* Add data */
						MOTObject.Body.Add(vecbiNewData, iSegmentSize,
							iSegmentNum);
					}
				}
				else
				{
					/* Check segment number and transport ID */
					if ((MOTObject.Body.iDataSegNum + 1 != iSegmentNum) ||
						(MOTObject.iTransportID != iTransportID))
					{
						/* A packet is missing or wrong ID, reset body */
						MOTObject.Body.Reset();
					}
					else
					{
						/* Add data */
						MOTObject.Body.Add(vecbiNewData, iSegmentSize,
							iSegmentNum);
					}
				}

				/* Test if last flag is active */
				if (biLastFlag == TRUE)
					if (MOTObject.Body.bOK == TRUE)
						MOTObject.Body.bReady = TRUE;
			}


			/* Use MOT object ----------------------------------------------- */
			/* Test if MOT object is ready */
			if ((MOTObject.Header.bReady == TRUE) &&
				(MOTObject.Body.bReady == TRUE))
			{
				/* Get relevant data from header */
				MOTObject.Header.vecbiData.ResetBitAccess();
				MOTObject.Header.vecbiData.Separate(41);

				/* Content type and content sup-type */
				iContentType = (int) MOTObject.Header.vecbiData.Separate(6);
				iContentSubType = (int) MOTObject.Header.vecbiData.Separate(9);

				/* We only use images */
				if (iContentType == 2 /* image */)
				{
					/* Check range of content sub type */
					if (iContentSubType < 4)
					{
						char cFileName[100];

						/* Set correct name */
						switch (iContentSubType)
						{
						case 0: /* gif */
							strcpy(cFileName, "DreamReceivedDataFileTMP.gif");
							break;

						case 1: /* jfif */
							strcpy(cFileName, "DreamReceivedDataFileTMP.jpeg");
							break;

						case 2: /* bmp */
							strcpy(cFileName, "DreamReceivedDataFileTMP.bmp");
							break;

						case 3: /* png */
							strcpy(cFileName, "DreamReceivedDataFileTMP.png");
							break;
						}
						
						/* Now get data from body */
						MOTObject.Body.vecbiData.ResetBitAccess();

						/* Write data in binary file */
						pFiBody = fopen(cFileName, "wb");

						/* Data size in bytes */
						iDaSiBytes =
							MOTObject.Body.vecbiData.Size() / SIZEOF__BYTE;

						for (int i = 0; i < iDaSiBytes;	i++)
						{
								/* Extract one byte of data from stream */
							_BYTE byData =
								(_BYTE) MOTObject.Body.vecbiData.Separate(8);

							/* Write this byte in file */
							fwrite((void*) &byData, size_t(1), size_t(1),
								pFiBody);
						}

						/* Close the file afterwards */
						fclose(pFiBody);

						/* Call application to show the image */
#ifdef _WIN32
						ShellExecute(NULL, "open", cFileName, NULL,NULL,
							SW_SHOWNORMAL);
#endif
					}
				}

				/* Reset MOT object */
				MOTObject.Header.Reset();
				MOTObject.Body.Reset();
			}
		}
	}
}

void CDABData::CDataUnit::Add(CVector<_BINARY>& vecbiNewData, int iSegmentSize,
							  int iSegNum)
{
	/* Add new data (bit-wise copying) */
	int iOldDataSize = vecbiData.Size();

	vecbiData.Enlarge(iSegmentSize * SIZEOF__BYTE);

	/* Read useful bits */
	for (int i = 0; i < iSegmentSize * SIZEOF__BYTE; i++)
		vecbiData[iOldDataSize + i] =
			(_BINARY) vecbiNewData.Separate(1);

	/* Set new segment number */
	iDataSegNum = iSegNum;
}

void CDABData::CDataUnit::Reset()
{
	vecbiData.Init(0);
	bOK = FALSE;
	bReady = FALSE;
	iDataSegNum = -1;
}
