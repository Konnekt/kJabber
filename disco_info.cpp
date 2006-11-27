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
		return "Nag³ówki";
	} else if (cat == "server") {
		return "Serwer";
	}
	return cat + " :: " + type;
}
CStdString JEP::ExplainIdentity(const CStdString & cat, const CStdString & type) {
	if (cat == "conference") {
		return "Konferencje/chaty umo¿liwiaj¹ rozmowê w pokojach dyskusyjnych.";
/*	} else if (cat == "directory") {
		return "Katalog";*/
	} else if (cat == "gateway") {
		return "Bramka umo¿liwiaj¹ca wymianê wiadomoœci z u¿ytkownikami sieci \""+type+"\"";
	} else if (cat == "headline") {
		return "Po zarejestrowaniu siê do tej us³ugi bêdziesz mia³ dostêp do najœwie¿szych wiadomoœci.";
	} else
		return "";
}
CStdString JEP::TranslateFeature(const CStdString & f) {
	if (f == "jabber:iq:search") {
		return "Wyszukiwanie";
	} else if (f == "jabber:iq:register") {
		return "Rejestracja";
	} else if (f == "jabber:iq:vcard-temp") {
		return "Wizytówka";
	} else
		return f;
}
CStdString JEP::ExplainFeature(const CStdString & f) {
	if (f=="jabber:iq:register") {
		return "Mo¿esz zarejestrowaæ siê w tej us³udze.";
	} else
		return "";
}
