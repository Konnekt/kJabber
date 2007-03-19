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

#include "disco_window.h"
using namespace JEP;

list<class cDiscoWindow *> JEP::currentWindows;

void cJabber::ServiceDiscovery(const CStdString jid) {
	if (currentWindows.size()) {
		currentWindows.front()->OpenJID(jid);
		currentWindows.front()->ActivateWindow();
	} else
		new cDiscoWindow(*this , jid);
}
DiscoDB::Item * cJabber::GetDiscoItem(const CStdString &jid, bool items, bool fetch) {
	DiscoDB::Item * item = session.discoDB().find(jid , "");
/*	if (fetch && (!item || (item->getState() != DiscoDB::Item::stReady && !(item->getState() & DiscoDB::Item::stQuerying)))) {
		session.discoDB().cache(jid , SigC::slot(*this , CacheCallback) , items);
		if (!item)
			item = session.discoDB().find(jid , "");
	}*/
	return item;
}


cDiscoWindow::cDiscoWindow(cJabber & jab ,  CStdString jid) : jab(jab), cs(/*GetCurrentThreadId()*/) {
	viewType = vtNone;
	rootJID = jid;
	currentItem = NULL;
	rootItem = this->CreateRootItem();
	historyPosition = history.end();
	/* Wype³niamy listê prosto z cache'a */
	for (DiscoDB::iterator it = DiscoDB().begin(); it != DiscoDB().end(); ++it) {
		if (FindParent(it->second) == rootItem)
			rootItem->appendChild(it->second);
	}

	this->hwnd = CreateDialogParam(jab.GetPlug()->hDll() , MAKEINTRESOURCE(IDD_DISCO)
		, 0 , (DLGPROC)this->WndProc , (LPARAM)this);
	OpenJID(jid);
	::ShowWindow(this->hwnd, SW_SHOW);
	currentWindows.push_back(this);
}
cDiscoWindow::~cDiscoWindow() {
	currentWindows.remove(this);
	ListItemsClear();
	delete rootItem;
}

void cDiscoWindow::ActivateWindow() {
	SetForegroundWindow(hwnd);
}

void cDiscoWindow::Clear(bool reloadDisco) {
	LockerCS cl(CS());
	ListItemsClear();
	if (reloadDisco) {
		DiscoDB().clear();
		delete rootItem;
		rootItem = this->CreateRootItem();
		history.clear();
		this->historyPosition = history.end();
	}
	currentItem = rootItem;
	ListItemsBuild();
	SetButtons();
}
void cDiscoWindow::Refresh() {
	ListItemsBuild();
}
DiscoDB::Item * cDiscoWindow::CreateRootItem() {
	DiscoDB::Item * item = new DiscoDB::Item("");
	item->setState(DiscoDB::Item::stReady);
	return item;
}

void cDiscoWindow::ReadJID(CStdString jid , CStdString node, bool refresh) {
	if (jid.empty())
		return;
	LockerCS cl(CS());
	DiscoDB::Item * item = refresh ? 0 : DiscoDB().find(jid , node);
	// uzupe³niamy cache
	if (!item || (item->getState() != DiscoDB::Item::stReady && !(item->getState() & DiscoDB::Item::stQuerying)) ) {
		if (node.empty())
			DiscoDB().cache(jid , SigC::slot(*this , &cDiscoWindow::CacheCallback) , true);
		else
			DiscoDB().cache(jid , node , SigC::slot(*this , &cDiscoWindow::CacheCallback) , true);
		// wczytujemy nowy
		if (!item)
			item = DiscoDB().find(jid , node);
		if (!item) { 
			// coœ niedobrze...
			IMDEBUG(DBG_ERROR , "!DISCO - RefreshJID didn't refresh anything!");
			return;
		}
		// dodajemy do root'a wszystkich elementów, jezeli nie ma swojego parent'a
		if (this->FindParent(item) == rootItem && !rootItem->hasChild(item))
			rootItem->appendChild(item);
		RefreshVisible(item);
	}
}

void cDiscoWindow::OpenJID(CStdString jid , CStdString node) {
	IMDEBUG(DBG_FUNC, "OpenJID(%s, %s)", jid.c_str(), node.c_str());
	ReadJID(jid, node);
	if (jid.empty()) 
		currentItem = rootItem;
	else
		currentItem = DiscoDB().find(jid , node);
	
	if (historyPosition == history.end() || *historyPosition != tHistoryItem(jid , node)) {
		if (history.end() - historyPosition > 1)
			history.erase(historyPosition + 1 , history.end());
		history.push_back(tHistoryItem(jid , node));
		historyPosition = -- history.end();
	}
	SetButtons();

	if (!currentItem) 
		currentItem = rootItem;

	if (viewType == vtNone) return;
	if (viewType == vtTree) {
		// Znajdujemy element, otwieramy go i zaznaczamy
		for (tListItems::iterator it = listItems.begin(); it != listItems.end(); it++) {
			if ((*it->second) == currentItem) {
				it->second->Open(this);
				break;
			}
		}
	} else {
		// Trzeba zbudowaæ listê od nowa
		ListItemsBuild();
	}

}



// ------------------------------------------------------
// PRIVATE

void cDiscoWindow::SetViewType(enViewType vt) {
	LockerCS cl(CS());
	this->ListItemsClear();
	Jab().GetPlug()->DTsetInt(DTCFG, 0, CFG::discoViewType, vt);
	int icoSize = 16;
	if (vt != vtTree) {
		if (vt == vtSmallIcon)
			icoSize = 24;
		else if (vt == vtIcon)
			icoSize = 32;
	}
	if (il[ilList].GetSize() != icoSize) { // ³adujemy ikony...
		il[ilList] = cIL(icoSize);
		il[ilList].LoadIcon(IDI_SERVICE);
		il[ilList].LoadIcon(IDI_FEATURE);
		il[ilList].LoadIcon(IDI_IDENTITY);
		il[ilList].LoadIcon(IDI_CONFERENCE);
		il[ilList].CopyUIIcon(jab, UIIcon(IT_STATUS, NET_GG, ST_ONLINE, 0));
		il[ilList].CopyUIIcon(jab, UIIcon(IT_STATUS, NET_AIM, ST_ONLINE, 0));
		il[ilList].CopyUIIcon(jab, UIIcon(IT_STATUS, NET_ICQ, ST_ONLINE, 0));
		il[ilList].CopyUIIcon(jab, UIIcon(IT_STATUS, NET_YAHOO, ST_ONLINE, 0));
		il[ilList].CopyUIIcon(jab, UIIcon(IT_STATUS, NET_MSN, ST_ONLINE, 0));
	}

	if (vt == vtTree) {
		ShowWindow(this->hwndList , SW_HIDE);
		ListView_SetImageList(this->hwndList, 0, LVSIL_NORMAL);
		ListView_SetImageList(this->hwndList, 0, LVSIL_SMALL);
		TreeView_SetImageList(this->hwndTree, il[ilList].GetList(), TVSIL_NORMAL);
		ShowWindow(this->hwndTree , SW_SHOW);
		//ShowWindow(this->hwndTreeHeader , SW_SHOW);
	} else {
		ShowWindow(this->hwndList , SW_SHOW);
		ShowWindow(this->hwndTree , SW_HIDE);
		TreeView_SetImageList(this->hwndTree, 0, TVSIL_NORMAL);
		ListView_SetImageList(this->hwndList, il[ilList].GetList(), (vt == vtIcon || vt == vtSmallIcon) ? LVSIL_NORMAL : LVSIL_SMALL);
		int listType =  (vt == vtSmallIcon) ? LVS_ICON : (int) vt;
		//ShowWindow(this->hwndTreeHeader , SW_HIDE);
		SetWindowLong(hwndList , GWL_STYLE 
			, (GetWindowLong(hwndList , GWL_STYLE) & ~3) | listType );
	}
	this->viewType = vt;
	this->ListItemsBuild();
}


void cDiscoWindow::ListItemsClear() {
	if (viewType == vtNone) return;
	LockerCS cl(CS());
	while (!listItems.empty())
		listItems.begin()->second->Destroy(this);
	// teoretycznie ju¿ nie powinno byæ niczego na liœcie...
	if (viewType == vtTree)
		TreeView_DeleteAllItems(hwndTree);
	else
		ListView_DeleteAllItems(hwndList);
}
void cDiscoWindow::ListItemsBuild() {
	ListItemsClear();
	if (!currentItem) 
		currentItem = rootItem;
	if (viewType == vtNone) return;
	LockerCS cl(CS());
	// tworzony element listy automatycznie rejestruje sie w listItems
	// i tworzy swoje dzieci i w³aœciwoœci
	if (viewType == vtTree) {
		(new cDiscoItemRoot(this))->Open(this);
	} else { // zwyk³a lista...
		if (currentItem == rootItem)
			(new cDiscoItemRoot(this))->Open(this);
		else
			(new cDiscoItem(this , 0 , currentItem))->Open(this);
	}
}
void CALLBACK cDiscoWindow::CacheCallbackAPC(sCacheCallbackAPC * apc) {
	apc->owner->CacheCallback(apc->item);
	apc->done = true;
}

void cDiscoWindow::CacheCallback(const DiscoDB::Item* item) {
	if (jab.GetPlug()->Is_TUS(0)) {
		sCacheCallbackAPC apc;
		apc.owner = this;
		apc.item = item;
		apc.done = false;
		HANDLE thread = (HANDLE)jab.GetPlug()->ICMessage(IMC_GET_MAINTHREAD);
		if (thread) {
			QueueUserAPC((PAPCFUNC) CacheCallbackAPC, thread, (ULONG_PTR)&apc);
			while (!apc.done)
				Sleep(1);
			return;
		}
	}
	RefreshVisible(item);
}
cDiscoItem * cDiscoWindow::RefreshVisible(const DiscoDB::Item * item) {
	// sprawdzamy aktualn¹ listê, odœwie¿amy wszystkie instancje...
	LockerCS cl(CS());
	cDiscoItem * found = NULL;
	for (tListItems::iterator it = listItems.begin(); it != listItems.end(); it++) {
		if (*it->second != item || !(it->second->GetType() & cDiscoItem_base::tItem)) continue;
		cDiscoItem_base * obj = it->second;
		obj->Refresh(this);
		if (!found)
			found = static_cast<cDiscoItem*>(obj);
		it = FindOnList(obj); // zaczynamy poszukiwania od ostatniego miejsca...
		if (it == listItems.end())
			break;
	}
	if (found)
		return found;
	// skoro nie ma go ju¿ na liœcie upewniamy siê, czy w ogóle jest tu potrzebny...
	// w przypadku drzewka dodajemy wszystko, dla list trzeba sprawdziæ
	cDiscoItem * parent = 0;
	if (viewType != vtTree) {
		if (!currentItem) return 0;
		if (currentItem != item && !currentItem->hasChild(item) && !(currentItem == rootItem && FindParent(item) == rootItem))
			return 0; // je¿eli nie czytamy aktywnego i nie jesteœmy jego dzieciakiem, olewamy sprawê
		parent = (currentItem == item) ? 0 : FindOnList(currentItem); // na liœcie nie ma hierarchii...
	} else {
		// trzeba znaleŸæ parenta na liœcie...
		parent = FindOnList(FindParent(item));
	}
	// tworzymy element
	return new cDiscoItem(this , parent , item);
}
cDiscoItem * cDiscoWindow::FindOnList(const DiscoDB::Item * item) {
	if (!item) return NULL;
	LockerCS cl(CS());
	for (tListItems::iterator it = listItems.begin(); it != listItems.end(); it++)
		if (it->second->GetType() & cDiscoItem_base::tItem && *it->second == item) {
			return static_cast<cDiscoItem*>(it->second);
		}
	return NULL;
}
cDiscoWindow::tListItems::iterator cDiscoWindow::FindOnList(const cDiscoItem_base * item) {
	tListItems::iterator it = listItems.begin();
	for ( ; it != listItems.end(); it++) {
		if (it->second == item)
			break;
	}
	return it;
}

DiscoDB::Item * cDiscoWindow::FindParent(const DiscoDB::Item * item) {
	for (DiscoDB::ItemMap::iterator it = DiscoDB().begin(); it != DiscoDB().end(); it++) {
		if (it->second->hasChild(item))
			return it->second;
	}
	return rootItem;
}
cDiscoItem_base * cDiscoWindow::GetListObject(HANDLE item) {
	tListItems::iterator it = listItems.find(item);
	if (it != listItems.end())
		return it->second;
	else
		return 0;
}

void cDiscoWindow::SetInfo(const CStdString & info) {
	SetWindowText(hwndInfo , info);
}

void cDiscoWindow::SetInfoType(RichEditFormat & f, const CStdString & item) {
	if (item.empty())
		return;
	f.setAlignment(PFA_CENTER);
	f.setBold(true, false);
	f.setUnderline(true, false);
	f.push();
	f.setColor(RGB(0,0,128));
	f.insertText(item);
	f.insertNewLine(2);
	f.pop();
	f.pop();
}
void cDiscoWindow::SetInfoName(RichEditFormat & f, const CStdString & name) {
	if (name.empty())
		return;
	f.setColor(RGB(0,128,0), false);
	f.setBold(true);
	f.insertText(name + "\r\n");
	f.pop();
}
void cDiscoWindow::SetInfoJID(RichEditFormat & f, const CStdString & jid) {
	if (jid.empty())
		return;
	f.setBold(true);
	f.insertText("JID: ");
	f.pop();
	f.insertText(jid + "\r\n");
}
void cDiscoWindow::SetInfoExpl(RichEditFormat & f, const CStdString & txt) {
	if (txt.empty())
		return;
	f.setItalic(true);
	f.insertText(txt + "\r\n");
	f.pop();
}
void cDiscoWindow::SetInfoState(RichEditFormat & f, const CStdString & txt) {
	if (txt.empty())
		return;
	f.setItalic(true, false);
	f.setColor(RGB(200, 0, 0), false);
	f.push();
	f.insertText("\r\n" + txt + "\r\n");
	f.pop();
}
void cDiscoWindow::SetInfoSection(RichEditFormat & f, const CStdString & title) {
	if (title.empty())
		return;
	f.insertNewLine(2);
	f.setAlignment(PFA_CENTER);
	f.setBold(true, false);
	f.setColor(RGB(128,0,0), false);
	f.push();
	f.insertText(title);
	f.insertNewLine(2);
	f.pop();
}


void cDiscoWindow::SetButtons() {
	SendMessage(hwndToolbar , TB_ENABLEBUTTON , IDI_TB_BACK , MAKELONG(historyPosition > history.begin() , 0));
	SendMessage(hwndToolbar , TB_ENABLEBUTTON , IDI_TB_FORWARD , MAKELONG(history.end() - historyPosition > 1 , 0));
	SendMessage(hwndToolbar , TB_ENABLEBUTTON , IDI_TB_UP , MAKELONG(currentItem != rootItem , 0));
	SendMessage(hwndToolbar , TB_ENABLEBUTTON , IDI_TB_REFRESH , MAKELONG(currentItem != rootItem , 0));
}

void cDiscoWindow::LocationHistoryFill() {
	CStdString current;
	GetWindowText(hwndLocation, current.GetBuffer(255), 255);
	current.ReleaseBuffer();
    sMRU MRU;
    MRU.name = "kJabber/disco/location";
    MRU.buffSize = 255;
    MRU.count = 20;
    MRU.values = new char * [MRU.count];
    int i;
    for (i = 0; i<MRU.count; i++) 
		MRU.values[i] = new char [MRU.buffSize];

	while (SendMessage(hwndLocation, CB_GETCOUNT, 0, 0))
		SendMessage(hwndLocation, CB_DELETESTRING, 0, 0);
	int c = Jab().GetPlug()->IMessage(&sIMessage_MRU(IMC_MRU_GET , &MRU));
    for (i=0; i<c; i++)
        SendMessage(hwndLocation , CB_ADDSTRING , 0 , (LPARAM)MRU.values[i]);
    for (i = 0; i<MRU.count; i++) 
        delete MRU.values[i];
    delete [] MRU.values;
	SetWindowText(hwndLocation, current);
}
void cDiscoWindow::LocationHistorySave(const CStdString & location) {
	if (location.empty()) 
		return;
	sMRU MRU;
	MRU.count = 20;
	MRU.flags = MRU_SET_LOADFIRST | MRU_GET_USETEMP;
	MRU.current = location;
	MRU.name = "kJabber/disco/location";
	Ctrl->IMessage(&sIMessage_MRU(IMC_MRU_SET , &MRU));
	LocationHistoryFill();
}


void cDiscoWindow::SetSize() {
	RECT rc, rcClient, rcList, rcLocation;
	GetClientRect(hwnd, &rcClient);
	GetClientRect(hwndLocation, &rcLocation);
	Size szTB = toolBar_getSize(hwndToolbar);
	HDWP hdwp = BeginDeferWindowPos(5);
	const int margin = 5;
	int infoWidth = Jab().GetPlug()->DTgetInt(DTCFG, 0, CFG::discoInfo) ? 200 : 0;
	hdwp = DeferWindowPos(hdwp, hwndToolbar, 0, 0, 0, rcClient.right, szTB.cy, SWP_NOZORDER);
	// Ustalamy wielkoœæ pola "location"
	SendMessage(hwndToolbar , TB_GETRECT , IDC_LOCATION , (LPARAM)&rc);
	int locationWidth = rcClient.right - (szTB.cx - (rc.right - rc.left));
	TBBUTTONINFO bbi;
	bbi.cbSize = sizeof(bbi);
	bbi.dwMask = TBIF_SIZE;
	bbi.cx = locationWidth;
	SendMessage(hwndToolbar, TB_SETBUTTONINFO, IDC_LOCATION , (LPARAM)&bbi);
	SendMessage(hwndToolbar, TB_AUTOSIZE, 0, 0);

	hdwp = DeferWindowPos(hdwp, hwndLocation, 0, rc.left+10, rc.top + 1, locationWidth - 10, rcLocation.bottom, SWP_NOZORDER);
	rcList.left = margin;
	rcList.top = szTB.cy + margin;
	rcList.bottom = rcClient.bottom - margin;
	rcList.right = rcClient.right - margin - (infoWidth ? infoWidth + margin: 0);
	hdwp = DeferWindowPos(hdwp, hwndTree, 0, rcList.left, rcList.top, rcList.right-rcList.left, rcList.bottom-rcList.top, SWP_NOZORDER);
	hdwp = DeferWindowPos(hdwp, hwndList, 0, rcList.left, rcList.top, rcList.right-rcList.left, rcList.bottom-rcList.top, SWP_NOZORDER);
	hdwp = DeferWindowPos(hdwp, hwndInfo, 0, rcClient.right - margin - infoWidth, rcList.top, infoWidth, rcList.bottom-rcList.top, SWP_NOZORDER | (infoWidth ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
	EndDeferWindowPos(hdwp);
	ComboBox_SetMinVisible(hwndLocation, 10);
}

void cDiscoWindow::BuildUI() {
	// ikonki...
	il[ilState] = cIL(16);
	il[ilUI] = cIL(16);
	il[ilState].LoadIcon(IDI_STATE_READY); // i tak nie ma ikony...
	il[ilState].LoadIcon(IDI_STATE_NONE);
	il[ilState].LoadIcon(IDI_STATE_ERROR);
	il[ilState].LoadIcon(IDI_STATE_QUERYING);

	il[ilUI].LoadIcon(IDI_STATE_NONE);
	il[ilUI].LoadIcon(IDI_STATE_QUERYING);
	il[ilUI].LoadIcon(IDI_CONFERENCE);
	il[ilUI].LoadIcon(IDI_TB_BACK);
	il[ilUI].LoadIcon(IDI_TB_ENTER);
	il[ilUI].LoadIcon(IDI_TB_FORWARD);
	il[ilUI].LoadIcon(IDI_TB_GO);
	il[ilUI].LoadIcon(IDI_TB_OPTIONS);
	il[ilUI].LoadIcon(IDI_TB_REFRESH);
	il[ilUI].LoadIcon(IDI_TB_STOP);
	il[ilUI].LoadIcon(IDI_TB_UP);
	il[ilUI].CopyUIIcon(jab, IDI_TB_OPTIONS);
	il[ilUI].CopyUIIcon(jab, IDI_TB_ADD);
	il[ilUI].LoadIcon(IDI_MENU_REGISTER);
	il[ilUI].LoadIcon(IDI_MENU_SEARCH);


	ListView_SetImageList(this->hwndList, il[ilState].GetList(), LVSIL_STATE);
	TreeView_SetImageList(this->hwndTree, il[ilState].GetList(), TVSIL_STATE);

	ListView_AddColumn(hwndList, "", 200);
	ListView_AddColumn(hwndList, "Wêze³", 50);
	ListView_AddColumn(hwndList, "Identyfikator", 100);
	ListView_AddColumn(hwndList, "Typ", 50);

	int listViewStyle = LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT;
	ListView_SetExtendedListViewStyleEx(hwndList, listViewStyle , listViewStyle);

	if (jab.GetPlug()->ICMessage(IMC_ISWINXP)) {
		LVGROUP lg;
		lg.cbSize = sizeof(lg);
		lg.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER | LVGF_STATE;
		lg.stateMask = -1;
		lg.state = LVGS_COLLAPSED;
		lg.uAlign = LVGA_HEADER_LEFT;
		lg.pszHeader = L"Element";
		lg.cchHeader = 3;
		lg.iGroupId = lgThis;
		ListView_InsertGroup(hwndList, lgThis, &lg);
		lg.pszHeader = L"Podelementy";
		lg.state = LVGS_COLLAPSED;
		lg.iGroupId = lgItems;
		ListView_InsertGroup(hwndList, lgItems, &lg);
		lg.pszHeader = L"W³aœciwoœci";
		lg.iGroupId = lgFeatures;
		ListView_InsertGroup(hwndList, lgFeatures, &lg);
		ListView_EnableGroupView(hwndList, jab.GetPlug()->DTgetInt(DTCFG, 0, CFG::discoViewGrouping));
	}

	SetViewType((enViewType)Jab().GetPlug()->DTgetInt(DTCFG, 0, CFG::discoViewType));

	RECT wndSize;
	GetClientRect(hwnd , &wndSize);
	int toolbarHeight = 50;
	int infoHeight = 60;
	int statusHeight = 20;
	hwndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, (LPSTR) NULL,
		WS_CHILD | WS_CLIPCHILDREN |WS_CLIPSIBLINGS | WS_VISIBLE
		|TBSTYLE_TRANSPARENT
		|CCS_NODIVIDER
		| TBSTYLE_FLAT
		| TBSTYLE_LIST
		//        | CCS_NOPARENTALIGN
		//        | CCS_NORESIZE
		| TBSTYLE_TOOLTIPS/* | TBSTYLE_WRAPABLE*/
		, 0, 0, wndSize.right, toolbarHeight , hwnd,
		(HMENU)0, 0, 0);
	SendMessage(hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)il[ilUI].GetList());
	SendMessage(hwndToolbar , TB_SETEXTENDEDSTYLE , 0 ,
					TBSTYLE_EX_MIXEDBUTTONS
					/*| TBSTYLE_EX_HIDECLIPPEDBUTTONS*/
					);
	SendMessage(hwndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
	// Set values unique to the band with the toolbar.
	TBBUTTON bb;
	TBBUTTONINFO bbi;
	memset(&bbi , 0 , sizeof(bbi));
	bbi.cbSize = sizeof(bbi);

	bb.fsState=TBSTATE_ENABLED;
	bb.dwData=0;
	// BACK
	bb.iBitmap = il[ilUI].GetIndex(IDI_TB_BACK);
	bb.idCommand = IDI_TB_BACK;
	bb.fsStyle = BTNS_AUTOSIZE | BTNS_BUTTON;
	bb.iString = (int)"Poprzedni";
	SendMessage(hwndToolbar, TB_INSERTBUTTON, 0x3FFF , (LPARAM)&bb);

	// BACK
	bb.iBitmap = il[ilUI].GetIndex(IDI_TB_FORWARD);
	bb.idCommand = IDI_TB_FORWARD;
	bb.iString = (int)"Nastêpny";
	SendMessage(hwndToolbar, TB_INSERTBUTTON, 0x3FFF , (LPARAM)&bb);

	bb.fsStyle=BTNS_SEP;
	bb.idCommand = 0;
	SendMessage(hwndToolbar, TB_INSERTBUTTON, 0x3FFF , (LPARAM)&bb);

	// UP
	bb.iBitmap = il[ilUI].GetIndex(IDI_TB_UP);
	bb.idCommand = IDI_TB_UP;
	bb.fsStyle = BTNS_AUTOSIZE | BTNS_BUTTON;
	bb.iString = (int)"Poziom wy¿ej";
	SendMessage(hwndToolbar, TB_INSERTBUTTON, 0x3FFF , (LPARAM)&bb);
	// REFRESH
	bb.iBitmap = il[ilUI].GetIndex(IDI_TB_REFRESH);
	bb.idCommand = IDI_TB_REFRESH;
	bb.fsStyle = BTNS_AUTOSIZE | BTNS_BUTTON;
	bb.iString = (int)"Odœwie¿";
	SendMessage(hwndToolbar, TB_INSERTBUTTON, 0x3FFF , (LPARAM)&bb);
/*	// Nowa linijka
	bb.fsStyle=BTNS_AUTOSIZE | BTNS_SEP;
	bb.fsState=TBSTATE_ENABLED;
	bb.idCommand = 1;
	SendMessage(hwndToolbar, TB_INSERTBUTTON, 0x3FFF , (LPARAM)&bb);
	bb.fsState=TBSTATE_ENABLED;
	bbi.dwMask = TBIF_SIZE;
	bbi.cx = 450;
	SendMessage(hwndToolbar, TB_SETBUTTONINFO, 1 , (LPARAM)&bbi);
*/

/*	// STOP
	bb.iBitmap = il[ilUI].GetIndex(IDI_TB_STOP);
	bb.idCommand = IDI_TB_STOP;
	bb.fsStyle = BTNS_AUTOSIZE | BTNS_BUTTON;
	bb.iString = (int)"Stop";
	SendMessage(hwndToolbar, TB_INSERTBUTTON, 0x3FFF , (LPARAM)&bb);
*/
	// Location
	bb.fsStyle = BTNS_SEP;
	bb.fsState = TBSTATE_ENABLED;
	bb.idCommand = IDC_LOCATION;
	SendMessage(hwndToolbar, TB_INSERTBUTTON, 0x3FFF , (LPARAM)&bb);
	// bbi
	bbi.dwMask = TBIF_SIZE;
	bbi.cx = 350;
	SendMessage(hwndToolbar, TB_SETBUTTONINFO, IDC_LOCATION , (LPARAM)&bbi);

	// GO
	bb.iBitmap = il[ilUI].GetIndex(IDI_TB_GO);
	bb.idCommand = IDI_TB_GO;
	bb.fsStyle = BTNS_AUTOSIZE | BTNS_BUTTON;
	bb.iString = (int)"GO";
	SendMessage(hwndToolbar, TB_INSERTBUTTON, 0x3FFF , (LPARAM)&bb);
	
	SendMessage(hwndToolbar, TB_AUTOSIZE, 0, 0);

	Rect rc = getChildRect(hwndTree);
	SetWindowPos(hwndList , 0 , rc.left , rc.top , rc.right - rc.left , rc.bottom - rc.top , SWP_NOZORDER);
	SendMessage(hwndToolbar , TB_GETRECT , IDC_LOCATION , (LPARAM)&rc);
	SetWindowPos(hwndLocation , 0 , rc.left + 10 , rc.top + 1 , rc.right - rc.left - 10 , rc.bottom - rc.top - 1 , SWP_NOZORDER);

	SetButtons();
	LocationHistoryFill();
	SetSize();
}

LRESULT cDiscoWindow::WndProc(HWND hwnd , UINT message , WPARAM wParam , LPARAM lParam) {
	cDiscoWindow * me;
	me = (cDiscoWindow *)GetProp(hwnd , "cDiscoWindow*");
	switch (message) {
	case WM_INITDIALOG: {
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)loadIconEx(Ctrl->hDll(), MAKEINTRESOURCE(IDI_DISCO), 16));
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)loadIconEx(Ctrl->hDll(), MAKEINTRESOURCE(IDI_DISCO), 32));
		me = (cDiscoWindow *)lParam;
		SetProp(hwnd , "cDiscoWindow*" , (HANDLE) me);
		me->hwnd = hwnd;
		me->hwndInfo = GetDlgItem(hwnd , IDC_INFO);
		RichEditHtml::init(me->hwndInfo);
		me->hwndTree = GetDlgItem(hwnd , IDC_TREE1);
		me->hwndList = GetDlgItem(hwnd , IDC_LIST1);
		me->hwndLocation = GetDlgItem(hwnd , IDC_LOCATION);
		me->BuildUI();
	

		break;}
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		delete me;
		DestroyIcon((HICON)SendMessage(hwnd, WM_GETICON, ICON_SMALL, 0));
		DestroyIcon((HICON)SendMessage(hwnd, WM_GETICON, ICON_BIG, 0));

		return 0;
	case WM_CONTEXTMENU: {
		HANDLE selected = (HANDLE) -1;
		cDiscoItem_base * item;
		POINT pt;
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		if ((HWND)wParam == me->hwndTree) {
			TVHITTESTINFO hti;
			hti.pt = pt;
			ScreenToClient(me->hwndTree , &hti.pt);
			TreeView_HitTest(me->hwndTree , &hti);
			if (hti.flags & TVHT_ONITEM)
				selected = hti.hItem;
		} else if ((HWND)wParam == me->hwndList) {
			LVHITTESTINFO hti;
			hti.pt = pt;
			ScreenToClient(me->hwndList , &hti.pt);
			ListView_HitTest(me->hwndList , &hti);
			if (hti.flags & LVHT_ONITEM)
				selected = (HANDLE) hti.iItem;
		} else
			break;
		if (selected == (HANDLE) -1) { //trafiamy w puste ... pobieramy element zwiazany z aktualnym okienkiem...
			item = me->FindOnList(me->GetViewType() == vtTree ? me->rootItem : me->currentItem);
		} else {
			item = me->GetListObject(selected);
		}
		if (!item)
			break;
		item->OnRightClick(me , selected , pt);
		return 0;}
	case WM_NOTIFY: {
        NMHDR * nm=(NMHDR*)lParam;
		cDiscoItem_base * item;
		if (!me) return 0;
		if (nm->hwndFrom == me->hwndTree) {
			switch (nm->code) {
			case TVN_ITEMEXPANDING: {
				NMTREEVIEW * ntv = (NMTREEVIEW*)lParam;
				if (!(item = me->GetListObject(ntv->itemNew.hItem)))
					break;
				item->OnExpand(me , ntv->itemNew.hItem);
				return false;
				}
			case TVN_SELCHANGED: {
				NMTREEVIEW * ntv = (NMTREEVIEW*)lParam;
				if (!(item = me->GetListObject(ntv->itemNew.hItem)))
					break;
				item->OnSelect(me , ntv->itemNew.hItem);
				break;
				}
			}
		} else if (nm->hwndFrom == me->hwndList) {
			switch(nm->code) {
			case LVN_ITEMCHANGED: {
				NMLISTVIEW * nmlv = (NMLISTVIEW *)lParam;
				if (!(nmlv->uChanged & LVIF_STATE)) break;
				if (!(nmlv->uNewState & LVIS_SELECTED)) break;
				}
			case NM_CLICK: {
				int index = ListView_GetSelectionMark(nm->hwndFrom);
				if (!(item = me->GetListObject((HANDLE)index)))
					break;
				item->OnSelect(me , (HANDLE)index);
				break;
				}
			case NM_DBLCLK: {
				int index = ListView_GetSelectionMark(nm->hwndFrom);
				if (!(item = me->GetListObject((HANDLE)index)))
					break;
				item->OnDoubleClick(me , (HANDLE)index);
				break;
				}
			}
		}
		break;
		}
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED) {
			LockerCS lc(me->CS());
			switch (LOWORD(wParam)) {
			case IDI_TB_BACK:
				if (me->historyPosition > me->history.begin())
					me->OpenJID(*--me->historyPosition);
				break;
			case IDI_TB_FORWARD:
				if (me->historyPosition < me->history.end())
					me->OpenJID(*++me->historyPosition);
				break;
			case IDI_TB_UP: {
				me->OpenJID(me->FindParent(me->currentItem));
				break;}
			case IDI_TB_GO: {
				CStdString location, node;
				int size = GetWindowTextLength(me->hwndLocation);
				GetWindowText(me->hwndLocation, location.GetBuffer(size + 1), size + 1);
				location.ReleaseBuffer();
				if (location.empty())
					break;
				size_t nodePos = location.find("::");
				if (nodePos != location.npos) {
					node = location.substr(nodePos+2);
					location.erase(nodePos);
				}
				me->OpenJID(location, node);
				me->LocationHistorySave(location + (node.empty() ? "" : "::" + node));
				break;}
			case IDI_TB_REFRESH:
				me->ReadJID(me->currentItem, true);
				break;
			case IDOK:
				//IMLOG("GetFocus = %x, %x", GetFocus(), GetParent(GetFocus()));
				if (GetParent(GetFocus()) == me->hwndLocation)
					WndProc(hwnd, WM_COMMAND, MAKELONG(IDI_TB_GO, BN_CLICKED), (LPARAM)hwnd);
				break;
			}
		}
		break;
	case WM_MEASUREITEM: {
        MEASUREITEMSTRUCT *mis ; mis=(MEASUREITEMSTRUCT *)lParam;
        if (!wParam) { // MENU
			mis->itemHeight=16;
			mis->itemWidth=18;
			return 1;
        }
		return 0;}

	case WM_DRAWITEM:
		if (!wParam) { // MENU
			DRAWITEMSTRUCT * dis;
			dis=(DRAWITEMSTRUCT *)lParam;
			/* w itemData siedzi identyfikator ikonki... */
			int ico = me->il[ilUI].GetIndex((int)dis->itemData);
			if (ico == -1) return 0;
			IMAGELISTDRAWPARAMS ildp;
			ildp.cbSize=sizeof(IMAGELISTDRAWPARAMS);
			ildp.himl=me->il[ilUI].GetList();
			ildp.i=ico;
			ildp.hdcDst=dis->hDC;
	//      ImageList_GetIconSize(iml[imList] , &ildp.x , &ildp.y);
			ildp.x=1;
			ildp.y=1;
			ildp.cx=ildp.cy=ildp.xBitmap=ildp.yBitmap=0;
			ildp.rgbBk=CLR_NONE;
			ildp.rgbFg=CLR_NONE;
			ildp.fStyle = ILD_TRANSPARENT;
			ildp.fState = ILS_SHADOW;
			ildp.dwRop=0;
			ildp.Frame=0;
			ildp.crEffect=0x00FF0000;
			if (!ImageList_DrawIndirect(&ildp))
				ImageList_Draw(ildp.himl,ildp.i,ildp.hdcDst , ildp.x,ildp.y,0);
			return 1;
		} // menu
		return 0;
    case WM_SIZING:
		clipSize(wParam , (RECT *)lParam , 400 , 200);
		return 1;
	case WM_SIZE:
		me->SetSize();
		return 0;
	}
	return 0;
}

// ------------------------------------ IL
cDiscoWindow::cIL::cIL(int size) {
	if (size == 0)
		list = 0;
	else
		list = ImageList_Create(size, size, ILC_COLOR32|ILC_MASK, 4, 5);
}
cDiscoWindow::cIL & cDiscoWindow::cIL::operator= (cIL & il) {
	this->list = il.list;
	il.list = 0;
	this->index = il.index;
	return *this;
}

cDiscoWindow::cIL::~cIL() {
	if (list)
		ImageList_Destroy(list);
}
int cDiscoWindow::cIL::GetIndex(int id) {
	if (index.find(id) != index.end())
		return index[id];
	else
		return -1;
}
void cDiscoWindow::cIL::LoadIcon(int id) {
	if (!list) return;
	HICON ico = loadIconEx(Ctrl->hDll(), MAKEINTRESOURCE(id), GetSize());
	index[id] = ImageList_AddIcon(list, ico);
	DestroyIcon(ico);
}
void cDiscoWindow::cIL::CopyUIIcon(cJabber & jab, int id) {
	if (!list) return;
	HICON ico = (HICON)jab.GetPlug()->ICMessage(IMI_ICONGET, id, IML_16);
	index[id] = ImageList_AddIcon(list, ico);
	DestroyIcon(ico);
}

int cDiscoWindow::cIL::GetSize() {
	if (!list) return 0;
	int x = 0, y = 0;
	ImageList_GetIconSize(list, &x, &y);
	return x;
}

