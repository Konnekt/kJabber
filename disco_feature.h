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
#include "xData.h"

namespace kJabber {
	namespace JEP {

		class cFeatureXData: public SigC::Object {
		public:
			cFeatureXData(cJabber & jab, HWND hwnd, const CStdString & jid, const CStdString & title);
			virtual ~cFeatureXData();
		protected:
			virtual void OnCommand(int command);
			virtual void Submit(bool cancel) {}
			judo::Element * BuildIQ(const CStdString & type = "get");
			judo::Element * BuildX(judo::Element & parent, bool cancel);
			struct tSafeOnIQ {
				jabberoo::ElementCallbackFunc f;
				const judo::Element * e;
				bool processed;
				
			};

			static void CALLBACK SafeOnIQAPC(tSafeOnIQ * safe);
			bool SafeOnIQ(jabberoo::ElementCallbackFunc f , const judo::Element & e);
			
			bool IsError(const judo::Element & e);
			void Create();

			cXData xData;
			cJabber & jab;
			CStdString id;
			CStdString jid;
		};


		class cFeatureNeg: public cFeatureXData {
		public:
			cFeatureNeg(cJabber & jab, HWND hwnd, const CStdString & jid, const CStdString & title, const CStdString & feature);
		private:
			void OnIQ(const judo::Element & e);
			void Submit(bool cancel);

			CStdString feature;
		};

		class cFeatureIQRegister: public cFeatureXData {
		public:
			cFeatureIQRegister(cJabber & jab, HWND hwnd, const CStdString & jid, const CStdString & title);
		private:
			void OnIQ(const judo::Element & e);
			void Submit(bool cancel);
			void OnCommand(int command);
		};

		class cFeatureIQSearch: public cFeatureXData {
		public:
			cFeatureIQSearch(cJabber & jab, HWND hwnd, const CStdString & jid, const CStdString & title);
		private:
			void OnIQ(const judo::Element & e);
			void Submit(bool cancel);
			void OnCommand(int command);
		};

	}
}

