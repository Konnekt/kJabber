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


#pragma once

using namespace jabberoo;
using namespace judo;

extern char nextSlot;

namespace kJabber {

	CStdString LocaleToUTF8(CStdString txt);
	CStdString UTF8ToLocale(CStdString utf8);


	/* Obs³uga 'slota' jabberowego... W przysz³oœci na raz bêdzie
	   ich dzia³aæ kilka */
	class cJabber : public cJabberBase ,  public SigC::Object {
	public:
		cJabber(cCtrl * ctrl); // inicjalizacja

	// Podstawowa dzia³alnoœæ

		void StartSSL();
		int FindContact(const CStdString & jid);
		int CompareJID(const CStdString & a, const CStdString & b);


		/* Obs³uga komunikatów Konnekta */

		int IMessageProc(sIMessage_base * msgBase);
		int ActionCfgProc(sUIActionNotify_base * anBase);
		int ActionProc(sUIActionNotify_base * anBase);
		void UIPrepare();
		void SetColumns();

		void ActionRegisterAccount();
		void ActionEditRegistration();
		/* CFG */
		enSynchDirection CFGsynchDirection() {return (enSynchDirection)plug->DTgetInt(DTCFG , 0 , kJabber::CFG::synchDirection);}
		int CFGsynchRemoveContacts() {return plug->DTgetInt(DTCFG , 0 , CFG::synchRemoveContacts);}
		int CFGsynchAddContacts() {return plug->DTgetInt(DTCFG , 0 , CFG::synchAddContacts);}
		int CFGautoSubscribe() {return plug->DTgetInt(DTCFG , 0 , CFG::autoSubscribe);}
		int CFGautoAuthorize() {return plug->DTgetInt(DTCFG , 0 , CFG::autoAuthorize);}


		/* Interfejs ni¿szego poziomu */

		const CStdString GetUID(); 
		const CStdString GetHost(bool useReal) {
			if (this->useHost.empty()) {
				if (useReal == false || this->realHost.empty()) {
					return plug->DTgetStr(DTCFG , 0 , plug->DTgetNameID(DTCFG , dtPrefix + "/host") );
				} else {
					return this->realHost;
				}
			} else {
				return this->useHost;
			}
		}
		int GetPort(bool useReal) {
			if (useReal == true && this->realPort) {
				return this->realPort;
			} else {
				return plug->DTgetInt(DTCFG , 0 , plug->DTgetNameID(DTCFG , dtPrefix + "/port") );
			}
		}
		SOCKET GetSocket() {return socket;}
		jabberoo::Session & GetSession() {return session;}
		const CStdString & GetName();

		int ACTcfgGroup() {return kJabber::ACT::CfgGroup + this->GetSlot();}

		unsigned int ListenThread(void);
		bool Connected() {
			return socket && thread && session.getConnState() == jabberoo::Session::csConnected;
		}
		void DisconnectSocket();
		void SetContact(int cntID , Roster::Item & item);
		void SetContact(int cntID , void * item) {SetContact(cntID , *((Roster::Item*) item) );}

		void SetRosterItem(int cntID);
		void SetCntIcon(int cntID , int status , const CStdString & jid);
		int  GetCntIcon(int status , const CStdString & jid);
		bool SendPresence(const CStdString & to , int status , const char * info);
		bool SendSubRequest(const CStdString & to , bool subscribe);
		bool SendSubscribed(const CStdString & to , bool subscribed);
		bool SendCntSearch(sCNTSEARCH * sr);
		bool SendVCardRequest(int cntID);
		bool SendVCardSet(bool window);
		bool SendMessage(cMessage * m);
		bool RosterDeleteJID(const CStdString & jid);
		bool RosterContainsJID(const CStdString & JID);
		bool SendPing();
		void CntSearch(sCNTSEARCH * sr);


		void vCardSetEmail(judo::Element * vCard , CStdString email , const char * location);
		void vCardSetPhone(judo::Element * vCard , CStdString phone , const char * type , const char * location);
		judo::Element * vCardSetAddress(judo::Element * vCard , bool window , const CStdString & path);
		void vCardGetValue(const judo::Element * vCard , bool window , int cnt , int col , const char * path);

// Eventy

		void OnTransmitXML(const char* c);
		void OnAuthError(int errorcode , const char* errormessage);
		void OnDisconnected();
		void OnConnected(const judo::Element& e);
		void OnRoster(const judo::Element& e);
		void OnMessage(const jabberoo::Message & m);
		void OnPresence(const Presence& presence, Presence::Type previousType);
		void OnPresenceRequest(const Presence & presence);
		void OnVersion(string & name , string & version , string & os);
		void OnLast(string & seconds);
		void OnTime(string & localTime, string & timeZone);
		void OnRosterItemRemoving(Roster::Item &);
		void OnRosterItemUpdated(Roster::Item &);
		void OnRosterItemAdded(Roster::Item &);

		void OnIQSearchReply(const judo::Element&);
		void OnIQVCardRequestReply(const judo::Element&);
		void OnIQVCardSetReply(const judo::Element&);

		void OnCantConnect(bool automated);

/* DODATKI */
		void ServiceDiscovery(const CStdString jid);
		DiscoDB::Item * GetDiscoItem(const CStdString &jid, bool items, bool fetch);

	private:
		jabberoo::Session session;
		SOCKET socket;
		SSL * ssl;
		SSL_CTX * ssl_ctx;
		CStdString priority;
		bool canReconnect;
		bool createAccount;
		CStdString realHost;
		unsigned short realPort;
	};


}