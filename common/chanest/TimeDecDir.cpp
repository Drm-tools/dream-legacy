/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Alexander Kurpiers, Volker Fischer
 *
 * Description:
 *	Decision directed interpolation in time direction
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

#include "TimeDecDir.h"


/* Implementation *************************************************************/
void CTimeDecDir::Estimate(CVectorEx<_COMPLEX>* pvecInputData, 
						   CComplexVector& veccOutputData, 
						   CVector<int>& veciMapTab, 
						   CVector<_COMPLEX>& veccPilotCells)
{
	int	i;





// Hiermit speichere ich die timing-Verschiebungen um später die complexen
// Werte richtig rotieren zu können. Timing Information in ".GetExData().iCurTimeCorr"
	/* Timing correction history -------------------------------------------- */
	/* Update timing correction history. Shift old values */
//	for (i = 0; i < iLenTiCorrHist - 1; i++)
//		vecTiCorrHist[i] = vecTiCorrHist[i + 1];
//
//	vecTiCorrHist[iLenTiCorrHist - 1] = 0;
//
//	/* Add new one to all history values */
//	for (i = 0; i < iLenTiCorrHist; i++)
//		vecTiCorrHist[i] += (*pvecInputData).GetExData().iCurTimeCorr;

// Korrigiert wird dann mit
//cCorrectedPilot = Rotate(cCelleDieZuDrehenIst, iTrägerNummer, iZeitVerschiebungZumAktuellenWert);




	/* Main loop ------------------------------------------------------------ */
	for (i = 0; i < iNoCarrier; i++)
	{
		if (veciMapTab[i] & CM_SCAT_PI)
		{
			// Diese Zelle ist scattered Pilot
			// Complexer Wert in "veccPilotCells[i]"
		}

		if (veciMapTab[i] & CM_TI_PI)
		{
			// Diese Zelle ist time Pilot
			// Complexer Wert in "veccPilotCells[i]"
		}

		if (veciMapTab[i] & CM_FRE_PI)
		{
			// Diese Zelle ist frequency Pilot
			// Complexer Wert in "veccPilotCells[i]"
		}

		if (veciMapTab[i] & CM_DC)
		{
			// Diese Zelle ist DC carrier
			// Nicht benutzt!!! Laut Standard
		}

		if (veciMapTab[i] & CM_FAC)
		{
			// Diese Zelle ist FAC
			// Immer 4-QAM
		}

		if (veciMapTab[i] & CM_SDC)
		{
			// Diese Zelle ist SDC
			// QAM modulation in "iSDCQAM"
		}

		if (veciMapTab[i] & CM_MSC)
		{
			// Diese Zelle ist MSC
			// QAM modulation in "iMSCQAM"
		}
	}





}

int CTimeDecDir::Init(CParameter& Parameter)
{
// Das hier muss sein!
	/* Init base class, must be at the beginning of this routine! */
	CPilotModiClass::InitRot(Parameter);


// Für dich vorbereitet	
	/* Get modulation alphabet of MSC and SDC */
	if (Parameter.eMSCCodingScheme == CParameter::CS_2_SM)
		iMSCQAM = 16;
	else
		iMSCQAM = 64;

	if (Parameter.eSDCCodingScheme == CParameter::CS_1_SM)
		iSDCQAM = 4;
	else
		iSDCQAM = 16;


// Vorschlag von mir um zu zeigen, wie das mit den Vektoren so läuft
	/* Set No of carriers with DC */
	iNoCarrier = Parameter.iNoCarrier;

	/* Allocate memory for channel estimation ------------------------------- */
	veccChanEst.Init(iNoCarrier);

	


// Gib' zurück, wie das delay deiner interpolation ist	
	return 0;
}
