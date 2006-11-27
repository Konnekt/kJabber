/******************************************
 * DNS                                    *
 ******************************************
 * Copyright (C) 2004 Piotr Pawluczuk     *
 *                                        *
 * email: piotrek@piopawlu.net            *
 * www:   http://www.piopawlu.net         *
 ******************************************/

#include "dns.h"

int dns_query(unsigned long dnsaddr,unsigned char* buffer,unsigned int qlen,
			  unsigned char* out,unsigned int outlen)
{
	SOCKET s = 0;
	sockaddr_in dst = {0};
	sockaddr_in src = {0};
	timeval tv = {10,10};
	fd_set fd={0};
	int result = 0;

	s = socket(AF_INET,SOCK_DGRAM,IPPROTO_IP);
	if(s == INVALID_SOCKET){
		return 0;
	}

	dst.sin_addr.s_addr = (dnsaddr);
	dst.sin_port = htons(53);
	dst.sin_family = AF_INET;

	if(sendto(s,(const char*)buffer,(int)qlen,0,(const sockaddr*)&dst,sizeof(dst)) == SOCKET_ERROR){
		goto Error;
	}

	fd.fd_count = 1;
	*fd.fd_array = s;

	if(select(s+1,&fd,NULL,NULL,&tv) > 0)
	{
		result = sizeof(src);
		if((result = recvfrom(s,(char*)out,(int)outlen,0,(sockaddr*)&src,(int*)&result)) <= 0){
			goto Error;
		}else{
			closesocket(s);
			return result;
		}
	}
Error:
	printf("%d",WSAGetLastError());
	closesocket(s);
	return 0;
}

unsigned int dns_preparequery(const char* data,unsigned char* buff,
					unsigned int bufflen,unsigned char type,short id,short opt)
{
	unsigned char* cb = buff;
	LPDNSHEADER header = (LPDNSHEADER)cb;

	if(bufflen <= strlen(data) + 20){
		return NULL;
	}

	header->ID = htons(id);
	header->Flags = htons(opt);
	header->QDCount = htons(1);
	header->NSCount = 0;
	header->ANCount = 0;
	header->ARCount = 0;

	cb += 12;
	cb = dns_createqname(data,cb);
	if(!cb){
		return NULL;
	}

	//QTYPE # QCLASS_IN
	*cb++ = 0;
	*cb++ = type;
	*cb++ = 0;
	*cb++ = 1;
	return (cb - buff);
}

unsigned long dns_processreply(unsigned char* buff,int bufflen,
							   sDnsRecord** table,int ga)
{
	int n = 0;
	int count = 0;
	unsigned char* cb = buff;
	sDnsRecord* temp = NULL;

	LPDNSHEADER header = (LPDNSHEADER)cb;

	header->ANCount = ntohs(header->ANCount);
	header->ARCount = ntohs(header->ARCount);
	header->NSCount = ntohs(header->NSCount);
	header->QDCount = ntohs(header->QDCount);
	header->ID = ntohs(header->ID);
	cb += 12;
	//buff += 12;

	if(!(cb = dns_skipqname(cb,bufflen))){
		return 0;
	}
	cb += 4;

	count = (header->ANCount) + ((ga)?(header->ARCount + header->NSCount):(0));
	temp = (sDnsRecord*)malloc(sizeof(sDnsRecord) * count);

	if(!temp){
		return 0;
	}
	memset(temp,0,sizeof(sDnsRecord) * count);

	for(int i=0;i<header->ANCount;i++)
	{
		cb = dns_getrecord(buff,cb,bufflen,&temp[n++]);
		if(!cb){
			goto Error;
		}
	}

	if(ga)
	{
		for(int i=0;i<header->NSCount;i++)
		{
			cb = dns_getrecord(buff,cb,bufflen,&temp[n++]);
			if(!cb){
				goto Error;
			}
		}

		for(int i=0;i<header->ARCount;i++)
		{
			cb = dns_getrecord(buff,cb,bufflen,&temp[n++]);
			if(!cb){
				goto Error;
			}
		}
	}

	*table = temp;
	return n;
Error:
	if(temp){
		free(temp);
	}
	return 0;
}

unsigned char* dns_decodename(unsigned char* buff,unsigned char* cb,
							  unsigned int bufflen,unsigned char* out)
{
	unsigned char* os = cb;
	unsigned char* max = (buff + bufflen);
	int first = 1;
	int jumped = 0;
	int len = 0;

	while(*cb && (cb < max))
	{
		if(*cb >= 0xC0){
			if(!jumped++){
				os = cb;
			}
			cb = buff + ((*cb - 0xC0) << 8) + *(cb+1);
		}else if(*cb <= 0x3F){
			if(first){
				first = 0;
			}else{
				*out++ = '.';
			}
			len = *cb++;
			if(len + buff > max){
				return NULL;
			}
			while(len-- > 0){
				*out++ = *cb++;
			}
		}else{
			return NULL;
		}
	}
	return (jumped)?(os + 2):(cb+1);
}

unsigned char* dns_getrecord(unsigned char* buff,unsigned char* cb,int bufflen,sDnsRecord *record)
{
	unsigned short rlen;

	cb = dns_decodename(buff,cb,bufflen,record->m_qname);
	if(!cb){
		return NULL;
	}

	record->m_type = dns_short(cb,1);
	cb+=2;
	record->m_class = dns_short(cb,1);
	cb+=2;
	record->m_ttl = dns_long(cb,1);
	cb+=4;
	rlen = dns_short(cb,1);
	cb+=2;

	if(rlen >= 512){
		return NULL;
	}

	record->m_size = (short)strlen((char*)record->m_buffer);

	switch(record->m_type)
	{
	case QT_MX:
		record->mx.priority = dns_short(cb,1);
		if(!dns_decodename(buff,cb+2,bufflen,record->m_buffer)){
			return NULL;
		}
		break;
	case QT_MR:
	case QT_MB:
	case QT_MD:
	case QT_MF:
	case QT_MG:
	case QT_NS:
	case QT_PTR:
	case QT_SOA:
	case QT_CNAME:
		if(!dns_decodename(buff,cb,bufflen,record->m_buffer)){
			return NULL;
		}
		record->m_size = (short)strlen((char*)record->m_buffer);
		break;
	case QT_A:
		record->addr = dns_long(cb,1);
		break;
	case QT_SRV:
		record->srv.port = dns_short(cb,1);
		record->srv.weight = dns_short(cb+2,1);
		record->srv.priority = dns_short(cb+4,1);
		if(!dns_decodename(buff,cb+6,bufflen,(unsigned char*)record->srv.domain)){
			return NULL;
		}
		break;
	default:
		record->m_size = rlen;
		memcpy(record->m_buffer,cb,rlen);
	}
	return cb + rlen;
}

unsigned char* dns_skipqname(unsigned char* buff,int bufflen)
{
	unsigned char* max = buff + bufflen;
	while(*buff){
		buff += *buff + 1;
	}

	if(buff > max){
		return NULL;
	}else{
		return buff + 1;
	}
}

inline long dns_long(unsigned char* buff,unsigned char net){
	return (net)?(ntohl(*((unsigned long*)buff))):(*((unsigned long*)buff));
}

inline short dns_short(unsigned char* buff,unsigned char net){
	return (net)?(ntohs(*((unsigned short*)buff))):(*((unsigned short*)buff));
}

unsigned char* dns_createqname(const char* a,unsigned char* buff)
{
	int l = 0;
	unsigned char* b=buff;
	unsigned char* lp=b;

	*b++ = '\0';

	while(*a){
		if(*a == '.')
		{
			*b='\0';
			lp = b++;
			a++;
		}else{
			(*lp)++;
			*b++ = *a++;
		}
	}
	*b++ = '\0';
	return b;
}