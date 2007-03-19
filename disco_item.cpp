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
#include "disco_feature.h"
using namespace JEP;

// --------------------------------------------------------------
// ITEMS
// --------------------------------------------------------------
// --------------------------------------------------------------

cDiscoItem_base::cDiscoItem_base(cDiscoWindow * owner) {
}
void cDiscoItem_base::Destroy(cDiscoWindow * owner, cDiscoItem_base * parent) {
	// kasujemy wszystkie dzieci...
	// kasujemy element listy...
	LockerCS cl(owner->CS());
	bool unregistered = false;
	if (owner->GetViewType() == cDiscoWindow::vtTree) {
		DestroyChildren(owner, parent);
		if (!parent) { 
			// przy pierwszym koszeniu musimy znaleŸæ wszystkie pasuj¹ce elementy w drzewie
			const DiscoDB::Item * item = ((GetType() == tItem) ? static_cast<cDiscoItem*>(this)->GetItem() : 0);
			if (item) {
				cDiscoWindow::tListItems::reverse_iterator it = owner->rbegin();
				while (it != owner->rend()) {
					if (it->second != this && *it->second == item) {
						it->second->Destroy(owner, parent); // dzieki podaniu parent (chocia¿ nie jest prawdziwy!) nie wrócimy znowu do tego miejsca...
						// z powy¿szego powodu liœæ usun¹æ musimy sami...
						TreeView_DeleteItem(owner->hwndTree , (HTREEITEM) it->second->GetListItem(owner));
						it = owner->rbegin(); // restart
					} else
						it++;
				}
			}
			// wszystkie podelementy usun¹ siê same...
			HTREEITEM listItem = (HTREEITEM) GetListItem(owner);
			Unregister(owner);
			unregistered = true;
			TreeView_DeleteItem(owner->hwndTree , listItem);
		}
	} else {
		if (!parent) {
			DestroyChildren(owner, parent); // ew. usuniête dzieci nie bêd¹ mia³y nast. dzieci...
			Unregister(owner);
			unregistered = true;
			ListView_DeleteAllItems(owner->hwndList);
		}
		//ListView_DeleteItem(owner->hwndList , (int) GetListItem(owner));
	}
	if (!unregistered)
		Unregister(owner);
	delete this;
}
void cDiscoItem_base::Register(cDiscoWindow * owner , HANDLE item) {
	owner->listItems.insert(pair<HANDLE , cDiscoItem_base *> (item , this));
}
void cDiscoItem_base::Unregister(cDiscoWindow * owner) {
	cDiscoWindow::tListItems::iterator it = owner->FindOnList(this);
	if (it != owner->listItems.end())
		owner->listItems.erase(it);
}
HANDLE cDiscoItem_base::GetListItem(cDiscoWindow * owner , HANDLE def) const {
	cDiscoWindow::tListItems::iterator it = owner->FindOnList(this);
	if (it != owner->listItems.end())
		return it->first;
	else
		return def;
}

bool cDiscoItem_base::IsChildOf(cDiscoWindow * owner , const cDiscoItem_base * item) {
	if (this == item) return false;
	if (owner->GetViewType() == cDiscoWindow::vtTree) {
		return TreeView_GetParent(owner->hwndTree , (HTREEITEM) this->GetListItem(owner)) == item->GetListItem(owner);
	} else {
		// 0 to odpytywany element na liœcie... pozosta³e wiêc s¹ na pewno jego dzieæmi...
		return ((int)item->GetListItem(owner)) == 0;
	}
}
cDiscoItem_base * cDiscoItem_base::GetParent(cDiscoWindow * owner) {
	if (owner->GetViewType() == cDiscoWindow::vtTree) {
		return owner->GetListObject((HANDLE) TreeView_GetParent(owner->hwndTree , (HTREEITEM) this->GetListItem(owner)));
	} else {
		// 0 to odpytywany element na liœcie... pozosta³e wiêc s¹ na pewno jego dzieæmi...
		return (GetListItem(owner) > 0) ? owner->GetListObject(0) : 0;
	}
}
void cDiscoItem_base::DestroyChildren(cDiscoWindow * owner, cDiscoItem_base * parent, bool removeItems) {
// Wersja zoptymalizowana pod typ kontrolek...
	LockerCS cl(owner->CS());
	if (owner->GetViewType() == cDiscoWindow::vtTree) {
		// przy pierwszym 
		HTREEITEM hti = TreeView_GetChild(owner->hwndTree, (HTREEITEM)GetListItem(owner));
		while (hti) {
			cDiscoItem_base * obj = owner->GetListObject(hti);
			if (obj)
				obj->Destroy(owner, this);
			HTREEITEM newHti = TreeView_GetNextSibling(owner->hwndTree, hti);
			if (removeItems)
				TreeView_DeleteItem(owner->hwndTree , hti);
			hti = newHti;
		}
	} else {
		// Je¿eli element nie jest aktualnym - nie mozna usuwac dzieci
		if (GetListItem(owner) != 0)
			return;
		cDiscoWindow::tListItems::reverse_iterator it = owner->rbegin();
		while (it != owner->rend()) {
			// usuwamy wszystko poza zerem - od konca...
			if (it->second != this && it->first != 0) {
				if (removeItems)
					ListView_DeleteItem(owner->hwndList, it->first);
				it->second->Destroy(owner, this);
				it = owner->rbegin();
			} else {
				++it;
			}

		}
	}
	
	/*
	cDiscoWindow::tListItems::reverse_iterator it = owner->rbegin();
	while (it != owner->rend()) {
		if (it->second->IsChildOf(owner , this)) {
			it->second->Destroy(owner);
			// przynajmniej na razie - po usuniêciu elementu przegl¹damy listê od nowa
			it = owner->rbegin();
			//it++;
		} else
			it++;
	}
*/
}
void cDiscoItem_base::Open(cDiscoWindow * owner) { ///< Otwiera element na liœcie...
	if (owner->GetViewType() == cDiscoWindow::vtTree) {
		HTREEITEM ti = (HTREEITEM) GetListItem(owner);
		TreeView_Select(owner->hwndTree , ti, TVGN_FIRSTVISIBLE | TVGN_CARET);
		TreeView_Expand(owner->hwndTree , ti, TVE_EXPAND);
	}
}
void cDiscoItem_base::Refresh(cDiscoWindow * owner) { ///< Odœwie¿a element na liœcie...
	DestroyChildren(owner, 0, true);
	CreateItem(owner , static_cast<cDiscoItem*>(GetParent(owner)));
}

void cDiscoItem_base::CreateItem(cDiscoWindow * owner , cDiscoItem * parent) {
//	owner->jab.GetPlug()->IMDEBUG(DBG_FUNC , "> CreateItem(this=%s, parent=%s)" , this->item->getJID().c_str() , parent ? parent->item->getJID().c_str() : "null");
	LockerCS cl(owner->CS());
	CStdString name = GetName(owner , parent);
	int image = owner->GetIcon(cDiscoWindow::ilList, GetImage(owner));
	int state = owner->GetIcon(cDiscoWindow::ilState, GetState(owner));
	if (owner->GetViewType() == cDiscoWindow::vtTree) {
		HTREEITEM hti = (HTREEITEM) GetListItem(owner);
		TVITEMEX tvi;
		memset(&tvi , 0 , sizeof(tvi));
		tvi.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_TEXT | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvi.hItem = hti;
		tvi.iImage = tvi.iSelectedImage = image;
		tvi.stateMask = INDEXTOSTATEIMAGEMASK(0xF);
		tvi.state = INDEXTOSTATEIMAGEMASK(state);
		/* skoro jest zaznaczony, rozwijamy go */
		if (hti && TreeView_GetSelection(owner->hwndTree) == hti) {
			tvi.state |= TVIS_EXPANDED;
			tvi.stateMask |= TVIS_EXPANDED;
		}
		tvi.pszText = (char*) name.c_str();
		SetItem(owner , parent , &tvi);
		if (!hti) {
			TVINSERTSTRUCT tis;
			memset(&tis , 0 , sizeof(tis));
			tis.hParent = parent ? (HTREEITEM) parent->GetListItem(owner) : NULL;
			tis.hInsertAfter = TVI_LAST;
			tis.itemex = tvi;
			tvi.hItem = hti = TreeView_InsertItem(owner->hwndTree , &tis);
			Register(owner , (HANDLE) hti);
			//owner->jab.GetPlug()->IMDEBUG(DBG_MISC , "- TreeView_InsertItem item=%x parent=%x" , hti , tis.hParent);
		} else {
			TreeView_SetItem(owner->hwndTree , &tvi);
			//owner->jab.GetPlug()->IMDEBUG(DBG_MISC , "- TreeView_SetItem item=%x" , hti);
		}
		SetItemAfter(owner , parent , &tvi);
	} else {
		int index = (int) GetListItem(owner , (HANDLE)0x7FFFFFFF);
		LVITEM lvi;
		memset(&lvi , 0 , sizeof(lvi));
		lvi.mask = LVIF_STATE | LVIF_TEXT | LVIF_IMAGE | LVIF_GROUPID;
		lvi.stateMask = INDEXTOSTATEIMAGEMASK(0xF);
		lvi.state = INDEXTOSTATEIMAGEMASK(state + 1);
		lvi.iImage = image;
		lvi.iItem = index;
		lvi.pszText = (char*)name.c_str();
		lvi.iGroupId = parent ? cDiscoWindow::lgItems : cDiscoWindow::lgThis;
		SetItem(owner , parent , &lvi);
		if (index == 0x7FFFFFFF) {
			lvi.iItem = index = (int) ListView_InsertItem(owner->hwndList , &lvi);
			Register(owner , (HANDLE) lvi.iItem);
		} else {
			ListView_SetItem(owner->hwndList , &lvi);
		}
		if (owner->GetViewType() == cDiscoWindow::vtReport) {
			lvi.mask = LVIF_TEXT;
			CStdString txt;
			txt = GetReportNode(owner);
			lvi.iSubItem = 1;
			lvi.pszText = (char*)txt.c_str();
			ListView_SetItem(owner->hwndList , &lvi);
			txt = GetReportJID(owner);
			lvi.iSubItem = 2;
			lvi.pszText = (char*)txt.c_str();
			ListView_SetItem(owner->hwndList , &lvi);
			txt = GetReportType(owner);
			lvi.iSubItem = 3;
			lvi.pszText = (char*)txt.c_str();
			ListView_SetItem(owner->hwndList , &lvi);
		}
		SetItemAfter(owner , parent , &lvi);

	}
}

void cDiscoItem_base::OnRightClick(cDiscoWindow * owner , HANDLE item , POINT pt) {
    HMENU menu = (HMENU)CreatePopupMenu();
	MENUINFO mif;
	mif.cbSize=sizeof(MENUINFO);
	mif.fMask=MIM_STYLE;
	mif.dwStyle = MNS_CHECKORBMP;
	SetMenuInfo(menu , &mif);

	this->PopupBuild(owner , menu);
	if (GetMenuItemCount(menu) > 0) {
		this->PopupResult(owner , menu ,  TrackPopupMenu(menu,TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, owner->hwnd, 0));
	}
	DestroyMenu(menu);
}
void cDiscoItem_base::OnSelect(cDiscoWindow * owner , HANDLE item) {
	RichEditFormat f(owner->hwndInfo);
	f.clear();
	owner->SetInfoType(f, this->GetReportType(owner));
	//owner->SetInfo(this->GetInfo(owner));
	this->FillInfo(owner, f);
}

void cDiscoItem_base::InsertMenuItem(HMENU menu, int id, const CStdString & title, int icon, int pos, int type, int state, HMENU submenu) {
    MENUITEMINFO mi;
    mi.cbSize=sizeof(MENUITEMINFO);
    mi.fMask=MIIM_DATA|MIIM_STATE|MIIM_ID|MIIM_FTYPE;
	if (type != MFT_SEPARATOR) {
		mi.fMask|=MIIM_STRING | MIIM_BITMAP;
		mi.dwTypeData=(char *)title.c_str();
        mi.hbmpItem=HBMMENU_CALLBACK;
		mi.dwItemData = icon;
	}
	if (submenu) {
		mi.fMask |= MIIM_SUBMENU;
		mi.hSubMenu = submenu;
	}
	mi.fType=type;
	mi.fState=state;
    mi.cch=0;
	mi.wID = id;
	::InsertMenuItem(menu , pos == -1 ? GetMenuItemCount(menu) : pos , 1 , &mi);
}


void cDiscoItem_base::PopupBuild(cDiscoWindow * owner , HMENU menu) { 
	cDiscoWindow::enViewType vt = owner->GetViewType();
	MENUINFO mif;
	mif.cbSize=sizeof(MENUINFO);
	mif.fMask=MIM_STYLE;
	mif.dwStyle = MNS_CHECKORBMP;
	HMENU view = CreatePopupMenu();
	SetMenuInfo(view , &mif);

	if (GetMenuItemCount(menu))
		AppendMenu(menu , MF_SEPARATOR , 0 , 0);
	AppendMenu(view , MF_STRING | ( vt == cDiscoWindow::vtTree ? MF_CHECKED : 0 ) , idView | cDiscoWindow::vtTree , "Drzewko");
	AppendMenu(view , MF_STRING | ( vt == cDiscoWindow::vtList ? MF_CHECKED : 0 ) , idView | LVS_LIST , "Lista");
	AppendMenu(view , MF_STRING | ( vt == cDiscoWindow::vtIcon ? MF_CHECKED : 0 ) , idView | LVS_ICON , "Du¿e ikony");
	AppendMenu(view , MF_STRING | ( vt == cDiscoWindow::vtSmallIcon ? MF_CHECKED : 0 ) , idView | LVS_SMALLICON , "Ma³e ikony");
	AppendMenu(view , MF_STRING | ( vt == cDiscoWindow::vtReport ? MF_CHECKED : 0 ) , idView | LVS_REPORT , "Szczegó³y");
	if (owner->Jab().GetPlug()->ICMessage(IMC_ISWINXP))
		InsertMenuItem(view , idCfgViewGrouping , "Grupuj", 0, -1, MFT_STRING, MFS_ENABLED | (owner->Jab().GetPlug()->DTgetInt(DTCFG, 0, CFG::discoViewGrouping)) ? MF_CHECKED : 0);	
	AppendMenu(menu , MF_POPUP | MF_STRING , (UINT_PTR) view , "Widok");
	HMENU options = CreatePopupMenu();
	SetMenuInfo(options , &mif);

	InsertMenuItem(options , idCfgReadAll , "Wczytuj wszystkie", 0, -1, MFT_STRING, MFS_ENABLED | (owner->Jab().GetPlug()->DTgetInt(DTCFG, 0, CFG::discoReadAll)) ? MF_CHECKED : 0);
	InsertMenuItem(options , idCfgShowFeatures , "Pokazuj cechy", 0, -1, MFT_STRING, MFS_ENABLED | (owner->Jab().GetPlug()->DTgetInt(DTCFG, 0, CFG::discoShowFeatures)) ? MF_CHECKED : 0);
	InsertMenuItem(options , idCfgInfo , "Poka¿ informacjê", 0, -1, MFT_STRING, MFS_ENABLED | (owner->Jab().GetPlug()->DTgetInt(DTCFG, 0, CFG::discoInfo)) ? MF_CHECKED : 0);
	InsertMenuItem(menu , 0 , "Opcje", IDI_TB_OPTIONS, -1, MFT_STRING, MFS_ENABLED, options);
	InsertMenuItem(options , idClear , "Zresetuj bazê", IDI_TB_REFRESH);

}
void cDiscoItem_base::PopupResult(cDiscoWindow * owner , HMENU menu , int id) {
	if ((id & ~0xFFF) == idView) {
		owner->SetViewType((cDiscoWindow::enViewType) (id & 0xFFF));
	} else if (id == idCfgReadAll) {
		owner->Jab().GetPlug()->DTsetInt(DTCFG, 0, CFG::discoReadAll, !owner->Jab().GetPlug()->DTgetInt(DTCFG, 0, CFG::discoReadAll));
		owner->Refresh();
	} else if (id == idCfgShowFeatures) {
		owner->Jab().GetPlug()->DTsetInt(DTCFG, 0, CFG::discoShowFeatures, !owner->Jab().GetPlug()->DTgetInt(DTCFG, 0, CFG::discoShowFeatures));
		owner->Refresh();
	} else if (id == idCfgInfo) {
		owner->Jab().GetPlug()->DTsetInt(DTCFG, 0, CFG::discoInfo, !owner->Jab().GetPlug()->DTgetInt(DTCFG, 0, CFG::discoInfo));
		owner->SetSize();
	} else if (id == idCfgViewGrouping) {
		owner->Jab().GetPlug()->DTsetInt(DTCFG, 0, CFG::discoViewGrouping, !owner->Jab().GetPlug()->DTgetInt(DTCFG, 0, CFG::discoViewGrouping));
		ListView_EnableGroupView(owner->hwndList, owner->Jab().GetPlug()->DTgetInt(DTCFG, 0, CFG::discoViewGrouping));
	} else if (id == idClear) {
		owner->Clear(true);
	}

}	



// --------------------------------------------------------------
// Item 
cDiscoItem::cDiscoItem(cDiscoWindow * owner , cDiscoItem * parent , const DiscoDB::Item * item, bool createItem): cDiscoItem_base(owner) {
	this->item = item;
	if (createItem)
		CreateItem(owner , parent);
}
void cDiscoItem::Open(cDiscoWindow * owner) { ///< Otwiera element na liœcie...
	CStdString location = item->getJID();
	if (!item->getNode().empty()) location += "::" + item->getNode();
	SetWindowText(owner->hwndLocation , location);
	cDiscoItem_base::Open(owner);
}
CStdString cDiscoItem::GetName(cDiscoWindow * owner , cDiscoItem * parent) {
	return item->getName().empty() ? item->getJID() : UTF8ToLocale(item->getName());
}
void cDiscoItem::FillInfo(cDiscoWindow * owner, RichEditFormat & f) {
	owner->SetInfoName(f, GetName(owner, 0));
	if (item->getJID().empty() == false) {
		owner->SetInfoJID(f, item->getJID() + (item->getNode().empty()?"": "::" + item->getNode()));
	}
	if (item->getIdentityList().empty() == false) {
		owner->SetInfoSection(f, "Rodzaj");
		for (DiscoDB::Item::IdentityList::const_iterator it = item->getIdentityList().begin(); it != item->getIdentityList().end(); ++it) {
			owner->SetInfoName(f, TranslateIdentity((*it)->getCategory(), (*it)->getType()));
			owner->SetInfoExpl(f, ExplainIdentity((*it)->getCategory(), (*it)->getType()));
		}
	}
	if (item->getFeatureList().empty() == false) {
		owner->SetInfoSection(f, "Cechy");
		for (DiscoDB::Item::FeatureList::const_iterator it = item->getFeatureList().begin(); it != item->getFeatureList().end(); ++it) {
			owner->SetInfoName(f, TranslateFeature(*it));
			owner->SetInfoExpl(f, ExplainFeature(*it));
		}
	}
	if (item->getState() & DiscoDB::Item::stQuerying) {
		owner->SetInfoState(f, "Zapytanie zosta³o wys³ane, czekam na odpowiedŸ.");
	} else if (item->getState() & DiscoDB::Item::stError) {
		owner->SetInfoState(f, "Wyst¹pi³ b³¹d.");
	} else if (item->getState() == DiscoDB::Item::stReady) {
		/* nic */
	} else if (item->getState() & DiscoDB::Item::stInfo) {
		owner->SetInfoState(f, "¯eby zobaczyæ podelementy, odœwie¿ informacje o tej us³udze.");
	} else if (item->getState() == DiscoDB::Item::stNone) {
		owner->SetInfoState(f, "Informacje nie by³y pobierane.");
	}
}

void cDiscoItem::CreateItem(cDiscoWindow * owner , cDiscoItem * parent) {
	LockerCS cl(owner->CS());
	cDiscoItem_base::CreateItem(owner , parent);
	/* musimy upewniæ siê, czy nie zrobimy pêtli... */
/*	if (owner->GetViewType() == cDiscoWindow::vtTree) {
		cDiscoItem_base * parent = this;
		while ((parent = parent->GetParent(owner)))
			if (*parent == this->item) { // sprawdzamy, czy oba maj¹ ten sam item...
				return;
			}
	}*/
	// Tworzymy wszystkie dzieci... Gdy jest drzewko, lub tworzymy dzieci pierwszego poziomu...
	// Je¿eli element jest dopiero czytany, dzieci nie pokazujemy...
	bool createChildren = false;
	if (owner->GetViewType() == cDiscoWindow::vtTree) {
		//IMDEBUG(DBG_MISC, "ItemState [%s][%x] = %d [%x]", item->getJID().c_str(),GetListItem(owner),  TreeView_GetItemState(owner->hwndTree, GetListItem(owner), TVIS_EXPANDED), TreeView_GetItemState(owner->hwndTree, GetListItem(owner), -1));
		if (TreeView_GetItemState(owner->hwndTree, GetListItem(owner), TVIS_EXPANDED) & TVIS_EXPANDED)
			createChildren = true;
	} else if (!parent)
		createChildren = true;
	if (createChildren && !(item->getState() & DiscoDB::Item::stQuerying)) {
		this->CreateChildren(owner);
	}
}
int cDiscoItem::GetImage(cDiscoWindow * owner) {
	int icon = owner->Jab().GetCntIcon(ST_ONLINE, item->getJID());
	return icon ? icon : IDI_SERVICE;
}
int cDiscoItem::GetState(cDiscoWindow * owner) {
	if (this->item->getState() & DiscoDB::Item::stQuerying)
		return IDI_STATE_QUERYING;
	else if (this->item->getState() & DiscoDB::Item::stError)
		return IDI_STATE_ERROR;
	else if (this->item->getState() == DiscoDB::Item::stReady)
		return IDI_STATE_READY;
	else
		return IDI_STATE_NONE;
}
CStdString cDiscoItem::GetReportType(cDiscoWindow * owner) {
	if (item->getIdentityList().size()) {
		CStdString id = TranslateIdentity(item->getIdentityList().front()->getCategory(), item->getIdentityList().front()->getType());
		if (!id.empty())
			return id;
	}
	return item->getNode().empty() ? "us³uga" : "wêze³";
}

void cDiscoItem::SetItem(cDiscoWindow * owner , cDiscoItem * parent , HANDLE hItem) {
	if (owner->GetViewType() == cDiscoWindow::vtTree) {
		TVITEMEX & tvi = *(TVITEMEX*)hItem;
		if (this->item->getState() == DiscoDB::Item::stReady) 
			tvi.state |= TVIS_BOLD;
		tvi.stateMask |= TVIS_BOLD;
		if (!(this->item->getState() & DiscoDB::Item::stQuerying)) {
			tvi.cChildren = (this->item->getState() != DiscoDB::Item::stReady ||
				item->hasChildren() 
				|| (owner->Jab().GetPlug()->DTgetInt(DTCFG, 0, CFG::discoShowFeatures) && (!item->getFeatureList().empty()	|| !item->getIdentityList().empty()))
				) ? 1 : 0;
		}
	} else {
	}
}

void cDiscoItem::CreateChildren(cDiscoWindow * owner) {
	LockerCS cl(owner->CS());
	bool readChildren = owner->Jab().GetPlug()->DTgetInt(DTCFG, 0, CFG::discoReadAll) != 0;
	if (!(item->getState() & DiscoDB::Item::stInfo))
		return;
	for (DiscoDB::Item::const_iterator it = item->begin(); it != item->end(); it++) {
		if ((*it)->getJID().empty())
			continue; /*HACK: Puste elementy powinny byc przeszloscia...*/
		if (readChildren && (*it)->getState() == DiscoDB::Item::stNone) {
			owner->ReadJID((*it)->getJID(), (*it)->getNode());
		} else {
			new cDiscoItem(owner , this , *it);
		}
	}

	if (owner->Jab().GetPlug()->DTgetInt(DTCFG, 0, CFG::discoShowFeatures)) {
		// Tworzymy listê identities i features
		for (DiscoDB::Item::IdentityList::const_iterator it = item->getIdentityList().begin(); it != item->getIdentityList().end(); ++it) {
			new cDiscoItemIdentity(owner , this , *it);
		}

		for (DiscoDB::Item::FeatureList::const_iterator it = item->getFeatureList().begin(); it != item->getFeatureList().end(); ++it) {
			new cDiscoItemFeature(owner , this , *it);
		}
	}

}



void cDiscoItem::OnExpand(cDiscoWindow * owner , HANDLE item) {
	HTREEITEM ti = (HTREEITEM) GetListItem(owner);
	TreeView_Select(owner->hwndTree , ti, TVGN_FIRSTVISIBLE | TVGN_CARET);
	//TreeView_Expand(owner->hwndTree , ti, TVE_EXPAND);
	if (this->item->getState() == DiscoDB::Item::stReady) {
		if (TreeView_GetChild(owner->hwndTree, ti) == NULL) {
			CreateChildren(owner);
		}
	} else { // ³adujemy...
		owner->ReadJID(this->item->getJID() , this->item->getNode());
	}

}
void cDiscoItem::OnDoubleClick(cDiscoWindow * owner , HANDLE hItem) {
	if (owner->GetViewType() != cDiscoWindow::vtTree) {
		owner->OpenJID(item->getJID() , item->getNode());
	}
}

void cDiscoItem::PopupBuild(cDiscoWindow * owner , HMENU menu) {
	InsertMenuItem(menu, idOpen, "Otwórz", IDI_TB_GO);
	if (item->getState() == DiscoDB::Item::stNone) 
		InsertMenuItem(menu, idRefresh, "Wczytaj", IDI_STATE_QUERYING);
	else
		InsertMenuItem(menu, idRefresh, "Odœwie¿", IDI_TB_REFRESH);
	if (owner->Jab().FindContact(item->getJID()) == -1 && item->getNode().empty())
		InsertMenuItem(menu, idAddToList, "Dodaj do listy", IDI_TB_ADD);
	AppendMenu(menu, MF_SEPARATOR, 0, 0);
	bool ready = item->getState() == DiscoDB::Item::stReady;
	if (!ready || HasFeature("jabber:iq:register")) {
		InsertMenuItem(menu, idRegister, "Rejestracja", ready ? IDI_MENU_REGISTER : IDI_STATE_NONE);
	}
	if (!ready || HasFeature("jabber:iq:search")) {
		InsertMenuItem(menu, idSearch, "Szukaj", ready ? IDI_MENU_SEARCH : IDI_STATE_NONE);
	}
	if (HasIdentity("conference")) {
		InsertMenuItem(menu, idEnterConference, "Do³¹cz do konferencji", ready ? IDI_CONFERENCE : IDI_STATE_NONE);
	}

	cDiscoItem_base::PopupBuild(owner , menu);

}

void cDiscoItem::PopupResult(cDiscoWindow * owner , HMENU menu , int id) {
	if (id == idRefresh) {
		owner->ReadJID(item->getJID(), item->getNode(), true);
	} else if (id == idAddToList) {
		cCtrl * ctrl = owner->Jab().GetPlug();
		int id = ctrl->ICMessage(IMC_CNT_ADD, owner->Jab().GetNet(), (int)item->getJID().c_str());
		if (id == -1)
			return;
		ctrl->DTsetStr(DTCNT, id, CNT_GROUP, ctrl->DTgetStr(DTCFG, 0, CFG_CURGROUP));
		ctrl->DTsetStr(DTCNT, id, CNT_DISPLAY, UTF8ToLocale(item->getName()).c_str());
		ctrl->ICMessage(IMC_CNT_CHANGED, id);
	} else if (id == idRegister) {
		new cFeatureIQRegister(owner->Jab(), owner->GetHWND(), item->getJID(), "Rejestracja - " + this->GetName(owner, 0));
		return;
	} else if (id == idSearch) {
		new cFeatureIQSearch(owner->Jab(), owner->GetHWND(), item->getJID(), "Wyszukiwanie - " + this->GetName(owner, 0));
		return;
	} else if (id == idEnterConference) {
		/* ...Konferencja... */
		CStdString room = JID::getUser(item->getJID());
		CStdString nick, jid;
		if (room.empty()) {
			sDIALOG_enter sde;
			sde.id = "kJabber/conference/room";
			sde.title = "Nazwa pokoju";
			CStdString info = "Wpisz nazwê pokoju do którego chcesz do³¹czyæ na " + item->getJID();
			sde.info = info;
			sde.handle = (void*)owner->hwnd;
			if (!owner->Jab().GetPlug()->ICMessage(IMI_DLGENTER, (int) &sde) || !sde.value || !*sde.value)
				return;
			room = sde.value;
		}
		sDIALOG_enter sde;
		sde.id = "kJabber/conference/nick";
		sde.title = "Twój nick";
		CStdString info = "Wpisz pseudonim którego chcesz u¿ywaæ na " + room + "@" + CStdString( JID::getHost(item->getJID()) );
		sde.info = info;
		sde.handle = (void*)owner->hwnd;
		if (!owner->Jab().GetPlug()->ICMessage(IMI_DLGENTER, (int) &sde) || !sde.value || !*sde.value)
			return;
		nick = sde.value;
		jid = room + '@' + CStdString( JID::getHost(item->getJID()) ) + "/" + nick;
		int cnt = owner->Jab().GetPlug()->ICMessage(IMC_CNT_FIND, owner->Jab().GetNet(), (int)jid.c_str());
		if (cnt == -1) { // dodajemy kontakt...
			cnt = owner->Jab().GetPlug()->ICMessage(IMC_CNT_ADD, owner->Jab().GetNet(), (int)jid.c_str());
			owner->Jab().GetPlug()->DTsetStr(DTCNT, cnt, CNT_GROUP, owner->Jab().GetPlug()->DTgetStr(DTCFG, 0, CFG_CURGROUP));
			owner->Jab().GetPlug()->DTsetInt(DTCNT, cnt, CNT_STATUS, ST_NOTINLIST, ST_NOTINLIST);
			owner->Jab().GetPlug()->ICMessage(IMC_CNT_CHANGED, cnt);
		}
		// otwieramy okno rozmowy...
		owner->Jab().GetPlug()->ICMessage(IMI_ACTION_CALL, (int)&sUIActionNotify_2params(sUIAction(IMIG_CNT, IMIA_CNT_MSG, cnt), ACTN_ACTION, 0, 0));
		// wrzucamy informacjê do okienka...
		cMessage m;
		m.body = "Konferencje w <b>kJabberze</b> dzia³aj¹ jak na razie tylko w zgodzie z czêœci¹ standardu <i>groupchat</i>."
			" Oznacza to, ¿e mo¿emy braæ udzia³ <b>tylko</b> we wspólnej dyskusji i na dzieñ dzisiejszy nie mo¿na obejrzeæ listy uczestników.<br/><br/>"
			"¯eby do³¹czyæ do konferencji nale¿y wys³aæ jej swój status, oraz 'ukryæ' siê ¿eby j¹ opuœciæ. "
				"Mo¿emy to zrobiæ z menu kontaktu-konferencji (lub oknie <i>wiêcej</i>) pod pozycj¹ <b>status</b>. "
				"Zaznaczaj¹c opcjê <i>wysy³aj status</i> wejdziemy do pokoju za ka¿dym razem, gdy staniemy siê dostêpni.<br/><br/>"
				"Do pokoji mo¿emy siê dostaæ z <i>us³ug</i> lub dodaj¹c do listy kontakt o jidzie w postaci:<br/>"
				"<b>pokój</b>@JID.us³ugi/<b>nasz_nick</b> - np. konnekt@conference.jabber.org/Jan_Kowalski.";
		m.flag = MF_HANDLEDBYUI | MF_HTML;
		m.net = owner->Jab().GetNet();
		m.fromUid = (char*)jid.c_str();
		m.type = MT_QUICKEVENT;
		sMESSAGESELECT ms;
		if ((ms.id = owner->Jab().GetPlug()->ICMessage(IMC_NEWMESSAGE, (int)&m))>0)
			owner->Jab().GetPlug()->ICMessage(IMC_MESSAGEQUEUE, (int) &ms);

		// wysy³amy status
		owner->Jab().SendPresence(jid, -1 , 0);
	} else if (id == idOpen) {
		owner->OpenJID(this->item);
		return;
	}
	cDiscoItem_base::PopupResult(owner , menu , id);
}
bool cDiscoItem::HasIdentity(const CStdString & cat, const CStdString & type) {
	for (DiscoDB::Item::IdentityList::const_iterator it = item->getIdentityList().begin(); it != item->getIdentityList().end(); ++it) {
		if ((cat.empty() || (*it)->getCategory() == cat) && (type.empty() || (*it)->getType() == type))
			return true;
	}
	return false;
}

// --------------------------------------------------------------
// Root
cDiscoItemRoot::cDiscoItemRoot(cDiscoWindow * owner):cDiscoItem(owner , 0 , owner->rootItem, false) {
	CreateItem(owner , 0);
}
void cDiscoItemRoot::CreateItem(cDiscoWindow * owner , cDiscoItem * parent) {
		CreateChildren(owner);
		Register(owner , 0);
}

//------------------------------------------------------------------
// PROPS
cDiscoItemProp::cDiscoItemProp(cDiscoWindow * owner , cDiscoItem * parent):cDiscoItem_base(owner) {
}

bool cDiscoItemProp::IsChildOf(cDiscoWindow * owner , const jabberoo::DiscoDB::Item * item) {
	return *GetParent(owner) == item;
}
void cDiscoItemProp::SetItem(cDiscoWindow * owner , cDiscoItem * parent , HANDLE item) {
	if (owner->GetViewType() != cDiscoWindow::vtTree) {
		LVITEM * lvi = (LVITEM*)item;
		lvi->iGroupId = cDiscoWindow::lgFeatures;
	}
}


cDiscoItemIdentity::cDiscoItemIdentity(cDiscoWindow * owner ,  cDiscoItem * parent , DiscoDB::Item::Identity * identity):cDiscoItemProp(owner, parent), identity(*identity) {
	CreateItem(owner , parent);
}
CStdString cDiscoItemIdentity::GetName(cDiscoWindow * owner , cDiscoItem * parent) {
	return TranslateIdentity(UTF8ToLocale(identity.getCategory()), UTF8ToLocale(identity.getType()));
}
void cDiscoItemIdentity::FillInfo(cDiscoWindow * owner, RichEditFormat & f) {
	owner->SetInfoName(f, GetName(owner, 0));
	owner->SetInfoName(f, TranslateIdentity(identity.getCategory(), identity.getType()));
	owner->SetInfoExpl(f, ExplainIdentity(identity.getCategory(), identity.getType()));
}

cDiscoItemFeature::cDiscoItemFeature(cDiscoWindow * owner , cDiscoItem * parent , const CStdString & feature):cDiscoItemProp(owner, parent), feature(feature) {
	CreateItem(owner , parent);
}
CStdString cDiscoItemFeature::GetName(cDiscoWindow * owner , cDiscoItem * parent) {
	return TranslateFeature(UTF8ToLocale(feature));
}
void cDiscoItemFeature::FillInfo(cDiscoWindow * owner, RichEditFormat & f) {
	CStdString name = GetName(owner, 0);
	owner->SetInfoName(f, name);
	if (name != feature)
		f.insertText(feature + "\r\n");
	owner->SetInfoExpl(f, ExplainFeature(UTF8ToLocale(feature)));
}

void cDiscoItemFeature::PopupBuild(cDiscoWindow * owner , HMENU menu) {
	cDiscoItem_base::PopupBuild(owner , menu);
	InsertMenuItem(menu , idXData, "Negocjuj");
}
void cDiscoItemFeature::PopupResult(cDiscoWindow * owner , HMENU menu , int id) {
	cDiscoItem_base::PopupResult(owner , menu , id);
	if (id == idXData) {
		new cFeatureNeg(owner->Jab(), owner->GetHWND(), this->GetJID(owner), this->feature, "XData: " + this->feature);
	}
}

