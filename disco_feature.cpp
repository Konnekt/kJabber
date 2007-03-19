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
using namespace JEP;
cFeatureXData::cFeatureXData(cJabber & jab, HWND hwnd, const CStdString & jid,  const CStdString & title)
:xData(jab, title, hwnd),jab(jab) {
	this->jid = jid;
}
cFeatureXData::~cFeatureXData() {
	if (!id.empty())
		jab.GetSession().unregisterIQ(id);
}
void cFeatureXData::Create() {
	/* Pierwszy pakiet do wys³ania... */
	xData.evtCommand.connect(SigC::slot(*this, &cFeatureXData::OnCommand));
	Submit(false);
	/* Dalej ¿yje ju¿ w³asnym ¿yciem... */
}

void cFeatureXData::OnCommand(int command) {
	if (command == IDCANCEL) { 
		if (xData.GetType() == cXData::tForm) 
			Submit(true);
		delete this;
	} else if (command == IDOK) {
		if (xData.GetType() == cXData::tForm || xData.GetType() == cXData::tObsolete)
			Submit(false);
		else
			delete this;
	}
}


judo::Element * cFeatureXData::BuildIQ(const CStdString & type) {
	judo::Element * iq = new judo::Element ("iq");
	iq->putAttrib("type", type);
	iq->putAttrib("from", jab.GetSession().getLocalJID());
	iq->putAttrib("to", jid);
	this->id = jab.GetSession().getNextID();
	iq->putAttrib("id", id);
	return iq;
}
judo::Element * cFeatureXData::BuildX(judo::Element & parent, bool cancel) {
	judo::Element * x = parent.addElement("x");
	x->putAttrib("xmlns", "jabber:x:data");
	if (cancel) {
		x->putAttrib("type", "cancel");
	} else {
		x->putAttrib("type", "submit");
	}
	return x;
}
bool cFeatureXData::IsError(const judo::Element & e) {
	const judo::Element * error = e.findElement("error");
	if (error) {
		CStdString msg = UTF8ToLocale(error->getCDATA());
		if (msg.empty() && error->size())
			msg = UTF8ToLocale((*error->begin())->getName());
		xData.ProcessError("Kod: " + error->getAttrib("code") + "\r\n" + (msg.empty() ? "Nieznany b³¹d" : msg), xData.IsForm() == false);
		return true;
	}
	return false;
}

void CALLBACK cFeatureXData::SafeOnIQAPC(tSafeOnIQ * soi) {
	soi->f(*soi->e);
	soi->processed = true;
}
bool cFeatureXData::SafeOnIQ(jabberoo::ElementCallbackFunc f , const judo::Element & e) {
	if (jab.GetPlug()->Is_TUS(0)) {
		tSafeOnIQ soi;
		soi.e = &e;
		soi.f = f;
		soi.processed = false;
		HANDLE thread = (HANDLE)jab.GetPlug()->ICMessage(IMC_GET_MAINTHREAD);
		if (!thread)
			return true;
		QueueUserAPC((PAPCFUNC) SafeOnIQAPC, thread, (ULONG_PTR)&soi);
		while (!soi.processed) {
			jab.GetPlug()->WMProcess();
		}
		return false;
	} else
		return true;
}


// ---------------------------------------------------------------------------

cFeatureNeg::cFeatureNeg(cJabber & jab, HWND hwnd, const CStdString & jid, const CStdString & title, const CStdString & feature)
:cFeatureXData(jab, hwnd, jid, title) {
	this->feature = feature;
	Create();
}
void cFeatureNeg::Submit(bool cancel) {
	xData.ProcessInfo("Czekam na informacje od serwera");
	xData.Enable(false);
	judo::Element * iq = BuildIQ();
	judo::Element * feat = iq->addElement("feature");
	feat->putAttrib("xmlns", "http://jabber.org/protocol/feature-neg");
	judo::Element * x = BuildX(*feat, cancel);
	if (!cancel) {
		/* Skoro nie mamy formularza, pobieramy ficzer */
		if (xData.GetType() != cXData::tForm) {
			judo::Element * field = x->addElement("field");
			field->putAttrib("var", feature);
		} else {
			/* Wstawiamy wynik formularza... */
			xData.InjectResult(*feat);
		}
		jab.GetSession().registerIQ(id, SigC::slot(*this, &cFeatureNeg::OnIQ));
	}
	jab.GetSession() << jabberoo::Packet(*iq);
}

void cFeatureNeg::OnIQ(const judo::Element & e) {
	if (!SafeOnIQ(SigC::slot(*this, &cFeatureNeg::OnIQ), e))
		return;
	this->id = ""; // odebrany - usuniêty... mo¿na o nim zapomnieæ...
	if (IsError(e))
		return;
	const judo::Element * feat = e.findElement("feature");
	if (!feat) {
		xData.ProcessError("Z³a odpowiedŸ!");
		return;
	}
	xData.Process(*feat, false);
	if (xData.GetState() == cXData::tError) {
		xData.ProcessError("Problemy z przetwarzaniem danych...");
	}
}


// ---------------------------------------------------------------------------


cFeatureIQRegister::cFeatureIQRegister(cJabber & jab, HWND hwnd, const CStdString & jid, const CStdString & title)
:cFeatureXData(jab, hwnd, jid, title) {
	Create();
}
void cFeatureIQRegister::Submit(bool cancel) {
	xData.SetButtonSpecial("");
	xData.ProcessInfo("Czekam na informacje od serwera");
	xData.Enable(false);
	judo::Element * iq = BuildIQ(xData.IsForm() ? "set" : "get");
	judo::Element * query = iq->addElement("query");
	query->putAttrib("xmlns", "jabber:iq:register");

	if (xData.IsForm()) {
		xData.InjectResult(*query);
	} else {
		// pierwsza wysy³ka - wystarczy sam get
	}
	if (!cancel)
		jab.GetSession().registerIQ(id, SigC::slot(*this, &cFeatureIQRegister::OnIQ));
	jab.GetSession() << jabberoo::Packet(*iq);
}

void cFeatureIQRegister::OnIQ(const judo::Element & e) {
	if (!SafeOnIQ(SigC::slot(*this, &cFeatureIQRegister::OnIQ), e))
		return;
	this->id = ""; // odebrany - usuniêty... mo¿na o nim zapomnieæ...
	if (IsError(e))
		return;
	const judo::Element * query = e.findElement("query");
	if (!query || query->size() == 0) { // Potwierdzenie...
		xData.ProcessInfo("Operacja siê powiod³a", true);
		return;
	}
	if (query->findElement("registered"))
		xData.SetButtonSpecial("Wyrejestruj");
	else
		xData.SetButtonSpecial("");
	xData.Process(*query, false);
	if (xData.GetState() == cXData::stError) {
		xData.ProcessError("Problemy z przetwarzaniem danych...");
	}
}

void cFeatureIQRegister::OnCommand(int command) {
	if (command == IDB_SPECIAL) { // unregister
		if (! jab.GetPlug()->IMessage(&sIMessage_msgBox(
			IMI_CONFIRM, "Na pewno chcesz siê wyrejestrowaæ z " + this->jid + "?", "kJabber"
			))) {
			return;
		}
		if (this->jid == jab.GetHost(false) && jab.GetPlug()->IMessage(&sIMessage_msgBox(
			IMI_CONFIRM, "Próbujesz wyrejestrowaæ swoje konto na serwerze " + jab.GetHost(false) + ".\r\nZostanie usuniêta z serwera Twoja lista kontaktów i byæ mo¿e ktoœ przejmie Twój identyfikator (" + jab.GetUID() + ")\r\n\r\nCzy na pewno chcesz kontynuowaæ?", "kJabber", MB_ICONWARNING | MB_YESNO
			)) == IDNO) {
			return;
		}
		xData.SetButtonSpecial("");
		xData.ProcessInfo("Wyrejestrowywujê..." , true);
		judo::Element * iq = BuildIQ("set");
		judo::Element * query = iq->addElement("query");
		query->putAttrib("xmlns", "jabber:iq:register");
		query->addElement("remove");
		jab.GetSession().registerIQ(id, SigC::slot(*this, &cFeatureIQRegister::OnIQ));
		jab.GetSession() << jabberoo::Packet(*iq);
		return;
	}
	__super::OnCommand(command);
}



// ---------------------------------------------------------------------------


cFeatureIQSearch::cFeatureIQSearch(cJabber & jab, HWND hwnd, const CStdString & jid, const CStdString & title)
:cFeatureXData(jab, hwnd, jid, title) {
	Create();
}
void cFeatureIQSearch::Submit(bool cancel) {
	xData.SetButtonSpecial("");
	xData.ProcessInfo("Czekam na informacje od serwera");
	xData.Enable(false);
	judo::Element * iq = BuildIQ(xData.IsForm() ? "set" : "get");
	judo::Element * query = iq->addElement("query");
	query->putAttrib("xmlns", "jabber:iq:search");

	if (xData.IsForm()) {
		xData.InjectResult(*query);
	} else {
		// pierwsza wysy³ka - wystarczy sam get
	}
	if (!cancel)
		jab.GetSession().registerIQ(id, SigC::slot(*this, &cFeatureIQSearch::OnIQ));
	jab.GetSession() << jabberoo::Packet(*iq);
}

void cFeatureIQSearch::OnIQ(const judo::Element & e) {
	if (!SafeOnIQ(SigC::slot(*this, &cFeatureIQSearch::OnIQ), e))
		return;
	this->id = ""; // odebrany - usuniêty... mo¿na o nim zapomnieæ...
	if (IsError(e))
		return;
	const judo::Element * query = e.findElement("query");
	if (!query) { // Potwierdzenie...
		xData.ProcessError("Z³a odpowiedŸ!");
		return;
	}
	xData.Process(*query, false);
	//xData.SetButtonSpecial(xData.IsForm() ? "Do wyszukiwarki" : "");

	if (xData.GetState() == cXData::stError) {
		xData.ProcessError("Problemy z przetwarzaniem danych...");
	}
}

void cFeatureIQSearch::OnCommand(int command) {
	if (command == IDB_SPECIAL) { // do wyszukiwarki...
		/*TODO: Wysylanie wynikow do wyszukiwarki...*/
		xData.SetButtonSpecial("");
		return;
	}
	__super::OnCommand(command);
}
