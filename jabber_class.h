/*

Deklaracja podstawowego interfejsu, ��cz�cego og�ln� ide� dzia�ania protoko�u Jabber z programem
Konnekt. Powinna by� dobrym punktem startowym do ��czenia dowolnej biblioteki obs�uguj�cej
protok� Jabbero-podobny z Konnektem.
Kod zosta� wzi�ty z ma�ymi modyfikacjami z wtyczki kJabber.

Ustawienia przechowywane s� w tablicy z identyfikatorami tekstowymi (name) w formacie:
this->dtPrefix + "/" + nazwa_zmiennej
Standardowo ustawiane i u�ywane s� tylko zmienne user , pass i status

Klasa musi by� zbudowana tak, �eby by�a kompatybilna przynajmniej z dwiema aktualnie wykorzystuj�cymi
j� wtyczkami - kJabber i DwuTlenek. Dodatkowo bez wi�kszych zmian , powinno da�
si� u�y� dw�ch r�nych po��cze� r�wnolegle (Gdy Konnekt b�dzie to ju� supportowa�).


	* License information can be found in license.txt.
    * Please redistribute only as a whole package.


(C) 2004 Rafa� Lindemann - www.STAMINA.eu.org - www.KONNEKT.info

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
	const int IMsetTimer = 12010; // id komunikatu, po kt�rym powinien zosta� ustawiony timer od pingowania...
	const int pingInterval = 30000; // ping co p� minuty (max. op�nienie - pingInterval)
	class cJabberBase {
	public:
		cJabberBase(cCtrl * ctrl); // inicjalizacja
		virtual ~cJabberBase();

		/* Podstawowe funkcje obs�uguj�ce "logik�" protoko�u Jabber 
		   Teoretycznie, dla wszystkich jabbero-podobnych protoko��w nie trzeba
		   modyfikowa� ich dzia�ania (z wyj�tkiem StartSSL, kt�rego nie ma tu z czystego lenistwa ;))
		
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
		/** Czy mo�e nawi�za� po��czenie z sieci� */
		virtual bool CanConnect() {return !this->GetHost(false).empty();}
		/** Nawi�zywanie po��czenia z sieci� */
		virtual void Connect(bool automated=false);
		/** W��czenie obs�ugi SSL */
		virtual void StartSSL();
		/** W��czenie obs�ugi Proxy */
		virtual bool StartProxy();
		/** Roz��czenie */
		virtual void Disconnect();
		/** Stan po��czenia */
		virtual bool Connected() = 0;
		/** Czy lista jest ju� zsynchronizowana? */
		virtual bool IsSynchronized() {
			return synchronized;
		}
		/** Ustawia status dla sesji */
		virtual void SetStatus(int status , bool set , const char * info = 0);
		/** Wyszukiwanie kontaktu z podanym JIDem */
		virtual int FindContact(const CStdString & jid);
		/** Por�wnywanie JID�w */
		virtual int CompareJID(const CStdString & a, const CStdString & b) {
			return stricmp(a , b);
		}

		void IgnoreMessage(cMessage * m , int cnt);

		/* Obs�uga komunikat�w Konnekta 
		   Przynajmniej cz�ciowo powinno si� pokrywa�...
		*/

		/** Obs�uga komunikat�w API konnekta */
		virtual int IMessageProc(sIMessage_base * msgBase);
		/** Obs�uga komunikat�w dla akcji konfiguracji */
		virtual int ActionCfgProc(sUIActionNotify_base * anBase) {return 0;}
		/** Obs�uga komunikat�w dla pozosta�ych akcji */
		virtual int ActionProc(sUIActionNotify_base * anBase);

		/** Przygotowanie interfejsu */
		virtual void UIPrepare();
		/** Rejestrowanie kolumn */
		virtual void SetColumns();
		/** Inicjacja wyszukiwania kontaktu */
		virtual void CntSearch(sCNTSEARCH * sr);
		/** Inicjacja pobierania informacji o kontakcie */
		virtual bool VCardRequest(int cntID);
		/** Inicjacja zapisywania informacji o u�ytkowniku */
		virtual bool VCardSet();

	/* W odpowiedzi na okre�lone zdarzenia, trzeba wywo�a� odpowiednie funkcje
	   zdarze�...

	   Klasa protoko�u musi w pe�ni sama zaj�� si�:
	    - przyjmowaniem wiadomo�ci i wstawianiem ich do kolejki...
		- obs�ug� zmian stan�w kontakt�w i informoaniem o autoryzacji
		- obs�ug� znalezionych w katalogu kontakt�w
	*/

		void OnConnected();
		void OnRosterAnswer();
		void OnRosterItemRemoving(const CStdString & jid);
		void OnRosterItemUpdated(const CStdString & jid , void * item);
		void OnRosterItemAdded(const CStdString & jid , void * item);
		/** W momencie otrzymania pro�by o autoryzacj� (request==true), lub jej odm�wienia */
		void OnPresenceRequest(const CStdString & jid , bool request);
		
		void OnSearchAnswer();
		void OnVCardRequestAnswer();
		void OnVCardSetAnswer();

		virtual void OnCantConnect(bool automated) {}

	/* Ka�da jabberowa klasa musi tutaj okre�li� swoje zachowanie */

	/* Interfejs ni�szego poziomu */

	/* Pobieranie zmiennych i ustawie� */
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

		/** G��wny w�tek do nawi�zywania/obs�ugi po��czenia */
		virtual unsigned int ListenThread(void) = 0;
		/** Wysy�a nowy status w�asny... */
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

		/** Roz��cza po��czenie sieciowe */
		virtual void DisconnectSocket() = 0;

		/** Aktualizuje kontakt w rosterze */
		virtual void SetRosterItem(int cntID) = 0;
		/** Aktualizuje kontakt z rostera. Wska�nik do item przekazywany jest ze zdarze� */
		virtual void SetContact(int cntID , void * item) = 0;
		/** Ustala (w razie potrzeby) specjaln� ikon� statusu kontaktu */
		virtual void SetCntIcon(int cntID , int status , const CStdString & jid) = 0;


	protected:
		cCtrl * plug;
		char slotID; ///< Identyfikator "slotu". W tej chwili mo�e istnie� i tak tylko jeden slot...
		int net; ///< Warto�� net wtyczki...
		CStdString dtPrefix; ///< Prefix nazw kolumn w bazie
		HANDLE thread;
		int currentStatus;
		CStdString currentStatusInfo;
		bool inAutoAway;
		bool synchronized;
		// aktualnie usuwany kontakt (ID). -1 - �aden kontakt nie jest usuwany
		int removingCnt;
		CRITICAL_SECTION cs;
		sDIALOG_long * dlgSearch , * dlgVCardReq , * dlgVCardSet;
		enConnectionState state;
		HANDLE timerPing;
		__time64_t lastPing;
		/** U�ywa poni�szego loginu/has�a zamiast tych z konfiguracji */
		CStdString useUser, usePass, useHost;

		/// Zapisuje login/has�o po udanym ��czeniu...
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