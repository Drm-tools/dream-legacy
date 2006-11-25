/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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

#ifndef AUIDOSOURCEENCODERINTERFACE_H
#define AUIDOSOURCEENCODERINTERFACE_H

#include "../GlobalDefinitions.h"

/* Interfaces ********************************************************************/
class CAudioSourceEncoderInterface
{
public:
	virtual void AddTextMessage(const string& strText)=0;
	virtual void ClearTextMessages()=0;

	virtual void AddPic(const string& strFileName, const string& strFormat)=0;
	virtual void ClearPics()=0;
	virtual _BOOLEAN GetTransStat(string& strCPi, _REAL& rCPe)=0;
};

#endif
