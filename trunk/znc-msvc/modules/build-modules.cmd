@echo off

cl /O2 /Oi /GL /I ".." /I "..\win32\src" /I "..\..\_myinclude" /I "..\win32\c-ares" /D "_CRT_SECURE_NO_WARNINGS" /D "NOMINMAX" /D "_MODULES" /D "HAVE_LIBSSL" /D "WIN_MSVC" /D "HAVE_IPV6" /D "HAVE_ARES" /D "_WINDLL" /D "_MBCS" /FD /EHsc /MD /Gy /Yc"stdafx.hpp" /Fp"..\win32\msvc\Release\modules\ZNC.pch" /Fo"..\win32\msvc\Release\stdafx_mods.obj" /Fd"..\win32\msvc\Release\modules\vc90.pdb" /W3 /nologo /c /Zi /TP "..\stdafx.cpp"

FOR %%b in (admin adminlog antiidle autoattach autocycle autoop autoreply autovoice away awaynick block_lagchk block_motd blockserver blockuser buffextras certauth certchecker chansaver clientpart connect_throttle crypt ctcpflood discon_kick dns fail2ban fakeonline fish fixfreenode flooddetach highlightattach imapauth keepnick kickrejoin lastseen listsockets log motdfile nickserv noctcp notify notify_connect partyline perform q qauth raw rawlog route_replies savebuff schat send_raw simple_away stickychan watch watch_only_detached webadmin) DO cl /O2 /Oi /GL /I ".." /I "..\win32\src" /I "..\..\_myinclude" /I "..\win32\c-ares" /D "_CRT_SECURE_NO_WARNINGS" /D "NOMINMAX" /D "_MODULES" /D "HAVE_LIBSSL" /D "WIN_MSVC" /D "HAVE_IPV6" /D "HAVE_ARES" /D "_WINDLL" /D "_MBCS" /FD /EHsc /MD /Gy /Yu"stdafx.hpp" /Fp"..\win32\msvc\Release\modules\ZNC.pch" /Fo"..\win32\msvc\Release\modules\%%b.obj" /Fd"..\win32\msvc\Release\modules\vc90.pdb" /W3 /nologo /c /Zi /TP "%%b.cpp"

FOR %%c in (admin adminlog antiidle autoattach autocycle autoop autoreply autovoice away awaynick block_lagchk block_motd blockserver blockuser buffextras certauth certchecker chansaver clientpart connect_throttle crypt ctcpflood discon_kick dns fail2ban fakeonline fish fixfreenode flooddetach highlightattach imapauth keepnick kickrejoin lastseen listsockets log motdfile nickserv noctcp notify notify_connect partyline perform q qauth raw rawlog route_replies savebuff schat send_raw simple_away stickychan watch watch_only_detached webadmin) DO link /DLL /OUT:"..\win32\msvc\Release\modules\%%c.dll" /INCREMENTAL:NO /NOLOGO /LIBPATH:"..\..\Trillian\TrillShared\lib" /LIBPATH:"..\win32\msvc\Release" /OPT:REF /OPT:ICF /LTCG /DYNAMICBASE /NXCOMPAT /MACHINE:X86 /ERRORREPORT:PROMPT ws2_32.lib libeay32.lib ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ZNC.lib stdafx_mods.obj "..\win32\msvc\Release\modules\%%c.obj"
