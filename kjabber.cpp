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
#include <Konnekt/lib.h>

/*
#ifdef _DEBUG
#pragma comment(lib , "../../__libs/jabberoo_d.lib")
#else
#pragma comment(lib , "../../__libs/jabberoo.lib")
#endif
*/

#pragma comment(lib , "Ws2_32.lib")
#pragma comment(lib , "ssleay32.lib")
#pragma comment(lib , "libeay32.lib")
#pragma comment(lib , "comctl32.lib")
using namespace kJabber;

cJabber * jabber = 0;
char nextSlot = 0;


int __stdcall DllMain(void * hinstDLL, unsigned long fdwReason, void * lpvReserved)
{
        return true;
}
//---------------------------------------------------------------------------
/* Wszystkie poni¿sze funkcje nie s¹ wymagane (z wyj¹tekim IMEssageProc)...
   ale z nimi wszystko wygl¹da czytelniej :)
*/
int Init() {
	WSADATA wsaData;
	WSAStartup( MAKEWORD( 2, 0 ), &wsaData );
	jabber = new cJabber(Ctrl);
    return 1;
}

int DeInit() {
	delete jabber;
	jabber = 0;
    return 1;
}

int IStart() {
	return 1;
}
int IEnd() {
  /* Tutaj wtyczkê wy³¹czamy */
	return 1;
}

int ISetCols() {
  /* Tutaj rejestrujemy kolumny w plikach ustawieñ.
     Pamiêtaj o tym ¿e identyfikatory MUSZ¥ byæ UNIKATOWE? */
//	SetColumn(
//	SetColumn(DTCNT , kJabber::CNT::synched , DT_CT_INT | DT_CF_NOSAVE , 0);
	SetColumn(DTCFG , kJabber::CFG::synchAddContacts , DT_CT_INT , 1 , "kJabber/synch/addContacts");
	SetColumn(DTCFG , kJabber::CFG::synchRemoveContacts , DT_CT_INT , 1 , "kJabber/synch/removeContacts");
	SetColumn(DTCFG , kJabber::CFG::autoAuthorize , DT_CT_INT , 1 , "kJabber/auto/authorize");
	SetColumn(DTCFG , kJabber::CFG::autoSubscribe , DT_CT_INT , 1 , "kJabber/auto/subscribe");
	SetColumn(DTCFG , kJabber::CFG::synchDirection , DT_CT_INT , kJabber::sd_toServer , "kJabber/synch/direction");

	SetColumn(DTCFG, kJabber::CFG::discoShowFeatures, DT_CT_INT, 0 , "kJabber/disco/showFeatures");
	SetColumn(DTCFG, kJabber::CFG::discoReadAll, DT_CT_INT, 1 , "kJabber/disco/readAll");
	SetColumn(DTCFG, kJabber::CFG::discoViewType, DT_CT_INT, LVS_ICON , "kJabber/disco/viewType");
	SetColumn(DTCFG, kJabber::CFG::discoViewGrouping, DT_CT_INT, 1 , "kJabber/disco/viewGrouping");
	SetColumn(DTCFG, kJabber::CFG::discoInfo, DT_CT_INT, 1 , "kJabber/disco/info");

	return 1;
}

void IPrepare_subscription(int parent) {
	UIActionAdd(parent , ACT::subShow , ACTT_CHECK , "Wysy³aj status" , 0);
	UIActionAdd(parent , ACT::subSubscribed , ACTT_CHECK , "Pobieraj status" , 0);
	UIActionAdd(parent , 0 , ACTT_SEPARATOR);
	UIActionAdd(parent , ACT::sendStatus , ACTR_INIT , "Wyœlij status" , 0);
	UIActionAdd(parent , ACT::sendUnavailable , 0 , "Ukryj (jednorazowo)" , UIIcon(IT_STATUS , kJabber::net , ST_OFFLINE , 0));
}

int IPrepare() {

	IconRegister(IML_16 , UIIcon(IT_LOGO , jabber->GetNet() , 0 , 0) , Ctrl->hDll() , IDI_LOGO);
	IconRegister(IML_16 , UIIcon(IT_STATUS , jabber->GetNet() , ST_ONLINE , 0) , Ctrl->hDll() , IDI_ST_ONLINE);
	IconRegister(IML_16 , UIIcon(IT_STATUS , jabber->GetNet() , ST_AWAY , 0) , Ctrl->hDll() , IDI_ST_AWAY);
	IconRegister(IML_16 , UIIcon(IT_STATUS , jabber->GetNet() , ST_OFFLINE , 0) , Ctrl->hDll() , IDI_ST_OFFLINE);
	IconRegister(IML_16 , UIIcon(IT_STATUS , jabber->GetNet() , ST_AWAY , 0) , Ctrl->hDll() , IDI_ST_AWAY);
	IconRegister(IML_16 , UIIcon(IT_STATUS , jabber->GetNet() , ST_NA , 0) , Ctrl->hDll() , IDI_ST_NA);
	IconRegister(IML_16 , UIIcon(IT_STATUS , jabber->GetNet() , ST_DND , 0) , Ctrl->hDll() , IDI_ST_DND);
	IconRegister(IML_16 , UIIcon(IT_STATUS , jabber->GetNet() , ST_CHAT , 0) , Ctrl->hDll() , IDI_ST_CHAT);
	IconRegister(IML_16 , UIIcon(IT_STATUS , jabber->GetNet() , ST_HIDDEN , 0) , Ctrl->hDll() , IDI_ST_HIDDEN);
	IconRegister(IML_16 , UIIcon(IT_STATUS , jabber->GetNet() , ST_CONNECTING , 0) , Ctrl->hDll() , IDI_ST_CONNECTING);
	IconRegister(IML_ICO , UIIcon(IT_OVERLAY , jabber->GetNet() , OVR_NETLOGO , 0) , Ctrl->hDll() , IDI_OVERLAY);

	if (jabber->GetSlot() == 0) {

		IconRegister(IML_16 , IDI_AGENT , Ctrl->hDll() , IDI_AGENT);
		IconRegister(IML_16 , IDI_CONFERENCE , Ctrl->hDll() , IDI_CONFERENCE);
		HANDLE bmp;
		if (!ICMessage(IMI_ICONEXISTS , UIIcon(IT_STATUS , ST_OFFLINE , NET_AIM , 0) , IML_16)) {
			bmp = LoadImage(Ctrl->hDll() , "setAIM" , IMAGE_BITMAP , 0 , 0 , 0);
			int ids [] = {UIIcon(IT_STATUS , NET_AIM , ST_ONLINE , 0),
				UIIcon(IT_STATUS , NET_AIM , ST_CHAT , 0),
				UIIcon(IT_STATUS , NET_AIM , ST_AWAY , 0),
				UIIcon(IT_STATUS , NET_AIM , ST_NA , 0),
				UIIcon(IT_STATUS , NET_AIM , ST_DND , 0),
				UIIcon(IT_STATUS , NET_AIM , ST_OFFLINE , 0)
			};
			IconRegisterList(IML_16 , sizeof(ids)/4 , ids , bmp , 0 , 16);
			DeleteObject(bmp);
		}
		if (!ICMessage(IMI_ICONEXISTS , UIIcon(IT_STATUS , NET_GG , ST_OFFLINE , 0) , IML_16)) {
			bmp = LoadImage(Ctrl->hDll() , "setGG" , IMAGE_BITMAP , 0 , 0 , 0);
			int ids [] = {UIIcon(IT_STATUS , NET_GG , ST_ONLINE , 0),
				UIIcon(IT_STATUS , NET_GG , ST_OFFLINE , 0),
				UIIcon(IT_STATUS , NET_GG , ST_AWAY , 0)
			};
			IconRegisterList(IML_16 , sizeof(ids)/4 , ids , bmp , 0 , 16);
			DeleteObject(bmp);
		}
		if (!ICMessage(IMI_ICONEXISTS , UIIcon(IT_STATUS , NET_ICQ , ST_OFFLINE , 0) , IML_16)) {
			bmp = LoadImage(Ctrl->hDll() , "setICQ" , IMAGE_BITMAP , 0 , 0 , 0);
			int ids [] = {UIIcon(IT_STATUS , NET_ICQ , ST_ONLINE , 0),
				UIIcon(IT_STATUS , NET_ICQ , ST_CHAT , 0),
				UIIcon(IT_STATUS , NET_ICQ , ST_AWAY , 0),
				UIIcon(IT_STATUS , NET_ICQ , ST_NA , 0),
				UIIcon(IT_STATUS , NET_ICQ , ST_DND , 0),
				UIIcon(IT_STATUS , NET_ICQ , ST_OFFLINE , 0)
			};
			IconRegisterList(IML_16 , sizeof(ids)/4 , ids , bmp , 0 , 16);
			DeleteObject(bmp);
		}
		if (!ICMessage(IMI_ICONEXISTS , UIIcon(IT_STATUS , NET_MSN , ST_OFFLINE , 0) , IML_16)) {
			bmp = LoadImage(Ctrl->hDll() , "setMSN" , IMAGE_BITMAP , 0 , 0 , 0);
			int ids [] = {UIIcon(IT_STATUS , NET_MSN , ST_ONLINE , 0),
				UIIcon(IT_STATUS , NET_MSN , ST_OFFLINE , 0),
				UIIcon(IT_STATUS , NET_MSN , ST_DND , 0),
				UIIcon(IT_STATUS , NET_MSN , ST_AWAY , 0),0,0,
				UIIcon(IT_STATUS , NET_MSN , ST_CHAT , 0)
			};
			IconRegisterList(IML_16 , sizeof(ids)/4 , ids , bmp , 0 , 16);
			DeleteObject(bmp);
		}
		if (!ICMessage(IMI_ICONEXISTS , UIIcon(IT_STATUS , NET_YAHOO , ST_OFFLINE , 0) , IML_16)) {
			bmp = LoadImage(Ctrl->hDll() , "setYAHOO" , IMAGE_BITMAP , 0 , 0 , 0);
			int ids [] = {UIIcon(IT_STATUS , NET_YAHOO , ST_ONLINE , 0),
				UIIcon(IT_STATUS , NET_YAHOO , ST_CHAT , 0),
				UIIcon(IT_STATUS , NET_YAHOO , ST_AWAY , 0),
				UIIcon(IT_STATUS , NET_YAHOO , ST_NA , 0),
				UIIcon(IT_STATUS , NET_YAHOO , ST_DND , 0),
				UIIcon(IT_STATUS , NET_YAHOO , ST_OFFLINE , 0)
			};
			IconRegisterList(IML_16 , sizeof(ids)/4 , ids , bmp , 0 , 16);
			DeleteObject(bmp);
		}
	}
	/* HACK: Znowu hack na sloty... */
	UIGroupAdd(IMIG_CNT , jabber->GetNet() * 1000 + ACT::subscription_off , ACTR_INIT , "Status" , UIIcon(IT_LOGO , kJabber::net , 0 , 0));
	IPrepare_subscription(jabber->GetNet() * 1000 + ACT::subscription_off);
	UIGroupAdd(IMIG_NFO_TB , jabber->GetNet() * 1000 + ACT::subscriptionNfo_off , ACTR_INIT , "Status" , UIIcon(IT_LOGO , kJabber::net , 0 , 0));
	IPrepare_subscription(jabber->GetNet() * 1000 + ACT::subscriptionNfo_off);

	UIActionAdd(IMIG_NFO_SAVE , jabber->GetNet() * 1000 + kJabber::ACT::vcardSet_off , ACTR_INIT , "Zapisz jako vCard (" + jabber->GetName() + ")", UIIcon(IT_LOGO , jabber->GetNet() , 0 , 0));
	UIActionAdd(IMIG_NFO_REFRESH , jabber->GetNet() * 1000 + kJabber::ACT::vcardRequest_off , ACTR_INIT , "Pobierz vCard (" + jabber->GetName() + ")", UIIcon(IT_LOGO , jabber->GetNet() , 0 , 0));

	
//	IconRegister(IML_16 , UIIcon(IT_MESSAGE , kJabber::net , MT_MESSAGE , 0) , Ctrl->hDll() , IDI_MT_MESSAGE);

	if (!jabber->GetSlot()) {
		UIGroupAdd(IMIG_CFG_USER , kJabber::ACT::CfgGroup , 0 , "Jabber" , UIIcon(IT_LOGO , kJabber::net , 0 , 0));
		UIActionCfgAddPluginInfoBox2(kJabber::ACT::CfgGroup, 
			"<div><b>kJabber</b> umo¿liwia komunikacjê przy pomocy protoko³u <span class='under_dot'><b>XMPP</b></span> znanego równie¿ jako <span class='under_dot'><b>Jabber</b></span>. "
				"Wiêcej informacji znajdziesz pod adresem http://www.jabberpl.org/ "
				, "Za obs³ugê protoko³u odpowiada biblioteka <b>JabberOO</b>."
				"<br/>Obs³uga DNS SRV: <b>Piotr Pawluczuk</b>"
				"<br/><br/>Copyright ©2003,2004,2005 <b>Stamina</b>"
				, "res://dll/12000.ico", -3);
/*
		UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_GROUP ,  "");{
			UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_IMAGE | ACTSC_INLINE , "res://dll/12000.ico" , 0 , 32 , 32);
			UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_HTMLINFO ,  
				"<div class='title'>kJabber (¯abber)</div>"
				"<div>¯abber umo¿liwia rozmawianie za pomoc¹ protoko³u <span class='under_dot'><b>XMPP</b></span> (<b>Jabber</b>). "
				"Wiêcej informacji znajdziesz pod adresem http://www.jabberpl.org/ </div>"
				"<div class='note'>Wtyczka dzia³a dziêki bibliotece <b>JabberOO</b>.</div>"
				"<br><div class='copy'>©2003,2004 stamina (<b>http://www.konnekt.info</b>)." 
				, 0 , 0 , 100);
	//		UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_INFO ,  
	//			"");
		} UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_GROUPEND);
		*/
	}
	jabber->UIPrepare();
	if (!jabber->GetSlot()) {
		if (ShowBits::checkLevel(ShowBits::levelNormal)) {
			UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_GROUP ,  "Synchronizacja listy kontaktów");{
		//		UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_INFO , "");
				UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_COMBO | ACTSCOMBO_BYPOS | ACTSCOMBO_LIST ,  "Nie synchronizuj" CFGICO "116" "\nSynchronizuj do serwera" CFGICO "0x20C00000" "\nSynchronizuj do Konnekta" CFGICO "0x65" 
					AP_PARAMS AP_TIPRICH "W normalnych warunkach powinieneœ u¿ywaæ synchronizacji <b>do serwera</b>. Pamiêtaj jednak, ¿e w tej sytuacji zmiany na liœcie kontaktów w trybie offline zostan¹ skasowane po po³¹czeniu z serwerem!"
					"<br/><div class=\"warn\">Wy³¹czenie synchronizacji mo¿e zupe³nie uniemo¿liwiæ porozumiewanie siê!</div>" 
					, kJabber::CFG::synchDirection , 170);
				if (ShowBits::checkLevel(ShowBits::levelAdvanced)) {
					UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_CHECK , "Dodawaj kontakty na synchronizowanej liœcie" , CFG::synchAddContacts);
					UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_CHECK , "Usuwaj kontakty z synchronizowanej listy" , CFG::synchAddContacts);
				}
			} UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_GROUPEND);
		}
		if (ShowBits::checkLevel(ShowBits::levelIntermediate)) {
			UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_GROUP ,  "Autoryzacja");{
				if (ShowBits::checkBits(ShowBits::showInfoBeginner)) {
					UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_TIPBUTTON | ACTSC_INLINE , AP_TIP  AP_TIPIMAGEURL "file://%KonnektData%/img/jabber_auth.png");
					UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_INFO , "Bez autoryzacji nie bêdziesz widzia³ czyjegoœ statusu. Jej stan mo¿esz podejrzeæ/zmieniæ bezpoœrednio na liœcie kontaktów." , 0, 0, -2);
				}
				UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_CHECK , "Automatycznie pytaj o autoryzacjê nowe kontakty"  , CFG::autoSubscribe);
				UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_CHECK , "Automatycznie potwierdzaj autoryzacjê" , CFG::autoAuthorize);
			} UIActionAdd(kJabber::ACT::CfgGroup , 0 , ACTT_GROUPEND);
		}
	}
    return 1;
}

int ActionCfgProc(sUIActionNotify_base * anBase) {
  /* Tutaj obs³ugujemy akcje dla okna konfiguracji */ 
  /* Sytuacja taka sama jak przy ActionProc  */
  sUIActionNotify_2params * an = (anBase->s_size>=sizeof(sUIActionNotify_2params))?static_cast<sUIActionNotify_2params*>(anBase):0;
  switch (anBase->act.id & ~IMIB_CFG) {
  default:
	  return jabber->ActionCfgProc(anBase);
  }
  return 0;
}

ActionProc(sUIActionNotify_base * anBase) {
  /* Tutaj obs³ugujemy akcje */ 
  /* Poni¿sza linijka s³u¿y TYLKO waszej wygodzie! 
     Wiêkszoœæ (o ile nie wszystkie) powiadomieñ przesy³ana jest jako sUIActionNotify_2params,
     korzystamy wtedy z obiektu an, w przeciwnym razie z anBase, lub castujemy do spodziewanego typu.
  */
  sUIActionNotify_2params * an = (anBase->s_size>=sizeof(sUIActionNotify_2params))?static_cast<sUIActionNotify_2params*>(anBase):0;

  if ((anBase->act.id & IMIB_) == IMIB_CFG) return ActionCfgProc(anBase); 
  switch (anBase->act.id) {
  default:
	  return jabber->ActionProc(anBase);
  }
  return 0;
}



int __stdcall IMessageProc(sIMessage_base * msgBase) {

	sIMessage_2params * msg = (msgBase->s_size>=sizeof(sIMessage_2params))?static_cast<sIMessage_2params*>(msgBase):0;
    switch (msgBase->id) {
    /* Wiadomoœci na które TRZEBA odpowiedzieæ */
		/*HACK: Sloty...*/
	case IM_PLUG_NET:        return jabber ? jabber->GetNet() : -1; // Zwracamy wartoœæ NET, która MUSI byæ ró¿na od 0 i UNIKATOWA!
	case IM_PLUG_TYPE:       return IMT_CONFIG | IMT_CONTACT | IMT_MESSAGE | IMT_MSGUI | IMT_NET | IMT_NETSEARCH | IMT_NETUID | IMT_PROTOCOL | IMT_UI; // Zwracamy jakiego typu jest nasza wtyczka (które wiadomoœci bêdziemy obs³ugiwaæ)
    case IM_PLUG_VERSION:    return 0; // Wersja wtyczki tekstowo major.minor.release.build ...
    case IM_PLUG_SDKVERSION: return KONNEKT_SDK_V;  // Ta linijka jest wymagana!
    case IM_PLUG_SIG:        return (int)"KJABBER"; // Sygnaturka wtyczki (krótka, kilkuliterowa nazwa)
    case IM_PLUG_CORE_V:     return (int)"W98"; // Wymagana wersja rdzenia
    case IM_PLUG_UI_V:       return 0; // Wymagana wersja UI
    case IM_PLUG_NAME:       return (int)"kJabber"; // Pe³na nazwa wtyczki
	case IM_PLUG_NETNAME:    return jabber ? (int)jabber->GetName().c_str() : (int)"Jabber"; // Nazwa obs³ugiwanej sieci (o ile jak¹œ sieæ obs³uguje)
	case IM_PLUG_NETSHORTNAME: return (int)"Jabber";
	case IM_PLUG_UIDNAME:    return (int)"JID";
    case IM_PLUG_INIT:       Plug_Init(msg->p1,msg->p2);return Init();
    case IM_PLUG_DEINIT:     Plug_Deinit(msg->p1,msg->p2);return DeInit();
    case IM_PLUG_PRIORITY:   return PLUGP_LOW;

	case IM_SETCOLS:         if (jabber->GetSlot() == 0) ISetCols();break;

	case IM_UI_PREPARE:      IPrepare();  return 1; // Wywo³ujemy to WEWN¥TRZ!
	case IM_START:           IStart();  break;
    case IM_END:             IEnd();  break;

	case IM_ALLPLUGINSINITIALIZED: {
		if (jabber->GetSlot()) {
			int verA = ICMessage(IMC_PLUG_VERSION , ICMessage(IMC_PLUGID_POS , ICMessage(IMC_FINDPLUG , kJabber::net , IMT_PROTOCOL)));
			int verB = ICMessage(IMC_PLUG_VERSION , ICMessage(IMC_PLUGID_POS , Ctrl->ID()));
			if (verA != verB)
			{
				Ctrl->IMessage(&sIMessage_plugOut(Ctrl->ID() , "Wszystkie kopie wtyczki kJabber, ¿eby dzia³aæ jednoczeœnie, musz¹ byæ w TYCH SAMYCH WERSJACH!\r\nPo zamkniêciu programu, zaktualizuj kopie i uruchom go ponownie..." , sIMessage_plugOut::erShut , sIMessage_plugOut::euNow));
			}
		}
		return 0;
	}


    case IM_UIACTION:        return ActionProc((sUIActionNotify_base*)msg->p1);

    /* Tutaj obs³ugujemy wszystkie pozosta³e wiadomoœci */
	}
	if (jabber) {
		return jabber->IMessageProc(msgBase);
	} else {
        if (Ctrl) Ctrl->setError(IMERROR_NORESULT);
		return 0;
	}
}

