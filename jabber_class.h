/*

Deklaracja podstawowego interfejsu, ³¹cz¹cego ogóln¹ ideê dzia³ania protoko³u Jabber z programem
Konnekt. Powinna byæ dobrym punktem startowym do ³¹czenia dowolnej biblioteki obs³uguj¹cej
protokó³ Jabbero-podobny z Konnektem.
Kod zosta³ wziêty z ma³ymi modyfikacjami z wtyczki kJabber.

Ustawienia przechowywane s¹ w tablicy z identyfikatorami tekstowymi (name) w formacie:
this->dtPrefix + "/" + nazwa_zmiennej
Standardowo ustawiane i u¿ywane s¹ tylko zmienne user , pass i status

Klasa musi byæ zbudowana tak, ¿eby by³a kompatybilna przynajmniej z dwiema aktualnie wykorzystuj¹cymi
j¹ wtyczkami - kJabber i DwuTlenek. Dodatkowo bez wiêkszych zmian , powinno daæ
siê u¿yæ dwóch ró¿nych po³¹czeñ równolegle (Gdy Konnekt bêdzie to ju¿ supportowa³).


	* License information can be found in license.txt.
    * Please redistribute only as a whole package.


(C) 2004 Rafa³ Lindemann - www.STAMINA.eu.org - www.KONNEKT.info

*/

#pragma once
#ifndef __JABBER_CLASS__H__
#define __JABBER_CLASS__H__

namespace kJabber {
	enum enSynchDirection {
		sd_disabled = 0 ,
		sd_toServer = 1 ,
		sd_toClient = 2
	};
	enum enConnectionState {
		cs_offline , 
		cs_connecting , 
		cs_online ,
		cs_disconnecting
	};
	const int IMsetTimer = 12010; // id komunikatu, po którym powinien zostaæ ustawiony timer od pingowania...
	const int pingInterval = 30000; // ping co pó³ minuty (max. opóŸnienie - pingInterval)
	class cJabberBase {
	public:
		cJabberBase(cCtrl * ctrl); // inicjalizacja
		virtual ~cJabberBase();

		/* Podstawowe funkcje obs³uguj¹ce "logikê" protoko³u Jabber 
		   Teoretycznie, dla wszystkich jabbero-podobnych protoko³ów nie trzeba
		   modyfikowaæ ich dzia³ania (z wyj¹tkiem StartSSL, którego nie ma tu z czystego lenistwa ;))
		
		*/

		enConnectionState GetState() {return state;}
		char GetSlotID() {return slotID;}
		unsigned int GetSlot() {return slotID - '0';}
		unsigned int GetNet() {return net;}
		cCtrl * GetPlug() {return plug;}

		virtual void Lock();
		virtual void Unlock();

		/** Wrapper dla CreateThread */
		static unsigned int __stdcall ListenThread_(cJabberBase * jab) {
			return jab->ListenThread();
		}
		/** Czy mo¿e nawi¹zaæ po³¹czenie z sieci¹ */
		virtual bool CanConnect() {return !this->GetHost(false).empty();}
		/** Nawi¹zywanie po³¹czenia z sieci¹ */
		virtual void Connect(bool automated=false);
		/** W³¹czenie obs³ugi SSL */
		virtual void StartSSL();
		/** W³¹czenie obs³ugi Proxy */
		virtual bool StartProxy();
		/** Roz³¹czenie */
		virtual void Disconnect();
		/** Stan po³¹czenia */
		virtual bool Connected() = 0;
		/** Czy lista jest ju¿ zsynchronizowana? */
		virtual bool IsSynchronized() {
			return synchronized;
		}
		/** Ustawia status dla sesji */
		virtual void SetStatus(int status , bool set , const char * info = 0);
		/** Wyszukiwanie kontaktu z podanym JIDem */
		virtual int FindContact(const CStdString & jid);
		/** Porównywanie JIDów */
		virtual int CompareJID(const CStdString & a, const CStdString & b) {
			return stricmp(a , b);
		}

		void IgnoreMessage(cMessage * m , int cnt);

		/* Obs³uga komunikatów Konnekta 
		   Przynajmniej czêœciowo powinno siê pokrywaæ...
		*/

		/** Obs³uga komunikatów API konnekta */
		virtual int IMessageProc(sIMessage_base * msgBase);
		/** Obs³uga komunikatów dla akcji konfiguracji */
		virtual int ActionCfgProc(sUIActionNotify_base * anBase) {return 0;}
		/** Obs³uga komunikatów dla pozosta³ych akcji */
		virtual int ActionProc(sUIActionNotify_base * anBase);

		/** Przygotowanie interfejsu */
		virtual void UIPrepare();
		/** Rejestrowanie kolumn */
		virtual void SetColumns();
		/** Inicjacja wyszukiwania kontaktu */
		virtual void CntSearch(sCNTSEARCH * sr);
		/** Inicjacja pobierania informacji o kontakcie */
		virtual bool VCardRequest(int cntID);
		/** Inicjacja zapisywania informacji o u¿ytkowniku */
		virtual bool VCardSet();

	/* W odpowiedzi na okreœlone zdarzenia, trzeba wywo³aæ odpowiednie funkcje
	   zdarzeñ...

	   Klasa protoko³u musi w pe³ni sama zaj¹æ siê:
	    - przyjmowaniem wiadomoœci i wstawianiem ich do kolejki...
		- obs³ug¹ zmian stanów kontaktów i informoaniem o autoryzacji
		- obs³ug¹ znalezionych w katalogu kontaktów
	*/

		void OnConnected();
		void OnRosterAnswer();
		void OnRosterItemRemoving(const CStdString & jid);
		void OnRosterItemUpdated(const CStdString & jid , void * item);
		void OnRosterItemAdded(const CStdString & jid , void * item);
		/** W momencie otrzymania proœby o autoryzacjê (request==true), lub jej odmówienia */
		void OnPresenceRequest(const CStdString & jid , bool request);
		
		void OnSearchAnswer();
		void OnVCardRequestAnswer();
		void OnVCardSetAnswer();

		virtual void OnCantConnect(bool automated) {}

	/* Ka¿da jabberowa klasa musi tutaj okreœliæ swoje zachowanie */

	/* Interfejs ni¿szego poziomu */

	/* Pobieranie zmiennych i ustawieñ */
		virtual const CStdString GetHost(bool useReal) = 0;
		virtual int GetPort(bool useReal) = 0;
		virtual const CStdString GetUID() = 0;
		virtual void GetUserAndPass(CStdString & user , CStdString & pass);
		virtual enSynchDirection CFGsynchDirection() = 0;
		virtual int CFGsynchAddContacts() = 0;
		virtual int CFGsynchRemoveContacts() = 0;
		virtual int CFGautoSubscribe() = 0;
		virtual int CFGautoAuthorize() = 0;
		virtual SOCKET GetSocket() {return 0;}
		virtual const CStdString & GetName() {static CStdString name="Jabber"; return name;}

		virtual int ACTcfgGroup() = 0;

	/* Wykonywanie konkretnych operacji */

		/** G³ówny w¹tek do nawi¹zywania/obs³ugi po³¹czenia */
		virtual unsigned int ListenThread(void) = 0;
		/** Wysy³a nowy status w³asny... */
		virtual bool SendPresence(const CStdString & to , int status , const char * info) = 0;
		virtual bool SendSubRequest(const CStdString & to , bool subscribe) = 0;
		virtual bool SendSubscribed(const CStdString & to , bool subscribed) = 0;
		virtual bool SendCntSearch(sCNTSEARCH * sr) = 0;
		virtual bool SendVCardRequest(int cntID) = 0;
		virtual bool SendVCardSet(bool window) = 0;
		virtual bool SendMessage(cMessage * m) = 0;
		virtual bool RosterDeleteJID(const CStdString & jid) = 0;
		virtual bool RosterContainsJID(const CStdString & jid) = 0;

		static void CALLBACK PingTimerRoutine(cJabberBase * jab, DWORD l,  DWORD h);
		virtual bool SendPing() = 0;

		/** Roz³¹cza po³¹czenie sieciowe */
		virtual void DisconnectSocket() = 0;

		/** Aktualizuje kontakt w rosterze */
		virtual void SetRosterItem(int cntID) = 0;
		/** Aktualizuje kontakt z rostera. WskaŸnik do item przekazywany jest ze zdarzeñ */
		virtual void SetContact(int cntID , void * item) = 0;
		/** Ustala (w razie potrzeby) specjaln¹ ikonê statusu kontaktu */
		virtual void SetCntIcon(int cntID , int status , const CStdString & jid) = 0;


	protected:
		cCtrl * plug;
		char slotID; ///< Identyfikator "slotu". W tej chwili mo¿e istnieæ i tak tylko jeden slot...
		int net; ///< Wartoœæ net wtyczki...
		CStdString dtPrefix; ///< Prefix nazw kolumn w bazie
		HANDLE thread;
		int currentStatus;
		CStdString currentStatusInfo;
		bool inAutoAway;
		bool synchronized;
		// aktualnie usuwany kontakt (ID). -1 - ¿aden kontakt nie jest usuwany
		int removingCnt;
		CRITICAL_SECTION cs;
		sDIALOG_long * dlgSearch , * dlgVCardReq , * dlgVCardSet;
		enConnectionState state;
		HANDLE timerPing;
		__time64_t lastPing;
		/** U¿ywa poni¿szego loginu/has³a zamiast tych z konfiguracji */
		CStdString useUser, usePass, useHost;

		/// Zapisuje login/has³o po udanym ³¹czeniu...
		enum { 
			supsNo, supsTable=1, supsConfig=2, supsTableAndConfig=3,
			supsKeep=4
		} storeUPonSuccess; 

	/* identyfikatory kontrolek */

	};

	class cExceptionQuiet {
	private:
		const cJabberBase * jab;
		CStdString where;
		CStdString msg;
	public:
		cExceptionQuiet(const cJabberBase * jab , const char * where = "" , const char * msg = "") {
			this->msg = msg;
			this->jab = jab;
			this->where = where;
		}
		const CStdString GetMsg() {return msg;}
		const CStdString GetWhere() {return where;}
		const cJabberBase * GetJab() {return jab;}
	};
	class cException: public cExceptionQuiet {
	public:
		cException(const cJabberBase * jab , const char * where , const char * msg):cExceptionQuiet(jab , where , msg) {
		}
	};

};

#endif