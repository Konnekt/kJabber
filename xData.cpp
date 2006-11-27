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
#pragma comment(lib, "comctl32.lib")
using namespace JEP;

// -------------------------------------------------------------------
// WINDOW

cXData::cXData(cJabber & jabber, const CStdString & title, HWND parent):jab(jabber) {
	this->title = title;
	x = y = w = h = 0;
	newInfoH = newContH = newContW = -1;
	state = stNone;
	type = tNone;
	this->hwnd = CreateDialogParam(jab.GetPlug()->hDll() , MAKEINTRESOURCE(IDD_XDATA)
		, parent , (DLGPROC)this->WndProc , (LPARAM)this);
	SetTitle("");

	status = CreateStatusWindow(/*SBARS_SIZEGRIP | */WS_CHILD | WS_VISIBLE,
			"",hwnd,0);
	int sbw []={/*200 ,*/ -1};
	SendMessage(status , SB_SETPARTS , 1 , (LPARAM)sbw);
//	SendMessage(statusWnd , SB_SETTEXT , 1 , (int) );
	info = GetDlgItem(hwnd, IDC_INFO);
	SendMessage(info , EM_SETLANGOPTIONS , 0 , 0);

    WNDCLASSEX wcex;
	memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc	= (WNDPROC)ContainerProc;
	wcex.hInstance		= jab.GetPlug()->hInst();
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= GetSysColorBrush(COLOR_BTNFACE);
	wcex.lpszMenuName	= "";
	wcex.lpszClassName	= "xDataContainer";
	RegisterClassEx(&wcex);
	container = CreateWindowEx(WS_EX_CONTROLPARENT, "xDataContainer", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN, 
		0, 0, 0, 0, hwnd, 0, 0, 0);

	cItem_hwnd::font = (HFONT)GetStockObject(DEFAULT_GUI_FONT)/* SendMessage(hwnd, WM_GETFONT, 0, 0)*/;
	//cItem_hwnd::SetFont(container);
	this->SetInfoFormat();
	this->SetInstructions("...proszê czekaæ...");
	this->SetSize(ssYes);
}

cXData::~cXData() {
	DestroyWindow(hwnd);
	this->hwnd = 0;
}

void cXData::Process(const judo::Element & e , bool wait) {
	ProcessInfo("Gotowe...");
	Enable(true);
	const judo::Element * x = e.findElement("x");
	if (!x || x->getAttrib("xmlns") != "jabber:x:data") {
		/* ostatnia szansa - register lub search */
		if (e.getName() == "query" 
			&& (e.getAttrib("xmlns") == "jabber:iq:search" || e.getAttrib("xmlns") == "jabber:iq:register")
		) {
			state = stWait;
			ProcessObsolete(e);
			return;
		}

		state = stError;
		return;
	}
	state = stWait;
	if (x->getAttrib("type") == "result") {
		ProcessResult(*x);
/*	} else if (x->getAttrib("type") == "error") {
		ProcessError(e);*/
	} else if (x->getAttrib("type") == "form") {
		ProcessForm(*x);
	} else {
		ProcessError("Nieznany rodzaj odpowiedzi: " + x->getAttrib("type"));
		state = stError;
	}
	ShowWindow();
	while (wait && !this->IsAnswered()) {
		jab.GetPlug()->WMProcess();
	}
}
/*void cXData::ProcessError(const judo::Element & e) {
	type = tError;
	this->SetDescription();
}*/
void cXData::ProcessError(CStdString msg, bool interrupt) {
	ShowWindow();
	if (interrupt) {
		SetContWidth(defFormWidth);
		SetContHeight(0);
		SetInfoFormat(0xFFFFFF , 0x000080, true);
		Enable(true);
		this->clear();
		state = stWait;
		type = tError;
		
		SetInstructions(msg);
		SetStatus("B³¹d!");
		SetSize(ssYes);
	} else {
		if (MessageBox(hwnd, msg, this->title, MB_ICONERROR | MB_RETRYCANCEL) 
			== IDCANCEL)
			evtCommand(IDCANCEL);
		else
			Enable(true);
	}
}
void cXData::ProcessInfo(CStdString msg, bool interrupt) {
	ShowWindow();
	if (interrupt) {
		Enable(true);
		SetContWidth(defFormWidth);
		SetContHeight(0);
		SetInfoFormat(-1 , -1, true);
		this->clear();
		state = stWait;
		type = tInfo;
		SetInstructions(msg);
		SetStatus("");
		SetSize(ssYes);
	} else {
		SetStatus(msg);
	}

}
void cXData::SetStatus(const CStdString & txt) {
	SendMessage(status , SB_SETTEXT , 0 , (int) txt.c_str());
}

void cXData::ProcessForm(const judo::Element & e) {
	SetContWidth(defFormWidth);
	SetInfoFormat();
	this->clear();
	type = tForm;
	this->SetTitle(kJabber::UTF8ToLocale(e.getChildCData("title")));
	this->SetInstructions(kJabber::UTF8ToLocale(e.getChildCData("instructions")));
	SetStatus("Wype³nij formularz...");
	for (judo::Element::const_iterator i = e.begin(); i != e.end(); i++) {
		if ((*i)->getType() != judo::Node::ntElement || (*i)->getName() != "field")
			continue;
		const judo::Element * it = static_cast<const judo::Element *>(*i);
		CStdString type = it->getAttrib("type");
		cItem * item = 0;
		if (type == "boolean") {
			item = new cItem_boolean();
		} else if (type == "fixed") {
			item = new cItem_fixed();
		} else if (type == "hidden") {
			item = new cItem_hidden();
		} else if (type == "jid-multi") {
			item = new cItem_jid_multi();
		} else if (type == "jid-single") {
			item = new cItem_jid_single();
		} else if (type == "list-multi") {
			item = new cItem_list_multi();
		} else if (type == "list-single") {
			item = new cItem_list_single();
		} else if (type == "text-multi") {
			item = new cItem_text_multi();
		} else if (type == "text-private") {
			item = new cItem_text_private();
		} else if (type == "text-single") {
			item = new cItem_text_single();
		}
		if (item) {
			item->Create(this, *it);
			if (item->GetHeight(this))
				this->OffsetY(item->GetHeight(this) + 5);
		}
	}
	SetContHeight(this->h);
	SetSize(ssYes);
}
void cXData::ProcessResult(const judo::Element & e) {
	if (!e.findElement("reported")) {
		/* Skoro nie ma reported, wynik jest tylko jeden i mo¿na go pokazaæ normalnie... */
		ProcessForm(e);
		type = tResult;
		return;
	}
	SetContWidth(defResultWidth);
	SetInfoFormat();
	this->clear();
	type = tResult;
	this->SetTitle(kJabber::UTF8ToLocale(e.getChildCData("title")));
	this->SetInstructions(kJabber::UTF8ToLocale(e.getChildCData("instructions")));
	SetContHeight(defResultHeight);
	SetSize(ssYes); // wczesniej, zeby mogl dobrze wypozycjonowac okno...
	SetStatus("Gotowe");
	cItem_result * result = new cItem_result();
	result->Create(this, e);
	this->OffsetY(result->GetHeight(this));
	//SetSize(ssYes);
}
void cXData::ProcessObsolete(const judo::Element & e) {
	SetContWidth(defFormWidth);
	SetInfoFormat();
	this->clear();
	type = tObsolete;
	this->SetTitle(kJabber::UTF8ToLocale(e.getChildCData("title")));
	this->SetInstructions(kJabber::UTF8ToLocale(e.getChildCData("instructions")));
	SetStatus("Wype³nij formularz...");
	for (judo::Element::const_iterator i = e.begin(); i != e.end(); i++) {
		if ((*i)->getType() != judo::Node::ntElement)
			continue;
		const judo::Element * it = static_cast<const judo::Element *>(*i);
		CStdString name = it->getName();
		cItem * item = 0;
		if (name == "password") {
			item = new cItem_obsolete_private();
		} else if (name == "key") {
			item = new cItem_obsolete_hidden();
		} else if (name == "registered" || name == "instructions" || name == "title" || name == "remove") {
			/* tylko do naszej wiadomoœci... */
		} else {
			item = new cItem_obsolete_text();
		}
		if (item) {
			item->Create(this, *it);
			if (item->GetHeight(this))
				this->OffsetY(item->GetHeight(this) + 5);
		}
	}
	SetContHeight(this->h);
	SetSize(ssYes);
}


void cXData::InjectResult (judo::Element & e) const {
	judo::Element * x;
	if (type != tObsolete) {
		x = e.findElement("x");
		if (!x) {
			x =	e.addElement("x");
			x->putAttrib("xmlns", "jabber:x:data");
			x->putAttrib("type", state==stSubmit ? "submit" : "cancel");
		}
	} else {
		x = &e;
	}
	if (state!=stCancel) {
		cXData::const_iterator it;
		for (it = items.begin(); it != items.end(); it++) {
			judo::Element * element = (*it)->Build();
			if (element)
				x->appendChild(element);
		}
	}
}
void cXData::clear() {
	this->y = 0;
	this->h = 0;
	this->OffsetY(0);
	this->SetContHeight(0);
	SetScrollPos(container , SB_VERT , 0, true);

	cXData::iterator it;
	for (it = items.begin(); it != items.end(); it++) {
		(*it)->Destroy(this);
	}
	items.clear();

	this->SetSize();
}



LRESULT cXData::WndProc(HWND hwnd , UINT message , WPARAM wParam , LPARAM lParam) {
	cXData * me = (cXData*)GetProp(hwnd, "cXData");
	switch (message) {
		case WM_INITDIALOG:
			SetProp(hwnd , "cXData", (HANDLE)lParam);
			break;
		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED) {
				switch (LOWORD(wParam)) {
				case IDOK:
					me->state = stSubmit;
					break;
				case IDCANCEL:
					me->state = stCancel;
					break;
				}
				me->evtCommand(LOWORD(wParam));
			}
			break;
		case WM_CLOSE:
			me->state = stCancel;
			me->evtCommand(IDCANCEL);
			break;
			// Specjalnie dla okienka raportowania....
		case WM_NOTIFY: {
			NMHDR * nm=(NMHDR*)lParam;
			if (!me) return 0;
			switch(nm->code) {
				case NM_DBLCLK: { // najpewniej raport!
					cItem_result * result = (cItem_result *)GetProp(nm->hwndFrom, "cItem_result");
					if (!result)
						break;
					result->OnDoubleClick(me);
					break;
					}
			}
		break;
		}


	};
	return 0;
}

LRESULT cXData::ContainerProc(HWND hwnd , UINT message , WPARAM wParam , LPARAM lParam) {
	cXData * me = (cXData*)GetProp(GetParent(hwnd), "cXData");
	switch (message) {
        case WM_MOUSEWHEEL:{
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
//            for (int i=0; i<3; i++)
                SendMessage(hwnd , WM_VSCROLL , (delta>0?SB_PAGEUP:SB_PAGEDOWN) , 0);
            break;}

		case WM_VSCROLL: {
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
			GetScrollInfo(hwnd , SB_VERT , &si);
			int oldPos = si.nPos;
			switch (LOWORD(wParam)) {
				case SB_THUMBTRACK:
					si.nPos = HIWORD(wParam);
					break;
				case SB_LINEDOWN:
					si.nPos += 10;
					break;
				case SB_LINEUP:
					si.nPos -= 10;
					break;
				case SB_PAGEDOWN:
					si.nPos += 40;
					break;
				case SB_PAGEUP:
					si.nPos -= 40;
					break;
				case SB_TOP:
					si.nPos = si.nMin;
					break;
				case SB_BOTTOM:
					si.nPos = si.nMax - si.nPage;
					break;
			}
			si.fMask = SIF_POS;
			SetScrollInfo(hwnd , SB_VERT , &si , 1);
			GetScrollInfo(hwnd , SB_VERT , &si);
			if (si.nPos != oldPos) {
				ScrollWindow(hwnd , 0 , oldPos - si.nPos , 0 , 0);
				//InvalidateRect(hwnd , &rc , 0);
				UpdateWindow(hwnd);
			}
			return 0;}
/*
		case WM_NOTIFY: {
			NMHDR * nm=(NMHDR*)lParam;
			if (!me) return 0;
			switch(nm->code) {
				case NM_DBLCLK: { // najpewniej raport!
					cItem_result * result = (cItem_result *)GetProp(nm->hwndFrom, "cItem_result");
					if (!result)
						break;
					result->OnDoubleClick(me);
					break;
					}
			}
		break;
		}
*/
	};
    return DefWindowProc(hwnd, message, wParam, lParam);
}

void cXData::SetSize(enSetSize resize) {
	RECT rcCont;
	GetClientRect(container, &rcCont);
	if (resize == ssForce || (resize == ssYes && (newContH != -1 || newContW != -1 || newInfoH != -1))) {

		HDWP hdwp;
		hdwp=BeginDeferWindowPos(8);
		RECT rcInfo, rcButton, rcStatus, rcWindow, rcClient;
		HWND border = GetDlgItem(hwnd, IDC_BORDER);
		GetClientRect(info, &rcInfo);
		GetClientRect(status, &rcStatus);
		GetClientRect(GetDlgItem(hwnd, IDOK), &rcButton);
		GetWindowRect(hwnd, &rcWindow);
		GetClientRect(hwnd, &rcClient);
		cItem_hwnd::minLineHeight = rcButton.bottom;
		const int margin = 5;
		const int marginCont = 5;
		const int marginBorder = 8;
		const int minWidth = 200;
		const int maxInfoHeight = 100;
		const int maxContHeight = 350;
		if (newContW <= 0)
			newContW = rcCont.right;
		newContW = max(minWidth, newContW);
		//if (h > containerH)  // zawsze zostawiamy miejsce na scrolla...
		rcCont.right = newContW;
			newContW += GetSystemMetrics(SM_CXVSCROLL);
		if (newContH < 0)
			newContH = rcCont.bottom;
		newContH = min(maxContHeight, newContH);
		rcCont.bottom = newContH;
		if (newInfoH < 0)
			newInfoH = rcInfo.bottom;
		else { // mierzymy wysokosc dla info...
			RECT rc;
			rc.left = rc.top = rc.bottom = 0;
			rc.right = newContW/* + 2*marginCont*/;
			CStdString txt;
			int size = GetWindowTextLength(info);
			GetWindowText(info, txt.GetBuffer(size + 1), size + 1);
			txt.ReleaseBuffer(size);
			FitText(info, txt, rc);
			newInfoH = rc.bottom ? min(maxInfoHeight , rc.bottom + 2) : 0;
		}

		hdwp = DeferWindowPos(hdwp, info, 0, margin, margin, newContW + 2*marginCont, newInfoH, (newInfoH ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOZORDER);

		hdwp = DeferWindowPos(hdwp, border, 0, margin, margin + newInfoH, newContW + 2*marginCont, newContH + 2*marginCont + marginBorder, (newContH ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOZORDER);
		hdwp = DeferWindowPos(hdwp, container, 0, margin + marginCont, margin + newInfoH + marginCont + marginBorder, newContW, newContH, (newContH ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOZORDER);

		int wndW = 2*margin + newContW + 2*marginCont;
		int wndH = 2*margin + 2*margin + newInfoH + (newContH ? 2*marginCont + marginBorder + newContH : 0) + rcStatus.bottom + rcButton.bottom;
		int wndWdiff = wndW - rcClient.right;
		int wndHdiff = wndH - rcClient.bottom;
		int buttonY = wndH - 2*margin - rcStatus.bottom - rcButton.bottom;
		
		hdwp = DeferWindowPos(hdwp, GetDlgItem(hwnd, IDB_SPECIAL), 0, margin, buttonY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		hdwp = DeferWindowPos(hdwp, GetDlgItem(hwnd, IDOK), 0, wndW - margin - rcButton.right, buttonY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		hdwp = DeferWindowPos(hdwp, GetDlgItem(hwnd, IDCANCEL), 0, wndW - 2*margin - 2*rcButton.right, buttonY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
//		hdwp = DeferWindowPos(hdwp, status, 0, 0, wndH - rcStatus.bottom, wndW, rcStatus.bottom, SWP_NOZORDER | SWP_NOMOVE);
		EndDeferWindowPos(hdwp);
		SetWindowPos(hwnd, 0, rcWindow.left - wndWdiff / 2, rcWindow.top - wndHdiff / 2, rcWindow.right - rcWindow.left + wndWdiff, rcWindow.bottom - rcWindow.top + wndHdiff, SWP_NOZORDER);
		SendMessage(status , WM_SIZE , 0,MAKELPARAM(wndW, wndH));

		newContH = newInfoH = newContW = -1;
	}
	this->x = 0;
	this->w = ((newContW == -1) ? rcCont.right - GetSystemMetrics(SM_CXVSCROLL) : newContW) - 3;
	this->containerH = (newContH == -1) ? rcCont.bottom : newContH;
	UpdateScroll();


	// zmienia rozmiary...
}
void cXData::FitText(HWND hwnd, const CStdString & txt, RECT & rc) {
	if (!hwnd) hwnd = this->hwnd;
	HDC dc = GetDC(hwnd);
	HFONT oldFont = (HFONT)SelectObject(dc, cItem_hwnd::font);
	RECT calc = rc;
	DrawText(dc, txt, -1, &calc, DT_CALCRECT | DT_NOPREFIX | DT_WORDBREAK);
	SelectObject(dc, oldFont);
	ReleaseDC(hwnd, dc);
	if (calc.bottom > rc.bottom)
		rc.bottom = calc.bottom;
}
int cXData::FitTextHeight(HWND hwnd, const CStdString & txt) {
	RECT rc = {0,0,0,0};
	FitText(hwnd, txt, rc);
	return rc.bottom - rc.top;
}

int cXData::OffsetY(int offset) {
	y += offset;
	h += offset;
	return y;
}
void cXData::UpdateScroll() {
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
	GetScrollInfo(container , SB_VERT , &si);
	ShowScrollBar(container , SB_VERT , h > containerH);
	if (h > containerH) {
		si.fMask = SIF_PAGE | SIF_RANGE;
		si.nMin = 0;
		si.nMax = h;
	//	si.nPos = rc.top-ymin;
		si.nPage = containerH;
		SetScrollInfo(container , SB_VERT , &si , 1);
	}
}
void cXData::Enable(bool enable) {
	EnableWindow(container, enable);
	EnableWindow(GetDlgItem(hwnd, IDOK), enable);
//	EnableWindow(GetDlgItem(hwnd, IDCANCEL), enable);
	EnableWindow(GetDlgItem(hwnd, IDB_SPECIAL), enable);
}


void cXData::SetInstructions(const CStdString & txt) {
	SetWindowText(info , txt);
	SetInfoHeight(1);
}
void cXData::SetInfoFormat(int color, int bgColor, bool bold) {
	CHARFORMAT2 cf;
	memset(&cf, 0, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR | CFM_BOLD;
	cf.crTextColor = color == -1 ? GetSysColor(COLOR_WINDOWTEXT) : color;
	cf.dwEffects = bold ? CFE_BOLD : 0;
	SendMessage(info, EM_SETCHARFORMAT , SCF_ALL, (LPARAM)&cf);
	SendMessage(info, EM_SETBKGNDCOLOR, 0, bgColor == -1 ? GetSysColor(COLOR_3DFACE) : bgColor);
}

void cXData::SetTitle(const CStdString & txt) {
	SetWindowText(hwnd , txt.size() < 10 ? title + " - " + txt : txt);
}

void cXData::SetButtonSpecial(const CStdString & txt) {
	::ShowWindow(GetDlgItem(hwnd, IDB_SPECIAL) , txt.empty() ? SW_HIDE : SW_SHOW);
	SetDlgItemText(hwnd, IDB_SPECIAL , txt);
}
