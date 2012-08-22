/*
 * Copyright (C) 2012 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "znc_tray.hpp"

CResourceBitmap::CResourceBitmap(int resId) :
	m_hGlobal(NULL)
{
	HRSRC hRes = ::FindResource(g_hInstance, MAKEINTRESOURCE(resId), L"PNG");

	if(!hRes)
	{
		return;
	}

	DWORD dwSize = ::SizeofResource(g_hInstance, hRes);
	const void *pResData = ::LockResource(
		::LoadResource(g_hInstance, hRes));

	if(dwSize < 1 || !pResData)
	{
		return;
	}

	HGLOBAL hGlob = ::GlobalAlloc(GMEM_MOVEABLE, dwSize);

	if(hGlob)
	{
		void *pGlobBuf = ::GlobalLock(hGlob);

		if(pGlobBuf)
		{
			memcpy_s(pGlobBuf, dwSize, pResData, dwSize);

			IStream *pStream;
			if(SUCCEEDED(::CreateStreamOnHGlobal(hGlob, FALSE, &pStream)))
			{
				Gdiplus::Bitmap* gdipBmp = Gdiplus::Bitmap::FromStream(pStream);

				// done its duty here, Bitmap probably keeps a ref:
				pStream->Release();

				if(gdipBmp && gdipBmp->GetLastStatus() == Gdiplus::Ok)
				{
					// it worked!
					m_hGlobal = hGlob;
					m_bmp = std::shared_ptr<Gdiplus::Bitmap>(gdipBmp);

					return; // postpone cleanup to destructor! ;)
				}

				delete gdipBmp;
			}

			::GlobalUnlock(pGlobBuf);
		}

		::GlobalFree(hGlob);
	}
}

CResourceBitmap::~CResourceBitmap()
{
	m_bmp.reset();

	if(m_hGlobal)
	{
		::GlobalUnlock(m_hGlobal);
		::GlobalFree(m_hGlobal);
	}
}
