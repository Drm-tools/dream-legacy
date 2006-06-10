/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Oliver Haffenden
 *
 * Description:
 *	see RSCITagItemDecoders.cpp
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

#ifndef RSCI_TAG_ITEM_DECODERS_H_INCLUDED
#define RSCI_TAG_ITEM_DECODERS_H_INCLUDED

#include "TagItemDecoder.h"

class CDRMReceiver;


// RSCI control

class CTagItemDecoderCfre : public CTagItemDecoder      
{
public:
	CTagItemDecoderCfre(CDRMReceiver *pReceiver) : pDRMReceiver(pReceiver) {}
	void SetReceiver(CDRMReceiver *pReceiver) {pDRMReceiver = pReceiver;}
	virtual string GetTagName(void);
	virtual void DecodeTag(CVector<_BINARY>& vecbiTag, const int iLenDataBits);
private:
	CDRMReceiver *pDRMReceiver;
};

#endif
