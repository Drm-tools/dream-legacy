/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	MOT Slide Show application
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

#include "MOTSlideShow.h"


/* Implementation *************************************************************/
/******************************************************************************\
* Encoder                                                                      *
\******************************************************************************/
void CMOTSlideShowEncoder::GetDataUnit(CVector<_BINARY>& vecbiNewData)
{
	/* Get new data group from MOT encoder. If the last MOT object was
	   completely transmitted, this functions returns true. In this case, put
	   a new picture to the MOT encoder object */
	if (MOTDAB.GetDataGroup(vecbiNewData) == TRUE)
		AddNextPicture();
}

void CMOTSlideShowEncoder::Init()
{
	/* Reset picutre counter for browsing in the vector of file names. Start
	   with first picture */
	iPictureCnt = 0;

	MOTDAB.Reset();

	AddNextPicture();
}

void CMOTSlideShowEncoder::AddNextPicture()
{
	/* Make sure at least one picture is in container */
	if (vecMOTPicture.Size() > 0)
	{
		/* Get current file name */
		MOTDAB.SetMOTObject(vecMOTPicture[iPictureCnt]);

		/* Set file counter to next picture, test for wrap around */
		iPictureCnt++;
		if (iPictureCnt == vecMOTPicture.Size())
			iPictureCnt = 0;
	}
}

void CMOTSlideShowEncoder::AddFileName(const string& strFileName,
									   const string& strFormat)
{
	/* Only ContentSubType "JFIF" (JPEG) and ContentSubType "PNG" are allowed
	   for SlideShow application (not tested here!) */
	/* Try to open file binary */
	FILE* pFiBody = fopen(strFileName.c_str(), "rb");

	if (pFiBody != NULL)
	{
		_BYTE byIn;

		/* Enlarge vector storing the picture objects */
		const int iOldNumObj = vecMOTPicture.Size();
		vecMOTPicture.Enlarge(1);

		/* Store file name and format string */
		vecMOTPicture[iOldNumObj].strName = strFileName;
		vecMOTPicture[iOldNumObj].strFormat = strFormat;

		/* Fill body data with content of selected file */
		vecMOTPicture[iOldNumObj].vecbRawData.Init(0);

		while (fread((void*) &byIn, size_t(1), size_t(1), pFiBody) !=
			size_t(0))
		{
			/* Add one byte = SIZEOF__BYTE bits */
			vecMOTPicture[iOldNumObj].vecbRawData.Enlarge(SIZEOF__BYTE);
			vecMOTPicture[iOldNumObj].vecbRawData.
				Enqueue((_UINT32BIT) byIn, SIZEOF__BYTE);
		}

		/* Close the file afterwards */
		fclose(pFiBody);
	}
}


/******************************************************************************\
* Decoder                                                                      *
\******************************************************************************/
void CMOTSlideShowDecoder::AddDataUnit(CVector<_BINARY>& vecbiNewData)
{
	/* Add new data group (which is in one DRM data unit if SlideShow
	   application is used) and check if new MOT object is ready after adding
	   this new data group */
	if (MOTDAB.AddDataGroup(vecbiNewData) == TRUE)
	{
		/* Get new received SlideShow picture */
		MOTDAB.GetMOTObject(MOTPicture);
		bNewPicture = TRUE; /* Set flag for new picture */
	}
}

_BOOLEAN CMOTSlideShowDecoder::GetPicture(CMOTObject& NewPic)
{
	const int iRawDataSize = MOTPicture.vecbRawData.Size();

	/* Init output object */
	NewPic.Reset();

	/* Only copy picture content if picture is available */
	if (iRawDataSize != 0)
		NewPic = MOTPicture;

	/* Check if this is an old or a new picture and return result */
	_BOOLEAN bWasNewPicture = FALSE;
	if (bNewPicture == TRUE)
	{
		bNewPicture = FALSE;
		bWasNewPicture = TRUE;
	}

	return bWasNewPicture;
}
