// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define _WIN32_WINNT 0x0501
#define _WIN32_IE 0x501
#define _WIN32_WINDOWS 0x0410

// Windows Header Files:
#include <windows.h>
#include <commctrl.h>
#include <Shellapi.h>
#include <stdstring.h> 
#include <io.h>
#include <fstream>
#include <vector>
#include <stack>
#include <direct.h>
#include <sigc++/sigc++.h> 
#include <sigc++/class_slot.h> 
#include <Winsock2.h>
#include <Richedit.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h> 
#include <openssl/rand.h> 

#include "konnekt/plug_export.h"
#include "konnekt/ui.h"
#include "konnekt/plug_func.h"
#include "konnekt/knotify.h"
#include "konnekt/core_assert.h"

#include <Stamina/RegEx.h>
#include <Stamina/Helpers.h>
#include <Stamina/WinHelper.h>
#include <Stamina/CriticalSection.h>
#include <Stamina/UI/RichHtml.h>

#include "jabberoo.hh"
#include "jabberoox.hh"
#include "jabberoo.hh"
#include "judo.hpp"
#include "vCard.h"

#include "konnekt/kjabber.h"
#include "../dwutlenek/jabber_class.h" // na razie tam...
#include "kjabber_main.h"
#include "resource.h"
#include "afxres.h"
using namespace std;
using namespace kJabber;

using namespace Stamina;
using namespace Stamina::UI;
// TODO: reference additional headers your program requires here
