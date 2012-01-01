/*
 * Copyright (C) 2012 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "znc_tray.hpp"
#include "control_window.hpp"
#include "resource.h"

using namespace ZNCTray;


CControlWindow::CControlWindow() :
	m_hwndDlg(0),
	m_hIconSmall(0), m_hIconBig(0),
	m_hwndStatusBar(0)
{
}


bool CControlWindow::CreateDlg()
{
	// load some resources:

	m_hIconSmall = ::LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICONZBLUE));
	m_hIconBig = (HICON)::LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_ICONZNC), IMAGE_ICON,
		::GetSystemMetrics(SM_CXICON),::GetSystemMetrics(SM_CYICON), 0);

	m_bmpLogo = std::shared_ptr<CResourceBitmap>(new CResourceBitmap(IDB_ZNCLOGO));

	m_serviceStatus = std::shared_ptr<CServiceStatus>(new CServiceStatus(L"ZNC"));

	// our main window is a simple #32770 dialog!

	::CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_DLGCONTROL), NULL,
		DialogProcStatic, reinterpret_cast<LONG_PTR>(this));

	if(!m_hwndDlg) // set by message proc, CreateDialogParam won't return until WM_INITDIALOG has run
	{
		return false;
	}

	return true;
}


void CControlWindow::InitialSetup()
{
	if(!WM_TASKBARCREATED)
	{
		WM_TASKBARCREATED = ::RegisterWindowMessage(L"TaskbarCreated");
	}

	// set up window icons:
	::SendMessage(m_hwndDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(m_hIconSmall));
	::SendMessage(m_hwndDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(m_hIconBig));
	
	// create status bar:
	m_hwndStatusBar = ::CreateWindowEx(0, STATUSCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_hwndDlg, NULL, g_hInstance, NULL);
	::SendMessage(m_hwndStatusBar, SB_SIMPLE, TRUE, 0);

	// set up tray icon:
	m_trayIcon = std::shared_ptr<CTrayIcon>(new CTrayIcon(m_hwndDlg, L"ZNC"));

	m_trayIcon->Add('B');


	m_serviceStatus->Init();
}


void CControlWindow::Show()
{
	::ShowWindow(m_hwndDlg, SW_SHOWNORMAL);
}


void CControlWindow::Hide()
{
	::ShowWindow(m_hwndDlg, SW_HIDE);
}


void CControlWindow::OnBeforeDestroy()
{
}


BOOL CALLBACK CControlWindow::DialogProcStatic(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LONG_PTR l_ptr = ::GetWindowLongPtr(hDlg, GWLP_USERDATA);

	// set up GWLP_USERDATA during init:
	if(!l_ptr && uMsg == WM_INITDIALOG)
	{
		l_ptr = lParam;
		::SetWindowLongPtr(hDlg, GWLP_USERDATA, l_ptr);
	}

	// extract ptr to CControlWindow instance:
	CControlWindow *l_wnd = reinterpret_cast<CControlWindow*>(l_ptr);

	if(l_wnd)
	{
		// store hwnd, may not present because we get here before returning from CreateDialogParam:
		if(!l_wnd->m_hwndDlg) l_wnd->m_hwndDlg = hDlg;

		// hand over to instance proc:
		return (l_wnd->DialogProc(uMsg, wParam, lParam) ? TRUE : FALSE);
	}

	return FALSE;
}


bool CControlWindow::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		this->InitialSetup();
		this->Show();
		return true;

	case WM_PAINT:
		this->OnPaint();
		return true;

	case WM_COMMAND:
		this->OnWmCommand(LOWORD(wParam));
		return true;

	case WM_DESTROY:
		this->OnBeforeDestroy();
		::PostQuitMessage(0);
		return true;

	case WM_CLOSE:
		this->Hide();
		return true;

	case WM_TRAYICONEVENT:
		m_trayIcon->OnWmTrayIconEvent(wParam, lParam);
		return true;
	}

	// catch explorer.exe restarts:
	if(uMsg == WM_TASKBARCREATED)
	{
		m_trayIcon->OnWmTaskbarCreated();
	}

	return false;
}


bool CControlWindow::OnWmCommand(UINT uCmd)
{
	switch(uCmd)
	{
	case WMC_SHOW_CONTROL_WIN:
		this->Show();
		return true;

	case IDM_HIDE:
		this->Hide();
		return true;

	case WMC_CLOSE_TRAY:
	case IDM_QUIT:
		if(::MessageBox(m_hwndDlg,
			L"You are closing ZNC's tray icon. The ZNC service will keep running, if started. "
			L"Do you want to continue?", L"Confirm", MB_ICONQUESTION | MB_YESNO)
			== IDYES)
		{
			::DestroyWindow(m_hwndDlg);
		}
		return true;
	}

	return false;
}


void CControlWindow::OnPaint()
{
	// let's make it nice
	PAINTSTRUCT ps;
	HDC hDC = ::BeginPaint(m_hwndDlg, &ps);

	// white background:
	RECT rectClient;
	RECT rectStatusBar;
	if(::GetClientRect(m_hwndDlg, &rectClient) && ::GetClientRect(m_hwndStatusBar, &rectStatusBar))
	{
		// remove status bar from client rect:
		rectClient.bottom -= (rectStatusBar.bottom - rectStatusBar.top);

		HBRUSH hbWhite = ::CreateSolidBrush(RGB(255, 255, 255));
		::FillRect(hDC, &rectClient, hbWhite);
		::DeleteObject(hbWhite);
	}
	
	// draw PNG logo:
	if(m_bmpLogo && m_bmpLogo->GetBmp())
	{
		Gdiplus::Graphics gr(hDC);

		gr.DrawImage(m_bmpLogo->GetBmp().get(), 10,
			(rectClient.bottom - rectClient.top) / 2 - m_bmpLogo->GetBmp()->GetHeight() / 2,
			m_bmpLogo->GetBmp()->GetWidth(), m_bmpLogo->GetBmp()->GetHeight());
	}

	::EndPaint(m_hwndDlg, &ps);
}


int CControlWindow::MessageLoop()
{
	MSG msg;
	int status;

	while((status = ::GetMessage(&msg, 0, 0, 0)) != 0)
	{
		if(status == -1)
			return -1;

		// YOU SPIN ME RIGHT ROUND, BABY
		// RIGHT ROUND
		// LIKE A RECORD BABY
		// RIGHT ROUND ROUND ROUND
		// love this song

		if(!::IsDialogMessage(m_hwndDlg, &msg))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}


int CControlWindow::Run()
{
	if(!this->CreateDlg())
	{
		return 1;
	}

	return this->MessageLoop();
}


CControlWindow::~CControlWindow()
{
	::DestroyIcon(m_hIconSmall);
	::DestroyIcon(m_hIconBig);
}


/* global var */
UINT WM_TASKBARCREATED = 0;
