/*
 *  kJabber
 *  
 *  Please READ & ACCEPT /License.txt FIRST! 
 * 
 *  Copyright (C)2005,2006 Rafa³ Lindemann, STAMINA
 *  http://www.stamina.pl/ , http://www.konnekt.info/
 *
 *  $Id: $
 */



#include "stdafx.h"
#include "resource.h"
extern "C" {
#include "base64.h"
};
#include "Iphlpapi.h"
#include "dns.h"

cJabber::cJabber(cCtrl * ctrl):cJabberBase(ctrl) {
	net = kJabber::net;
	/* Prosta sztuczka na wiele kont */
	if (IMessage(IM_PLUG_NAME , net , IMT_PROTOCOL)) {
		do {
			net = 170 + (++slotID) - '0';
		} while (IMessage(IM_PLUG_NAME , net , IMT_PROTOCOL));
	}
	dtPrefix = "kJabber/slot";
	dtPrefix += slotID;
	socket = 0;
	ssl = 0;
	ssl_ctx = 0;
	createAccount = false;
	realHost = "";
	realPort = 0;
	session.evtTransmitXML.connect(SigC::slot(*this , &cJabber::OnTransmitXML));
	session.evtAuthError.connect(SigC::slot(*this , &cJabber::OnAuthError));
	session.evtConnected.connect(SigC::slot(*this , &cJabber::OnConnected));
	session.evtDisconnected.connect(SigC::slot(*this , &cJabber::OnDisconnected));
	session.evtOnRoster.connect(SigC::slot(*this , &cJabber::OnRoster));
	session.evtMessage.connect(SigC::slot(*this , &cJabber::OnMessage));
	session.evtPresence.connect(SigC::slot(*this , &cJabber::OnPresence));
	session.evtPresenceRequest.connect(SigC::slot(*this , &cJabber::OnPresenceRequest));
	session.evtOnVersion.connect(SigC::slot(*this , &cJabber::OnVersion));
	session.evtOnLast.connect(SigC::slot(*this , &cJabber::OnLast));
	session.evtOnTime.connect(SigC::slot(*this , &cJabber::OnTime));
	session.roster().evtUpdateDone.connect(SigC::slot(*this , &cJabber::OnRosterItemUpdated));
	session.roster().evtRemovingItem.connect(SigC::slot(*this , &cJabber::OnRosterItemRemoving));
	session.roster().evtItemAdded.connect(SigC::slot(*this , &cJabber::OnRosterItemAdded));

	session.addFeature("http://jabber.org/protocol/xhtml-im");
}// inicjalizacja

const CStdString & cJabber::GetName() {
	static CStdString n("");
	if (!n.empty()) 
		return n;
	if (this->GetSlot()) {
		CStdString file;
		GetModuleFileName(plug->hDll() , file.GetBuffer(255) , 255);
		file.ReleaseBuffer();
		file.Replace("/" , "\\");
		file.erase(0 , file.find_last_of("\\")+1);
		file.erase(file.find_last_of("."));
		return n = file;
	}
	return n = "Jabber";
}


void cJabber::StartSSL() {
	if (!plug->DTgetInt(DTCFG , 0 , this->dtPrefix + "/SSL"))
		return;
	plug->IMDEBUG(DBG_FUNC , "- StartSSL()");
	OpenSSL_add_ssl_algorithms();

	if (!RAND_status()) {
		char rdata[1024];
		struct {
			time_t time;
			void *ptr;
		} rstruct;
		time(&rstruct.time);
		rstruct.ptr = (void *) &rstruct;			
		RAND_seed((void *) rdata, sizeof(rdata));
		RAND_seed((void *) &rstruct, sizeof(rstruct));
	}
	ssl_ctx = SSL_CTX_new(TLSv1_client_method());
	if (!ssl_ctx) {
		throw "SSL_CTX_new returned 0";
	}
	SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, NULL);
	ssl = SSL_new(ssl_ctx);
	if (!ssl)
		throw "SSL_new returned 0";

	SSL_set_fd(ssl , socket);
	// * * * * * * * * * * * * * 
	plug->IMDEBUG(DBG_NET , "- SSL Negotiation");
	while (1) {
		int res;
		if ((res = SSL_connect(ssl)) <= 0) {
			int err = SSL_get_error(ssl, res);
			if (res == 0) {
				throw "Disconnected during negotiation";
			}
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
				continue;
			} else {
				char buf[1024];
				ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
				throw buf;
			}
		}
		break; // sztuczka
	}
	IMDEBUG(DBG_MISC, "- TLS ready cipher=%s\n", SSL_get_cipher_name(ssl));
	X509 * peer = SSL_get_peer_certificate(ssl);
	if (!peer)
		IMDEBUG(DBG_WARN , "! Couldn't obtain peer certificate!");
	else {
		char buf[1024];
		X509_NAME_oneline(X509_get_subject_name(peer), buf, sizeof(buf));
		IMDEBUG(DBG_MISC, "- Certificate subject: %s" , buf);
		X509_NAME_oneline(X509_get_issuer_name(peer), buf, sizeof(buf));
		IMDEBUG(DBG_MISC, "- Certificate issuer: %s" , buf);
	}

}

std::string discoverService(std::string host, unsigned short& port, const std::string& service) {
	unsigned char query[256];
	unsigned char reply[512];
	unsigned long  ql = 0;
	int result = 0;

	FIXED_INFO fi;
	const char* dnsIp = "194.204.159.1";
	ULONG size = sizeof(FIXED_INFO);
	if (GetNetworkParams(&fi, &size) == ERROR_SUCCESS) {
		if (fi.DnsServerList.IpAddress.String[0] != 0) {
			//dnsIp = fi.DnsServerList.IpAddress.String;
		}
	}

	unsigned long dnsAddr = inet_addr(dnsIp);
	sDnsRecord* records = NULL;

	IMDEBUG(DBG_FUNC, "DNS SRV (%s.%s)", service.c_str(), host.c_str());

	ql = dns_preparequery((service + "." + host).c_str(),query,sizeof(query),QT_SRV,2323,FLAG_RD);
	result = dns_query(dnsAddr,query,ql,reply,512);
	
	if(result) {

		ql = dns_processreply(reply,result + 1,&records,0);

		if (ql) {
			if (records->m_type == QT_SRV) {
				host = records->srv.domain;
				if (port == 0 && records->srv.port) {
					port = records->srv.port;
				}
				IMDEBUG(DBG_FUNC, "DNS SRV discovered! (host=%s port=%d)", host.c_str(), records->srv.port);
			} else if (records->m_type == QT_CNAME) {
				host = (char*)records->m_buffer;
				IMDEBUG(DBG_FUNC, "DNS CNAME discovered! (host=%s)", host.c_str());
			}
		}

		if(records){
			free(records);
		}

	}

	return host;

}

// g³ówny w¹tek...

unsigned int cJabber::ListenThread(void) {
	this->canReconnect = true;
	state = cs_connecting;
	synchronized = false;
	plug->ICMessage(IMC_SETCONNECT , 0);
	plug->IMessage(&sIMessage_StatusChange(IMC_STATUSCHANGE , 0 , ST_CONNECTING , ""));
	currentStatus = ST_CONNECTING;	
	try {
		bool proxy = GETINT(CFG_PROXY) && GETINT(CFG_PROXY_HTTP_ONLY)==0;

		this->realHost = "";
		this->realPort = 0; // ¿eby mo¿na by³o wczytaæ wartosci z ustawien...
		this->realPort = this->GetPort(false);
		CStdString xmppHost = this->GetHost(false);
		this->realHost = discoverService(this->GetHost(false), this->realPort, "_xmpp-client._tcp");

		CStdString server = proxy ? GETSTR(CFG_PROXY_HOST) : this->GetHost(true);

		IMDEBUG(DBG_NET, "Using %s:%d as host:port", this->GetHost(true).c_str(), this->GetPort(true));

		this->socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (!this->socket)
			throw "Could not create socket!";
		sockaddr_in sin;
		sin.sin_family = AF_INET;
		sin.sin_port = htons( proxy ? (GETINT(CFG_PROXY_PORT)?GETINT(CFG_PROXY_PORT):8080) : this->GetPort(true) );
		plug->IMDEBUG(DBG_MISC , "- gethostbyname");
		hostent * he = gethostbyname(server);
		if (!he) {
			throw "Could not resolve!";
		}

		IMDEBUG(DBG_NET, "Connecting to %s:%d (addr=%x)", server.c_str(), ntohs(sin.sin_port), he->h_addr_list[0]);

		memcpy(&sin.sin_addr  ,  he->h_addr_list[0] , 4);
		plug->IMDEBUG(DBG_MISC , "- connecting");
		if (connect(socket, (struct sockaddr*) &sin, sizeof(sin))) { 
			throw "";
		}
		plug->IMDEBUG(DBG_MISC , "- connected");
		if (proxy && !StartProxy()) {
			OnAuthError(0 , "B³¹d autoryzacji proxy!");
			throw "Couldn't start proxy!";
		}
		StartSSL();
		{
			this->priority = plug->DTgetStr(DTCFG , 0 , plug->DTgetNameID(DTCFG , dtPrefix + "/priority"));
			CStdString login , pass;
			GetUserAndPass(login , pass);
			if (login.empty()) throw kJabber::cExceptionQuiet(this , "Login" , "Cancelled");
			session.connect(xmppHost , jabberoo::Session::atAutoAuth 
				,  login , GETSTR(plug->DTgetNameID(DTCFG , dtPrefix + "/resource")) , pass , this->createAccount , true);
		}

		const int size = 1024;
		char buffer[size+1]; 
		while (1) {
			int r;
			if (ssl) {
				r = SSL_read(ssl , buffer , size);
				if (r <= 0) {
					int err = ssl ? SSL_get_error(ssl , r) : -1;
					if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
						continue;
					else {
						char buf[1024];
						ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
						throw buf;
					}
				}
			} else {
				r = recv(socket , buffer , size , 0);
				if (r <= 0) {
					if (r<0 && (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == ERROR_IO_PENDING))
						continue; //loop
					else
						throw r;
				}
			}
			buffer[r] = 0;
//			this->Lock();
			plug->IMDEBUG(DBG_TRAFFIC , "--<< RCV [%s] <<--" , buffer);
			this->session.push(buffer , r);
//			this->Unlock();
		}
	} catch (kJabber::cExceptionQuiet e) {
		plug->IMDEBUG(DBG_ERROR , "!ListenThread Exception - %s :: %s" , e.GetWhere().c_str() , e.GetMsg().c_str());
	} catch (const char * e) {
		plug->IMDEBUG(DBG_ERROR , "!ListenThread Error - %s" , e);
	} catch (int e) {
		plug->IMDEBUG(DBG_ERROR , "Socket Error! e=%d WSA=%d SSL=%d" , e , WSAGetLastError() , ssl?SSL_get_error(ssl,e):0);
	}
	bool onRequest = (currentStatus == ST_OFFLINE);
	Disconnect();
	if (!onRequest && this->canReconnect)
		plug->ICMessage(IMC_SETCONNECT , 1);
	CloseHandle(thread);

    int c = plug->ICMessage(IMC_CNT_COUNT);
    for (int i=1 ; i<c;i++) {
        if (GETCNTI(i,CNT_NET) == this->net) {
			CntSetStatus(i , ST_OFFLINE , "");
			plug->ICMessage(IMI_CNT_DEACTIVATE , i);
			SETCNTI(i , CNT_STATUS , 0 , ST_HIDEBOTH);
			SetCntIcon(i , ST_OFFLINE , plug->DTgetStr(DTCNT , i , CNT_UID));
        }
    }
    plug->ICMessage(IMI_REFRESH_LST);
	session.roster().clear();
	thread = 0;
	if (socket)
		closesocket(socket);
	if (ssl)
		SSL_free(ssl);
	if (ssl_ctx)
		SSL_CTX_free(ssl_ctx);
	socket = 0;
	ssl = 0;
	ssl_ctx = 0;
	plug->IMDEBUG(DBG_MISC , "- thread finished");
	state = cs_offline;
	return 0;
}

void cJabber::DisconnectSocket() {
	session.disconnect();
	session.evtDisconnected();
}
bool cJabber::SendPresence(const CStdString & to , int status , const char * info) {
	if (status == -1) status = this->currentStatus;
	if (info == 0) info = this->currentStatusInfo;
	if (this->Connected()) {
		Presence::Show show = status == ST_ONLINE ? jabberoo::Presence::stOnline 
			: status == ST_AWAY ? Presence::stAway 
			: status == ST_NA ? Presence::stXA
			: status == ST_CHAT ? Presence::stChat
			: status == ST_DND ? Presence::stDND 
			: status == ST_HIDDEN ? (Presence::Show)0 
			: jabberoo::Presence::stOffline
			;
		this->session << jabberoo::Presence(to 
			, status == ST_HIDDEN ? Presence::ptInvisible : status != ST_OFFLINE?jabberoo::Presence::ptAvailable : jabberoo::Presence::ptUnavailable 
			, show , kJabber::LocaleToUTF8(info) , this->priority);
	}
	return true;
}

bool cJabber::SendSubscribed(const CStdString & to , bool subscribed) {
	session << Presence( to , subscribed ? Presence::ptSubscribed : Presence::ptUnsubscribed);
	return true;
}
bool cJabber::SendSubRequest(const CStdString & to , bool subscribe) {
	session << Presence( to , subscribe ? Presence::ptSubRequest : Presence::ptUnsubRequest);
	return true;
}

void cJabber::CntSearch(sCNTSEARCH * sr) {
	if ((!sr->email || !*sr->email) && (!sr->nick || !*sr->nick)
		&& (!sr->name || !*sr->name) && (!sr->surname || !*sr->surname))
	{
		plug->IMessage(&sIMessage_msgBox(IMI_ERROR , "Musisz podaæ email/nick/imiê/nazwisko!" , GetName() + " - wyszukiwanie"));
		return;
	}
	cJabberBase::CntSearch(sr);
}


bool cJabber::SendCntSearch(sCNTSEARCH * sr) {
    judo::Element iq("iq");
    iq.putAttrib("type", "set");
	iq.putAttrib("to", plug->DTgetStr(DTCFG , 0 , plug->DTgetNameID(DTCFG , this->dtPrefix + "/JUDserver")));
	iq.putAttrib("from", session.getLocalJID());
	CStdString id = session.getNextID();
	iq.putAttrib("id" , id);
    judo::Element* query = iq.addElement("query");
    query->putAttrib("xmlns", "jabber:iq:search");
	if (sr->email && *sr->email) {
		query->addElement("email" , kJabber::LocaleToUTF8(sr->email));
	}
	if (sr->nick && *sr->nick) {
		query->addElement("nick" , kJabber::LocaleToUTF8(sr->nick));
	}
	if (sr->name && *sr->name) {
		query->addElement("first" , kJabber::LocaleToUTF8(sr->name));
	}
	if (sr->surname && *sr->surname) {
		query->addElement("last" , kJabber::LocaleToUTF8(sr->surname));
	}
	session.registerIQ(id , SigC::slot(*this , OnIQSearchReply));
	session << iq;
	return true;
}

bool cJabber::SendVCardRequest(int cntID) {
    judo::Element iq("iq");
    iq.putAttrib("type", "get");
	CStdString id = session.getNextID();
	iq.putAttrib("id" , id);
	if (plug->DTgetPos(DTCNT , cntID)) {
		iq.putAttrib("to", plug->DTgetStr(DTCNT , cntID , CNT_UID));
	} else {
		// From powinien dodaæ serwer?
		iq.putAttrib("from", session.getLocalJID());
	}
    judo::Element* vCard = iq.addElement("vCard");
	vCard->putAttrib("xmlns" , "vcard-temp");
	vCard->putAttrib("version", "2.0"); 
	session.registerIQ(id , SigC::slot(*this , OnIQVCardRequestReply));
	session << iq;
	return true;
}

void cJabber::vCardSetEmail(judo::Element * vCard , CStdString emails , const char * location) {
	size_t start = 0;
	size_t end;
	emails.Remove('\\r');
	while (start < emails.size()) {
		end = emails.find("\n",start);
		if (end == -1) end = emails.size();
		CStdString userid = emails.substr(start , end - start);
		userid = userid.Trim();
		if (userid.find("@") == -1) {
			start = end + 1;
			continue;
		}
		judo::Element * email = vCard->addElement("EMAIL");
		if (location)
			email->addElement(location);
		email->addElement("USERID" , LocaleToUTF8( userid ));
		start = end + 1;
	}
}
void cJabber::vCardSetPhone(judo::Element * vCard , CStdString phones , const char * type , const char * location) {
	size_t start = 0;
	size_t end;
	phones.Remove('\\r');
	while (start < phones.length()) {
		end = phones.find("\n",start);
		if (end == -1) end = phones.length();
		CStdString number = phones.substr(start , end - start);
		number = number.Trim();
		if (number.empty()) {
			start = end + 1;
			continue;
		}
		judo::Element * tel = vCard->addElement("TEL");
		if (location)
			tel->addElement(location);
		if (type)
			tel->addElement(type);
		tel->addElement("NUMBER" , LocaleToUTF8( number ));
		start = end + 1;
	}
}
judo::Element * cJabber::vCardSetAddress(judo::Element * vCard , bool window , const CStdString & path) {
	judo::Element * addr = vCard->addElement("ADR");
	addr->addElement("EXTADR" , LocaleToUTF8( CntGetInfoValue(window , 0 , plug->DTgetNameID(DTCNT , path + "Address_More"))));
	addr->addElement("STREET" , LocaleToUTF8( CntGetInfoValue(window , 0 , plug->DTgetNameID(DTCNT , path + "Street"))));
	addr->addElement("POBOX" , LocaleToUTF8( CntGetInfoValue(window , 0 , plug->DTgetNameID(DTCNT , path + "POBox"))));
	addr->addElement("PCODE" , LocaleToUTF8( CntGetInfoValue(window , 0 , plug->DTgetNameID(DTCNT , path + "PostalCode"))));
	addr->addElement("LOCALITY" , LocaleToUTF8( CntGetInfoValue(window , 0 , plug->DTgetNameID(DTCNT , path + "Locality"))));
	addr->addElement("REGION" , LocaleToUTF8( CntGetInfoValue(window , 0 , plug->DTgetNameID(DTCNT , path + "Region"))));
	addr->addElement("CTRY" , LocaleToUTF8( CntGetInfoValue(window , 0 , plug->DTgetNameID(DTCNT , path + "Country"))));
	return addr;
}


bool cJabber::SendVCardSet(bool window) {
    judo::Element iq("iq");
    iq.putAttrib("type", "set");
	CStdString id = session.getNextID();
	iq.putAttrib("id" , id);
    judo::Element* vCard = iq.addElement("vCard");
	vCard->putAttrib("xmlns" , "vcard-temp");
	vCard->putAttrib("version", "2.0"); 

	/* Czêœæ pól za³atwiamy prostsz¹ drog¹... */
	jabberoo::vCard vc(iq);
	vc[vCard::Field::Firstname] = LocaleToUTF8( CntGetInfoValue(window , 0 , CNT_NAME) );
	vc[vCard::Field::Middlename] = LocaleToUTF8( CntGetInfoValue(window , 0 , CNT_MIDDLENAME) );
	vc[vCard::Field::Lastname] = LocaleToUTF8( CntGetInfoValue(window , 0 , CNT_SURNAME) );
//	vc[vCard::Field::Fullname] = vc[vCard::Field::Firstname] + " " + vc[vCard::Field::Lastname];
	int born = atoi(CntGetInfoValue(window , 0 , CNT_BORN));
	char buff [16];
	sprintf(buff , "%04d-%02d-%02d" , born >> 16 , (born & 0xFF00) >> 8 , born & 0xFF);
	vc[vCard::Field::Birthday] = buff;
	vc[vCard::Field::Nickname] = LocaleToUTF8( CntGetInfoValue(window , 0 , CNT_NICK) );
	vc[vCard::Field::Description] = LocaleToUTF8( CntGetInfoValue(window , 0 , CNT_DESCRIPTION) );
	vc[vCard::Field::URL] = LocaleToUTF8( CntGetInfoValue(window , 0 , CNT_URL) );
	vc[vCard::Field::OrgName] = LocaleToUTF8( CntGetInfoValue(window , 0 , CNT_WORK_ORGANIZATION) );
	vc[vCard::Field::OrgUnit] = LocaleToUTF8( CntGetInfoValue(window , 0 , CNT_WORK_ORG_UNIT) );
	vc[vCard::Field::PersonalTitle] = LocaleToUTF8( CntGetInfoValue(window , 0 , CNT_WORK_TITLE) );
	vc[vCard::Field::PersonalRole] = LocaleToUTF8( CntGetInfoValue(window , 0 , CNT_WORK_ROLE) );
	iq = vc.getBaseElement(); // wykona update...
	vCard = iq.findElement("vCard");
	/* Resztê lecimy rêcznie... */
	vCardSetEmail(vCard , CntGetInfoValue(window , 0 , CNT_EMAIL) , "INTERNET");
	vCardSetEmail(vCard , CntGetInfoValue(window , 0 , CNT_EMAIL_MORE) , "INTERNET");
	vCardSetEmail(vCard , CntGetInfoValue(window , 0 , CNT_WORK_EMAIL) , "WORK");
	vCardSetPhone(vCard , CntGetInfoValue(window , 0 , CNT_PHONE) , "VOICE" , "HOME");
	vCardSetPhone(vCard , CntGetInfoValue(window , 0 , CNT_CELLPHONE) , "CELL" , 0);
	vCardSetPhone(vCard , CntGetInfoValue(window , 0 , CNT_FAX) , "FAX" , "HOME");
	vCardSetPhone(vCard , CntGetInfoValue(window , 0 , CNT_PHONE_MORE) , 0 , 0);
	vCardSetPhone(vCard , CntGetInfoValue(window , 0 , CNT_WORK_PHONE) , 0 , "WORK");
	vCardSetPhone(vCard , CntGetInfoValue(window , 0 , CNT_WORK_FAX) , "FAX" , "WORK");
	vCard->addElement("JABBERID" , JID::getUserHost(session.getLocalJID()));
	vCardSetAddress(vCard , window , "")->addElement("HOME");
	vCardSetAddress(vCard , window , "Work/")->addElement("WORK");
	session.registerIQ(id , SigC::slot(*this , OnIQVCardSetReply));
	session << iq;
	return true;
}


bool cJabber::SendMessage(cMessage * m) {
	if (!this->Connected()) 
		return false;
	if (!strcmp(m->toUid , "*")) {
		// specja³
		session << LocaleToUTF8(m->body).c_str();
	} else {
		jabberoo::Message jabMsg(m->toUid , "" , Message::mtChat);
		DiscoDB::Item * item;
		item = session.discoDB().find(m->toUid , "");
		if (!item || !(item->getState() & DiscoDB::Item::stInfo))
			item = session.discoDB().find(JID::getUserHost(m->toUid), "");
		if (!item || !(item->getState() & DiscoDB::Item::stInfo))
			item = session.discoDB().find(JID::getHost(m->toUid), "");
		if (item && (item->getState() & DiscoDB::Item::stInfo) && item->getIdentityList().size() > 0) {
			if (item->getIdentityList().front()->getCategory() == "conference") {
				jabMsg.setType(Message::mtGroupchat);
				jabMsg.setTo(JID::getUserHost(m->toUid));
			}
		}
		if (!GetExtParam(m->ext, MEX_TITLE).empty()) {
			jabMsg.setSubject(LocaleToUTF8(GetExtParam(m->ext, MEX_TITLE)));
		}
		if (m->flag & MF_HTML) {
			try {
				// zamieniamy na plain-text
				CStdString plain = m->body;
				RegEx preg;
				plain = preg.replace("/<br.*?>/" , "\n" , plain);
				plain = preg.replace("/<.+?>/" , "" , plain);
				// w body ustawi siê plain-tekstowa wersja
				jabMsg.setBody(LocaleToUTF8(plain), true);
				// a z orygina³u robimy xhtml-im
				judo::ElementStream es(0);
				judo::Element * childs = es.parseAtOnce("<html xmlns='http://jabber.org/protocol/xhtml-im'><body xmlns='http://www.w3.org/1999/xhtml'>" + LocaleToUTF8(m->body) + "</body></html>");
				jabMsg.getBaseElement().appendChild(childs);
				session << jabMsg;
			} catch (...) {
				plug->IMessage(&KNotify::sIMessage_notify("[kJabber] Wiadomoœæ formatowana zawiera b³¹d i nie mog³a zostaæ wys³ana!" , (unsigned int)0 , KNotify::sIMessage_notify::tError));
			}
		} else {
			jabMsg.setBody(LocaleToUTF8(m->body));
			session << jabMsg;
		}
	}
	return true;
}

bool cJabber::SendPing() {
	this->OnTransmitXML(" ");
	return false;
}


bool cJabber::RosterDeleteJID(const CStdString & jid) {
	session.roster().deleteUser( jid );
	return true;
}

bool cJabber::RosterContainsJID(const CStdString & jid) {
	return session.roster().containsJID(jid);
}

void cJabber::SetContact(int cntID , Roster::Item & item) {
	bool changed = false;
	enSynchDirection direction = CFGsynchDirection();
	if (cntID == -1) {
		if (!plug->DTgetInt(DTCFG , 0 , kJabber::CFG::synchAddContacts) || direction == sd_disabled)
			return;
//		cntID = plug->ICMessage(IMC_CNT_ADD , this->net , (int)JID::getUserHost(item.getJID()).c_str());
		cntID = plug->ICMessage(IMC_CNT_ADD , this->net , (int)item.getJID().c_str());
		changed = true;
	}
	if (direction != sd_toClient && direction != sd_disabled) {
		plug->DTsetStr(DTCNT , cntID , CNT_DISPLAY , UTF8ToLocale(item.getNickname()).c_str());
		CStdString group = item.empty() ? "" : UTF8ToLocale(*(item.begin()));
		if (group == "Unfiled" || group == "Pending")
			group = ""; // obejœcie...
		plug->ICMessage(IMC_GRP_ADD , (int)group.c_str());
		plug->DTsetStr(DTCNT , cntID , CNT_GROUP , group);
	}
	int subs = 0;
	switch (item.getSubsType()) {
		case Roster::rsNone: subs = ST_HIDEBOTH; break;
		case Roster::rsFrom: subs = ST_HIDESHISSTATUS; break;
		case Roster::rsTo: subs = ST_HIDEMYSTATUS; break;
		case Roster::rsBoth: subs = 0; break;
	}
	plug->DTsetInt(DTCNT , cntID , CNT_STATUS , subs , ST_HIDEBOTH);
	plug->ICMessage(changed?IMC_CNT_CHANGED : IMI_REFRESH_CNT , cntID);
}

void cJabber::SetRosterItem(int cntID) {
	if (cntID == -1) 
		return;
	CStdString jid = plug->DTgetStr(DTCNT , cntID , CNT_UID);
	if (plug->DTgetInt(DTCNT , cntID , CNT_STATUS) & ST_NOTINLIST)
		return;
	if (!session.roster().containsJID(jid)
		&& !plug->DTgetInt(DTCFG , 0 , kJabber::CFG::synchAddContacts))
		return;
	jabberoo::Roster::Item item(jid , LocaleToUTF8(plug->DTgetStr(DTCNT , cntID , CNT_DISPLAY)));
	const char * group = plug->DTgetStr(DTCNT , cntID , CNT_GROUP);
	if (* group) {
		item.addToGroup(LocaleToUTF8(group));
	}
	session.roster() << item;
}

void cJabber::SetCntIcon(int cntID , int status , const CStdString & jid) {
	plug->DTsetInt(DTCNT , cntID , CNT_STATUS_ICON , plug->DTgetInt(DTCFG , 0 , CFG_UISAMECNTSTATUS) ? 0 : GetCntIcon(status, jid));
}
int cJabber::GetCntIcon(int status , const CStdString & jid) {
	CStdString host = JID::getHost(jid);
	CStdString category, type;
	DiscoDB::Item * item = GetDiscoItem(JID::getHost(jid), false, false);
	if (item  && item->getIdentityList().size() > 0) {
		category = item->getIdentityList().front()->getCategory();
		type = item->getIdentityList().front()->getType();
	}
	status &= ST_MASK;
	int icon = 0;

	if (!strnicmp(host , "gg." , 3) || type=="x-gadugadu")
		icon = UIIcon(IT_STATUS , NET_GG , status , 0);
	else if (!strnicmp(host , "aim." , 4) || type=="aim")
		icon = UIIcon(IT_STATUS , NET_AIM , status , 0);
	else if (!strnicmp(host , "msn." , 4) || type=="msn")
		icon = UIIcon(IT_STATUS , NET_MSN , status , 0);
	else if (!strnicmp(host , "yahoo." , 6) || type=="yahoo")
		icon = UIIcon(IT_STATUS , NET_YAHOO , status , 0);
	else if (!strnicmp(host , "icq." , 4) || type=="icq")
		icon = UIIcon(IT_STATUS , NET_ICQ , status , 0);
	else if (!strnicmp(host , "chat." , 4) || category == "conference")
		icon = IDI_CONFERENCE;

	else if (jid.find('@') == string::npos)
		icon = IDI_AGENT;
	return icon;
}

const CStdString cJabber::GetUID() {
	return JID::getUserHost(session.getLocalJID());
	//return GETSTR(plug->DTgetNameID(DTCFG , dtPrefix + "/user")) + string("@") + GetHost();
}



int cJabber::FindContact(const CStdString & jid_) {
	// najpierw szukamy strictowo...
	int found = -1;
	if ((found = plug->ICMessage(IMC_CNT_FIND, this->net, (int)jid_.c_str())) != -1)
		return found;
	// teraz bez resource...
	CStdString jid = JID::getUserHost(jid_);
	if (jid == JID::getUserHost(session.getLocalJID())) return 0;
	int c = plug->ICMessage(IMC_CNT_COUNT);
	found = -1;
	plug->DTlock(DTCNT , -1);
	for (int i=1; i < c; i++) {
		if (plug->DTgetInt(DTCNT , i , CNT_NET) == this->net && !stricmp(JID::getUserHost(plug->DTgetStr(DTCNT , i , CNT_UID)).c_str() , jid.c_str()) ) {
			found = plug->DTgetID(DTCNT , i);
			break;
		}
	}
	plug->DTunlock(DTCNT , -1);
	return found;
}

int cJabber::CompareJID(const CStdString & a, const CStdString & b) {
	return stricmp(JID::getUserHost(a).c_str() , JID::getUserHost(b).c_str());
}



CStdString kJabber::LocaleToUTF8(CStdString txt) {
	CStdStringW unicode;
	size_t length = txt.size();
	MultiByteToWideChar(CP_ACP , 0 , txt.c_str() , txt.size()+1 , unicode.GetBuffer(length*2+2) , length*2+2);
	unicode.ReleaseBuffer();
	WideCharToMultiByte(CP_UTF8 , 0 , unicode.c_str() , unicode.size()+1/*null*/ , txt.GetBuffer(length*2+2) , length*2+2 , 0 , 0);
	txt.ReleaseBuffer();
	return txt;
}
CStdString kJabber::UTF8ToLocale(CStdString txt) {
	CStdStringW unicode;
	size_t length = txt.size();
	MultiByteToWideChar(CP_UTF8 , 0 , txt.c_str() , txt.size()+1 , unicode.GetBuffer(length*2+2) , length*2+2);
	unicode.ReleaseBuffer();
	WideCharToMultiByte(CP_ACP , 0 , unicode.c_str() , unicode.size()+1/*null*/ , txt.GetBuffer(length) , length , 0 , 0);
	txt.ReleaseBuffer();
	return txt;
}


