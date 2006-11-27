/*
 *  kJabber
 *  
 *  Please READ & ACCEPT /License.txt FIRST! 
 * 
 *  Copyright (C)2005,2006 Rafa� Lindemann, STAMINA
 *  http://www.stamina.pl/ , http://www.konnekt.info/
 *
 *  $Id: $
 */


#include "stdafx.h"

#include "disco_window.h"
using namespace JEP;

CStdString JEP::TranslateIdentity(const CStdString & cat, const CStdString & type) {
	if (cat == "")
		return "?";
	else if (cat == "client") {
		if (type == "bot")
			return "Bot";
		else
			return "Klient";
	} else if (cat == "conference") {
		if (type == "irc")
			return "IRC";
		else
			return "Konferencje";
	} else if (cat == "directory") {
		return "Katalog";
	} else if (cat == "gateway") {
		return "Bramka ("+type+")";
	} else if (cat == "headline") {
		return "Nag��wki";
	} else if (cat == "server") {
		return "Serwer";
	}
	return cat + " :: " + type;
}
CStdString JEP::ExplainIdentity(const CStdString & cat, const CStdString & type) {
	if (cat == "conference") {
		return "Konferencje/chaty umo�liwiaj� rozmow� w pokojach dyskusyjnych.";
/*	} else if (cat == "directory") {
		return "Katalog";*/
	} else if (cat == "gateway") {
		return "Bramka umo�liwiaj�ca wymian� wiadomo�ci z u�ytkownikami sieci \""+type+"\"";
	} else if (cat == "headline") {
		return "Po zarejestrowaniu si� do tej us�ugi b�dziesz mia� dost�p do naj�wie�szych wiadomo�ci.";
	} else
		return "";
}
CStdString JEP::TranslateFeature(const CStdString & f) {
	if (f == "jabber:iq:search") {
		return "Wyszukiwanie";
	} else if (f == "jabber:iq:register") {
		return "Rejestracja";
	} else if (f == "jabber:iq:vcard-temp") {
		return "Wizyt�wka";
	} else
		return f;
}
CStdString JEP::ExplainFeature(const CStdString & f) {
	if (f=="jabber:iq:register") {
		return "Mo�esz zarejestrowa� si� w tej us�udze.";
	} else
		return "";
}
