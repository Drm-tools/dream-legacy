/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable, Oliver Haffenden
 *
 * Description:
 *  
 * see PacketSourceFile.h
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

#include "PacketSourceFile.h"
#include <iostream>
#include <errno.h>
#include <qtimer.h>
#include <qstringlist.h>

#ifdef _WIN32
  /* Always include winsock2.h before windows.h */
    /* winsock2.h is already included into libpcap */
# include <winsock2.h>
# include <ws2tcpip.h>
# include <windows.h>
#else
# include <netinet/in.h>
# include <arpa/inet.h>
/* Some defines needed for compatibility when using Linux, Darwin, ... */
typedef int SOCKET;
# define SOCKET_ERROR				(-1)
# define INVALID_SOCKET				(-1)
#endif

#ifdef HAVE_LIBPCAP
# include <pcap.h>
#endif
#ifdef HAVE_LIBWTAP
extern "C" {
# include <wtap.h>
}
#endif

const int iMaxPacketSize = 4096;
const int iAFHeaderLen = 10;
const int iAFCRCLen = 2;

CPacketSourceFile::CPacketSourceFile():pPacketSink(NULL), timeKeeper(), pf(NULL), bRaw(TRUE)
{
}

_BOOLEAN
CPacketSourceFile::SetOrigin(const string& str)
{
	if(str.rfind(".pcap") == str.length()-5)
	{
#ifdef HAVE_LIBPCAP
		char errbuf[PCAP_ERRBUF_SIZE];
		pf = pcap_open_offline(str.c_str(), errbuf);
#endif
#ifdef HAVE_LIBWTAP
		int err;
		gchar *err_info;
		pf = wtap_open_offline(str.c_str(), &err, &err_info, FALSE);
#endif
		bRaw = FALSE;
	}
	else
	{
		pf = fopen(str.c_str(), "rb");
	}
	if ( pf != NULL)
	{
		timeKeeper = QTime::currentTime();
		QTimer::singleShot(400, this, SLOT(OnDataReceived()));
    }
	return pf != NULL;
}

CPacketSourceFile::~CPacketSourceFile()
{
	if(bRaw && pf)
		fclose((FILE*)pf);
	else if(pf)
#ifdef HAVE_LIBPCAP
		pcap_close((pcap_t*)pf)
#endif
#ifdef HAVE_LIBWTAP
		wtap_close((wtap*)pf)
#endif
;
}

// Set the sink which will receive the packets
void
CPacketSourceFile::SetPacketSink(CPacketSink * pSink)
{
	pPacketSink = pSink;
}

// Stop sending packets to the sink
void
CPacketSourceFile::ResetPacketSink()
{
	pPacketSink = NULL;
}

void
CPacketSourceFile::OnDataReceived ()
{
	if(pf==NULL)
		return;

	vector < _BYTE > vecbydata (iMaxPacketSize);
	if(bRaw)
	{
		/* TODO read a raw or FF encapsulated file, but for raw we need to read enough
		 * of the DCP header to find a length, or read too much and get the sink to
		 * buffer
		 */
		vecbydata.resize(0);

		//const int iAFHeaderLen = 10;

		_BYTE header[iAFHeaderLen];
		// get the sync bytes
		fread(header, iAFHeaderLen, sizeof(_BYTE), (FILE *) pf);

		if (header[0] != 'A' || header[1] != 'F') // Not an AF file - return. TODO: add PF and re-synch on AF bytes
		{
			fclose((FILE *) pf);
			pf = 0;
			return;
		}

		// get the length
		int iAFPayloadLen = (header[2]<<24) + (header[3]<<16) + (header[4]<<8) + header[5];

		if (iAFPayloadLen + iAFHeaderLen + iAFCRCLen> iMaxPacketSize)
		{
			// throw?
			fclose((FILE *) pf);
			pf = 0;
			return;
		}

		// initialise the output vector
		vecbydata.resize(iAFPayloadLen + iAFHeaderLen + iAFCRCLen);

		int i;

		// Copy header into output vector
		for (i=0; i<iAFHeaderLen; i++)
		{
			vecbydata[i] = header[i];
		}

		// Copy payload into output vector
		_BYTE data;
		for (i=0; i<iAFPayloadLen + iAFCRCLen; i++)
		{
			fread(&data, 1, sizeof(_BYTE), (FILE *)pf);
			vecbydata[iAFHeaderLen + i] = data;
		}

	}
	else
	{ 
#if defined(HAVE_LIBWTAP) || defined(HAVE_LIBPCAP)
		int link_len = 0;
		const _BYTE* pkt_data;
#ifdef HAVE_LIBPCAP
   		struct pcap_pkthdr *header;
		int res;
		const u_char* data;
		/* Retrieve the packet from the file */
		if((res = pcap_next_ex( (pcap_t*)pf, &header, &data)) != 1)
		{
			pcap_close((pcap_t*)pf);
			pf = NULL;
			return;
		}
		else
		{
			int lt = pcap_datalink((pcap_t*)pf);
			pkt_data = (_BYTE*)data;
			/* 14 bytes ethernet header */
			if(lt==DLT_EN10MB)
			{
				link_len=14;
			}
			/* raw IP header ? */
			if(lt==DLT_RAW)
			{
				link_len=0;
			}
		}
#endif
#ifdef HAVE_LIBWTAP
		int res, err;
		gchar *err_info;
		long int data_offset;
		/* Retrieve the packet from the file */
		if((res = wtap_read((wtap*)pf, &err, &err_info, &data_offset)) == 0)
		{
			cout << "wtap read result: " << res << endl;
			wtap_close((wtap*)pf);
			pf = NULL;
			return;
		}
		else
		{
		 	struct wtap_pkthdr *phdr = wtap_phdr((wtap*)pf);
			pkt_data = (_BYTE*)wtap_buf_ptr((wtap*)pf);
			/* 14 bytes ethernet header */
			if(phdr->pkt_encap == WTAP_ENCAP_ETHERNET)
			{
				link_len=14;
			}
			/* raw IP header ? */
			if(phdr->pkt_encap == WTAP_ENCAP_RAW_IP)
			{
				link_len=0;
			}
		}
#endif
		/* 4n bytes IP header, 8 bytes UDP header */
		int udp_ip_hdr_len = 4*(pkt_data[link_len] & 0x0f) + 8;
		int ip_packet_len = ntohs(*(uint16_t*)&pkt_data[link_len+2]);
		int data_len = ip_packet_len - udp_ip_hdr_len;
		vecbydata.resize (data_len);
		for(int i=0; i<data_len; i++)
			vecbydata[i] = pkt_data[link_len+udp_ip_hdr_len+i];
#endif
	}
	/* Decode the incoming packet */
	if (pPacketSink != NULL)
		pPacketSink->SendPacket(vecbydata);
	QTime iNow = QTime::currentTime();
	int iDelay = 400 - timeKeeper.msecsTo(iNow);
	if(iDelay < 0)
		iDelay = 0;
	QTimer::singleShot(iDelay, this, SLOT(OnDataReceived()));
	timeKeeper = timeKeeper.addMSecs(400);
}
