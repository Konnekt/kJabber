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

namespace kJabber {
	namespace JEP {
class cXData {

public:

	class cItem {
	public:
		cItem() {} /* inicjalizacja odbywa siê przez Create / Destroy! */
		virtual ~cItem() {}
		virtual void Create(cXData * owner , const judo::Element & e);
		virtual void Destroy(cXData * owner);
		virtual CStdString GetValue () const = 0 ;
		virtual void SetValue(const CStdString & value)=0;
		virtual CStdString GetVar () const {return var;}
		virtual CStdString GetType () const = 0 ;
		virtual CStdString Validate() {return "";}
		judo::Element * Build (bool addValue) const;
		virtual judo::Element * Build () const {return Build(true);}
		virtual int GetHeight(const cXData * owner) {return 0;}

		virtual CStdString GetValueFromXML (const judo::Element & e, char * multiline = 0) const;
		virtual CStdString GetLabelFromXML (const judo::Element & e) const {
			if (e.findElement("desc"))
				return  UTF8ToLocale(e.getChildCData("desc"));
			else if (e.getAttrib("label").empty() == false)
				return  UTF8ToLocale(e.getAttrib("label"));
			else
				return "[" + var + "]";
		}
	protected:
		CStdString var;
	};



	class cItem_hidden: public virtual cItem {
	public:
		void Create(cXData * owner, const judo::Element & e) {
			__super::Create(owner, e);
			this->value = GetValueFromXML(e);
		}
		CStdString GetType() const {return "hidden";}
		void SetValue(const CStdString & value) {this->value = value;}
		CStdString GetValue() const {return this->value;}
	protected:
		CStdString value;
	};

	class cItem_hwnd: public virtual cItem {
	public:
		void Create(cXData * owner, const judo::Element & e);
		void Destroy(cXData * owner);
		void SetValue(const CStdString & value);
		CStdString GetValue () const;
		virtual int GetX(const cXData * owner);
		virtual int GetY(const cXData * owner, int offset = 0);
		virtual int GetWidth(const cXData * owner);
		int GetHeight(const cXData * owner);
		static void SetFont(HWND hwnd);
		const static int margin = 1;
		static HFONT font;
		static int minLineHeight;
	protected:
		HWND hwnd;
	};
	class cItem_fixed: public cItem_hwnd {
	public:
		void Create(cXData * owner, const judo::Element & e);
		CStdString GetType() const {return "fixed";}
		judo::Element * Build() const{return 0;}
		int GetHeight(const cXData * owner) {return height;}
	private:
		int height;
	};
	class cItem_boolean: public cItem_hwnd {
	public:
		void Create(cXData * owner, const judo::Element & e);
		CStdString GetType() const {return "boolean";}
		void SetValue(const CStdString & value);
		CStdString GetValue () const;
	};



	class cItem_labeled: public cItem_hwnd {
	public:
		void Create(cXData * owner, const judo::Element & e);
		void Destroy(cXData * owner);
		int GetX(const cXData * owner);
		int GetWidth(const cXData * owner);
		int GetHeight(const cXData * owner);
	protected:
		int labelHeight;
		HWND label;
	};

	class cItem_text_single: public cItem_labeled {
	public:
		void Create(cXData * owner, const judo::Element & e);
		CStdString GetType() const {return "text-single";}
	};
	class cItem_text_multi: public cItem_labeled {
	public:
		void Create(cXData * owner, const judo::Element & e);
		int GetHeight(const cXData * owner);
		CStdString GetType() const {return "text-multi";}
	};
	class cItem_text_private: public cItem_labeled {
	public:
		void Create(cXData * owner, const judo::Element & e);
		CStdString GetType() const {return "text-private";}
	};

	class cItem_list_multi: public cItem_labeled {
	public:
		void Create(cXData * owner, const judo::Element & e, bool multi);
		void Create(cXData * owner, const judo::Element & e) {Create(owner, e, true);}
		int GetHeight(const cXData * owner);
		void SetValue(const CStdString & value);
		CStdString GetValue () const;
		CStdString GetType() const {return "list-multi";}
	protected:
        typedef vector<CStdString> tValues;
		tValues values;
	};
	class cItem_list_single: public cItem_labeled {
	public:
		void Create(cXData * owner, const judo::Element & e);
		CStdString GetType() const {return "list-single";}
		void SetValue(const CStdString & value);
		CStdString GetValue () const;
	protected:
        typedef vector<CStdString> tValues;
		tValues values;
	};

	class cItem_jid_single: public cItem_text_single {
	public:
		CStdString Validate();
		static bool ValidateJID(CStdString JID);
		CStdString GetType() const {return "jid-single";}
	};
	class cItem_jid_multi: public cItem_text_multi {
	public:
		CStdString Validate();
		CStdString GetType() const {return "jid-multi";}
	};

	class cItem_result: public virtual cItem_hwnd {
	public:
		void Create(cXData * owner, const judo::Element & e);
		void Destroy(cXData * owner);
		void SetValue(const CStdString & value) {return;}
		CStdString GetValue () const {return "";}
		int GetHeight(const cXData * owner) {return owner->GetContainerHeight();}
		judo::Element * Build() const {return 0;}
		CStdString GetType() const {return "";}
		void OnDoubleClick(cXData * owner);
	protected:
		int jidColumn;
	};


	// -------------------------    STARE protoko³y
#pragma warning( push )
#pragma warning(disable: 4250)
	class cItem_obsolete: public virtual cItem {
	public:
		void Create(cXData * owner, const judo::Element & e);
		CStdString GetValueFromXML (const judo::Element & e, char * multiline = 0) const;
		CStdString GetLabelFromXML (const judo::Element & e) const;
		judo::Element * Build () const;
	};
	class cItem_obsolete_text: public cItem_text_single, public cItem_obsolete {
	public:
		void Create(cXData * owner, const judo::Element & e) {
			cItem_obsolete::Create(owner,e);
			cItem_text_single::Create(owner,e);
		}
	};
	class cItem_obsolete_private: public cItem_text_private, public cItem_obsolete {
	public:
		void Create(cXData * owner, const judo::Element & e) {
			cItem_obsolete::Create(owner,e);
			cItem_text_private::Create(owner,e);
		}
	};
	class cItem_obsolete_hidden: public cItem_hidden, public cItem_obsolete {
	public:
		void Create(cXData * owner, const judo::Element & e) {
			cItem_obsolete::Create(owner,e);
			cItem_hidden::Create(owner,e);
		}
	};
#pragma warning( pop )


//=================================================

public:
	typedef list<class cItem*> tItems;
	typedef tItems::iterator iterator;
	typedef tItems::const_iterator const_iterator;

	enum enState {
		stNone, stWait, stSubmit, stCancel, stError
	};
	enum enType {
		tNone, tError, tForm, tResult, tInfo, tObsolete
	};

	cXData(cJabber & jabber, const CStdString & title, HWND parent);
	~cXData();
	/*  Pakiet, którego dzieckiem powinien byæ element X
		wait - czy czekaæ na reakcjê u¿ytkownika
	*/
	void Process(const judo::Element & e , bool wait);
	void ProcessError(CStdString msg, bool interrupt = false);
	void ProcessInfo(CStdString msg, bool interrupt = false);
	void ProcessForm(const judo::Element & e);
	void ProcessResult(const judo::Element & e);
	void ProcessObsolete(const judo::Element & e);
	bool IsCanceled() const {return state == stCancel;}
	bool IsAnswered() const {return state != stWait;}
	bool IsForm() const {return type == tForm || type == tObsolete;}
	/* Wstawia wynik zapytania */
	void InjectResult (judo::Element & e) const;
	/* Usuwa wszystkie elementy */
	void push_back(cItem* item) {items.push_back(item);}

	void SetInstructions(const CStdString & txt);
	void SetInfoFormat(int color = -1, int bgColor = -1, bool bold = false);
	void SetTitle(const CStdString & txt);
	void SetStatus(const CStdString & txt);
	void SetButtonSpecial(const CStdString & txt);
	void ShowWindow() {::ShowWindow(hwnd, SW_SHOW);}
	int GetWidth() const {return w;}
	int GetX() const {return x;}
	int GetY() const {return y;}
	int GetContainerHeight() const {return containerH - 10;}
	cJabber & Jab() {return jab;}
	int OffsetY(int offset);
	void FitText(HWND hwnd, const CStdString & txt, RECT & rc);
	int FitTextHeight(HWND hwnd, const CStdString & txt);
	void UpdateScroll();
	HWND GetContainer() const {return container;}
	enState GetState() const {return state;}
	enType GetType() const {return type;}
	CStdString GetTitle() const {return title;}
	void Enable(bool enable);

	void SetInfoHeight(int h) {newInfoH = h;}
	void SetContWidth(int w) {newContW = w;}
	void SetContHeight(int h) {newContH = h;}
	enum enSetSize {
		ssNo, ssYes, ssForce
	};
	void SetSize(enSetSize resize = ssNo);

	SigC::Signal1<void, int> evtCommand;
	
private:
	static LRESULT WndProc(HWND hwnd , UINT message , WPARAM wParam , LPARAM lParam);
	static LRESULT ContainerProc(HWND hwnd , UINT message , WPARAM wParam , LPARAM lParam);

	void clear();

	cJabber & jab;
	int w , h, x , y, containerH;
	int newInfoH, newContH, newContW;
	HWND hwnd;
	HWND status;
	HWND container;
	HWND info;
	CStdString title;
	enState state; /* Stan okna */
	enType type; /* Typ okna... */
	tItems items;

	const static int defFormWidth = 300;
	const static int defResultWidth = 500;
	const static int defResultHeight = 200;
};
};
};