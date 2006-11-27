/******************************************
 * DNS                                    *
 ******************************************
 * Copyright (C) 2004 Piotr Pawluczuk     *
 *                                        *
 * email: piotrek@piopawlu.net            *
 * www:   http://www.piopawlu.net         *
 ******************************************/

#ifndef _DNS_H__
#define _DNS_H__

#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#pragma comment(lib,"ws2_32")

#pragma once

//Flagi bitowe
#define FLAG_QRresp	0x8000
#define FLAG_OP_IQ	0x4000
#define FLAG_OP_ST	0x2000
#define FLAG_AA		0x0400
#define FLAG_TC		0x0200
#define FLAG_RD		0x0100
#define FLAG_RA		0x80

#define RCODE_FERR		8
#define RCODE_SFAIL		4
#define RCODE_NAMER		12
#define RCODE_NOTINP	2
#define RCODE_REFUSED	7
#define RCODE_ERROR		15

//<QTYPE>
#define QT_A		0x01 //a host address
#define QT_NS		0x02 //an authoritative name server
#define QT_MD		0x03 //a mail destination (Obsolete - use MX)
#define QT_MF		0x04 //a mail forwarder (Obsolete - use MX)
#define QT_CNAME	0x05 //the canonical name for an alias
#define QT_SOA		0x06 //marks the start of a zone of authority
#define QT_MB		0x07 //a mailbox domain name (EXPERIMENTAL)
#define QT_MG		0x08 ////a mail group member (EXPERIMENTAL)
#define QT_MR		0x09 //a mail rename domain name (EXPERIMENTAL)
#define QT_NULL		0x0A //a null RR (EXPERIMENTAL)
#define QT_WKS		0x0B //a well known service description
#define QT_PTR		0x0C //a domain name pointer
#define QT_HINFO	0x0D //host information
#define QT_MINFO	0x0E //mailbox or mail list information
#define QT_MX		0x0F //mail exchange
#define QT_TXT		0x10 //text strings
#define QT_AAAA		0x1C //aaaa
#define QT_SRV		0x21 //srv
//</QTYPE>

//<QCLASS>
#define QCLASS_IN		1
#define QCLASS_CS		2
#define QCLASS_CH		3
#define QCLASS_HS		4
//</QCLASS>

#define BUFF_LEN 512

#pragma pack(push,1)
typedef struct tagDNSHeader
{
	short ID;		//16 bitowy identyfikator zapytania
	short Flags;	//|QR|Opcode |AA|TC|RD|RA|  Z  | RCODE |
					//|0 |1 2 3 4|5 |6 |7 |8 |9 0 1|2 3 4 5|
	short QDCount;	//Liczba wpisow w sekcji pytan
	short ANCount;	//Liczba wpisow w sekcji odpowiedzi
	short NSCount;	//Liczba wpisow na serwerze w sekcji Authority
	short ARCount;	//Liczba wpisow w sekcji dodatkowych rekordow
}DNSHEADER,*LPDNSHEADER;

//DNSRecord
struct sDnsRecord
{
	BYTE	m_qname[256];
	SHORT	m_type;
	SHORT	m_class;
	LONG	m_ttl;
	SHORT	m_size;
	union{
		BYTE m_buffer[BUFF_LEN];
		struct {
			unsigned short priority;
			unsigned short weight;
			unsigned short port;
			char domain[BUFF_LEN - 6];
		}srv;
		struct{
			unsigned short priority;
			char domain[BUFF_LEN - 2];
		}mx;
		struct{
			unsigned char b[6];
		}ip6;
		unsigned long addr;
	};
public:
	sDnsRecord(){memset(this,0,sizeof(sDnsRecord));}
};
#pragma pack(pop)


inline long dns_long(unsigned char* buff,unsigned char net);
inline short dns_short(unsigned char* buff,unsigned char net);

unsigned char* dns_getrecord(unsigned char* buff,unsigned char* cb,int bufflen,sDnsRecord *record);
unsigned char* dns_skipqname(unsigned char* buff,int bufflen);
unsigned char* dns_decodename(unsigned char* buff,unsigned char* cb,unsigned int bufflen,unsigned char* out);
unsigned char* dns_createqname(const char* address,unsigned char* buff);

unsigned int dns_preparequery(const char* data,unsigned char* buff,unsigned int bufflen,unsigned char type,short id,short opt);
unsigned long dns_processreply(unsigned char* buff,int bufflen,sDnsRecord** table,int get_addtional);

int dns_query(unsigned long dnsaddr,
			  unsigned char* buffer,
			  unsigned int qlen,
			  unsigned char* out,
			  unsigned int outlen);

#endif //_DNS_H__
