/*
 * Copyright (C) 2004-2010  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef _FILEUTILS_H
#define _FILEUTILS_H

#include "Socket.h"
#include "ZNCString.h"
#include <dirent.h>
#include <map>
#include <signal.h>
#include <vector>

using std::vector;
using std::map;

class ZNC_API CFile {
public:
	CFile();
	CFile(const CString& sLongName);
	~CFile();

	enum EFileTypes {
		FT_REGULAR,
		FT_DIRECTORY,
		FT_CHARACTER,
		FT_BLOCK,
		FT_FIFO,
		FT_LINK,
		FT_SOCK
	};

	void SetFileName(const CString& sLongName);
	static bool IsReg(const CString& sLongName, bool bUseLstat = false);
	static bool IsDir(const CString& sLongName, bool bUseLstat = false);
	static bool IsChr(const CString& sLongName, bool bUseLstat = false);
	static bool IsBlk(const CString& sLongName, bool bUseLstat = false);
	static bool IsFifo(const CString& sLongName, bool bUseLstat = false);
	static bool IsLnk(const CString& sLongName, bool bUseLstat = true);
	static bool IsSock(const CString& sLongName, bool bUseLstat = false);

	bool IsReg(bool bUseLstat = false) const;
	bool IsDir(bool bUseLstat = false) const;
	bool IsChr(bool bUseLstat = false) const;
	bool IsBlk(bool bUseLstat = false) const;
	bool IsFifo(bool bUseLstat = false) const;
	bool IsLnk(bool bUseLstat = true) const;
	bool IsSock(bool bUseLstat = false) const;

	// for gettin file types, using fstat instead
	static bool FType(const CString& sFileName, EFileTypes eType, bool bUseLstat = false);

	enum EFileAttr {
		FA_Name,
		FA_Size,
		FA_ATime,
		FA_MTime,
		FA_CTime,
		FA_UID
	};

	//
	// Functions to retrieve file information
	//
	bool Exists() const;
	off_t GetSize() const;
	time_t GetATime() const;
	time_t GetMTime() const;
	time_t GetCTime() const;
	uid_t GetUID() const;
	gid_t GetGID() const;
	static bool Exists(const CString& sFile);

	static off_t GetSize(const CString& sFile);
	static time_t GetATime(const CString& sFile);
	static time_t GetMTime(const CString& sFile);
	static time_t GetCTime(const CString& sFile);
	static uid_t GetUID(const CString& sFile);
	static gid_t GetGID(const CString& sFile);
	static int GetInfo(const CString& sFile, struct stat& st);

	//
	// Functions to manipulate the file on the filesystem
	//
	bool Delete();
	bool Move(const CString& sNewFileName, bool bOverwrite = false);
	bool Copy(const CString& sNewFileName, bool bOverwrite = false);

	static bool Delete(const CString& sFileName);
	static bool Move(const CString& sOldFileName, const CString& sNewFileName, bool bOverwrite = false);
	static bool Copy(const CString& sOldFileName, const CString& sNewFileName, bool bOverwrite = false);
	bool Chmod(mode_t mode);
	static bool Chmod(const CString& sFile, mode_t mode);
	bool Seek(off_t uPos);
	bool Truncate();
	bool Sync();
	bool Open(const CString& sFileName, int iFlags = O_RDONLY, mode_t iMode = 0644);
	bool Open(int iFlags = O_RDONLY, mode_t iMode = 0644);
	int Read(char *pszBuffer, int iBytes);
	bool ReadLine(CString & sData, const CString & sDelimiter = "\n");
	bool ReadFile(CString& sData, size_t iMaxSize = 512 * 1024);
	int Write(const char *pszBuffer, size_t iBytes);
	int Write(const CString & sData);
	void Close();
	void ClearBuffer();

	bool TryExLock(const CString& sLockFile, int iFlags = O_RDONLY | O_CREAT);
	bool TryExLock();
	bool ExLock();
	bool UnLock();

	bool IsOpen() const;
	CString GetLongName() const;
	CString GetShortName() const;
	CString GetDir() const;

private:
	// fcntl() locking wrapper
	bool Lock(int iType, bool bBlocking);

	CString m_sBuffer;
	int     m_iFD;

protected:
	CString m_sLongName;  //!< Absolute filename (m_sPath + "/" + m_sShortName)
	CString m_sShortName; //!< Filename alone, without path
};

class ZNC_API CDir : public vector<CFile*> {
public:

	CDir(const CString& sDir) {
		m_bDesc = false;
		m_eSortAttr = CFile::FA_Name;
		Fill(sDir);
	}

	CDir() {
		m_bDesc = false;
		m_eSortAttr = CFile::FA_Name;
	}

	~CDir() {
		CleanUp();
	}

	void CleanUp() {
		for (unsigned int a = 0; a < size(); a++) {
			delete (*this)[a];
		}

		clear();
	}

	size_t Fill(const CString& sDir) {
		return FillByWildcard(sDir, "*");
	}

	size_t FillByWildcard(const CString& sDir, const CString& sWildcard) {
		CleanUp();
		CString sPath(sDir);
		if(sPath.empty()) sPath = ".";
		DIR* dir = opendir(sPath.c_str());

		if (!dir) {
			return 0;
		}

		struct dirent * de;

		while ((de = readdir(dir)) != 0) {
			if ((strcmp(de->d_name, ".") == 0) || (strcmp(de->d_name, "..") == 0)) {
				continue;
			}
			if ((!sWildcard.empty()) && (!CString(de->d_name).WildCmp(sWildcard))) {
				continue;
			}

			CFile *file = new CFile(sPath + (sPath[sPath.size() - 1] == '/' ? "" : "/") + de->d_name/*, this*/);
				// @todo need to pass pointer to 'this' if we want to do Sort()
			push_back(file);
		}

		closedir(dir);
		return size();
	}

	static unsigned int Chmod(mode_t mode, const CString& sWildcard, const CString& sDir = ".") {
		CDir cDir;
		cDir.FillByWildcard(sDir, sWildcard);
		return cDir.Chmod(mode);
	}

	unsigned int Chmod(mode_t mode) {
		unsigned int uRet = 0;
		for (unsigned int a = 0; a < size(); a++) {
			if ((*this)[a]->Chmod(mode)) {
				uRet++;
			}
		}

		return uRet;
	}

	static unsigned int Delete(const CString& sWildcard, const CString& sDir = ".") {
		CDir cDir;
		cDir.FillByWildcard(sDir, sWildcard);
		return cDir.Delete();
	}

	unsigned int Delete() {
		unsigned int uRet = 0;
		for (unsigned int a = 0; a < size(); a++) {
			if ((*this)[a]->Delete()) {
				uRet++;
			}
		}

		return uRet;
	}

	CFile::EFileAttr GetSortAttr() { return m_eSortAttr; }
	bool IsDescending() { return m_bDesc; }

	// Check if sPath + "/" + sAdd (~/ is handled) is an absolute path which
	// resides under sPath. Returns absolute path on success, else "".
	static CString CheckPathPrefix(const CString& sPath, const CString& sAdd, const CString& sHomeDir = "");
	static CString ChangeDir(const CString& sPath, const CString& sAdd, const CString& sHomeDir = "");
	static bool MakeDir(const CString& sPath, mode_t iMode = 0700);

	static CString GetCWD() {
		CString sRet;
		char * pszCurDir = getcwd(NULL, 0);
		if (pszCurDir) {
			sRet = pszCurDir;
			free(pszCurDir);
		}

		return sRet;
	}

private:
protected:
	CFile::EFileAttr m_eSortAttr;
	bool             m_bDesc;
};

#ifndef _WIN32

//! @author imaginos@imaginos.net
class CExecSock : public CZNCSock {
public:
	CExecSock() : CZNCSock() {
		m_iPid = -1;
	}

	int Execute(const CString & sExec) {
		int iReadFD, iWriteFD;
		m_iPid = popen2(iReadFD, iWriteFD, sExec);
		if (m_iPid != -1) {
			ConnectFD(iReadFD, iWriteFD, "0.0.0.0:0");
		}
		return(m_iPid);
	}
	void Kill(int iSignal)
	{
		kill(m_iPid, iSignal);
		Close();
	}
	virtual ~CExecSock() {
		close2(m_iPid, GetRSock(), GetWSock());
		SetRSock(-1);
		SetWSock(-1);
	}

	int popen2(int & iReadFD, int & iWriteFD, const CString & sCommand);
	void close2(int iPid, int iReadFD, int iWriteFD);

private:
	int  m_iPid;
};

#endif // !_WIN32

#endif // !_FILEUTILS_H
