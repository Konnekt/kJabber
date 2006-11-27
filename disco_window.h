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

#ifdef _DEBUG
#define __OUTPUT(txt) OutputDebugString(txt " (" __FUNCTION__ " :: " __FILE__ ") \n");
#else
#define __OUTPUT(txt)
#endif

namespace kJabber {
	namespace JEP {
		CStdString TranslateIdentity(const CStdString & cat, const CStdString & type = "");
		CStdString ExplainIdentity(const CStdString & cat, const CStdString & type = "");
		CStdString TranslateFeature(const CStdString & f);
		CStdString ExplainFeature(const CStdString & f);

		extern list<class cDiscoWindow *> currentWindows;

class cDiscoWindow: public SigC::Object {
	friend class cDiscoItem_base;
	friend class cDiscoItem;
	friend class cDiscoItemRoot;
public:
	enum enViewType {
		vtNone = 0xFF , 
		vtIcon = LVS_ICON , vtSmallIcon = LVS_SMALLICON ,
		vtList = LVS_LIST , vtReport = LVS_REPORT ,
		vtTree = 0x80
	};
	enum enImageList {
		ilState, ilUI, ilList
	};
	enum enListGroup {
		lgThis, lgItems, lgFeatures
	};
	typedef multimap<HANDLE , class cDiscoItem_base *> tListItems;
	typedef std::pair<CStdString, CStdString> tHistoryItem;
	typedef vector<tHistoryItem> tHistory;

	cDiscoWindow(cJabber & jab ,  CStdString jid = "");
	~cDiscoWindow();
	void Clear(bool reloadDisco); ///< Czyœci listê wyników
	void Refresh();
	void ReadJID(CStdString jid , CStdString node = "", bool refresh = false); ///< Wczytuje JID'a (tworzy/aktualizuje wpis w drzewku jeœli jest taka potrzeba...)
	void ReadJID(const DiscoDB::Item * item, bool refresh = false) {ReadJID(item->getJID(), item->getNode(), refresh);} ///< Otwiera wybrany JID/node
	void OpenJID(CStdString jid , CStdString node = ""); ///< Otwiera wybrany JID/node
	void OpenJID(const DiscoDB::Item * item) {OpenJID(item->getJID(), item->getNode());} ///< Otwiera wybrany JID/node
	tListItems::iterator begin() {return listItems.begin();}
	tListItems::iterator end() {return listItems.end();}
	tListItems::reverse_iterator rbegin() {return listItems.rbegin();}
	tListItems::reverse_iterator rend() {return listItems.rend();}
	void SetViewType(enViewType vt);
	enViewType GetViewType() {return viewType;}
	HWND GetHWND() {return hwnd;}
	DiscoDB & DiscoDB() {return jab.GetSession().discoDB();}
	cJabber & Jab() {return jab;}
	int GetIcon(enImageList l, int id) {return il[l].GetIndex(id);}
	void Lock() {cs.lock();}
	void Unlock() {cs.unlock();}
	Stamina::Lock & CS() {return cs;}
	void ActivateWindow();

	void SetInfoType(RichEditFormat & f, const CStdString & item);
	void SetInfoName(RichEditFormat & f, const CStdString & name);
	void SetInfoJID(RichEditFormat & f, const CStdString & jid);
	void SetInfoExpl(RichEditFormat & f, const CStdString & txt);
	void SetInfoSection(RichEditFormat & f, const CStdString & title);
	void SetInfoState(RichEditFormat & f, const CStdString & name);

private:

	static LRESULT WndProc(HWND hwnd , UINT message , WPARAM wParam , LPARAM lParam);
	void BuildUI();
	void SetSize();
	void ListItemsClear(); ///< Usuwa elementy listy...
	void ListItemsBuild(); ///< Wype³nia listê od podstaw...
	void CacheCallback(const DiscoDB::Item* item);
	struct sCacheCallbackAPC {
		cDiscoWindow * owner;
		const DiscoDB::Item* item;
		bool done;
	};
	static void CALLBACK CacheCallbackAPC(sCacheCallbackAPC *);
	cDiscoItem * RefreshVisible(const DiscoDB::Item * item);
	cDiscoItem * FindOnList(const DiscoDB::Item * item);
	cDiscoWindow::tListItems::iterator FindOnList(const cDiscoItem_base * item);
	DiscoDB::Item * FindParent(const DiscoDB::Item * item);
	cDiscoItem_base * GetListObject(HANDLE item);	
	void OpenJID(tHistoryItem & i) {OpenJID(i.first , i.second);} ///< Otwiera wybrany JID/node
	void LocationHistoryFill();
	void LocationHistorySave(const CStdString & location);
	void SetInfo(const CStdString & info);
	void SetButtons();
	DiscoDB::Item * CreateRootItem();

	tListItems listItems; // elementy aktualnie wyœwietlanej listy
	tHistory history;
	tHistory::iterator historyPosition;

	HWND hwnd;    
	HWND hwndTree;
	HWND hwndTreeHeader;
	HWND hwndInfo;
	HWND hwndList;
	HWND hwndLocation;
	HWND hwndToolbar;
	HWND hwndStatus;
	enViewType viewType;
	cJabber & jab;
	DiscoDB::Item * currentItem;
	DiscoDB::Item * rootItem;
	CStdString rootJID; // 
	CriticalSection_blank cs;

	// listy ikon

	typedef map <int, int> tILIndex;
	class cIL{
	public:
		cIL(int size = 0);
		cIL & operator = (cIL & il);
		~cIL();
		int GetIndex(int id);
		void LoadIcon(int id);
		void CopyUIIcon(cJabber & jab, int id);
		HIMAGELIST GetList() {
			return list;
		}
		int GetSize();
	private:
		tILIndex index;
		HIMAGELIST list;
	};
	cIL il [3];
};

class cDiscoItem_base {
public:
	enum enType {
		tItem = 0xF , tRoot = 0xE , tFeature = 0x10 , tIdentity = 0x20
	};
	enum enID {
		idRefresh = 0x10,
		idAddToList = 0x11,
		idOpen = 0x12,
		idCfgReadAll = 0x20,
		idCfgShowFeatures = 0x21,
		idCfgInfo = 0x22,
		idCfgViewGrouping = 0x23,
		idClear = 0x30,

		idXData = 0x100,
		idRegister = 0x101,
		idSearch = 0x102,
		idEnterConference = 0x103,


		idView = 0x1000,


	};
	virtual void OnExpand(cDiscoWindow * owner , HANDLE item) {}
	virtual void OnDoubleClick(cDiscoWindow * owner , HANDLE item) {}
	virtual void OnRightClick(cDiscoWindow * owner , HANDLE item , POINT pt);
	virtual void OnSelect(cDiscoWindow * owner , HANDLE item);

	virtual void PopupBuild(cDiscoWindow * owner , HMENU menu); ///< Dodaje pozycje do menu
	virtual void PopupResult(cDiscoWindow * owner , HMENU menu , int id); ///< Przekazuje identyfikator wybranej pozycji menu
	static void InsertMenuItem(HMENU menu, int id, const CStdString & title, int icon = 0, int pos = -1, int type = MFT_STRING, int state = MFS_ENABLED, HMENU submenu = 0);

	virtual void Open(cDiscoWindow * owner); ///< Otwiera element na liœcie...
	virtual void Refresh(cDiscoWindow * owner); ///< Odœwie¿a element na liœcie...

	void Register(cDiscoWindow * owner , HANDLE item);
	void Unregister(cDiscoWindow * owner);
	HANDLE GetListItem(cDiscoWindow * owner , HANDLE def = NULL) const;
	virtual void CreateItem(cDiscoWindow * owner , cDiscoItem * parent);
	virtual void SetItem(cDiscoWindow * owner , cDiscoItem * parent , HANDLE item) {}
	virtual void SetItemAfter(cDiscoWindow * owner , cDiscoItem * parent , HANDLE item) {}
//	virtual int GetInsertionPoint(cDiscoWindow * owner , cDiscoItem * parent) {return TVI_LAST;}
	virtual CStdString GetName(cDiscoWindow * owner , cDiscoItem * parent)=0;
	virtual void FillInfo(cDiscoWindow * owner, RichEditFormat & f) {}
	virtual CStdString GetJID(cDiscoWindow * owner) {return "";}
	virtual int GetImage(cDiscoWindow * owner) {return IDI_SERVICE;}
	virtual int GetState(cDiscoWindow * owner) {return IDI_STATE_READY;}
	virtual CStdString GetReportNode(cDiscoWindow * owner) {return "";}
	virtual CStdString GetReportJID(cDiscoWindow * owner) {return "";}
	virtual CStdString GetReportType(cDiscoWindow * owner) {return "";}

	virtual void CreateChildren(cDiscoWindow * owner) {}
	virtual void DestroyChildren(cDiscoWindow * owner, cDiscoItem_base * parent = 0, bool removeItems = false);

	cDiscoItem_base(cDiscoWindow * owner);
	virtual ~cDiscoItem_base() {}
	virtual void Destroy(cDiscoWindow * owner, cDiscoItem_base * parent = 0);

	//virtual bool IsItem(const CStdString & jid , const CStdString & node)=0;
	virtual bool IsChildOf(cDiscoWindow * owner , const jabberoo::DiscoDB::Item * item)=0;

	virtual bool IsChildOf(cDiscoWindow * owner , const cDiscoItem_base * item);
	virtual cDiscoItem_base * GetParent(cDiscoWindow * owner);
	virtual bool operator==(const jabberoo::DiscoDB::Item * item) {return false;}
	bool operator!=(const jabberoo::DiscoDB::Item * item) {return !(*this == item);}

	virtual enType GetType()=0;
protected:
};

class cDiscoItem: public cDiscoItem_base { // normalny element...
public:


	cDiscoItem(cDiscoWindow * owner , cDiscoItem * parent , const DiscoDB::Item * item, bool createItem = true);
	void Open(cDiscoWindow * owner); ///< Otwiera element na liœcie...
	void CreateItem(cDiscoWindow * owner , cDiscoItem * parent);
	void SetItem(cDiscoWindow * owner , cDiscoItem * parent , HANDLE item);
	void CreateChildren(cDiscoWindow * owner);

	void OnDoubleClick(cDiscoWindow * owner , HANDLE item);

	void PopupBuild(cDiscoWindow * owner , HMENU menu);
	void PopupResult(cDiscoWindow * owner , HMENU menu , int id);

	CStdString GetName(cDiscoWindow * owner , cDiscoItem * parent);
	void FillInfo(cDiscoWindow * owner, RichEditFormat & f);
	CStdString GetJID(cDiscoWindow * owner) {return item->getJID();}
	int GetImage(cDiscoWindow * owner);
	int GetState(cDiscoWindow * owner);
	CStdString GetReportNode(cDiscoWindow * owner) {return item->getNode();}
	CStdString GetReportJID(cDiscoWindow * owner) {return item->getJID();}
	CStdString GetReportType(cDiscoWindow * owner);

	void OnExpand(cDiscoWindow * owner , HANDLE item);
	const DiscoDB::Item * GetItem() {return item;}
	//bool IsItem(const CStdString & jid , const CStdString & node);
	bool IsChildOf(cDiscoWindow * owner , const jabberoo::DiscoDB::Item * item) {return item->hasChild(this->item);}
	bool operator==(const jabberoo::DiscoDB::Item * item) {return this->item == item;}
	enType GetType() {return tItem;}
	bool HasFeature(const CStdString & feat) {
		return std::find(item->getFeatureList().begin(), this->item->getFeatureList().end(), feat) != item->getFeatureList().end(); 
	}
	bool HasIdentity(const CStdString & cat, const CStdString & type = "");
protected:
	const DiscoDB::Item * item;
};
class cDiscoItemRoot: public cDiscoItem { // root...
public:
	cDiscoItemRoot(cDiscoWindow * owner);
	void CreateItem(cDiscoWindow * owner , cDiscoItem * parent);
	bool IsChildOf(cDiscoWindow * owner , const jabberoo::DiscoDB::Item * item) {return false;}
	cDiscoItem_base * GetParent(cDiscoWindow * owner) {return 0;}
	void PopupBuild(cDiscoWindow * owner , HMENU menu) {cDiscoItem_base::PopupBuild(owner, menu);}

	//bool IsItem(const CStdString & jid , const CStdString & node) {return jid.empty() && node.empty();}
	bool operator==(const jabberoo::DiscoDB::Item * item) {return item->getJID().empty();}
	enType GetType() {return tRoot;}
};

class cDiscoItemProp: public cDiscoItem_base {
public:
	cDiscoItemProp(cDiscoWindow * owner , cDiscoItem * parent);
	bool IsChildOf(cDiscoWindow * owner , const jabberoo::DiscoDB::Item * item);
	bool operator==(const jabberoo::DiscoDB::Item * item) {return false;}
	CStdString GetJID(cDiscoWindow * owner) {return GetParent(owner)->GetJID(owner);}
	int GetState(cDiscoWindow * owner) {return IDI_STATE_READY;}
	void SetItem(cDiscoWindow * owner , cDiscoItem * parent , HANDLE item);
};

class cDiscoItemIdentity: public cDiscoItemProp {
public:
	cDiscoItemIdentity(cDiscoWindow * owner , cDiscoItem * parent , DiscoDB::Item::Identity * identity);
	CStdString GetName(cDiscoWindow * owner , cDiscoItem * parent);
	void FillInfo(cDiscoWindow * owner, RichEditFormat & f);
	enType GetType() {return tIdentity;}
	int GetImage(cDiscoWindow * owner) {return IDI_IDENTITY;}
	CStdString GetReportJID(cDiscoWindow * owner) {return UTF8ToLocale(identity.getCategory()) + " :: " + UTF8ToLocale(identity.getType());}
	CStdString GetReportType(cDiscoWindow * owner) {return "rodzaj";}
protected:
	DiscoDB::Item::Identity identity;
};

class cDiscoItemFeature: public cDiscoItemProp {
public:
	cDiscoItemFeature(cDiscoWindow * owner , cDiscoItem * parent , const CStdString & feature);
	CStdString GetName(cDiscoWindow * owner , cDiscoItem * parent);
	enType GetType() {return tFeature;}
	void FillInfo(cDiscoWindow * owner, RichEditFormat & f);
	void PopupBuild(cDiscoWindow * owner , HMENU menu); 
	void PopupResult(cDiscoWindow * owner , HMENU menu , int id); 
	int GetImage(cDiscoWindow * owner) {return IDI_FEATURE;}
	CStdString GetReportJID(cDiscoWindow * owner) {return UTF8ToLocale(feature);}
	CStdString GetReportType(cDiscoWindow * owner) {return "cecha";}
protected:
	CStdString feature;
};

}; // JEP
}; //kJabber