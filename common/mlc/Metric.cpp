/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 * The metric is cacluated as follows:
 * M = ||r - s * h||^2 = ||h||^2 * ||r / h - s||^2
 *	
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

#include "Metric.h"


/* Implementation *************************************************************/
void CMLCMetric::CalculateMetric(CVector<CEquSig>* pcInSymb, 
								 CVector<CDistance>& vecMetric, 
								 CVector<_BINARY>& vecbiSubsetDef1, 
								 CVector<_BINARY>& vecbiSubsetDef2,
								 CVector<_BINARY>& vecbiSubsetDef3, 
								 CVector<_BINARY>& vecbiSubsetDef4,
								 CVector<_BINARY>& vecbiSubsetDef5,
								 CVector<_BINARY>& vecbiSubsetDef6,
								 int iLevel, _BOOLEAN bIteration)
{
	int i, k;
	int iTabInd0;

	switch (eMapType)
	{
	case CParameter::CS_1_SM:
		/**********************************************************************/
		/* 4QAM	***************************************************************/
		/**********************************************************************/
		/* Metric according DRM-standard: (i_0  q_0) = (y_0,0  y_0,1) */
		for (i = 0, k = 0; i < iInputBlockSize; i++, k += 2)
		{
			/* Real part ---------------------------------------------------- */
			/* Distance to "0" */
			vecMetric[k].rTow0 = Minimum1((*pcInSymb)[i].cSig.real(), 
				rTableQAM4[0][0]) * (*pcInSymb)[i].rChan;

			/* Distance to "1" */
			vecMetric[k].rTow1 = Minimum1((*pcInSymb)[i].cSig.real(), 
				rTableQAM4[1][0]) * (*pcInSymb)[i].rChan;


			/* Imaginary part ----------------------------------------------- */
			/* Distance to "0" */
			vecMetric[k + 1].rTow0 = Minimum1((*pcInSymb)[i].cSig.imag(), 
				rTableQAM4[0][1]) * (*pcInSymb)[i].rChan;

			/* Distance to "1" */
			vecMetric[k + 1].rTow1 = Minimum1((*pcInSymb)[i].cSig.imag(), 
				rTableQAM4[1][1]) * (*pcInSymb)[i].rChan;
		}

		break;

	case CParameter::CS_2_SM:
		/**********************************************************************/
		/* 16QAM **************************************************************/
		/**********************************************************************/
		/* (i_0  i_1  q_0  q_1) = (y_0,0  y_1,0  y_0,1  y_1,1) */
		switch (iLevel)
		{
		case 0:
			for (i = 0, k = 0; i < iInputBlockSize; i++, k += 2)
			{
				if (bIteration == TRUE)
				{
					/* Real part -------------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef2" */
					iTabInd0 = vecbiSubsetDef2[k] & 1;

					/* Distance to "0" */
					vecMetric[k].rTow0 = 
						Minimum1((*pcInSymb)[i].cSig.real(), 
						rTableQAM16[iTabInd0][0]) * (*pcInSymb)[i].rChan;

					/* Distance to "1" */
					vecMetric[k].rTow1 = 
						Minimum1((*pcInSymb)[i].cSig.real(),
						rTableQAM16[iTabInd0 | (1 << 1)][0]) * 
						(*pcInSymb)[i].rChan;


					/* Imaginary part --------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef2" */
					iTabInd0 = vecbiSubsetDef2[k + 1] & 1;

					/* Distance to "0" */
					vecMetric[k + 1].rTow0 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM16[iTabInd0][1]) * (*pcInSymb)[i].rChan;

					/* Distance to "1" */
					vecMetric[k + 1].rTow1 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM16[iTabInd0 | (1 << 1)][1]) * 
						(*pcInSymb)[i].rChan;
				}
				else
				{
					/* There are two possible points for each bit. Both have to
					   be used. In the first step: {i_1}, {q_1} = 0
					   In the second step: {i_1}, {q_1} = 1 */
						
					/* Calculate distances */
					/* Real part */
					vecMetric[k].rTow0 = 
						Minimum2((*pcInSymb)[i].cSig.real(), 
						rTableQAM16[0 /* [0  0] */][0],
						rTableQAM16[1 /* [0  1] */][0]) * (*pcInSymb)[i].rChan;

					vecMetric[k].rTow1 = 
						Minimum2((*pcInSymb)[i].cSig.real(),
						rTableQAM16[2 /* [1  0] */][0],
						rTableQAM16[3 /* [1  1] */][0]) * (*pcInSymb)[i].rChan;

					/* Imaginary part */
					vecMetric[k + 1].rTow0 = 
						Minimum2((*pcInSymb)[i].cSig.imag(),
						rTableQAM16[0 /* [0  0] */][1],
						rTableQAM16[1 /* [0  1] */][1]) * (*pcInSymb)[i].rChan;

					vecMetric[k + 1].rTow1 = 
						Minimum2((*pcInSymb)[i].cSig.imag(), 
						rTableQAM16[2 /* [1  0] */][1],
						rTableQAM16[3 /* [1  1] */][1]) * (*pcInSymb)[i].rChan;
				}
			}

			break;

		case 1:
			for (i = 0, k = 0; i < iInputBlockSize; i++, k += 2)
			{
				/* Real part ------------------------------------------------ */
				/* Higest bit defined by "vecbiSubsetDef1" */
				iTabInd0 = ((vecbiSubsetDef1[k] & 1) << 1);

				/* Distance to "0" */
				vecMetric[k].rTow0 = Minimum1((*pcInSymb)[i].cSig.real(), 
					rTableQAM16[iTabInd0][0]) * (*pcInSymb)[i].rChan;

				/* Distance to "1" */
				vecMetric[k].rTow1 = Minimum1((*pcInSymb)[i].cSig.real(),
					rTableQAM16[iTabInd0 | 1][0]) * (*pcInSymb)[i].rChan;


				/* Imaginary part ------------------------------------------- */
				/* Higest bit defined by "vecbiSubsetDef1" */
				iTabInd0 = ((vecbiSubsetDef1[k + 1] & 1) << 1);

				/* Distance to "0" */
				vecMetric[k + 1].rTow0 = Minimum1((*pcInSymb)[i].cSig.imag(), 
					rTableQAM16[iTabInd0][1]) * (*pcInSymb)[i].rChan;

				/* Distance to "1" */
				vecMetric[k + 1].rTow1 = Minimum1((*pcInSymb)[i].cSig.imag(),
					rTableQAM16[iTabInd0 | 1][1]) * (*pcInSymb)[i].rChan;
			}

			break;
		}

		break;

	case CParameter::CS_3_SM:
		/**********************************************************************/
		/* 64QAM SM ***********************************************************/
		/**********************************************************************/
		/* (i_0  i_1  i_2  q_0  q_1  q_2) = 
		   (y_0,0  y_1,0  y_2,0  y_0,1  y_1,1  y_2,1) */
		switch (iLevel)
		{
		case 0:
			for (i = 0, k = 0; i < iInputBlockSize; i++, k += 2)
			{
				if (bIteration == TRUE)
				{
					/* Real part -------------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef3" next bit defined
					   by "vecbiSubsetDef2" */
					iTabInd0 = 
						(vecbiSubsetDef3[k] & 1) | 
						((vecbiSubsetDef2[k] & 1) << 1);

					vecMetric[k].rTow0 = 
						Minimum1((*pcInSymb)[i].cSig.real(), 
						rTableQAM64SM[iTabInd0][0]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[k].rTow1 = 
						Minimum1((*pcInSymb)[i].cSig.real(),
						rTableQAM64SM[iTabInd0 | (1 << 2)][0]) * 
						(*pcInSymb)[i].rChan;


					/* Imaginary part --------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef3" next bit defined
					   by "vecbiSubsetDef2" */
					iTabInd0 = 
						(vecbiSubsetDef3[k + 1] & 1) | 
						((vecbiSubsetDef2[k + 1] & 1) << 1);

					/* Calculate distances, imaginary part */
					vecMetric[k + 1].rTow0 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64SM[iTabInd0][1]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[k + 1].rTow1 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64SM[iTabInd0 | (1 << 2)][1]) * 
						(*pcInSymb)[i].rChan;

				}
				else
				{
					/* Real part -------------------------------------------- */
					vecMetric[k].rTow0 = 
						Minimum4((*pcInSymb)[i].cSig.real(), 
						rTableQAM64SM[0 /* [0 0 0] */][0],
						rTableQAM64SM[1 /* [0 0 1] */][0],
						rTableQAM64SM[2 /* [0 1 0] */][0],
						rTableQAM64SM[3 /* [0 1 1] */][0]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[k].rTow1 = 
						Minimum4((*pcInSymb)[i].cSig.real(),
						rTableQAM64SM[4 /* [1 0 0] */][0], 
						rTableQAM64SM[5 /* [1 0 1] */][0], 
						rTableQAM64SM[6 /* [1 1 0] */][0], 
						rTableQAM64SM[7 /* [1 1 1] */][0]) * 
						(*pcInSymb)[i].rChan;


					/* Imaginary part --------------------------------------- */
					vecMetric[k + 1].rTow0 = 
						Minimum4((*pcInSymb)[i].cSig.imag(),
						rTableQAM64SM[0 /* [0 0 0] */][1],
						rTableQAM64SM[1 /* [0 0 1] */][1],
						rTableQAM64SM[2 /* [0 1 0] */][1],
						rTableQAM64SM[3 /* [0 1 1] */][1]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[k + 1].rTow1 = 
						Minimum4((*pcInSymb)[i].cSig.imag(),
						rTableQAM64SM[4 /* [1 0 0] */][1], 
						rTableQAM64SM[5 /* [1 0 1] */][1], 
						rTableQAM64SM[6 /* [1 1 0] */][1], 
						rTableQAM64SM[7 /* [1 1 1] */][1]) * 
						(*pcInSymb)[i].rChan;
				}
			}

			break;

		case 1:
			for (i = 0, k = 0; i < iInputBlockSize; i++, k += 2)
			{
				if (bIteration == TRUE)
				{
					/* Real part -------------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef3",highest defined
					   by "vecbiSubsetDef1" */
					iTabInd0 = 
						((vecbiSubsetDef1[k] & 1) << 2) | 
						(vecbiSubsetDef3[k] & 1);

					vecMetric[k].rTow0 = Minimum1((*pcInSymb)[i].cSig.real(), 
						rTableQAM64SM[iTabInd0][0]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[k].rTow1 = Minimum1((*pcInSymb)[i].cSig.real(),
						rTableQAM64SM[iTabInd0 | (1 << 1)][0]) * 
						(*pcInSymb)[i].rChan;


					/* Imaginary part --------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef3",highest defined
					   by "vecbiSubsetDef1" */
					iTabInd0 = 
						((vecbiSubsetDef1[k + 1] & 1) << 2) | 
						(vecbiSubsetDef3[k + 1] & 1);

					vecMetric[k + 1].rTow0 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64SM[iTabInd0][1]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[k + 1].rTow1 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64SM[iTabInd0 | (1 << 1)][1]) * 
						(*pcInSymb)[i].rChan;
				}
				else
				{
					/* There are two possible points for each bit. Both have to
					   be used. In the first step: {i_2} = 0, Higest bit 
					   defined by "vecbiSubsetDef1" */

					/* Real part -------------------------------------------- */
					iTabInd0 = ((vecbiSubsetDef1[k] & 1) << 2);
					vecMetric[k].rTow0 = 
						Minimum2((*pcInSymb)[i].cSig.real(), 
						rTableQAM64SM[iTabInd0][0],
						rTableQAM64SM[iTabInd0 | 1][0]) * (*pcInSymb)[i].rChan;

					iTabInd0 = ((vecbiSubsetDef1[k] & 1) << 2) | (1 << 1);
					vecMetric[k].rTow1 = 
						Minimum2((*pcInSymb)[i].cSig.real(), 
						rTableQAM64SM[iTabInd0][0],
						rTableQAM64SM[iTabInd0 | 1][0]) * (*pcInSymb)[i].rChan;


					/* Imaginary part --------------------------------------- */
					iTabInd0 = ((vecbiSubsetDef1[k + 1] & 1) << 2);
					vecMetric[k + 1].rTow0 = 
						Minimum2((*pcInSymb)[i].cSig.imag(), 
						rTableQAM64SM[iTabInd0][1],
						rTableQAM64SM[iTabInd0 | 1][1]) * (*pcInSymb)[i].rChan;

					iTabInd0 = ((vecbiSubsetDef1[k + 1] & 1) << 2) | (1 << 1);
					vecMetric[k + 1].rTow1 = 
						Minimum2((*pcInSymb)[i].cSig.imag(),
						rTableQAM64SM[iTabInd0][1],
						rTableQAM64SM[iTabInd0 | 1][1]) * (*pcInSymb)[i].rChan;
				}
			}

			break;

		case 2:
			for (i = 0, k = 0; i < iInputBlockSize; i++, k += 2)
			{
				/* Real part ------------------------------------------------ */
				/* Higest bit defined by "vecbiSubsetDef1" next bit defined
				   by "vecbiSubsetDef2" */
				iTabInd0 = 
					((vecbiSubsetDef1[k] & 1) << 2) | 
					((vecbiSubsetDef2[k] & 1) << 1);

				vecMetric[k].rTow0 = Minimum1((*pcInSymb)[i].cSig.real(), 
					rTableQAM64SM[iTabInd0][0]) * (*pcInSymb)[i].rChan;

				vecMetric[k].rTow1 = Minimum1((*pcInSymb)[i].cSig.real(),
					rTableQAM64SM[iTabInd0 | 1][0]) * (*pcInSymb)[i].rChan;


				/* Imaginary part ------------------------------------------- */
				/* Higest bit defined by "vecbiSubsetDef1" next bit defined
				   by "vecbiSubsetDef2" */
				iTabInd0 = 
					((vecbiSubsetDef1[k + 1] & 1) << 2) | 
					((vecbiSubsetDef2[k + 1] & 1) << 1);

				/* Calculate distances, imaginary part */
				vecMetric[k + 1].rTow0 = Minimum1((*pcInSymb)[i].cSig.imag(),
					rTableQAM64SM[iTabInd0][1]) * (*pcInSymb)[i].rChan;

				vecMetric[k + 1].rTow1 = Minimum1((*pcInSymb)[i].cSig.imag(),
					rTableQAM64SM[iTabInd0 | 1][1]) * (*pcInSymb)[i].rChan;
			}

			break;
		}

		break;

	case CParameter::CS_3_HMSYM:
		/**********************************************************************/
		/* 64QAM HMsym ********************************************************/
		/**********************************************************************/
		/* (i_0  i_1  i_2  q_0  q_1  q_2) = 
		   (y_0,0  y_1,0  y_2,0  y_0,1  y_1,1  y_2,1) */
		switch (iLevel)
		{
		case 0:
			for (i = 0, k = 0; i < iInputBlockSize; i++, k += 2)
			{
				if (bIteration == TRUE)
				{
					/* Real part -------------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef3" next bit defined
					   by "vecbiSubsetDef2" */
					iTabInd0 = 
						(vecbiSubsetDef3[k] & 1) | 
						((vecbiSubsetDef2[k] & 1) << 1);

					vecMetric[k].rTow0 = Minimum1((*pcInSymb)[i].cSig.real(), 
						rTableQAM64HMsym[iTabInd0][0]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[k].rTow1 = Minimum1((*pcInSymb)[i].cSig.real(),
						rTableQAM64HMsym[iTabInd0 | (1 << 2)][0]) * 
						(*pcInSymb)[i].rChan;


					/* Imaginary part --------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef3" next bit defined
					   by "vecbiSubsetDef2" */
					iTabInd0 = 
						(vecbiSubsetDef3[k + 1] & 1) | 
						((vecbiSubsetDef2[k + 1] & 1) << 1);

					/* Calculate distances, imaginary part */
					vecMetric[k + 1].rTow0 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64HMsym[iTabInd0][1]) * (*pcInSymb)[i].rChan;

					vecMetric[k + 1].rTow1 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64HMsym[iTabInd0 | (1 << 2)][1]) * 
						(*pcInSymb)[i].rChan;

				}
				else
				{				
					/* Real part -------------------------------------------- */
					vecMetric[k].rTow0 = 
						Minimum4((*pcInSymb)[i].cSig.real(), 
						rTableQAM64HMsym[0 /* [0 0 0] */][0],
						rTableQAM64HMsym[1 /* [0 0 1] */][0],
						rTableQAM64HMsym[2 /* [0 1 0] */][0],
						rTableQAM64HMsym[3 /* [0 1 1] */][0]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[k].rTow1 = 
						Minimum4((*pcInSymb)[i].cSig.real(),
						rTableQAM64HMsym[4 /* [1 0 0] */][0], 
						rTableQAM64HMsym[5 /* [1 0 1] */][0], 
						rTableQAM64HMsym[6 /* [1 1 0] */][0], 
						rTableQAM64HMsym[7 /* [1 1 1] */][0]) * 
						(*pcInSymb)[i].rChan;


					/* Imaginary part --------------------------------------- */
					vecMetric[k + 1].rTow0 = 
						Minimum4((*pcInSymb)[i].cSig.imag(),
						rTableQAM64HMsym[0 /* [0 0 0] */][1],
						rTableQAM64HMsym[1 /* [0 0 1] */][1],
						rTableQAM64HMsym[2 /* [0 1 0] */][1],
						rTableQAM64HMsym[3 /* [0 1 1] */][1]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[k + 1].rTow1 = 
						Minimum4((*pcInSymb)[i].cSig.imag(),
						rTableQAM64HMsym[4 /* [1 0 0] */][1], 
						rTableQAM64HMsym[5 /* [1 0 1] */][1], 
						rTableQAM64HMsym[6 /* [1 1 0] */][1], 
						rTableQAM64HMsym[7 /* [1 1 1] */][1]) * 
						(*pcInSymb)[i].rChan;
				}
			}

			break;

		case 1:
			for (i = 0, k = 0; i < iInputBlockSize; i++, k += 2)
			{
				if (bIteration == TRUE)
				{
					/* Real part -------------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef3",highest defined
					   by "vecbiSubsetDef1" */
					iTabInd0 = 
						((vecbiSubsetDef1[k] & 1) << 2) | 
						(vecbiSubsetDef3[k] & 1);

					vecMetric[k].rTow0 = Minimum1((*pcInSymb)[i].cSig.real(), 
						rTableQAM64HMsym[iTabInd0][0]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[k].rTow1 = Minimum1((*pcInSymb)[i].cSig.real(),
						rTableQAM64HMsym[iTabInd0 | (1 << 1)][0]) * 
						(*pcInSymb)[i].rChan;


					/* Imaginary part --------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef3",highest defined
					   by "vecbiSubsetDef1" */
					iTabInd0 = 
						((vecbiSubsetDef1[k + 1] & 1) << 2) | 
						(vecbiSubsetDef3[k + 1] & 1);

					vecMetric[k + 1].rTow0 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64HMsym[iTabInd0][1]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[k + 1].rTow1 = 
						Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64HMsym[iTabInd0 | (1 << 1)][1]) * 
						(*pcInSymb)[i].rChan;
				}
				else
				{
					/* There are two possible points for each bit. Both have to
					   be used. In the first step: {i_2} = 0, Higest bit 
					   defined by "vecbiSubsetDef1" */

					/* Real part -------------------------------------------- */
					iTabInd0 = ((vecbiSubsetDef1[k] & 1) << 2);
					vecMetric[k].rTow0 = 
						Minimum2((*pcInSymb)[i].cSig.real(), 
						rTableQAM64HMsym[iTabInd0][0],
						rTableQAM64HMsym[iTabInd0 | 1][0]) * 
						(*pcInSymb)[i].rChan;

					iTabInd0 = ((vecbiSubsetDef1[k] & 1) << 2) | (1 << 1);
					vecMetric[k].rTow1 = 
						Minimum2((*pcInSymb)[i].cSig.real(), 
						rTableQAM64HMsym[iTabInd0][0],
						rTableQAM64HMsym[iTabInd0 | 1][0]) * 
						(*pcInSymb)[i].rChan;


					/* Imaginary part --------------------------------------- */
					iTabInd0 = ((vecbiSubsetDef1[k + 1] & 1) << 2);
					vecMetric[k + 1].rTow0 = 
						Minimum2((*pcInSymb)[i].cSig.imag(), 
						rTableQAM64HMsym[iTabInd0][1],
						rTableQAM64HMsym[iTabInd0 | 1][1]) * 
						(*pcInSymb)[i].rChan;

					iTabInd0 = ((vecbiSubsetDef1[k + 1] & 1) << 2) | (1 << 1);
					vecMetric[k + 1].rTow1 = 
						Minimum2((*pcInSymb)[i].cSig.imag(),
						rTableQAM64HMsym[iTabInd0][1],
						rTableQAM64HMsym[iTabInd0 | 1][1]) * 
						(*pcInSymb)[i].rChan;
				}
			}

			break;

		case 2:
			for (i = 0, k = 0; i < iInputBlockSize; i++, k += 2)
			{
				/* Real part ------------------------------------------------ */
				/* Higest bit defined by "vecbiSubsetDef1" next bit defined
				   by "vecbiSubsetDef2" */
				iTabInd0 = 
					((vecbiSubsetDef1[k] & 1) << 2) | 
					((vecbiSubsetDef2[k] & 1) << 1);

				vecMetric[k].rTow0 = Minimum1((*pcInSymb)[i].cSig.real(), 
					rTableQAM64HMsym[iTabInd0][0]) * (*pcInSymb)[i].rChan;

				vecMetric[k].rTow1 = Minimum1((*pcInSymb)[i].cSig.real(),
					rTableQAM64HMsym[iTabInd0 | 1][0]) * (*pcInSymb)[i].rChan;


				/* Imaginary part ------------------------------------------- */
				/* Higest bit defined by "vecbiSubsetDef1" next bit defined
				   by "vecbiSubsetDef2" */
				iTabInd0 = 
					((vecbiSubsetDef1[k + 1] & 1) << 2) | 
					((vecbiSubsetDef2[k + 1] & 1) << 1);

				/* Calculate distances, imaginary part */
				vecMetric[k + 1].rTow0 = Minimum1((*pcInSymb)[i].cSig.imag(),
					rTableQAM64HMsym[iTabInd0][1]) * (*pcInSymb)[i].rChan;

				vecMetric[k + 1].rTow1 = Minimum1((*pcInSymb)[i].cSig.imag(),
					rTableQAM64HMsym[iTabInd0 | 1][1]) * (*pcInSymb)[i].rChan;
			}

			break;
		}

		break;

	case CParameter::CS_3_HMMIX:
		/**********************************************************************/
		/* 64QAM HMmix ********************************************************/
		/**********************************************************************/
		/* (i_0  i_1  i_2  q_0  q_1  q_2) = 
		   (y_0,0Re  y_1,0Re  y_2,0Re  y_0,0Im  y_1,0Im  y_2,0Im) */
		switch (iLevel)
		{
		case 0:
			for (i = 0; i < iInputBlockSize; i++)
			{
				if (bIteration == TRUE)
				{
					/* Real part -------------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef5" next bit defined
					   by "vecbiSubsetDef3" */
					iTabInd0 = 
						(vecbiSubsetDef5[i] & 1) | 
						((vecbiSubsetDef3[i] & 1) << 1);

					vecMetric[i].rTow0 = Minimum1((*pcInSymb)[i].cSig.real(), 
						rTableQAM64HMmix[iTabInd0][0]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[i].rTow1 = Minimum1((*pcInSymb)[i].cSig.real(),
						rTableQAM64HMmix[iTabInd0 | (1 << 2)][0]) * 
						(*pcInSymb)[i].rChan;
				}
				else
				{
					/* Real part -------------------------------------------- */
					vecMetric[i].rTow0 = Minimum4((*pcInSymb)[i].cSig.real(), 
						rTableQAM64HMmix[0 /* [0 0 0] */][0],
						rTableQAM64HMmix[1 /* [0 0 1] */][0],
						rTableQAM64HMmix[2 /* [0 1 0] */][0],
						rTableQAM64HMmix[3 /* [0 1 1] */][0]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[i].rTow1 = Minimum4((*pcInSymb)[i].cSig.real(),
						rTableQAM64HMmix[4 /* [1 0 0] */][0], 
						rTableQAM64HMmix[5 /* [1 0 1] */][0], 
						rTableQAM64HMmix[6 /* [1 1 0] */][0], 
						rTableQAM64HMmix[7 /* [1 1 1] */][0]) * 
						(*pcInSymb)[i].rChan;
				}
			}

			break;

		case 1:
			for (i = 0; i < iInputBlockSize; i++)
			{
				if (bIteration == TRUE)
				{
					/* Imaginary part --------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef6" next bit defined
					   by "vecbiSubsetDef4" */
					iTabInd0 = 
						(vecbiSubsetDef6[i] & 1) | 
						((vecbiSubsetDef4[i] & 1) << 1);

					/* Calculate distances, imaginary part */
					vecMetric[i].rTow0 = Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64HMmix[iTabInd0][1]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[i].rTow1 = Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64HMmix[iTabInd0 | (1 << 2)][1]) * 
						(*pcInSymb)[i].rChan;
				}
				else
				{
					/* Imaginary part --------------------------------------- */
					vecMetric[i].rTow0 = Minimum4((*pcInSymb)[i].cSig.imag(),
						rTableQAM64HMmix[0 /* [0 0 0] */][1],
						rTableQAM64HMmix[1 /* [0 0 1] */][1],
						rTableQAM64HMmix[2 /* [0 1 0] */][1],
						rTableQAM64HMmix[3 /* [0 1 1] */][1]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[i].rTow1 = Minimum4((*pcInSymb)[i].cSig.imag(),
						rTableQAM64HMmix[4 /* [1 0 0] */][1], 
						rTableQAM64HMmix[5 /* [1 0 1] */][1], 
						rTableQAM64HMmix[6 /* [1 1 0] */][1], 
						rTableQAM64HMmix[7 /* [1 1 1] */][1]) * 
						(*pcInSymb)[i].rChan;
				}
			}

			break;

		case 2:
			for (i = 0; i < iInputBlockSize; i++)
			{
				if (bIteration == TRUE)
				{
					/* Real part -------------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef5",highest defined
					   by "vecbiSubsetDef1" */
					iTabInd0 = 
						((vecbiSubsetDef1[i] & 1) << 2) | 
						(vecbiSubsetDef5[i] & 1);

					vecMetric[i].rTow0 = Minimum1((*pcInSymb)[i].cSig.real(), 
						rTableQAM64HMmix[iTabInd0][0]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[i].rTow1 = Minimum1((*pcInSymb)[i].cSig.real(),
						rTableQAM64HMmix[iTabInd0 | (1 << 1)][0]) * 
						(*pcInSymb)[i].rChan;
				}
				else
				{
					/* There are two possible points for each bit. Both have to
					   be used. In the first step: {i_2} = 0, Higest bit 
					   defined by "vecbiSubsetDef1" */

					/* Real part -------------------------------------------- */
					iTabInd0 = ((vecbiSubsetDef1[i] & 1) << 2);
					vecMetric[i].rTow0 = Minimum2((*pcInSymb)[i].cSig.real(), 
						rTableQAM64HMmix[iTabInd0][0],
						rTableQAM64HMmix[iTabInd0 | 1][0]) * 
						(*pcInSymb)[i].rChan;

					iTabInd0 = ((vecbiSubsetDef1[i] & 1) << 2) | (1 << 1);
					vecMetric[i].rTow1 = Minimum2((*pcInSymb)[i].cSig.real(), 
						rTableQAM64HMmix[iTabInd0][0],
						rTableQAM64HMmix[iTabInd0 | 1][0]) * 
						(*pcInSymb)[i].rChan;
				}
			}

			break;

		case 3:
			for (i = 0; i < iInputBlockSize; i++)
			{
				if (bIteration == TRUE)
				{
					/* Imaginary part --------------------------------------- */
					/* Lowest bit defined by "vecbiSubsetDef6",highest defined
					   by "vecbiSubsetDef2" */
					iTabInd0 = 
						((vecbiSubsetDef2[i] & 1) << 2) | 
						(vecbiSubsetDef6[i] & 1);

					vecMetric[i].rTow0 = Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64HMmix[iTabInd0][1]) * 
						(*pcInSymb)[i].rChan;

					vecMetric[i].rTow1 = Minimum1((*pcInSymb)[i].cSig.imag(),
						rTableQAM64HMmix[iTabInd0 | (1 << 1)][1]) * 
						(*pcInSymb)[i].rChan;
				}
				else
				{
					/* There are two possible points for each bit. Both have to
					   be used. In the first step: {i_2} = 0, Higest bit 
					   defined by "vecbiSubsetDef1" */

					/* Imaginary part ------------------------------------------- */
					iTabInd0 = ((vecbiSubsetDef2[i] & 1) << 2);
					vecMetric[i].rTow0 = Minimum2((*pcInSymb)[i].cSig.imag(), 
						rTableQAM64HMmix[iTabInd0][1],
						rTableQAM64HMmix[iTabInd0 | 1][1]) * 
						(*pcInSymb)[i].rChan;

					iTabInd0 = ((vecbiSubsetDef2[i] & 1) << 2) | (1 << 1);
					vecMetric[i].rTow1 = Minimum2((*pcInSymb)[i].cSig.imag(),
						rTableQAM64HMmix[iTabInd0][1],
						rTableQAM64HMmix[iTabInd0 | 1][1]) * 
						(*pcInSymb)[i].rChan;
				}
			}

			break;

		case 4:
			for (i = 0; i < iInputBlockSize; i++)
			{
				/* Real part ------------------------------------------------ */
				/* Higest bit defined by "vecbiSubsetDef1" next bit defined
				   by "vecbiSubsetDef2" */
				iTabInd0 = 
					((vecbiSubsetDef1[i] & 1) << 2) | 
					((vecbiSubsetDef3[i] & 1) << 1);

				vecMetric[i].rTow0 = Minimum1((*pcInSymb)[i].cSig.real(), 
					rTableQAM64HMmix[iTabInd0][0]) * (*pcInSymb)[i].rChan;

				vecMetric[i].rTow1 = Minimum1((*pcInSymb)[i].cSig.real(),
					rTableQAM64HMmix[iTabInd0 | 1][0]) * (*pcInSymb)[i].rChan;
			}

			break;

		case 5:
			for (i = 0; i < iInputBlockSize; i++)
			{
				/* Imaginary part ------------------------------------------- */
				/* Higest bit defined by "vecbiSubsetDef1" next bit defined
				   by "vecbiSubsetDef2" */
				iTabInd0 = 
					((vecbiSubsetDef2[i] & 1) << 2) | 
					((vecbiSubsetDef4[i] & 1) << 1);

				/* Calculate distances, imaginary part */
				vecMetric[i].rTow0 = Minimum1((*pcInSymb)[i].cSig.imag(),
					rTableQAM64HMmix[iTabInd0][1]) * (*pcInSymb)[i].rChan;

				vecMetric[i].rTow1 = Minimum1((*pcInSymb)[i].cSig.imag(),
					rTableQAM64HMmix[iTabInd0 | 1][1]) * (*pcInSymb)[i].rChan;
			}

			break;
		}

		break;
	}
}

void CMLCMetric::Init(int iNewInputBlockSize, CParameter::ECodScheme eNewCodingScheme)
{
	iInputBlockSize = iNewInputBlockSize;
	eMapType = eNewCodingScheme;
}

_REAL CMLCMetric::Minimum1(_REAL rA, _REAL rB) const
{
	/* The minium in case of only one parameter is trivial */
	return (rA - rB) * (rA - rB);
}

_REAL CMLCMetric::Minimum2(_REAL rA, _REAL rB1, _REAL rB2) const
{
	/* First calculate all distances */
	_REAL rResult1 = fabs(rA - rB1);
	_REAL rResult2 = fabs(rA - rB2);

	/* Return smalles one */
	if (rResult1 < rResult2)
		return rResult1 * rResult1;
	else
		return rResult2 * rResult2;
}

_REAL CMLCMetric::Minimum4(_REAL rA, _REAL rB1, _REAL rB2, _REAL rB3, _REAL rB4) const
{
	/* First calculate all distances */
	_REAL rResult1 = fabs(rA - rB1);
	_REAL rResult2 = fabs(rA - rB2);
	_REAL rResult3 = fabs(rA - rB3);
	_REAL rResult4 = fabs(rA - rB4);

	/* Search for smalles one */
	_REAL rReturn = rResult1;
	if (rResult2 < rReturn)
		rReturn = rResult2;
	if (rResult3 < rReturn)
		rReturn = rResult3;
	if (rResult4 < rReturn)
		rReturn = rResult4;

	return rReturn * rReturn;
}
