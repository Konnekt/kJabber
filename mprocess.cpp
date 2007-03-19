/*
 *  kJabber
 *  
 *  Please READ & ACCEPT /License.txt FIRST! 
 * 
 *  Copyright (C)2005,2006 Rafa� Lindemann, STAMINA
 *  http://www.stamina.pl/ , http://www.konnekt.info/
 *
 *  $Id: $
 */


#include "stdafx.h"
void cJabber::SetColumns() {
	cJabberBase::SetColumns();
	plug->SetColumn(DTCFG , -1 , DT_CT_STR | DT_CF_CXOR , "" , this->dtPrefix + "/host");
	plug->SetColumn(DTCFG , -1 , DT_CT_INT , 5222 , this->dtPrefix + "/port");
	plug->SetColumn(DTCFG , -1 , DT_CT_STR | DT_CF_CXOR, "Konnekt" , this->dtPrefix + "/resource");
//	plug->SetColumn(DTCFG , -1 , DT_CT_INT , 1 , this->dtPrefix + "/syncRoster");
//	plug->SetColumn(DTCFG , -1 , DT_CT_INT , 0 , this->dtPrefix + "/createAccount");
	plug->SetColumn(DTCFG , -1 , DT_CT_PCHAR , "users.jabber.org" , this->dtPrefix + "/JUDserver");
	plug->SetColumn(DTCFG , -1 , DT_CT_INT , 0 , this->dtPrefix + "/SSL");
	plug->SetColumn(DTCFG , -1 , DT_CT_INT , 10 , dtPrefix + "/priority");
	plug->SetColumn(DTCFG , -1 , DT_CT_INT , 10 , dtPrefix + "/priority");
	SetColumn(DTCFG, -1 , DT_CT_INT, 1 , dtPrefix + "/acceptHTML");

}

void cJabber::UIPrepare() {
	/*HACK: wiele kont */
	int status = kJabber::ACT::Status + this->GetSlot();
	plug->UIActionInsert(IMIG_STATUS , status , -1 , ACTS_GROUP , this->GetName() , UIIcon(IT_LOGO , this->net , 0 , 0));

	plug->UIActionInsert(status , kJabber::ACT::services , -1 , ACTSMENU_BOLD | ACTR_INIT , "Us�ugi" , 0);

	plug->UIActionInsert(status , 0 , -1 , ACTT_SEPARATOR , "" , 0);

	plug->UIActionInsert(status , kJabber::ACT::stOnline , -1 , 0 , "Dost�pny" , UIIcon(IT_STATUS , this->net , ST_ONLINE , 0));
	plug->UIActionInsert(status , kJabber::ACT::stChat , -1 , 0 , "Pogadam" , UIIcon(IT_STATUS , this->net , ST_CHAT  , 0));
	plug->UIActionInsert(status , kJabber::ACT::stAway , -1 , 0 , "Zaraz wracam" , UIIcon(IT_STATUS , this->net , ST_AWAY  , 0));
	plug->UIActionInsert(status , kJabber::ACT::stNA , -1 , 0 , "Nieosi�galny" , UIIcon(IT_STATUS , this->net , ST_NA  , 0));
	plug->UIActionInsert(status , kJabber::ACT::stDND , -1 , 0 , "Nie przeszkadza�" , UIIcon(IT_STATUS , this->net , ST_DND  , 0));
	plug->UIActionInsert(status , kJabber::ACT::stHidden , -1 , 0 , "Ukryty" , UIIcon(IT_STATUS , this->net , ST_HIDDEN  , 0));
	plug->UIActionInsert(status , kJabber::ACT::stOffline , -1 , 0 , "Niedost�pny" , UIIcon(IT_STATUS , this->net , ST_OFFLINE , 0));
	/*HACK: wiele kont */
	int cfg = this->ACTcfgGroup();
	if (this->GetSlot()) {
		plug->UIActionInsert(kJabber::ACT::CfgGroup , cfg , -1 , ACTS_GROUP , this->GetName() , UIIcon(IT_LOGO , this->net , 0 , 0));
	}
//	plug->UIActionInsert(IMIG_CFG_USER , kJabber::ACT::CfgGroup , -1 , ACTS_GROUP , "Jabber");
	plug->UIActionInsert(cfg , 0 , -1 , ACTT_GROUP , "Serwer"); {
		plug->UIActionInsert(cfg , 0 , -1 , ACTT_COMMENT | ACTSC_INLINE , "Serwer" , 0 , 40);
		plug->UIActionInsert(cfg , IMIB_CFG , -1 , ACTT_COMBO | ACTSC_INLINE | ACTSCOMBO_NOICON | ACTSCOMBO_SORT , "jabber.atman.pl\nchrome.pl\njabberpl.org\nhisteria.pl\njabber.wp.pl\njabber.uznam.net.pl\njabber.gda.pl\njabber.aster.pl" 
		, plug->DTgetNameID(DTCFG , dtPrefix + "/host"));
		plug->UIActionInsert(cfg , 0 , -1 , ACTT_TIPBUTTON | ACTSC_INLINE , 
			AP_TIPRICH "Nazw� serwera znajdziesz w swoim identyfikatorze (<b>JID</b>) - konto@<b>serwer</b>."
			"<br/><br/>Je�eli jeszcze nie masz konta w sieci Jabber, wpisz adres serwera lub wybierz kt�ry� z listy. "
			"Pami�taj, �e b�dziesz m�g� rozmawia� z u�ytkownikami i korzysta� z us�ug innych serwer�w."
			AP_TIPRICH_WIDTH "300");
		plug->UIActionInsert(cfg , kJabber::ACT::servers , -1 , ACTT_BUTTON , "Zobacz list� serwer�w" AP_IMGURL "res://ui/url.ico" AP_ICONSIZE "16" AP_TIP "Link do zewn�trznej strony" , 0 , 150 , 24);
		//if (ShowBits::checkLevel(ShowBits::levelNormal)) {
			plug->UIActionInsert(cfg , 0 , -1 , ACTT_COMMENT | ACTSC_INLINE , "Port", 0, 40);
			plug->UIActionInsert(cfg , IMIB_CFG , -1 , ACTT_EDIT | ACTSC_INT | ACTSC_INLINE , "" AP_TIP "Domy�lnym portem jest 5222 (5223 dla po��cze� szyfrowanych)" , plug->DTgetNameID(DTCFG , dtPrefix + "/port") , 50 , 21);
			plug->UIActionInsert(cfg , 0 , -1 , ACTT_IMAGE | ACTSC_INLINE , "res://ui/secure.ico" , 0, 16, 16);
			plug->UIActionInsert(cfg , IMIB_CFG , -1 , ACTT_CHECK | ACTR_CHECK , "��cz bezpiecznie (SSL)" , plug->DTgetNameID(DTCFG , dtPrefix + "/SSL"));
		//}
	} plug->UIActionInsert(cfg , 0 , -1 , ACTT_GROUPEND);
	plug->UIActionInsert(cfg , 0 , -1 , ACTT_GROUP , "Konto"); {
		plug->UIActionInsert(cfg , 0 , -1 , ACTT_COMMENT | ACTSC_INLINE , "U�ytkownik" , 0 , 55);
		plug->UIActionInsert(cfg , IMIB_CFG , -1 , ACTT_EDIT | ACTSC_INLINE , "" , plug->DTgetNameID(DTCFG , dtPrefix + "/user"));
		plug->UIActionInsert(cfg , IMIB_CFG , -1 , ACTT_PASSWORD | ACTSC_INLINE , "" , plug->DTgetNameID(DTCFG , dtPrefix + "/pass"));
		plug->UIActionInsert(cfg , 0 , -1 , ACTT_COMMENT , "Has�o");
		plug->UIActionInsert(cfg , 0 , -1 , ACTT_SEPARATOR);
		plug->UIActionInsert(cfg , ACT::registerAccount , -1 , ACTT_BUTTON | ACTSC_BOLD | ACTSC_INLINE , "Za�� konto" AP_TIP "Zak�ada konto na wybranym serwerze" AP_IMGURL "res://ui/accountcreate.ico" AP_ICONSIZE "16", 0, 120, 30);
		plug->UIActionInsert(cfg , ACT::services , -1 , ACTT_BUTTON | ACTR_INIT | ACTSC_INLINE | ACTS_DISABLED , "Us�ugi" AP_TIP "Wy�wietla przegl�dark� us�ug dost�pnych na serwerze.\nNie wszystkie serwery to obs�uguj�.\nMusisz by� po��czony!"  AP_IMGURL "res://dll/12250.ico" AP_ICONSIZE "16", 0, 100, 30);
		plug->UIActionInsert(cfg , ACT::editRegistration , -1 , ACTT_BUTTON | ACTR_INIT , "Zmie� has�o/usu�" AP_TIP "Wy�wietla okno zmiany rejestracji na serwerze. \nNie wszystkie serwery to obs�uguj�.\nMusisz by� po��czony!"  AP_IMGURL "res://ui/changepassword.ico" AP_ICONSIZE "16", 0, 130, 30);
	} plug->UIActionInsert(cfg , 0 , -1 , ACTT_GROUPEND);
	if (ShowBits::checkLevel(ShowBits::levelNormal)) {
		plug->UIActionInsert(cfg , 0 , -1 , ACTT_GROUP , "Ustawienia"); {

			if (ShowBits::checkLevel(ShowBits::levelAdvanced)) {
				plug->UIActionInsert(cfg , 0 , -1 , ACTT_COMMENT | ACTSC_INLINE , "Zas�b" , 0 , 55);
				plug->UIActionInsert(cfg , 0 , -1 , ACTT_EDIT | ACTSC_INLINE , "" AP_TIPRICH "Nazwa zasobu w Twoim JIDzie - konto@serwer/<b>Zas�b</b>" , plug->DTgetNameID(DTCFG , dtPrefix + "/resource"));
				plug->UIActionInsert(cfg , 0 , -1 , ACTT_EDIT | ACTSC_INT | ACTSC_INLINE , "" AP_TIP "Priorytet tego po��czenia wzgl�dem innych Twoich aktywnych klient�w" , plug->DTgetNameID(DTCFG , dtPrefix + "/priority") , 30);
				plug->UIActionInsert(cfg , 0 , -1 , ACTT_COMMENT , "Priorytet");
			}

			plug->UIActionInsert(cfg , 0 , -1 , ACTT_COMMENT | ACTSC_INLINE , "Serwer JUD" , 0 , 55);
			plug->UIActionInsert(cfg , 0 , -1 , ACTT_COMBO | ACTSCOMBO_NOICON , "users.jabber.org" , plug->DTgetNameID(DTCFG , dtPrefix + "/JUDserver") , 150);
			plug->UIActionInsert(cfg , 0 , -1 , ACTT_CHECK, "Przyjmuj wiadomo�ci z formatowaniem (HTML)" , plug->DTgetNameID(DTCFG , dtPrefix + "/acceptHTML"));
		} plug->UIActionInsert(cfg , 0 , -1 , ACTT_GROUPEND);
	}

	cJabberBase::UIPrepare();
}



int cJabber::IMessageProc(sIMessage_base * msgBase) {
    sIMessage_2params * msg = (msgBase->s_size>=sizeof(sIMessage_2params))?static_cast<sIMessage_2params*>(msgBase):0;
    switch (msgBase->id) {

	case kJabber::IM::getSession:
		return (int)&this->session;

	default:
		return cJabberBase::IMessageProc(msgBase);

	}
	return 0;
}
