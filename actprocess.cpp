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
#include "disco_feature.h"

int cJabber::ActionCfgProc(sUIActionNotify_base * anBase) {
	sUIActionNotify_2params * an = (anBase->s_size>=sizeof(sUIActionNotify_2params))?static_cast<sUIActionNotify_2params*>(anBase):0;
/*	switch (anBase->act.id & ~IMIB_CFG) {
	    
	}
	*/
	if ((anBase->act.id & ~IMIB_CFG) == plug->DTgetNameID(DTCFG , this->dtPrefix + "/SSL")) {
		if (anBase->code == ACTN_ACTION) {
			char val [3];
			UIActionCfgGetValue(an->act , val , 2);
			sUIAction portAct = sUIAction(an->act.parent , IMIB_CFG | plug->DTgetNameID(DTCFG,this->dtPrefix + "/port"));
			int port = atoi(UIActionCfgGetValue(portAct,0,0));
			if (*val == '1' && port == 5222)
				UIActionCfgSetValue(portAct , "5223");
			else if (*val == '0' && port == 5223)
				UIActionCfgSetValue(portAct , "5222");
		}
	}
	return 0;
}

int cJabber::ActionProc(sUIActionNotify_base * anBase) {
	sUIActionNotify_2params * an = static_cast<sUIActionNotify_2params*>(anBase);
	switch (anBase->act.id) {
		case kJabber::ACT::servers:
			ACTIONONLY(anBase);
			ShellExecute(0 , "open" , "http://www.jabberpl.org/Serwery/Porownanie" , "" , "" , SW_MAXIMIZE);
			break;
		case kJabber::ACT::services:
			ACTIONONLY(anBase);
			IMDEBUG(DBG_DEBUG, "Open disco for: %s", session.getLocalJID().c_str());
			ServiceDiscovery(JID::getHost(session.getLocalJID()));
			break;
		case ACT::registerAccount:
			ACTIONONLY(anBase);
			this->ActionRegisterAccount();
			break;
		case ACT::editRegistration:
			ACTIONONLY(anBase);
			this->ActionEditRegistration();
			break;
		default:
			return cJabberBase::ActionProc(anBase);
	};
	return 0;
}
void cJabber::ActionEditRegistration() {
	if (!this->Connected()) {
		this->Connect();
		if (plug->IMessage(&sIMessage_msgBox(IMI_WARNING , "Edycja konta wymaga po³¹czenia z serwerem Jabber.\r\nZanim naciœniesz [OK] poczekaj na po³¹czenie.", "kJabber", MB_OKCANCEL))
			== IDCANCEL) return;
		this->ActionEditRegistration();
		return;
	}
	new JEP::cFeatureIQRegister(*this, (HWND) UIGroupHandle(sUIAction(0, IMIG_CFGWND)), this->GetHost(false), "Ustawienia rejestracji - " + this->GetHost(false));
}

void cJabber::ActionRegisterAccount() {
	sUIACTION act (this->ACTcfgGroup(), IMIB_CFG | plug->DTgetNameID(DTCFG , dtPrefix + "/host"));
	CStdString host;
	if (UIActionHandle(act)) {
		host = UIActionCfgGetValue(act, 0, 0);
	} else {
		host = plug->DTgetStr(DTCFG, 0, dtPrefix + "/host");
	}
	if (host.empty()) {
		plug->IMessage(&sIMessage_msgBox(IMI_ERROR , "Musisz najpierw wybraæ serwer, na którym chcesz za³o¿yæ konto!", "kJabber"));
		this->OnCantConnect(false);
		return;
	}
	if (this->Connected()) {
		this->Disconnect();
		plug->IMessage(&sIMessage_msgBox(IMI_WARNING , "Za³o¿enie konta wymaga nowego po³¹czenia z serwerem Jabber.\r\nZanim naciœniesz [OK] poczekaj a¿ roz³¹czanie siê zakoñczy.", "kJabber"));
		this->ActionRegisterAccount();
		return;
	}
	sDIALOG_access sda;
	sda.flag = DFLAG_PASS2;
	CStdString info = "Podaj login i has³o nowego konta na serwerze \"" + host + "\"";
	sda.info = info;
	CStdString title = this->GetName() + " - Zak³adanie konta";
	sda.title = title;
	sda.login = "";
	sda.pass = "";
	if (plug->IMessage(&sIMessage_2params(IMI_DLGLOGIN , (int) &sda , 0))) {
		if (!*sda.login || !*sda.pass) {
			plug->IMessage(&sIMessage_msgBox(IMI_ERROR , "Musisz podaæ login I has³o!", "kJabber"));
			this->ActionRegisterAccount();
			return;
		}
		CStdString login = sda.login;
		CStdString pass = sda.pass;
		if (plug->IMessage(&sIMessage_msgBox(IMI_INFORM , "kJabber spróbuje teraz po³¹czyæ siê z wybranym serwerem i za³o¿yæ konto.\r\nJe¿eli jest taka potrzeba, po³¹cz siê teraz z internetem, naciœnij [OK] i poczekaj na dalsze informacje.", "kJabber", MB_OKCANCEL))==IDOK) {
			this->useHost = host;
			this->usePass = pass;
			this->useUser = login;
			this->storeUPonSuccess = supsTableAndConfig;
			this->createAccount = true;
			this->Connect();
		}
	}
}