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

void cJabber::OnCantConnect(bool automated) {
	if (!automated) {
		this->plug->ICMessage(IMI_CONFIG, kJabber::ACT::CfgGroup, 0);
	}
}


void cJabber::OnTransmitXML(const char* c) {
	if (!this->socket)
		return;
	if (!K_CHECK_PTR(c)) {
		IMDEBUG(DBG_ERROR, "OnTransmitXML bad pointer %x", c);
		K_ASSERT(c);
		return;
	}
	if (c[0]!=' ' || c[1]!=0) // nie ping...
		plug->IMDEBUG(DBG_TRAFFIC , "-->> SND [%s] >>--" , c);
	this->Lock();
	if (ssl) {
		SSL_write(ssl , c , strlen(c));
	} else {
		send(this->socket , c , strlen(c) , 0);
	}
	_time64(&this->lastPing);
	this->Unlock();
}
void cJabber::OnAuthError(int errorcode , const char* errormessage) {
	this->canReconnect= false;
	if (!this->createAccount) {
		plug->IMDEBUG(DBG_ERROR , "! B³¹d autoryzacji! code=%d msg=%s" , errorcode , errormessage);
		string msg = "Wyst¹pi³ b³¹d autoryzacji "+this->GetUID()+"!\r\n\r\nByæ mo¿e poda³eœ nieprawid³owy serwer, login, lub has³o?\r\n\r\nSerwer zwróci³: \r\n";
		msg += inttostr(errorcode, 10);
		msg += ":";
		msg += errormessage;
		Disconnect();
		if (plug->IMessage(&sIMessage_msgBox(IMI_ERROR , msg.c_str(), "kJabber", MB_OKCANCEL))==IDOK) {
			this->OnCantConnect(false);
		}
	} else {
		this->createAccount = false;
		CStdString msg = "Konto \"" + this->useUser + "@" + this->useHost + "\" nie zosta³o utworzone!\r\n\r\nSerwer zwróci³: \r\n";
		msg += inttostr(errorcode, 10);
		msg += ":";
		msg += errormessage;
		Disconnect();
		plug->IMessage(&sIMessage_msgBox(IMI_ERROR , msg.c_str(), "kJabber"));
		this->OnCantConnect(false);
	}
}
void cJabber::OnDisconnected() {
	plug->IMDEBUG(DBG_NET , "Disconnected");
	if (this->socket) {
		if (ssl) {
			SSL_shutdown(ssl);
		}
		this->realHost = "";
		this->realPort = 0;
		shutdown(this->socket, 2);
		closesocket(this->socket);
	}
	if (!(this->storeUPonSuccess & supsKeep)) {
		useUser = usePass = useHost = "";
		this->storeUPonSuccess = supsNo;
	}
//	UIActionSetStatus(sUIAction(this->ACTCfgGroup(), ACT::editRegistration), -1, ACTS_DISABLED);
	UIActionSetStatus(sUIAction(this->ACTcfgGroup(), ACT::services), -1, ACTS_DISABLED);

}
void cJabber::OnConnected(const judo::Element& e) {
	plug->IMDEBUG(DBG_NET , "Connected");
	cJabberBase::OnConnected();
	if (this->createAccount) {
		this->createAccount = false;
		CStdString msg = "Twoje konto "+this->GetUID()+" zosta³o pomyœlnie utworzone...";
		plug->IMessage(&sIMessage_msgBox(IMI_INFORM , msg.c_str(), "kJabber"));
	}
//	UIActionSetStatus(sUIAction(this->ACTCfgGroup(), ACT::editRegistration), 0, ACTS_DISABLED);
	UIActionSetStatus(sUIAction(this->ACTcfgGroup(), ACT::services), 0, ACTS_DISABLED);

}
void cJabber::OnRoster(const judo::Element& e) {
	cJabberBase::OnRosterAnswer();
}

void cJabber::OnRosterItemRemoving(Roster::Item &item) {
	cJabberBase::OnRosterItemRemoving(item.getJID());
}

void cJabber::OnRosterItemUpdated(Roster::Item &item) {
	cJabberBase::OnRosterItemUpdated(item.getJID() , &item);
}
void cJabber::OnRosterItemAdded(Roster::Item &item) {
	cJabberBase::OnRosterItemAdded(item.getJID() , &item);
}


//--------------------------------------------------------------------------


void cJabber::OnMessage(const jabberoo::Message & jabMsg) {
//	if (jabMsg.getType() != Message::mtChat || jabMsg.getType() != Message::mtNormal
	cMessage m;
	CStdString ext;
	m.type = MT_MESSAGE;
	m.flag = MF_HANDLEDBYUI;
	CStdString body = "";
	judo::Element const *  bodyElement;
	bool inHtml = true;
	bodyElement = jabMsg.getBaseElement().findElement("html");
	if (!bodyElement || (Ctrl->DTgetInt(DTCFG, 0, dtPrefix + "/acceptHTML") == 0)) {// nie ma html-im, próbujemy plain-text...
		bodyElement = jabMsg.getBaseElement().findElement("body");
		inHtml = false;
	}
	// ¿eby móc czytaæ wiadomoœci od innych klientów nie zgodnych ze standardem
	// uznajemy nawet zwyk³e "body" za zwyk³y tekst...
	if (bodyElement) {
		if (inHtml == false || (bodyElement->size() == 1 && (*bodyElement->begin())->getType() == judo::Element::Node::ntCDATA)) {
			// plain-text
			if ((*bodyElement->begin())->getType() == judo::Element::Node::ntCDATA) {
				body = UTF8ToLocale(bodyElement->getCDATA());
			} else {
				body = UTF8ToLocale(bodyElement->toString());
			}
		} else {
			m.flag |= MF_HTML;
			body = UTF8ToLocale(bodyElement->toStringEx(true , true));
			IMLOG("- mgsGot %s" , body.c_str());
			RegEx preg;
			// usuwamy potencjalnie niebezpieczne elementy w ca³oœci - zostawiamy tylko wybrane bezpieczne
			body = preg.replace("#<(?!/?(b|strong|i|u|br|span|font)[ \t\r\n/>])[^>]+>#si" , "" , body);
			// usuwamy potencjalnie niebezpieczne atrybuty
			CStdString replaced;
			while (1) {
				replaced = preg.replace("#(<[^>]+)\\s+(?!color|style)\\w+?\\s*=\\s*(['\"]?)[^>]*?\\2#is" , "\\1" , body);
				if (replaced != body) {
					body = replaced;
				} else {
					break;
				}
			};
			// usuwamy niebezpieczne wyra¿enia
			while (1) {
				replaced = preg.replace("#(<[^>]+)(javascript|[()]+|expression|url)#is" , "\\1" , body);
				if (replaced != body) {
					body = replaced;
				} else {
					break;
				}
			};

			
			// specjalnie dla kijewa &apos;
			//body = preg.replace("#\\s+(?!color|style)\\w+?\\s*=\\s*(['\"]?)[^>]*?\\1#is" , "" , body);

			// czycimy potencjalnie niebezpieczne rzeczy

			IMLOG("- msgFiltered %s" , body.c_str());
		}
	}
	if (jabMsg.getType() == Message::mtError) {
		body = "B³¹d: ";
		body += UTF8ToLocale(jabMsg.getError());
		body += " [";
		body += body.substr(0 , 30);
		body += "...]";
		m.type = MT_QUICKEVENT;
	}

	if (body.empty()) return; // odrzucamy puste wiadomoœci...

	m.body = (char*)body.c_str();

	CStdString from = JID::getUserHost(jabMsg.getFrom());
	int cntID = this->FindContact(jabMsg.getFrom());//  plug->ICMessage(IMC_FINDCONTACT , this->net , (int) from.c_str());
	if (cntID > 0) {
		plug->ICMessage(IMI_CNT_ACTIVITY , cntID);
		// byæ mo¿e znaleziony kontakt ma inny resource - wstawiamy ten który znaleŸliœmy :)
		from = plug->DTgetStr(DTCNT, cntID, CNT_UID);
	}
	if (jabMsg.getType() == Message::mtGroupchat && !JID::getResource(jabMsg.getFrom()).empty()) {
		ext = SetExtParam(ext, MEX_DISPLAY, JID::getResource(jabMsg.getFrom()));
		if (jabMsg.getFrom() == from && abs((int)(jabMsg.getDateTime_() - time(0))) < 5) { // dostaliœmy wiadomoœæ od siebie samych!!!
			return;
		}
	} else if (jabMsg.getType() == Message::mtHeadline) {
		m.flag |= MF_AUTOMATED;
	}
	if (!jabMsg.getSubject().empty()) {
		ext = SetExtParam(ext, MEX_TITLE, UTF8ToLocale(jabMsg.getSubject()));
		ext = SetExtParam(ext, MEX_ADDINFO, "\"" + UTF8ToLocale(jabMsg.getSubject()) + "\"");
	}
	m.fromUid = (char*)from.c_str();
	m.net = this->net;
	m.time = jabMsg.getDateTime_();
	m.ext = (char*) ext.c_str();

	if ((cntID > 0 && plug->DTgetInt(DTCNT , cntID , CNT_STATUS) & ST_IGNORED)
		|| (cntID <=0 && plug->ICMessage(IMC_CNT_IGNORED , this->net , (int)m.fromUid)))
		return IgnoreMessage(&m , cntID);

	sMESSAGESELECT ms;
	ms.id = plug->ICMessage(IMC_NEWMESSAGE , (int) &m);
	if (ms.id > 0) {
		plug->ICMessage(IMC_MESSAGEQUEUE , (int) &ms);
	}
}
void cJabber::OnPresence(const Presence& presence, Presence::Type previousType) {
	plug->IMDEBUG(DBG_MISC , "- Presence from=%s type=%d show=%d status=%s prev=%d" , presence.getFrom().c_str() , presence.getType() , presence.getShow() , presence.getStatus().c_str() , previousType);
	if (presence.getType() == Presence::ptAvailable || presence.getType() == Presence::ptUnavailable) {
		int cntID = plug->ICMessage(IMC_CNT_FIND , this->net , (int) JID::getUserHost(presence.getFrom()).c_str());
		if (cntID > 0) {
			plug->ICMessage(IMI_CNT_ACTIVITY , cntID);
			Presence::Show show = presence.getShow();
			int status = show == Presence::stOnline ? ST_ONLINE 
				: show == Presence::stAway ? ST_AWAY
				: show == Presence::stChat ? ST_CHAT
				: show == Presence::stDND ? ST_DND
				: show == Presence::stXA ? ST_NA
				: ST_OFFLINE
				;
			CStdString info = UTF8ToLocale(presence.getStatus());
			if (status == (plug->DTgetInt(DTCNT , cntID , CNT_STATUS) & ST_MASK)
				&& info == plug->DTgetStr(DTCNT , cntID , CNT_STATUSINFO))
				return;
			// rozpoznawanie ikonki...
			this->SetCntIcon(cntID , status , presence.getFrom());
			CntSetStatus(cntID , status , info.c_str());
			plug->ICMessage(IMI_REFRESH_CNT , cntID);
		}
	} else if (presence.getType() == Presence::ptUnsubscribed) {
		CStdString notify = "Subskrypcja zosta³a odrzucona przez " + presence.getFrom();
		plug->IMessage(&KNotify::sIMessage_notify(notify , UIIcon(IT_MESSAGE,0,MT_EVENT,0),KNotify::sIMessage_notify::tInform,4));
	} else if (presence.getType() == Presence::ptSubscribed) {
		CStdString notify = "Zosta³eœ autoryzowany przez " + presence.getFrom();
		plug->IMessage(&KNotify::sIMessage_notify(notify , UIIcon(IT_MESSAGE,0,MT_EVENT,0),KNotify::sIMessage_notify::tInform,4));
	} else if (presence.getType() == Presence::ptError) {
		CStdString notify = presence.getFrom() + " - b³¹d: " + presence.getError();
		plug->IMessage(&KNotify::sIMessage_notify(notify , (unsigned int)0,KNotify::sIMessage_notify::tError,1));
	}
}
void cJabber::OnPresenceRequest(const Presence & presence) {
	cJabberBase::OnPresenceRequest(presence.getFrom() , presence.getType() == jabberoo::Presence::ptSubRequest);
}
void cJabber::OnVersion(string & name , string & version , string & os) {
	plug->IMDEBUG(DBG_MISC , "- Version");
	name = "Konnekt.kJabber";
	char * buff = (char*) plug->GetTempBuffer(32);
	plug->ICMessage(IMC_PLUG_VERSION , plug->ICMessage(IMC_PLUGID_POS , plug->ID()) , (int)buff);
	version = buff;
}
void cJabber::OnLast(string & seconds) {
	plug->IMDEBUG(DBG_MISC , "- Last");
}
void cJabber::OnTime(string & localTime, string & timeZone) {
	plug->IMDEBUG(DBG_MISC , "- Time");
}

// -------------------------------------------------

void cJabber::OnIQSearchReply(const judo::Element& iq) {
	/*TODO: b³¹d i anulowanie*/
	if (!this->dlgSearch || this->dlgSearch->cancel || iq.getAttrib("type")!="result") {
		if (iq.getAttrib("type") == "error") {
			const judo::Element * error = iq.findElement("error");
			if (error)
				plug->IMessage(&sIMessage_msgBox(IMI_ERROR , ("Wyst¹pi³ b³¹d podczas wyszukiwania:\r\n"+UTF8ToLocale(error->getCDATA())).c_str() , "kJabber - wyszukiwanie"));
		}
		OnSearchAnswer();
		return;
	}
	const judo::Element * query = iq.findElement("query");
	if (!query || query->getAttrib("xmlns") != "jabber:iq:search") return;

    for (judo::Element::const_iterator it = query->begin(); it != query->end(); ++it )
    {
		sCNTSEARCH srep;
		srep.net = this->net;
		srep.status = ST_ONLINE;
		if ((*it)->getType() != judo::Node::ntElement || (*it)->getName()!="item")
			continue;
		judo::Element& item = *static_cast<judo::Element*>(*it);
		strncpy(srep.uid , item.getAttrib("jid").c_str() , sizeof(srep.uid));
		const judo::Element * info = item.findElement("first");
		if (info)
			strncpy(srep.name , UTF8ToLocale(info->getCDATA()).c_str() , sizeof(srep.name));
		info = item.findElement("last");
		if (info)
			strncpy(srep.surname , UTF8ToLocale(info->getCDATA()).c_str() , sizeof(srep.surname));
		info = item.findElement("nick");
		if (info)
			strncpy(srep.nick , UTF8ToLocale(info->getCDATA()).c_str() , sizeof(srep.nick));
		info = item.findElement("email");
		if (info)
			strncpy(srep.email , UTF8ToLocale(info->getCDATA()).c_str() , sizeof(srep.email));
		plug->ICMessage(IMI_CNT_SEARCH_FOUND , (int)&srep);
	}
	OnSearchAnswer();
}

void cJabber::vCardGetValue(const judo::Element * vCard , bool window , int cnt , int col , const char * path) {
	XPath::Query xpath(path);
	XPath::Value * value = xpath.execute(const_cast<judo::Element *>(vCard));
	if (!value->check() || value->getElem()->getCDATA().empty()) return;
	CntSetInfoValue(window , cnt , col , UTF8ToLocale( value->getElem()->getCDATA() ).c_str());
}

void cJabber::OnIQVCardRequestReply(const judo::Element& iq) {
	if (!this->dlgVCardReq) return;
	int cnt = (int)this->dlgVCardReq->param; // this->FindContact(iq.getAttrib("from"));
    bool window = UIGroupHandle(sUIAction(0 , IMIG_NFOWND , cnt)) != 0;
	judo::Element * vCard = const_cast<judo::Element *>(iq.findElement("vCard"));
	if (!vCard || iq.getAttrib("type") == "error") {
		const judo::Element * error = iq.findElement("error");
		plug->IMessage(&sIMessage_msgBox(IMI_ERROR 
			, "Wyst¹pi³ b³¹d:\r\n" + CStdString(error ? (error->getAttrib("code") + " : " + error->getCDATA() ) :  "  kontakt nie ma wizytówki") , "Pobieranie vCard" , 0 , dlgVCardReq->handle));
		this->OnVCardRequestAnswer();
		return;
	}
	vCardGetValue(vCard , window , cnt , CNT_NAME , "N/GIVEN");
	vCardGetValue(vCard , window , cnt , CNT_MIDDLENAME , "N/MIDDLE");
	vCardGetValue(vCard , window , cnt , CNT_SURNAME , "N/FAMILY");
	vCardGetValue(vCard , window , cnt , CNT_NICK , "NICKNAME");
	vCardGetValue(vCard , window , cnt , CNT_DESCRIPTION , "DESC");
	vCardGetValue(vCard , window , cnt , CNT_URL , "URL");
	vCardGetValue(vCard , window , cnt , CNT_WORK_ORGANIZATION , "ORG/ORGNAME");
	vCardGetValue(vCard , window , cnt , CNT_WORK_ORG_UNIT , "ORG/ORGUNIT");
	vCardGetValue(vCard , window , cnt , CNT_WORK_TITLE , "TITLE");
	vCardGetValue(vCard , window , cnt , CNT_WORK_ROLE , "ROLE");

	/* Wczytujemy trochê na "skróty" */
	jabberoo::vCard vc(*vCard);
	int year = 0 , month = 0 , day = 0;
	sscanf(string(vc[vCard::Field::Birthday]).c_str(), "%d-%d-%d" , &year , &month , &day);
	if (year && month && day) {
		char buff [16];
		CntSetInfoValue(window , cnt , CNT_BORN , itoa((year << 16) | ((month & 0xFF) << 8) | (day & 0xFF) , buff , 10));
	}

	/* Obs³ugujemy zapytania, które mog¹ siê powtórzyæ (telefon , email , adres) */
	// Emaile
	{
		XPath::Query xpath("EMAIL");
		XPath::Value * value = xpath.execute(vCard);
		CStdString email , emails , work;
		if (value->check()) {
			for (XPath::Value::ElemList::const_iterator it = value->getList().begin() ; it != value->getList().end(); it++ ) {
				if ((*it)->findElement("WORK") && work.empty()) {
					work = (*it)->getChildCData("USERID");
					continue;
				}
				if (((*it)->findElement("PREF") || !(*it)->findElement("WORK")) && email.empty())
					email = (*it)->getChildCData("USERID");
				else
					emails += (*it)->getChildCData("USERID") + "\n";
			}
		}
		if (!email.empty())
			CntSetInfoValue(window , cnt , CNT_EMAIL , UTF8ToLocale( email ));
		if (!emails.empty())
			CntSetInfoValue(window , cnt , CNT_EMAIL_MORE , UTF8ToLocale( emails ));
		if (!work.empty())
			CntSetInfoValue(window , cnt , CNT_WORK_EMAIL , UTF8ToLocale( work ));
	}

	// Telefon
	{
		XPath::Query xpath("TEL");
		XPath::Value * value = xpath.execute(vCard);
		CStdString home, work, number;
		if (value->check()) {
			for (XPath::Value::ElemList::const_iterator it = value->getList().begin() ; it != value->getList().end(); it++ ) {
				number = (*it)->getChildCData("NUMBER");
				if ((*it)->findElement("WORK")) {
					bool found = false;
					if ((*it)->findElement("VOICE")) {
						CntSetInfoValue(window , cnt , CNT_WORK_PHONE , UTF8ToLocale( number ));
						found = true;
					}
					if ((*it)->findElement("FAX")) {
						CntSetInfoValue(window , cnt , CNT_WORK_FAX , UTF8ToLocale( number ));
						found = true;
					}
					if ((*it)->findElement("CELL")) {
						CntSetInfoValue(window , cnt , CNT_WORK_PHONE , UTF8ToLocale( number ));
						found = true;
					}
					if (!found) {
						work += number + "\n";
					}
					continue;
				} else {
					bool found = false;
					if ((*it)->findElement("VOICE")) {
						CntSetInfoValue(window , cnt , CNT_PHONE , UTF8ToLocale( number ));
						found = true;
					}
					if ((*it)->findElement("FAX")) {
						CntSetInfoValue(window , cnt , CNT_FAX , UTF8ToLocale( number ));
						found = true;
					}
					if ((*it)->findElement("CELL")) {
						CntSetInfoValue(window , cnt , CNT_CELLPHONE , UTF8ToLocale( number ));
						found = true;
					}
					if (!found) {
						home += number + "\n";
					}
					continue;
				}
			}
		}
		if (!work.empty()) {
			home += "Praca:\n" + work;
		}
		if (!home.empty())
			CntSetInfoValue(window , cnt , CNT_PHONE_MORE , UTF8ToLocale( home ));
		if (!home.empty())
			CntSetInfoValue(window , cnt , CNT_PHONE_MORE , UTF8ToLocale( home ));
	}

	// Adres

	{
		XPath::Query xpath("ADR");
		XPath::Value * value = xpath.execute(vCard);
		if (value->check()) {
			for (XPath::Value::ElemList::const_iterator it = value->getList().begin() ; it != value->getList().end(); it++ ) {
				bool work = (*it)->findElement("WORK") != 0;

				CStdString ext = (*it)->getChildCData("EXTADD");
				CStdString street = (*it)->getChildCData("STREET");
				CStdString pobox = (*it)->getChildCData("POBOX");
				CStdString locality = (*it)->getChildCData("LOCALITY");
				CStdString pcode = (*it)->getChildCData("PCODE");
				CStdString region = (*it)->getChildCData("REGION");
				CStdString country = (*it)->getChildCData("CTRY");


				if (!ext.empty()) {
					CntSetInfoValue(window, cnt , work ? CNT_WORK_ADDRESS_MORE : CNT_ADDRESS_MORE , UTF8ToLocale( ext ));
				}
				if (!street.empty()) {
					CntSetInfoValue(window, cnt , work ? CNT_WORK_STREET : CNT_STREET , UTF8ToLocale( street ));
				}
				if (!pobox.empty()) {
					CntSetInfoValue(window, cnt , work ? CNT_WORK_POBOX : CNT_POBOX , UTF8ToLocale( pobox ));
				}
				if (!locality.empty()) {
					CntSetInfoValue(window, cnt , work ? CNT_WORK_LOCALITY : CNT_LOCALITY , UTF8ToLocale( locality ));
				}
				if (!pcode.empty()) {
					CntSetInfoValue(window, cnt , work ? CNT_WORK_POSTALCODE : CNT_POSTALCODE , UTF8ToLocale( pcode ));
				}
				if (!region.empty()) {
					CntSetInfoValue(window, cnt , work ? CNT_WORK_REGION : CNT_REGION , UTF8ToLocale( region ));
				}
				if (!country.empty()) {
					CntSetInfoValue(window, cnt , work ? CNT_WORK_COUNTRY : CNT_COUNTRY , UTF8ToLocale( country ));
				}
			}
		}
	}


	this->OnVCardRequestAnswer();
}
void cJabber::OnIQVCardSetReply(const judo::Element& iq) {
	if (!this->dlgVCardSet) return;
	const judo::Element * vCard = iq.findElement("vCard");
	if (/*!vCard || */iq.getAttrib("type") == "error") {
		const judo::Element * error = iq.findElement("error");
		plug->IMessage(&sIMessage_msgBox(IMI_ERROR 
			, "Wyst¹pi³ b³¹d:\r\n" + CStdString(error ? (error->getAttrib("code") + " : " + error->getCDATA() ) :  "  z³a odpowiedŸ serwera") , "Zapisywanie vCard" , 0 , dlgVCardSet->handle));
	}
	this->OnVCardSetAnswer();
}
