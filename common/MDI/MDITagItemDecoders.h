/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Oliver Haffenden
 *
 * Description:
 *	see MDITagItemDecoders.cpp
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

#ifndef MDI_TAG_ITEM_DECODERS_H_INCLUDED
#define MDI_TAG_ITEM_DECODERS_H_INCLUDED

#include "TagItemDecoder.h"

class CMDIInPkt;

class CTagItemDecoderProTy : public CTagItemDecoder
{
public:
	CTagItemDecoderProTy(CMDIInPkt* pInPkt) : pMDIInPkt(pInPkt) {}
	virtual string GetTagName(void);
	virtual void DecodeTag(CVector<_BINARY>& vecbiTag, const int iLenDataBits);
private:
	CMDIInPkt * pMDIInPkt;
};

class CTagItemDecoderLoFrCnt : public CTagItemDecoder   
{
public:
	CTagItemDecoderLoFrCnt(CMDIInPkt* pInPkt) : pMDIInPkt(pInPkt) {}
	virtual string GetTagName(void);
	virtual void DecodeTag(CVector<_BINARY>& vecbiTag, const int iLenDataBits);
private:
	CMDIInPkt * pMDIInPkt;
};

class CTagItemDecoderFAC : public CTagItemDecoder       
{
public:
	CTagItemDecoderFAC(CMDIInPkt* pInPkt) : pMDIInPkt(pInPkt) {}
	virtual string GetTagName(void);
	virtual void DecodeTag(CVector<_BINARY>& vecbiTag, const int iLenDataBits);
private:
	CMDIInPkt * pMDIInPkt;
};

class CTagItemDecoderSDC : public CTagItemDecoder       
{
public:
	CTagItemDecoderSDC(CMDIInPkt* pInPkt) : pMDIInPkt(pInPkt) {}
	virtual string GetTagName(void);
	virtual void DecodeTag(CVector<_BINARY>& vecbiTag, const int iLenDataBits);
private:
	CMDIInPkt * pMDIInPkt;
};

class CTagItemDecoderRobMod : public CTagItemDecoder    
{
public:
	CTagItemDecoderRobMod(CMDIInPkt* pInPkt) : pMDIInPkt(pInPkt) {}
	virtual string GetTagName(void);
	virtual void DecodeTag(CVector<_BINARY>& vecbiTag, const int iLenDataBits);
private:
	CMDIInPkt * pMDIInPkt;
};

class CTagItemDecoderStr : public CTagItemDecoder       
{
public:
	CTagItemDecoderStr(CMDIInPkt* pInPkt, const int iStreamNum) : pMDIInPkt(pInPkt) , iStreamNumber(iStreamNum) {}
	void SetStreamNumber(const int iNumber); // Should be called just after construction to identify which tag it is
	virtual string GetTagName(void);
	virtual void DecodeTag(CVector<_BINARY>& vecbiTag, const int iLenDataBits);
private:
	int iStreamNumber;
	CMDIInPkt * pMDIInPkt;
};

class CTagItemDecoderSDCChanInf : public CTagItemDecoder
{
public:
	CTagItemDecoderSDCChanInf(CMDIInPkt* pInPkt) : pMDIInPkt(pInPkt) {}
	virtual string GetTagName(void);
	virtual void DecodeTag(CVector<_BINARY>& vecbiTag, const int iLenDataBits);
private:
	CMDIInPkt * pMDIInPkt;
};

class CTagItemDecoderInfo : public CTagItemDecoder      
{
public:
	CTagItemDecoderInfo(CMDIInPkt* pInPkt) : pMDIInPkt(pInPkt) {}
	virtual string GetTagName(void);
	virtual void DecodeTag(CVector<_BINARY>& vecbiTag, const int iLenDataBits);
private:
	CMDIInPkt * pMDIInPkt;
};

#endif
