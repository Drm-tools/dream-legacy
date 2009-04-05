/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	Abstract Interface for Receiver
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

#ifndef _RECEIVER_INTERFACE_H
#define _RECEIVER_INTERFACE_H

#include "Parameter.h"

class CSelectionInterface;
class CAMSSPhaseDemod;
class CAMSSDecode;
class CDataDecoder;
class CRigCaps;

class ReceiverInterface
{
public:
    virtual ~ReceiverInterface() {} // avoids some compiler warnings
	virtual CSelectionInterface*	GetSoundInInterface()=0;
	virtual CSelectionInterface*	GetSoundOutInterface()=0;
	virtual void					SetAnalogDemodAcq(_REAL)=0;
	virtual CParameter*				GetParameters()=0;
	virtual EType				    GetAnalogAGCType()=0;
	virtual ENoiRedType             GetAnalogNoiseReductionType()=0;
	virtual void					SetUseAnalogHWDemod(bool)=0;
	virtual bool					GetUseAnalogHWDemod()=0;
	virtual int						GetAnalogFilterBWHz()=0;
	virtual bool					AnalogAutoFreqAcqEnabled()=0;
	virtual bool					AnalogPLLEnabled()=0;
	virtual bool					GetAnalogPLLPhase(_REAL&)=0;
	virtual void					SetAnalogAGCType(const EType)=0;
	virtual void					SetAnalogNoiseReductionType(const ENoiRedType)=0;
	virtual void					SetAnalogFilterBWHz(int)=0;
	virtual void					EnableAnalogAutoFreqAcq(const bool)=0;
	virtual void					EnableAnalogPLL(const bool)=0;
	virtual void                    MuteAudio(bool)=0;
	virtual bool                    GetMuteAudio()=0;
	virtual bool                    GetIsWriteWaveFile()=0;
	virtual void                    StartWriteWaveFile(const string&)=0;
	virtual void                    StopWriteWaveFile()=0;

	virtual EAcqStat				GetAcquiState()=0;
	virtual bool		 			SetFrequency(int)=0;
	virtual int		 				GetFrequency()=0;

	virtual void                    SetFreqInt(ETypeIntFreq)=0;
	virtual ETypeIntFreq            GetFreqInt()=0;
	virtual void                    SetTimeInt(ETypeIntTime)=0;
	virtual ETypeIntTime            GetTimeInt() const =0;
	virtual void                    SetIntCons(const bool)=0;
	virtual bool                    GetIntCons()=0;

	virtual void                    SetTiSyncTracType(ETypeTiSyncTrac)=0;
	virtual ETypeTiSyncTrac         GetTiSyncTracType()=0;
	virtual void                    SetRecFilter(bool)=0;
	virtual bool                    GetRecFilter()=0;
	virtual void                    SetFlippedSpectrum(bool)=0;
	virtual bool                    GetFlippedSpectrum()=0;
	virtual void                    SetReverbEffect(bool)=0;
	virtual bool                    GetReverbEffect()=0;
	virtual void                    SetNumIterations(int)=0;
	virtual int                     GetInitNumIterations()=0;

	virtual void	 				SetEnableSMeter(bool)=0;
	virtual bool		 			GetEnableSMeter()=0;
	virtual void					SetRigModelForAllModes(int)=0;
	virtual void					SetRigModel(int)=0;
	virtual int						GetRigModel() const =0;
	virtual void					GetRigList(map<int, CRigCaps>&) const =0;
	virtual void					GetRigCaps(CRigCaps&) const =0;
	virtual void					GetRigCaps(int, CRigCaps&) const =0;
	virtual void					GetComPortList(map<string,string>&) const =0;
	virtual string					GetRigComPort() const =0;
	virtual void					SetRigComPort(const string&) =0;
	virtual bool				    GetRigChangeInProgress()=0;
	virtual bool				    UpstreamDIInputEnabled()=0;
	virtual void					SetIQRecording(bool)=0;
	virtual void					SetRSIRecording(bool, const char)=0;

	virtual CAMSSPhaseDemod*		GetAMSSPhaseDemod()=0;
	virtual CAMSSDecode*			GetAMSSDecode()=0;
	virtual CDataDecoder*			GetDataDecoder()=0;

	virtual bool                    End()=0;
};

#endif

