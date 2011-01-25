/*
 * Copyright (C) 2004-2011  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef _BUFFER_H
#define _BUFFER_H

#include "zncconfig.h"
#include "ZNCString.h"
#include <deque>
#include <assert.h>

using std::deque;

class ZNC_API CBufLine {
public:
#ifdef WIN_MSVC
	CBufLine() { assert(false); }; // dummy default constructor to shut up error...
#endif

	CBufLine(const CString& sPre, const CString& sPost, bool bIncNick);
	~CBufLine();
	void GetLine(const CString& sTarget, CString& sRet) const;

	const CString& GetPre() const { return m_sPre; }
	const CString& GetPost() const { return m_sPost; }
	bool GetIncNick() const { return m_bIncNick; }

	void SetPre(const CString& s) { m_sPre = s; }
	void SetPost(const CString& s) { m_sPost = s; }
	void SetIncNick(bool b) { m_bIncNick = b; }

private:
protected:
	CString m_sPre;
	CString m_sPost;
	bool    m_bIncNick;
};

#ifdef WIN_MSVC
class ZNC_API CBuffer : protected deque<CBufLine> {
// msvc can't dllexport classes with private inheritance... d'oh.
#else
class CBuffer : private deque<CBufLine> {
#endif
public:
	CBuffer(unsigned int uLineCount = 100);
	~CBuffer();

	size_t AddLine(const CString& sPre, const CString& sPost, bool bIncNick = true);
	/// Same as AddLine, but if there is already a line with sPre it is replaced.
	size_t UpdateLine(const CString& sPre, const CString& sPost, bool bIncNick = true);
	/// Same as UpdateLine, but does nothing if this exact line already exists
	size_t UpdateExactLine(const CString& sPre, const CString& sPost, bool bIncNick = true);
	bool GetNextLine(const CString& sTarget, CString& sRet);
	bool GetLine(const CString& sTarget, CString& sRet, unsigned int uIdx) const;
	bool IsEmpty() const { return empty(); }
	void Clear() { clear(); }

	// Setters
	void SetLineCount(unsigned int u);
	// !Setters

	// Getters
	unsigned int GetLineCount() const { return m_uLineCount; }
	// !Getters
private:
protected:
	unsigned int m_uLineCount;
};

#endif // !_BUFFER_H
