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
#include "xData.h"
using namespace JEP;


// -------------------------------------------------------------------
// ITEM


void cXData::cItem::Create(cXData * owner , const judo::Element & e) {
	if (var.empty())
		this->var = e.getAttrib("var");
	owner->push_back(this);
}
void cXData::cItem::Destroy(cXData * owner) {
/*
	cXData::iterator found = std::find(owner->begin(), owner->end(), this);
	if (found != owner->end())
		owner->erase(found);
*/
	delete this;
}
judo::Element * cXData::cItem::Build (bool addValue) const {
	judo::Element * e = new judo::Element("field");
	e->putAttrib("var", this->var);
	e->putAttrib("type", this->GetType());
	if (addValue) {
		CStdString value = kJabber::LocaleToUTF8(this->GetValue());
		/* wycinamy \r */
		value.Replace("\\r", "");
		tStringVector list;
		split(value, "\\n", list);
		for (tStringVector::iterator it = list.begin(); it != list.end(); ++it)
			e->addElement("value", *it);
	}
	return e;
}


CStdString cXData::cItem::GetValueFromXML (const judo::Element & e, char * multiline) const {
	if (multiline) {
		CStdString value;
		judo::Element::const_iterator it;
		for (it = e.begin(); it != e.end(); ++it) 
			if ((*it)->getType() == judo::Node::ntElement && (*it)->getName() == "value") {
				if (!value.empty())
					value += multiline;
				value += static_cast<const judo::Element *>(*it)->getCDATA();
			}
		return UTF8ToLocale(value);
	} else
		return  UTF8ToLocale(e.getChildCData("value"));
}

// ---------------------------------------------------
void cXData::cItem_hwnd::Create(cXData * owner, const judo::Element & e) {
	__super::Create(owner, e);
}
void cXData::cItem_hwnd::Destroy(cXData * owner) {
	DestroyWindow(hwnd);
	__super::Destroy(owner);
}
void cXData::cItem_hwnd::SetValue(const CStdString & value) {
	SetWindowText(hwnd, value);
}
CStdString cXData::cItem_hwnd::GetValue () const {
	CStdString value;
	int size = GetWindowTextLength(hwnd);
	GetWindowText(hwnd, value.GetBuffer(size + 1), size + 1);
	value.ReleaseBuffer();
	return value;
}
int cXData::cItem_hwnd::GetX(const cXData * owner) {
	return owner->GetX();
}
int cXData::cItem_hwnd::GetY(const cXData * owner, int offset) {
/*	int old = owner->GetY();
	if (offset) 
		owner->OffsetY(offset);
	return old;*/
	return owner->GetY();
}
int cXData::cItem_hwnd::GetWidth(const cXData * owner) {
	return owner->GetWidth();
}
int cXData::cItem_hwnd::GetHeight(const cXData * owner) {
	return minLineHeight;
}
void cXData::cItem_hwnd::SetFont(HWND hwnd) {
//	IMLOG("SetFont hwnd %x getfont %x" , GetParent(hwnd), SendMessage(GetParent(hwnd), WM_GETFONT, 0, 0));
	SendMessage(hwnd, WM_SETFONT, (int)font, TRUE);
}
HFONT cXData::cItem_hwnd::font = 0;
int cXData::cItem_hwnd::minLineHeight = 0;

//---------------------------------------------------------------------

void cXData::cItem_fixed::Create(cXData * owner, const judo::Element & e) {
	__super::Create(owner, e);
	CStdString text = GetValueFromXML(e);
	// ustalamy wysokoœæ...
	RECT rc = {0, 0, GetWidth(owner), 0};
	owner->FitText(0, text, rc);
	this->height = rc.bottom + margin;
	hwnd = CreateWindow("STATIC", text, WS_VISIBLE | WS_CHILD, GetX(owner), GetY(owner), GetWidth(owner), GetHeight(owner), owner->GetContainer(), 0, 0, 0);
	SetFont(hwnd);
}

//---------------------------------------------------------------------

void cXData::cItem_boolean::Create(cXData * owner, const judo::Element & e) {
	__super::Create(owner, e);
	hwnd = CreateWindow("BUTTON", GetLabelFromXML(e), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_LEFTTEXT | WS_TABSTOP, GetX(owner), GetY(owner), GetWidth(owner), GetHeight(owner), owner->GetContainer(), 0, 0, 0);
	SetValue(GetValueFromXML(e));
	SetFont(hwnd);
}

void cXData::cItem_boolean::SetValue(const CStdString & value) {
	SendMessage(this->hwnd, BM_SETCHECK, value == "1" ? BST_CHECKED : BST_UNCHECKED, 0);
}

CStdString cXData::cItem_boolean::GetValue () const {
	return (SendMessage(this->hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED) ? "1" : "0";
}

// -------------------------------------------------------------------

void cXData::cItem_labeled::Create(cXData * owner, const judo::Element & e) {
	__super::Create(owner, e);
	CStdString text = GetLabelFromXML(e);
	// ustalamy wysokoœæ...
	RECT rc = {0, 0, GetWidth(owner), 0};
	owner->FitText(0, text, rc);
	this->labelHeight = rc.bottom;
	label = CreateWindow("STATIC", text, WS_VISIBLE | WS_CHILD | SS_RIGHT, __super::GetX(owner), GetY(owner) + 1, GetWidth(owner), GetHeight(owner), owner->GetContainer(), 0, 0, 0);
	SetFont(label);
}
void cXData::cItem_labeled::Destroy(cXData * owner) {
	DestroyWindow(label);
	__super::Destroy(owner);
}
int cXData::cItem_labeled::GetX(const cXData * owner) {
	return __super::GetX(owner) + (__super::GetWidth(owner) / 2) + 5;
}
int cXData::cItem_labeled::GetWidth(const cXData * owner) {
	return (__super::GetWidth(owner) - 5) / 2;
}
int cXData::cItem_labeled::GetHeight(const cXData * owner) {
	return max(labelHeight, __super::GetHeight(owner));
}

// -------------------------------------------------------------------
void cXData::cItem_text_single::Create(cXData * owner, const judo::Element & e) {
	__super::Create(owner, e);
	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", GetValueFromXML(e), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, GetX(owner), GetY(owner), GetWidth(owner), GetHeight(owner) - margin, owner->GetContainer(), 0, 0, 0);
	SetFont(hwnd);
}

// -------------------------------------------------------------------
void cXData::cItem_text_private::Create(cXData * owner, const judo::Element & e) {
	__super::Create(owner, e);
	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", GetValueFromXML(e), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD | WS_TABSTOP, GetX(owner), GetY(owner), GetWidth(owner), GetHeight(owner) - margin, owner->GetContainer(), 0, 0, 0);
	SetFont(hwnd);
}

// -------------------------------------------------------------------
void cXData::cItem_text_multi::Create(cXData * owner, const judo::Element & e) {
	__super::Create(owner, e);
	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", GetValueFromXML(e, "\r\n"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOVSCROLL | ES_WANTRETURN | ES_MULTILINE, GetX(owner), GetY(owner), GetWidth(owner), GetHeight(owner) - margin, owner->GetContainer(), 0, 0, 0);
	SetFont(hwnd);
}
int cXData::cItem_text_multi::GetHeight(const cXData * owner) {
	return max(80, __super::GetHeight(owner));
}

// -------------------------------------------------------------------
void cXData::cItem_list_multi::Create(cXData * owner, const judo::Element & e, bool multi) {
	__super::Create(owner, e);
	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", GetValueFromXML(e), WS_VISIBLE | WS_CHILD | WS_TABSTOP 
		| LBS_NOINTEGRALHEIGHT | (multi ? (LBS_MULTIPLESEL | LBS_EXTENDEDSEL) : 0) , GetX(owner), GetY(owner), GetWidth(owner), GetHeight(owner) - margin, owner->GetContainer(), 0, 0, 0);
	SetFont(hwnd);
	judo::Element::const_iterator it;
	for (it = e.begin(); it != e.end(); ++it) {
		if ((*it)->getName() == "option") {
			values.push_back(UTF8ToLocale(static_cast<const judo::Element*>(*it)->getChildCData("value")));
			SendMessage(hwnd, LB_ADDSTRING, 0 , (LPARAM)UTF8ToLocale(static_cast<const judo::Element*>(*it)->getAttrib("label")).c_str());
		}
	}
	SetValue(GetValueFromXML(e, multi ? "\\n" : 0));
}
int cXData::cItem_list_multi::GetHeight(const cXData * owner) {
	return max(80, __super::GetHeight(owner)); // mo¿e label jest wiêkszy?
}
void cXData::cItem_list_multi::SetValue(const CStdString & value) {
	SendMessage(hwnd, LB_SETSEL, 0, -1);
	tStringVector list;
	split(value, "\\n", list);
	for (tStringVector::iterator item = list.begin(); item != list.end(); ++item) {
		tValues::iterator it = std::find(values.begin(), values.end(), *item);
		if (it != values.end())
			SendMessage(hwnd, LB_SETSEL, 1, it - values.begin());
	}
		
}
CStdString cXData::cItem_list_multi::GetValue () const {
	CStdString value;
	int c = SendMessage(hwnd, LB_GETCOUNT, 0, -1);
	for (int i=0; i < c; ++i) {
		if (SendMessage(hwnd, LB_GETSEL, i, 0)) {
			if (!value.empty())
				value += "\n";
			value += values[i];
		}
	}
	return value;
}

// -------------------------------------------------------------------
void cXData::cItem_list_single::Create(cXData * owner, const judo::Element & e) {
	__super::Create(owner, e);
	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "COMBOBOX", "", WS_VISIBLE | WS_CHILD | WS_TABSTOP 
		| CBS_DROPDOWNLIST , GetX(owner), GetY(owner), GetWidth(owner), 5*GetHeight(owner), owner->GetContainer(), 0, 0, 0);
	SetFont(hwnd);
	judo::Element::const_iterator it;
	for (it = e.begin(); it != e.end(); ++it) {
		if ((*it)->getName() == "option") {
			values.push_back(UTF8ToLocale(static_cast<const judo::Element*>(*it)->getChildCData("value")));
			SendMessage(hwnd, CB_ADDSTRING, 0 , (LPARAM)UTF8ToLocale(static_cast<const judo::Element*>(*it)->getAttrib("label")).c_str());
		}
	}
	SetValue(GetValueFromXML(e));
}
void cXData::cItem_list_single::SetValue(const CStdString & value) {
	tValues::iterator it = std::find(values.begin(), values.end(), value);
	if (it != values.end())
		SendMessage(hwnd, CB_SETCURSEL, it - values.begin(), 0);
	else
		SendMessage(hwnd, CB_SETCURSEL, -1, 0);
		
}
CStdString cXData::cItem_list_single::GetValue () const {
	int sel = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
	if (sel == CB_ERR) {
		return "";
	} else { 
		return values[sel];
	}
}

// -------------------------------------------------------------------
CStdString cXData::cItem_jid_single::Validate() {
	if (!ValidateJID(GetValue()))
		return "Nieprawid³owy JID: \"" + GetValue() + "\"\n";
	else return "";
}
bool cXData::cItem_jid_single::ValidateJID(CStdString JID) {
	CStdString user = JID::getUser(JID);
	if (!JID::isValidHost(JID::getHost(JID)))
		return false;
	if (!user.empty() && JID::isValidUser(user))
		return false;
	return true;
}

// -------------------------------------------------------------------
CStdString cXData::cItem_jid_multi::Validate() {
	CStdString info;
	tStringVector list;
	split(GetValue(), "\\r\\n", list, true);
	for (tStringVector::iterator it = list.begin(); it != list.end(); ++it) {
		if (!cItem_jid_single::ValidateJID(*it))
			info += "Nieprawid³owy JID: \"" + *it + "\"\n";
	}
	return info;
}

// -------------------------------------------------------------------
void cXData::cItem_result::Create(cXData * owner, const judo::Element & e) {
	jidColumn = -1;
	Rect rc = getChildRect(owner->GetContainer());
	
	// podstawiamy sie pod container...
	::ShowWindow(owner->GetContainer(), SW_HIDE);
	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW , "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_NOSORTHEADER | LVS_REPORT, 
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, GetParent(owner->GetContainer()), 0, 0, 0);
	SetProp(hwnd, "cItem_result", this);
	ListView_SetExtendedListViewStyle(hwnd, LVS_EX_GRIDLINES | LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);
	map <CStdString, int> header;
	// ladujemy reported
	const judo::Element * reported = e.findElement("reported");
	if (!reported) {
		owner->SetStatus("Z³a odpowiedŸ!");
		return;
	} // bug...
	for (judo::Element::const_iterator it = reported->begin(); it != reported->end(); ++it) {
		if ((*it)->getType() != judo::Node::ntElement || (*it)->getName() != "field")
			continue;
		const judo::Element * field = static_cast<const judo::Element *>(*it);
		int column = ListView_AddColumn(hwnd, UTF8ToLocale(field->getAttrib("label").empty() ? field->getAttrib("var") : field->getAttrib("label")) , 100);
		if (column >= 0)
			header[field->getAttrib("var")] = column;
		if (field->getAttrib("type") == "jid-single" && jidColumn == -1)
			jidColumn = column;
	}

	for (judo::Element::const_iterator _item = e.begin(); _item != e.end(); ++_item) {
		if ((*_item)->getType() != judo::Node::ntElement || (*_item)->getName() != "item")
			continue;
		const judo::Element * item = static_cast<const judo::Element *>(*_item);

		LVITEM lvi;
		lvi.mask = 0;
		lvi.iItem=0x7FFFFFFF;
		lvi.iSubItem=0;
		lvi.iItem = ListView_InsertItem(hwnd, &lvi);

		for (judo::Element::const_iterator _field = item->begin(); _field != item->end(); ++_field) {
			if ((*_field)->getType() != judo::Node::ntElement || (*_field)->getName() != "field")
				continue;
			const judo::Element * field = static_cast<const judo::Element *>(*_field);
			if (header.find(field->getAttrib("var")) == header.end())
				continue;
	        lvi.mask = LVIF_TEXT;
			CStdString value = GetValueFromXML(*field, " // ");
			lvi.pszText = (char*) value.c_str();
			lvi.iSubItem = header[field->getAttrib("var")];
		    if (lvi.iSubItem>=0) ListView_SetItem(hwnd, &lvi);
		}
	}

}
void cXData::cItem_result::Destroy(cXData * owner) {
	__super::Destroy(owner);
	// przywracamy container...
	::ShowWindow(owner->GetContainer(), SW_SHOW);
}
void cXData::cItem_result::OnDoubleClick(cXData * owner) {
	int pos = ListView_GetSelectionMark(hwnd);
	if (pos == -1 || this->jidColumn == -1)
		return;
	CStdString jid;
	ListView_GetItemText(hwnd, pos, this->jidColumn, jid.GetBuffer(200), 200);
	jid.ReleaseBuffer();
	if (owner->Jab().FindContact(jid) == -1) {
		int id = owner->Jab().GetPlug()->ICMessage(IMC_CNT_ADD, kJabber::net, (int)jid.c_str());
		if (id == -1)
			return;
		owner->Jab().GetPlug()->DTsetStr(DTCNT, id, CNT_GROUP, owner->Jab().GetPlug()->DTgetStr(DTCFG, 0, CFG_CURGROUP));
		owner->Jab().GetPlug()->ICMessage(IMC_CNT_CHANGED, id);
	}
}


// -------------------------------------------------------------------

void cXData::cItem_obsolete::Create(cXData * owner, const judo::Element & e) {
	this->var = e.getName();
}
judo::Element * cXData::cItem_obsolete::Build () const {
	judo::Element * e = new judo::Element(var);
	CStdString value = LocaleToUTF8(GetValue());
	e->addCDATA(value, value.size());
    return e;	
}
CStdString cXData::cItem_obsolete::GetValueFromXML (const judo::Element & e, char * multiline) const {
	return UTF8ToLocale(e.getCDATA());
}
CStdString cXData::cItem_obsolete::GetLabelFromXML (const judo::Element & e) const {
	return var;
}
