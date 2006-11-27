/* 
   Jabber _ Class

   License information can be found in license.txt.
   Please redistribute only as a whole package.
   
   (c)2004 Rafa³ Lindemann | Stamina
   http://www.konnekt.info
*/ 


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT 0x0501
// Windows Header Files:
#include <windows.h>
#include <stdstring.h>
#include <Winsock2.h>
extern "C" {
#include "base64.h"
}
#include "konnekt/plug_export.h"
#include "konnekt/ui.h"
#include "konnekt/plug_func.h"
#include "konnekt/knotify.h"
#include "konnekt/kjabber.h"
#include "jabber_class.h"

using namespace std;


#pragma hdrstop

namespace kJabber {
// -------------------------------------------------------------------------------------------
// "Wysoki" poziom
// -------------------------------------------------------------------------------------------

cJabberBase::cJabberBase(cCtrl * ctrl) {
	static int nextSlot = 0;
	slotID = (nextSlot++) + '0';
	plug = ctrl;
	InitializeCriticalSection(&cs);
	thread = 0;
	removingCnt	= -1;
	currentStatus = ST_OFFLINE;
	inAutoAway = false;
	synchronized = false;
	state = cs_offline;
	dlgSearch = dlgVCardReq = dlgVCardSet = 0;
	timerPing = CreateWaitableTimer(0 , false , 0);
	lastPing = 0;
	storeUPonSuccess=supsNo;

}

cJabberBase::~cJabberBase() {
	DeleteCriticalSection(&cs);
	CloseHandle(timerPing);
}

void cJabberBase::Lock() {
	EnterCriticalSection(&cs);
}
void cJabberBase::Unlock() {
	LeaveCriticalSection(&cs);
}

bool cJabberBase::StartProxy() {
#ifndef JABBER_NOPROXY
	if (!plug->DTgetInt(DTCFG , 0 , CFG_PROXY))
		return true;
	char buff [10];
	CStdString proxy = "CONNECT ";
	proxy += GetHost(true);
	proxy += ":";
	proxy += itoa(GetPort(true) , buff , 10);
	proxy += " HTTP/1.0\r\n";
	plug->IMDEBUG(DBG_TRAFFIC , "-->> SND_PROXY [%s] >>--" , proxy.c_str());
	if ((unsigned)send(this->GetSocket() , proxy.c_str() , proxy.size() , 0) < proxy.size())
		throw kJabber::cException(this , "Proxy" , "Couldn't send data to proxy!");
	if (plug->DTgetInt(DTCFG , 0 , CFG_PROXY_AUTH)) {
		CStdString auth = plug->DTgetStr(DTCFG , 0 , CFG_PROXY_LOGIN);
		auth += ":";
		auth += plug->DTgetStr(DTCFG , 0 , CFG_PROXY_PASS);
		size_t outLen;
		encode64(auth , auth.size() , proxy.GetBuffer(1024) , 1024 , &outLen);
		proxy.ReleaseBuffer(outLen);
		proxy = "Proxy-Authorization: Basic " + proxy + "\r\n";
		plug->IMDEBUG(DBG_TRAFFIC , "-->> SND_PROXY [%s] >>--" , proxy.c_str());
		if ((unsigned)send(GetSocket() , proxy.c_str() , proxy.size() , 0) < proxy.size())
			throw kJabber::cException(this , "Proxy" , "Couldn't send auth-data to proxy!");
	}
	if (send(GetSocket() , "\r\n" , 2 , 0) < 2)
		throw kJabber::cException(this , "Proxy" , "Couldn't send rn to proxy!");
	bool success = false;
	proxy = "";
	while (recv(GetSocket() , buff , 1 , 0)) {
		proxy += buff[0];
		if (buff[0] == '\n') { // mamy linijkê...
			if (strstr(proxy , "200 OK")) {
				success = true;
				break;
			}
		}
	}
	plug->IMDEBUG(DBG_TRAFFIC , "--<< RCV_PROXY [%s] <<--" , proxy.c_str());
	if (!success)
		return false;
#endif
	return true;
}

void cJabberBase::StartSSL() {
	return;
}


void cJabberBase::Connect(bool automated) {
	if (this->CanConnect() == false) {
		if (!thread) {
			this->OnCantConnect(automated);
			return;
		}
	}
	if (thread) 
		return;
	thread = (HANDLE)plug->BeginThread("cJabberBase::ListenThread", 0 , 0 , (cCtrl::fBeginThread)cJabberBase::ListenThread_ , this);
}

void cJabberBase::Disconnect() {
	if (!thread	|| state == cs_disconnecting) 
		return;
	CancelWaitableTimer(timerPing);
	state = cs_disconnecting;
	this->Lock();
	SetStatus(ST_OFFLINE , false);
	DisconnectSocket();
	this->Unlock();
}

void cJabberBase::SetStatus(int status , bool set , const char * info) {
	if (status == -1) {
		status = plug->DTgetInt(DTCFG , 0 , dtPrefix + "/status");
	}
	if (info == NULL) {
		info = plug->DTgetStr(DTCFG , 0 , dtPrefix + "/statusInfo");
	}
	if (status == ST_OFFLINE && this->currentStatus == ST_OFFLINE) {
		plug->ICMessage(IMC_SETCONNECT , 0);
		// wy³¹czamy auto-³¹czenie...
		plug->DTsetInt(DTCFG , 0 , dtPrefix + "/status" , status);
	}
	if (status == this->currentStatus && this->currentStatusInfo == info)
		return;
	if (set) {
		plug->DTsetInt(DTCFG , 0 , dtPrefix + "/status" , status);
		plug->DTsetStr(DTCFG , 0 , dtPrefix + "/statusInfo" , info);
	}
	if (status != ST_OFFLINE && state == cs_connecting) {
		//this->Unlock();
		return;
	}
	this->Lock();
	if (this->Connected() || currentStatus == ST_CONNECTING || status == ST_OFFLINE) {
		plug->IMessage(&sIMessage_StatusChange(IMC_STATUSCHANGE , 0 , status , info));
		this->SendPresence("" , status , info);
		currentStatus = status;
		currentStatusInfo = info;
	} else if (status != ST_OFFLINE)
		this->Connect();

	if (status == ST_OFFLINE) {
		this->Disconnect();
	}
	this->Unlock();
}

int cJabberBase::FindContact(const CStdString & jid) {
	if (jid.empty()) return -1;
	if (jid == this->GetUID()) return 0;
	return plug->ICMessage(IMC_CNT_FIND , this->net , (int)jid.c_str());
}




// -------------------------------------------------------------------------------------------
// Wspó³praca z Konnektem
// -------------------------------------------------------------------------------------------


int cJabberBase::ActionProc(sUIActionNotify_base * anBase) {
	sUIActionNotify_2params * an = (anBase->s_size>=sizeof(sUIActionNotify_2params))?static_cast<sUIActionNotify_2params*>(anBase):0;

	if ((anBase->act.id & IMIB_) == IMIB_CFG) return ActionCfgProc(anBase); 
	switch (anBase->act.id) {
		case kJabber::ACT::stOnline: {
			ACTIONONLY(anBase);
			SetStatus(ST_ONLINE , true);
			break;}
		case kJabber::ACT::stAway:
			ACTIONONLY(anBase);
			SetStatus(ST_AWAY , true);
			break;
		case kJabber::ACT::stChat:
			ACTIONONLY(anBase);
			SetStatus(ST_CHAT , true);
			break;
		case kJabber::ACT::stDND:
			ACTIONONLY(anBase);
			SetStatus(ST_DND , true);
			break;
		case kJabber::ACT::stHidden:
			ACTIONONLY(anBase);
			SetStatus(ST_HIDDEN , true);
			break;
		case kJabber::ACT::stNA:
			ACTIONONLY(anBase);
			SetStatus(ST_NA , true);
			break;

		case kJabber::ACT::stOffline:
			ACTIONONLY(anBase);
			// ¯eby nie próbowa³ ³¹czyæ znowu...
			//Disconnect();
			SetStatus(ST_OFFLINE , false);
			break;

		case kJabber::ACT::subShow: 
			ACTIONONLY(anBase);
			if (anBase->act.cnt <= 0 || plug->DTgetInt(DTCNT , anBase->act.cnt , CNT_NET) != this->net)
				return 0;
			SendSubscribed(plug->DTgetStr(DTCNT , anBase->act.cnt , CNT_UID) , (plug->DTgetInt(DTCNT , anBase->act.cnt , CNT_STATUS) & ST_HIDEMYSTATUS)!=0);
			break;
		case kJabber::ACT::subSubscribed: 
			ACTIONONLY(anBase);
			if (anBase->act.cnt <= 0 || plug->DTgetInt(DTCNT , anBase->act.cnt , CNT_NET) != this->net)
				return 0;
			SendSubRequest(plug->DTgetStr(DTCNT , anBase->act.cnt , CNT_UID) , (plug->DTgetInt(DTCNT , anBase->act.cnt , CNT_STATUS) & ST_HIDESHISSTATUS) != 0);
			break;
		case kJabber::ACT::sendStatus:
			if (anBase->code == ACTN_CREATE) {
				sUIActionInfo nfo(anBase->act);
				nfo.mask = UIAIM_ICO;
				nfo.p1 = UIIcon(IT_STATUS , this->net , this->currentStatus , 0);
				UIActionSet(nfo);
			} else if (anBase->code == ACTN_ACTION) {
				this->SendPresence(plug->DTgetStr(DTCNT , anBase->act.cnt , CNT_UID) , this->currentStatus , this->currentStatusInfo);
			}
			break;
		case kJabber::ACT::sendUnavailable:
			ACTIONONLY(anBase);
			this->SendPresence(plug->DTgetStr(DTCNT , anBase->act.cnt , CNT_UID) , ST_OFFLINE , "");
			break;
		default:
			if (anBase->act.id == (this->net * 1000 + kJabber::ACT::subscription_off) || anBase->act.id == (this->net * 1000 + kJabber::ACT::subscriptionNfo_off)) {
				bool ok = this->Connected() && anBase->act.cnt > 0 && plug->DTgetInt(DTCNT , anBase->act.cnt , CNT_NET) == this->net;
				if (anBase->code == ACTN_CREATE || (anBase->act.id == (this->net * 1000 + kJabber::ACT::subscriptionNfo_off) && anBase->code == ACTN_CREATEGROUP)) {
					sUIActionInfo ai(anBase->act);
					ai.mask = UIAIM_STATUS;
					ai.statusMask = ACTS_HIDDEN;
					ai.status = ok ? 0 : ACTS_HIDDEN;
					plug->ICMessage(IMI_ACTION_SET , (int)&ai);
					if (ok) {
						ai.statusMask = ACTS_CHECKED;
						ai.act.parent = ai.act.id;
						ai.act.id = ACT::subShow;
						ai.status = plug->DTgetInt(DTCNT , anBase->act.cnt , CNT_STATUS) & ST_HIDEMYSTATUS ? 0 : ACTS_CHECKED;
						plug->ICMessage(IMI_ACTION_SET , (int)&ai);
						ai.act.id = ACT::subSubscribed;
						ai.status = plug->DTgetInt(DTCNT , anBase->act.cnt , CNT_STATUS) & ST_HIDESHISSTATUS ? 0 : ACTS_CHECKED;
						plug->ICMessage(IMI_ACTION_SET , (int)&ai);
					}
				}
			} else if (anBase->act.id == (this->net * 1000 + kJabber::ACT::vcardRequest_off)) {
				if (anBase->code == ACTN_CREATE) {
					/* Pokazujemy pozycjê gdy jesteœmy po³¹czeni I (dla ustawieñ u¿ytkownika LUB gdy kontakt ma wybran¹ odpowiedni¹ sieæ) */
					UIActionSetStatus(anBase->act 
						, (this->Connected() 
						&& (
						Ctrl->DTgetPos(DTCNT , anBase->act.cnt)==0 
						|| atoi(UIActionCfgGetValue(sUIAction(IMIG_NFO_DETAILS , IMIB_CNT | CNT_NET , anBase->act.cnt),0,0)) == this->net
						)) ? 0 : -1 , ACTS_HIDDEN );
				} else if (anBase->code == ACTN_ACTION) {
					IMessageDirect(IM_CNT_DOWNLOAD , plug->ID() , anBase->act.cnt , 1);
					ICMessage(IMI_CNT_DETAILS_SUMMARIZE , anBase->act.cnt);
				}
			} else if (anBase->act.id == (this->net * 1000 + kJabber::ACT::vcardSet_off)) {
				if (anBase->code == ACTN_CREATE) {
					UIActionSetStatus(anBase->act , this->Connected() && Ctrl->DTgetPos(DTCNT,anBase->act.cnt)==0 ? 0 : -1 , ACTS_HIDDEN );
				} else if (anBase->code == ACTN_ACTION) {
					IMessageDirect(IM_CNT_UPLOAD , plug->ID() , anBase->act.cnt , 1);
				}
			}


			break;
	}
	return 0;
}

void cJabberBase::SetColumns() {
	plug->SetColumn(DTCFG , -1 , DT_CT_STR | DT_CF_CXOR, "" , this->dtPrefix + "/user");
	plug->SetColumn(DTCFG , -1 , DT_CT_STR | DT_CF_CXOR | DT_CF_SECRET, "" , this->dtPrefix + "/pass");
	plug->SetColumn(DTCFG , -1 , DT_CT_INT , ST_OFFLINE , this->dtPrefix + "/status");
	plug->SetColumn(DTCFG , -1 , DT_CT_PCHAR | DT_CF_CXOR , "" , this->dtPrefix + "/statusInfo");
}
void cJabberBase::UIPrepare() {
	plug->ICMessage(IMC_SETCONNECT , 1);
	plug->IMessage(&sIMessage_StatusChange(IMC_STATUSCHANGE , 0 , ST_OFFLINE , ""));

}


void cJabberBase::CntSearch(sCNTSEARCH * sr) {
	if (!this->Connected()) {
		plug->IMessage(&sIMessage_msgBox(IMI_ERROR , "Musisz byæ po³¹czony!" , GetName() + " - wyszukiwanie"));
		return;
	}
	if (this->dlgSearch) {
		plug->IMessage(&sIMessage_msgBox(IMI_ERROR , "W jednym momencie dozwolone jest tylko jedno zapytanie!" , GetName() + " - wyszukiwanie"));
		return;
	}
	dlgSearch = new sDIALOG_long();
	dlgSearch->flag = DLONG_CANCEL | DLONG_ARECV/* | DLONG_BLOCKING*/;
	dlgSearch->info = "Zapytanie zosta³o wys³ane...";
	CStdString title = GetName() + " - wyszukiwanie";
	dlgSearch->title = title;
	plug->ICMessage(IMI_LONGSTART , (int)dlgSearch);
	this->SendCntSearch(sr);
	while (!dlgSearch->cancel) {
		SleepEx(10 , true);
		plug->WMProcess();
	}
	plug->ICMessage(IMI_LONGEND , (int)dlgSearch);
	delete dlgSearch;
	dlgSearch = 0;
}

bool cJabberBase::VCardRequest(int cntID) {
	if (!this->Connected()) {
		plug->IMessage(&sIMessage_msgBox(IMI_ERROR , "Musisz byæ po³¹czony!" , GetName() + " - pobieranie wizytówki"));
		return false;
	}
	if (Ctrl->DTgetPos(DTCNT , cntID) < 0)
	{
		plug->IMessage(&sIMessage_msgBox(IMI_ERROR , "B³¹d wewnêtrzny?" , GetName() + " - VCard"));
		return false;
	}
	if (this->dlgVCardReq) {
		plug->IMessage(&sIMessage_msgBox(IMI_ERROR , "W jednym momencie dozwolone jest tylko jedno zapytanie!" , GetName() + " - pobieranie wizytówki"));
		return false;
	}
	dlgVCardReq = new sDIALOG_long();
	dlgVCardReq->flag = DLONG_CANCEL | DLONG_ARECV/* | DLONG_BLOCKING*/;
	dlgVCardReq->info = "Zapytanie zosta³o wys³ane...";
	dlgVCardReq->param = (void*) cntID; // Jako parametr zapisujemu identyfikator kontaktu...
	CStdString title = GetName() + " - pobieranie wizytówki";
	dlgVCardReq->title = title;
	plug->ICMessage(IMI_LONGSTART , (int)dlgVCardReq);
	this->SendVCardRequest(cntID);
	while (!dlgVCardReq->cancel) {
		SleepEx(10 , true);
		plug->WMProcess();
	}
	plug->ICMessage(IMI_LONGEND , (int)dlgVCardReq);
	delete dlgVCardReq;
	dlgVCardReq = 0;
	return true;
}
bool cJabberBase::VCardSet() {
	if (!this->Connected()) {
		plug->IMessage(&sIMessage_msgBox(IMI_ERROR , "Musisz byæ po³¹czony!" , GetName() + " - zapisywanie wizytówki"));
		return false;
	}
	if (this->dlgVCardSet) {
		plug->IMessage(&sIMessage_msgBox(IMI_ERROR , "W jednym momencie dozwolone jest tylko jedno zapytanie!" , GetName() + " - zapisywanie wizytówki"));
		return false;
	}
	dlgVCardSet = new sDIALOG_long();
	dlgVCardSet->flag = DLONG_CANCEL | DLONG_ARECV/* | DLONG_BLOCKING*/;
	dlgVCardSet->info = "Zapytanie zosta³o wys³ane...";
	CStdString title = GetName() + " - zapisywanie wizytówki";
	dlgVCardSet->title = title;
	plug->ICMessage(IMI_LONGSTART , (int)dlgVCardSet);
	this->SendVCardSet(UIGroupHandle(sUIAction(0 , IMIG_NFOWND , 0)) != 0);
	while (!dlgVCardSet->cancel) {
		SleepEx(10 , true);
		plug->WMProcess();
	}
	plug->ICMessage(IMI_LONGEND , (int)dlgVCardSet);
	delete dlgVCardSet;
	dlgVCardSet = 0;
	return true;
}


int cJabberBase::IMessageProc(sIMessage_base * msgBase) {
    sIMessage_2params * msg = (msgBase->s_size>=sizeof(sIMessage_2params))?static_cast<sIMessage_2params*>(msgBase):0;
    switch (msgBase->id) {
	case IM_SETCOLS:     
		this->SetColumns();
		return 1;
    case IM_START:       return 1;
	case IM_UI_PREPARE:         
		this->UIPrepare();
		return 1;
    case IM_END:         
		this->Disconnect();
		plug->IMDEBUG(DBG_MISC , "- Waiting for cJabber thread");
		while (thread) {
		    Ctrl->Sleep(10);
		}
		return 1;
	case IM_CONNECT:
		this->Connect(true);
		return 1;
    case IM_AWAY:
		if (!this->Connected() || (this->currentStatus != ST_ONLINE)) return 0;
        this->inAutoAway = true;
		this->SetStatus(ST_NA , false);
        return 0;

    case IM_BACK:
        if (!this->Connected() || !this->inAutoAway) return 0;
        this->inAutoAway = false;
		this->SetStatus(-1,false);
        return 0;

	case IM_ISCONNECTED: return this->Connected();

	case IM_GET_STATUS: return this->currentStatus;
    case IM_GET_STATUSINFO: {
		CStdString info = this->currentStatusInfo;
		char * buff = (char*)plug->GetTempBuffer(info.length() + 1);
		strcpy(buff , info.c_str());
		return (int)buff;
	}
	case IM_GET_UID: {
		CStdString uid = GetUID();
		char * buff = (char*)plug->GetTempBuffer(uid.length() + 1);
		strcpy(buff , uid.c_str());
		return (int)buff;
	}
	case IM_CHANGESTATUS:
		this->SetStatus(msg->p1 , true , (const char *) msg->p2);
		return 0;

	case IM_CNT_ADD: {
		if (GETCNTI(msg->p1 , CNT_NET) == this->net) {
			SetCntIcon(msg->p1 , ST_OFFLINE , plug->DTgetStr(DTCNT , msg->p1 , CNT_UID));
			if (this->Connected() && this->IsSynchronized()) 
			{
				SetRosterItem(msg->p1);
				if (msg->id == IMC_CNT_ADD 
					&& this->CFGautoSubscribe())
				{
					this->SendSubRequest(plug->DTgetStr(DTCNT , msg->p1 , CNT_UID) , true);
				}
			}
		}
		return 0;
	}
	case IM_CNT_CHANGED: {
		sIMessage_CntChanged cc(msgBase);
		if (!cc._changed.net && cc._changed.uid && 
			(!this->CompareJID(cc._oldUID, GETCNTC(cc._cntID , CNT_UID)))) {
			return 0;
		}
		if (( (cc._changed.net && (cc._oldNet == this->net))
			|| (cc._changed.uid 
			  && (GETCNTI(cc._cntID , CNT_NET) == this->net))
			)
			&& this->IsSynchronized()
			&& CFGsynchRemoveContacts()
			) 
		{
			this->RosterDeleteJID( cc._changed.uid ? cc._oldUID : GETCNTC(cc._cntID , CNT_UID) );
		}
		bool adding = (cc._changed.net || cc._changed.uid);
		if ((adding || cc._changed.group) && GETCNTI(cc._cntID , CNT_NET) == this->net) {
			if (this->IsSynchronized()
				&& (!adding || CFGsynchAddContacts()))
			{
				SetCntIcon(msg->p1 , plug->DTgetInt(DTCNT , cc._cntID , CNT_STATUS) & ST_MASK , plug->DTgetStr(DTCNT , cc._cntID , CNT_UID));
				SetRosterItem(cc._cntID);
				if (adding && CFGautoSubscribe())
				{
					this->SendSubRequest(plug->DTgetStr(DTCNT , msg->p1 , CNT_UID) , true);
				}
			}
		}
		return 0;
	}
	case IM_CNT_REMOVE: {
		if (GETCNTI(msg->p1 , CNT_NET) != this->net) return 0;
		if (this->IsSynchronized() && plug->DTgetInt(DTCFG , 0 , CFG::synchRemoveContacts)) {
			synchronized = false;
			this->RosterDeleteJID(GETCNTC(msg->p1 , CNT_UID));
			this->removingCnt = msg->p1;
		} else
			this->removingCnt = -1;
		return 0;
	}
	case IM_CNT_REMOVED: {
		if (msg->p1 == this->removingCnt)
			synchronized = true;
		return 0;
	}

	case IM_MSG_RCV: {
		cMessage * m = reinterpret_cast<cMessage *> (msg->p1);
		if (m->net == this->net) {
			if (m->type == MT_MESSAGE) {
				if (m->flag & MF_SEND) {
					return IM_MSG_ok;
				}
			} else if (((m->type & MT_MASK) == MT_AUTHORIZE) || ((m->type & MT_MASK) == MT_CNTEVENT)) {
				return IM_MSG_ok;
			}
		}
		return 0;
	}

	
	case IM_MSG_SEND: {
		cMessage * m = reinterpret_cast<cMessage *> (msg->p1);
		if (m->net == this->net) {
			if (m->type == MT_MESSAGE) {
				return SendMessage(m) ? IM_MSG_delete : 0;
			}
		}
		return 0;
	}
	case IM_MSG_OPEN: {
		cMessage * m = reinterpret_cast<cMessage *> (msg->p1);
		if (m->net == this->net) {
			if ((m->type & MT_MASK) == MT_AUTHORIZE) {
				CStdString msg = "Kontakt ";
				msg+=m->fromUid;
				msg+=" prosi o autoryzacjê.\r\n\t";
				msg+=m->body;
				msg+="\r\n\r\nJe¿eli wyrazisz zgodê, bêdzie widzia³ Twój status.";
				this->SendSubscribed(m->fromUid , plug->IMessage(&sIMessage_msgBox(IMI_CONFIRM , msg , this->GetName() + " - autoryzacja")) != 0 );
				return IM_MSG_delete;
			} else if ((m->type & MT_MASK) == MT_CNTEVENT) {
				CStdString msg = "Komunikat od ";
				msg+=m->fromUid;
				msg+=" :\r\n\t";
				msg+=m->body;
				plug->IMessage(&sIMessage_msgBox(IMI_INFORM , msg , this->GetName() + " - zdarzenie"));
				return IM_MSG_delete;
			}
		}
		return 0;
	}
	case IM_CNT_SEARCH:
		this->CntSearch(reinterpret_cast<sCNTSEARCH*>(msg->p1));
		return 0;

    case IM_CNT_DOWNLOAD: {
		if ((plug->DTgetPos(DTCNT , msg->p1) && GETCNTI(msg->p1,CNT_NET) != this->net) || !this->Connected())
			break;
		if (!this->VCardRequest(msg->p1))
			break;
        return 0;}
    case IM_CNT_UPLOAD:
		if (!Connected()) break;
		if (Ctrl->DTgetPos(DTCNT , msg->p1)) {
			if (plug->DTgetInt(DTCNT , msg->p1 , CNT_NET) != this->net)
				break;
			// jeszcze raz aktualizujemy info o kontakcie...
			SetRosterItem(msg->p1);
			break; // noresult
		}
		if (!this->VCardSet())
			break;
        return 0;

	case kJabber::IMsetTimer:{
		// Wykorzystujemy API Konnekta do prze³¹czenia siê na g³ówny w¹tek, ¿eby to w nim wywo³ywane by³y PINGi 
		if (this->state != cs_online || msg->sender != this->plug->ID()) return 0;
		IMESSAGE_TS_NOWAIT(0);
		LARGE_INTEGER lDueTime;
		lDueTime.QuadPart = -10000 * kJabber::pingInterval; // ping co minutê
		SetWaitableTimer(timerPing , &lDueTime , kJabber::pingInterval , (PTIMERAPCROUTINE)PingTimerRoutine , this , false);
		return 0;}

	}

	plug->setError(IMERROR_NORESULT);
	return 0;
}


void cJabberBase::GetUserAndPass(CStdString & user , CStdString & pass) {
	if (!this->useUser.empty()) {
		user = this->useUser;
		pass = this->usePass;
	} else {
		pass = plug->DTgetStr(DTCFG , 0 , plug->DTgetNameID(DTCFG , dtPrefix + "/pass"));
		user = plug->DTgetStr(DTCFG , 0 , plug->DTgetNameID(DTCFG , dtPrefix + "/user"));
	}
	if (user.empty() || pass.empty()) {
		sDIALOG_access sda;
		sda.flag = DFLAG_SAVE;
		CStdString info = "Podaj login/has³o na serwerze \"" + this->GetHost(false) + "\"";
		sda.info = info;
		CStdString title = this->GetName() + " - Logowanie";
		sda.title = title;
		sda.login = (char*)user.c_str();
		sda.pass = (char*)pass.c_str();
		sda.save = false;
		if (plug->IMessage(&sIMessage_2params(IMI_DLGLOGIN , (int) &sda , 0))) {
			user = sda.login;
			pass = sda.pass;
			if (sda.save) {
				this->useUser = user;
				this->usePass = pass;
				if (this->storeUPonSuccess == supsNo) {
					this->storeUPonSuccess = supsConfig;
				}
			}
		} else {
			user = pass = "";
		}
	}
}



// -------------------------------------------------------------------------------------------
// Zdarzenia
// -------------------------------------------------------------------------------------------
void CALLBACK cJabberBase::PingTimerRoutine(cJabberBase * jab , DWORD l,  DWORD h) {
	__time64_t curr;
	_time64(&curr);
	if (curr - jab->lastPing >= (pingInterval / 1000) && jab->SendPing()) {
		jab->lastPing = curr;
	}
}


void cJabberBase::OnConnected() {
	state = cs_online;
	SetStatus(-1 , false);
	plug->ICMessage(IMC_MESSAGEQUEUE , (int)&sMESSAGESELECT(this->net , 0 , MT_MESSAGE , MF_SEND));
	plug->IMessageDirect(IMsetTimer);
	if ((!this->useUser.empty() || !this->useHost.empty() || !this->usePass.empty() ) && this->storeUPonSuccess != supsNo) {
		if (this->storeUPonSuccess & supsConfig) {
			if (!this->useUser.empty())
				plug->DTsetStr(DTCFG, 0, dtPrefix + "/user", this->useUser);
			if (!this->usePass.empty())
				plug->DTsetStr(DTCFG, 0, dtPrefix + "/pass", this->usePass);
			if (!this->useHost.empty())
				plug->DTsetStr(DTCFG, 0, dtPrefix + "/host", this->useHost);
		}
		if (this->storeUPonSuccess & supsTable) {
			sUIAction act(this->ACTcfgGroup(), 0);
			if (!this->useUser.empty()) {
				act.id = plug->DTgetNameID(DTCFG, dtPrefix + "/user") | IMIB_CFG;
				if (UIActionHandle(act)) {
					UIActionCfgSetValue(act, this->useUser);
				}
			}
			if (!this->usePass.empty()) {
				act.id = plug->DTgetNameID(DTCFG, dtPrefix + "/pass") | IMIB_CFG;
				if (UIActionHandle(act)) {
					UIActionCfgSetValue(act, this->usePass);
				}
			}
			if (!this->useHost.empty()) {
				act.id = plug->DTgetNameID(DTCFG, dtPrefix + "/host") | IMIB_CFG;
				if (UIActionHandle(act)) {
					UIActionCfgSetValue(act, this->useHost);
				}
			}
		}
		if (!(this->storeUPonSuccess & supsKeep)) {
			useUser = usePass = useHost = "";
			this->storeUPonSuccess = supsNo;
		}
	}
/*
	LARGE_INTEGER lDueTime;
	lDueTime.QuadPart = -10000 * kJabber::pingInterval; // ping co minutê
	SetWaitableTimer(timerPing , &lDueTime , kJabber::pingInterval , (PTIMERAPCROUTINE)PingTimerRoutine , this , false);
*/
}

void cJabberBase::OnRosterAnswer() {
	enSynchDirection direction = CFGsynchDirection();
	if (IsSynchronized()
		|| direction == sd_disabled)
		return;
	if (direction != sd_toServer || CFGsynchRemoveContacts()) {
		this->Lock();
		int c = plug->ICMessage(IMC_CNT_COUNT);
		for (int i=1; i < c; i++) {
			if (plug->DTgetInt(DTCNT , i , CNT_NET) != this->net)
				continue;
			int cntID = plug->DTgetID(DTCNT , i);
			if (direction == sd_toClient) {
				// kontakty s¹ ju¿ pousuwane... trzeba je za to dodaæ i uzupe³niæ...
				SetRosterItem(cntID);
			} else {
				// trzeba pousuwaæ nieistniej¹ce kontakty
				CStdString jid = plug->DTgetStr(DTCNT , cntID , CNT_UID);
				if (jid != "*" && !(plug->DTgetInt(DTCNT , cntID , CNT_STATUS) & ST_NOTINLIST) && !RosterContainsJID(jid)) {
					plug->ICMessage(IMC_CNT_REMOVE , cntID);
					i--;
					c--;
				}
			}
		}
		this->Unlock();
	}
	synchronized = true;
}


void cJabberBase::OnRosterItemRemoving(const CStdString & jid) {
	// NIE POWINNO wyst¹piæ przy synchronizowaniu
	if (!IsSynchronized()
		|| this->CFGsynchDirection()  == sd_disabled
		|| !this->CFGsynchRemoveContacts())
		return;
	int cntID = FindContact(jid);
	plug->IMDEBUG(DBG_FUNC , "- OnRosterItemRemoving JID=%s cntID=%x" , jid.c_str() , cntID);
	if (cntID > 0) 
		plug->ICMessage(IMC_CNT_REMOVE , cntID);
}

void cJabberBase::OnRosterItemUpdated(const CStdString & jid , void * item) {
	// NIE POWINNO wyst¹piæ przy synchronizowaniu
	if (!IsSynchronized()
/*		|| plug->DTgetInt(DTCFG , 0 , kJabber::CFG::synchDirection) == CFG::sd_disabled)*/
		)
		return;
	int cntID = FindContact(jid);
	plug->IMDEBUG(DBG_FUNC , "- OnRosterItemUpdated JID=%s cntID=%x" , jid.c_str() , cntID);
	SetContact(cntID , item);
}

void cJabberBase::OnRosterItemAdded(const CStdString & jid , void * item) {
	enSynchDirection direction = CFGsynchDirection();
//	if (direction == CFG::sd_disabled)
//		return;
	int cntID = FindContact(jid);
	plug->IMDEBUG(DBG_FUNC , "- OnRosterItemAdded JID=%s cntID=%x dir=%d synch=%d" , jid.c_str() , cntID , direction , synchronized);
	if (IsSynchronized() || direction == sd_toServer || direction == sd_disabled) {
		SetContact(cntID , item);
	} else { // synchro do klienta
		if (cntID==-1 && CFGsynchRemoveContacts())
			RosterDeleteJID(jid);
		// reszt¹ zajmiemy siê podczas synchronizacji...
	}
}


void cJabberBase::OnPresenceRequest(const CStdString & jid , bool request) {
	if (!request) {
		CStdString notify = jid + " przesta³ pobieraæ Twój status";
		plug->IMessage(&KNotify::sIMessage_notify(notify , UIIcon(IT_MESSAGE,0,MT_EVENT,0),KNotify::sIMessage_notify::tInform,4));
		return;
	}
	plug->IMDEBUG(DBG_MISC , "- Presence request from=%s" , jid.c_str());
	if (CFGautoAuthorize())
		SendSubscribed(jid , true);
	else {
		cMessage m;
//		m.action = sUIAction(IMIG_EVENT , ACT::evtAuthorize);
//		m.notify = UIIcon(IT_MESSAGE , 0 , MT_AUTHORIZE , 0);
		m.type = MT_AUTHORIZE;
		int cntID = FindContact(jid);
		if (cntID==-1)
			m.type |= MT_MASK_NOTONLIST;
		m.net = this->net;
		m.body = "";
		CStdString from = cntID == -1 ? jid : plug->DTgetStr(DTCNT , cntID , CNT_UID);
		m.fromUid = (char*)from.c_str();
		m.flag = MF_MENUBYUI | MF_NOSAVE/* | MF_REQUESTOPEN*/;
		CStdString ext = SetExtParam("" , "ActionTitle" , "Odbierz proœbê o autoryzacjê");

		//= SetExtParam("" , "JID" , presence.getFrom().c_str());
		m.ext = (char*) ext.c_str();
		sMESSAGESELECT ms;
		ms.id = plug->ICMessage(IMC_NEWMESSAGE , (int)&m , true);
		if (ms.id > 0)
			plug->ICMessage(IMC_MESSAGEQUEUE , (int)&ms , true);
	}
}


void cJabberBase::OnSearchAnswer() {
	if (this->dlgSearch)
		this->dlgSearch->cancel = true;
}
void cJabberBase::OnVCardRequestAnswer() {
	if (this->dlgVCardReq)
		this->dlgVCardReq->cancel = true;
}
void cJabberBase::OnVCardSetAnswer() {
	if (this->dlgVCardSet)
		this->dlgVCardSet->cancel = true;
}

void cJabberBase::IgnoreMessage(cMessage * m , int cnt) {
	sHISTORYADD ha;
	ha.m = m;
	ha.dir = HISTORY_IGNORED_DIR;
	ha.name = HISTORY_IGNORED_NAME;
	ha.cnt = 0;
	ha.session = 0;
	ICMessage(IMI_HISTORY_ADD , (int)&ha);
}

} // Namespace
