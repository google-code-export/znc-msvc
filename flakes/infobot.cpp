/*
 * Copyright (C) 2009-2010 flakes @ EFNet
 * Updated: 14 November 2010
 *
 * Compiling: LIBS="-lpcrecpp -lpcre" ./znc-buildmod infobot.cpp
 * http://en.znc.in/wiki/Infobot
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "main.h"
#include "znc.h"
#include "Chan.h"
#include "Modules.h"
#include "User.h"

#include <pcre.h>
#include <pcrecpp.h>

#include <ctype.h>

#define NR_OF_TRIGGERS 11

using namespace pcrecpp;

typedef map<const CString, VCString> TEnabledChanMap;
typedef map<const CString, bool> TColorsEnabledMap;
typedef map<const CString, char> TTriggerCharMap;

class CInfoBotModule : public CModule
{
private:
	TEnabledChanMap m_enabledCmds;
	TColorsEnabledMap m_coloredChans;
	TTriggerCharMap m_triggerChars;
	int m_colorOne, m_colorTwo;

	static const char* TRIGGERS[NR_OF_TRIGGERS];
	static const char* TRIGGERS_STR;
	static const char* TRIGGER_DESCRIPTIONS[NR_OF_TRIGGERS];

	static bool IsTriggerSupported(const CString& sTriggerName);
	bool IsTriggerEnabled(const CString& sChan, const CString& sTriggerName);
	void DisableTrigger(const CString& sChan, const CString& sTriggerName);
	void CheckLineForTrigger(const CString& sMessage, const CString& sChannel, const CString& sNick);

	void SaveSettings();
	void LoadSettings();
public:
	MODCONSTRUCTOR(CInfoBotModule)
	{
		m_colorOne = 7;
		m_colorTwo = 14;
	}

	bool ColorsEnabled(const CString& sChan);
	char TriggerChar(const CString& sChan);
	void SendMessage(const CString& sSendTo, const CString& sMsg);
	static CString Do8Ball();

	bool OnLoad(const CString& sArgsi, CString& sMessage);
	void OnModCommand(const CString& sCommand);
	EModRet OnChanMsg(CNick& Nick, CChan& Channel, CString& sMessage);
	EModRet OnUserMsg(CString& sTarget, CString& sMessage);	
	~CInfoBotModule();
};

static const char* const g_szHTMLescapes[256] = {
	"&#0;", 0, 0, 0, 0, 0, 0, 0, 0, 0,               // 0-9
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                    // 10-19
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                    // 20-29
	0, 0, 0, 0, "&quot;", 0, 0, 0, "&amp;", "&#39;", // 30-39
	0, 0, 0, 0, 0, "&ndash;", 0, 0, 0, 0,            // 40-49
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                    // 50-59
	"&lt;", 0, "&gt;", 0, 0, 0, 0, 0, 0, 0,          // 60-69
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                    // 70-79
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                    // 80-89
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                    // 90-99
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                    // 100-109
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                    // 110-119
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                    // 120-129
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                    // 130-139
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                    // 140-149
	0, 0, 0, "&trade;", 0, 0, 0, 0, 0, 0,            // 150-159
	"&nbsp;",   // 160
	"&iexcl;",  // 161
	"&cent;",   // 162
	"&pound;",  // 163
	"&curren;", // 164
	"&yen;",    // 165
	"&brvbar;", // 166
	"&sect;",   // 167
	"&uml;",    // 168
	"&copy;",   // 169
	"&ordf;",   // 170
	"&laquo;",  // 171
	"&not;",    // 172
	"&shy;",    // 173
	"&reg;",    // 174
	"&macr;",   // 175
	"&deg;",    // 176
	"&plusmn;", // 177
	"&sup2;",   // 178
	"&sup3;",   // 179
	"&acute;",  // 180
	"&micro;",  // 181
	"&para;",   // 182
	"&middot;", // 183
	"&cedil;",  // 184
	"&sup1;",   // 185
	"&ordm;",   // 186
	"&raquo;",  // 187
	"&frac14;", // 188
	"&frac12;", // 189
	"&frac34;", // 190
	"&iquest;", // 191
	"&Agrave;", // 192
	"&Aacute;", // 193
	"&Acirc;",  // 194
	"&Atilde;", // 195
	"&Auml;",   // 196
	"&Aring;",  // 197
	"&AElig;",  // 198
	"&Ccedil;", // 199
	"&Egrave;", // 200
	"&Eacute;", // 201
	"&Ecirc;",  // 202
	"&Euml;",   // 203
	"&Igrave;", // 204
	"&Iacute;", // 205
	"&Icirc;",  // 206
	"&Iuml;",   // 207
	"&ETH;",    // 208
	"&Ntilde;", // 209
	"&Ograve;", // 210
	"&Oacute;", // 211
	"&Ocirc;",  // 212
	"&Otilde;", // 213
	"&Ouml;",   // 214
	"&times;",  // 215
	"&Oslash;", // 216
	"&Ugrave;", // 217
	"&Uacute;", // 218
	"&Ucirc;",  // 219
	"&Uuml;",   // 220
	"&Yacute;", // 221
	"&THORN;",  // 222
	"&szlig;",  // 223
	"&agrave;", // 224
	"&aacute;", // 225
	"&acirc;",  // 226
	"&atilde;", // 227
	"&auml;",   // 228
	"&aring;",  // 229
	"&aelig;",  // 230
	"&ccedil;", // 231
	"&egrave;", // 232
	"&eacute;", // 233
	"&ecirc;",  // 234
	"&euml;",   // 235
	"&igrave;", // 236
	"&iacute;", // 237
	"&icirc;",  // 238
	"&iuml;",   // 239
	"&eth;",    // 240
	"&ntilde;", // 241
	"&ograve;", // 242
	"&oacute;", // 243
	"&ocirc;",  // 244
	"&otilde;", // 245
	"&ouml;",   // 246
	"&divide;", // 247
	"&oslash;", // 248
	"&ugrave;", // 249
	"&uacute;", // 250
	"&ucirc;",  // 251
	"&uuml;",   // 252
	"&yacute;", // 253
	"&thorn;",  // 254
	"&yuml;",   // 255
};


static CString StripHTML(const CString& sFrom)
{
	CString sResult = sFrom;

	// remove tags:
	CString::size_type pos = sResult.find('<');

	while(pos != CString::npos)
	{
		CString::size_type endPos = sResult.find('>', pos);

		if(endPos != CString::npos)
		{
			sResult.erase(pos, endPos - pos + 1);
			pos = sResult.find('<', pos);
		}
		else
		{
			sResult.erase(pos);
			break;
		}
	}

	// remove stupid legay HTML entities:
	pos = sResult.find('&');

	while(pos != CString::npos)
	{
		CString::size_type endPos = sResult.find(';', pos);

		if(endPos != CString::npos)
		{
			bool found = false;

			for(unsigned int c = 0; c < 256; c++)
			{
				if(g_szHTMLescapes[c] && !strncasecmp(g_szHTMLescapes[c], (char*)(sResult.c_str() + pos), endPos - pos + 1))
				{
					sResult.erase(pos, endPos - pos + 1);
					sResult.insert(pos, 1, (char)c);
					found = true;
					break;
				}
			}

			if(!found && sResult[pos + 1] != '#')
				sResult.erase(pos, endPos - pos + 1);

			pos = sResult.find('&', pos + 1);
		}
		else
			break;
	}

	// remove numerical and XML entities:
	sResult = sResult.Escape_n(CString::EHTML, CString::EASCII);

	// because entitiy decoding is being done in two steps,
	// this screws up in certain situations, e.g. &#38;gt;
	// produces '>' instead of '&gt;' ... but whatever.


	// normalize whitespace:
	RE("\\s+").GlobalReplace(" ", &sResult);
	sResult.Trim();

	return sResult;
}


class CSimpleHTTPSock : protected CSocket
{
private:
	CString m_request;
	CString m_buffer;

protected:
	CInfoBotModule *m_pMod;

	void Get(const CString& sHost, const CString& sPath, unsigned short iPort = 80, bool bSSL = false)
	{
		m_request = "GET " + sPath + " HTTP/1.0\r\n";
		m_request += "Host: " + sHost + ((iPort == 80 && !bSSL) || (iPort == 443 && bSSL) ? CString("") : ":" + CString(iPort)) + "\r\n";
		m_request += "User-Agent: Mozilla/5.0 (" + CZNC::GetTag() + ")\r\n";
		m_request += "Connection: close\r\n";
		m_request += "\r\n";

		Connect(sHost, iPort, bSSL);
	}

	void Connected()
	{
		m_buffer.clear();
		Write(m_request);
		m_request.clear();
	}

	virtual void Timeout()
	{
		m_request.clear();
		Close();
	}

	void Disconnected()
	{
		OnRequestDone(m_buffer);
		Close();
	}

#if (VERSION_MAJOR == 0 && VERSION_MINOR <= 80)
	void ReadData(const char *data, int len)
#else
	void ReadData(const char *data, size_t len)
#endif
	{
		if(m_buffer.size() + len > 1024 * 1024)
		{
			// make sure our buffers don't EVER take up too much memory.
			// we just abort stuff in this case.
			m_buffer.clear();
			Close();
		}
		else
		{
			m_buffer.append(data, len);
		}
	}

	virtual void OnRequestDone(const CString& sResponse) = 0;
public:
	CSimpleHTTPSock(CInfoBotModule *pModInstance)
		: CSocket((CModule*)pModInstance)
	{
		m_pMod = pModInstance;

		DisableReadLine();
	}

	virtual ~CSimpleHTTPSock()
	{
	}

	static CString URLEscape(const CString& s)
	{
		return s.Escape_n(CString::EASCII, CString::EURL);
	}
};


class CTriggerHTTPSock : public CSimpleHTTPSock
{
protected:
	CString m_trigger, m_args, m_chan, m_nick;
	bool m_acceptEmptyArgs, m_timedOut;

	void Timeout()
	{
		m_pMod->SendMessage(m_chan, "ERROR: Sorry " + m_nick + ", I failed to contact the server.");
		m_timedOut = true;

		CSimpleHTTPSock::Timeout();
	}

public:
	CTriggerHTTPSock(CInfoBotModule *pModInstance) : CSimpleHTTPSock(pModInstance)
	{
		m_acceptEmptyArgs = true;
		m_timedOut = false;
	}

	void SetTriggerInfo(const CString& sTrigger, const CString& sArgs, const CString& sChan, const CString& sNick)
	{
		m_trigger = sTrigger;
		m_args = sArgs;
		m_chan = sChan;
		m_nick = sNick;
	}

	bool AcceptEmptyArgs() { return m_acceptEmptyArgs; }

	virtual void Request() = 0;
};


class CGoogleSock : public CTriggerHTTPSock
{
protected:
	CString ParseFirstResult(const CString& sResponse, bool bGetURLOnly = false)
	{
		RE linkRE("</h2>.+?class=.?r.+?href=[\"'](http://\\S+?)[\"'].*?>(.+?)</a", RE_Options(PCRE_CASELESS));
		string sURL_TMP, sTitle_TMP;

		if(linkRE.PartialMatch(sResponse.c_str(), &sURL_TMP, &sTitle_TMP))
		{
			CString sURL = StripHTML(sURL_TMP), sTitle = StripHTML(sTitle_TMP);

			return (bGetURLOnly ? sURL : sURL + " " + sTitle);
		}

		return "";
	}

	CString ParseCalc(const CString& sResponse)
	{
		RE calcRE("calc[a-z0-9_-]+\\.gif.+?<b>(.+?)</b>", RE_Options(PCRE_CASELESS | PCRE_DOTALL));
		string sTmp;

		if(calcRE.PartialMatch(sResponse.c_str(), &sTmp))
		{
			return StripHTML(sTmp);
		}

		return "";
	}

	CString ParseDidYouMean(const CString& sResponse)
	{
		RE didyoumeanRE("spell.?>(.+?)</a", RE_Options(PCRE_CASELESS));
		string sTmp;

		if(didyoumeanRE.PartialMatch(sResponse.c_str(), &sTmp))
		{
			return StripHTML(sTmp);
		}

		return "";
	}

	CString ParseDefine(const CString& sResponse)
	{
		RE definitionRE("std><li>(.+?)<(?:br|/?li)", RE_Options(PCRE_CASELESS));
		string sTmp;

		if(definitionRE.PartialMatch(sResponse.c_str(), &sTmp))
		{
			return StripHTML(sTmp);
		}

		return "";
	}

public:
	CGoogleSock(CInfoBotModule *pModInstance) : CTriggerHTTPSock(pModInstance)
	{
		m_acceptEmptyArgs = false;
	}

	void Request()
	{
		Get("www.google.com", "/search?safe=off&num=1&q=" + URLEscape(m_args));
	}

	void OnRequestDone(const CString& sResponse)
	{
		const CString sPrefix = "%CL1%[%CL2%Google%CL1%]%CLO% ";
		CString sResult = ParseDefine(sResponse);
		CString sDidYouMean = ParseDidYouMean(sResponse);

		if(sResult.empty())
		{
			sResult = ParseCalc(sResponse);

			if(sResult.empty())
			{
				CString sFirstLink = ParseFirstResult(sResponse);

				if(!sFirstLink.empty())
				{
					sResult = sFirstLink.Token(0) + " %CL1%[%CL2%" + sFirstLink.Token(1, true) + "%CL1%]";
				}
			}
		}

		if(sResult.empty())
		{
			m_pMod->SendMessage(m_chan, sPrefix + "Almighty Google didn't find anything, sorry.");
		}
		else
		{
			if(!sDidYouMean.empty())
			{
				m_pMod->SendMessage(m_chan, sPrefix + "Did you mean: " + sDidYouMean);
			}

			m_pMod->SendMessage(m_chan, sPrefix + sResult);
		}
	}
};


class CImdbComSock : public CTriggerHTTPSock
{
protected:
	CString m_imdbID;
	CString m_title, m_tagline, m_director, m_country;
	VCString m_genres;
	int m_year, m_runtime, m_votes;
	float m_rating;

	bool ParseResponse(const CString& sResponse)
	{
		/* reset stuff */
		m_year = m_runtime = m_votes = 0;
		m_rating = 0.0;
		m_title.clear(); m_tagline.clear(); m_director.clear(); m_country.clear();
		m_genres.clear();

		/* extract title */
		string sTitleWithYear, sTitle;
		RE titleRE("<h1.*?>(.+?)(<span\\s+class=\"pro-link\".+?)?</h1>", RE_Options(PCRE_CASELESS | PCRE_DOTALL));

		if(titleRE.PartialMatch(sResponse.c_str(), &sTitleWithYear))
		{
			RE titleYearRE("(\\(((?:19|20)\\d{2}(?:/\\w+|))\\).*?)$");
			string sTmpYear, sTmpTitle;

			sTitleWithYear = StripHTML(sTitleWithYear);

			if(titleYearRE.PartialMatch(sTitleWithYear.c_str(), &sTmpTitle, &sTmpYear))
			{
				sTitle = sTitleWithYear.substr(0, sTitleWithYear.size() - sTmpTitle.size());

				m_year = atoi(sTmpYear.c_str());
			}
			else
			{
				sTitle = sTitleWithYear;
			}

			m_title = sTitle;
			m_title.Trim();
		}
		else
		{
			// if even extracting the title failed, we give up.
			// we probably got an error page or IMDB completely
			// changed the page structure.
			return false;
		}

		/* extract rating */
		string sTmpRating;
		RE ratingRE("star.{1,100}(\\d{1,2}(?:\\.\\d{1,2})).*?/\\s*10", RE_Options(PCRE_CASELESS | PCRE_DOTALL));

		if(ratingRE.PartialMatch(sResponse.c_str(), &sTmpRating))
		{
			m_rating = (float)CString(sTmpRating).ToDouble();

			/* extract no of votes */
			string sTmpVotes;
			RE votesRE("<a[^>]*?\\w+=\"ratings\"[^>]*?>([0-9.,]+)\\s*votes", RE_Options(PCRE_CASELESS | PCRE_DOTALL));

			if(votesRE.PartialMatch(sResponse.c_str(), &sTmpVotes))
			{
				CString sVotesFixed;

				for(size_t p = 0; p < sTmpVotes.size(); p++)
				{
					if(isdigit(sTmpVotes[p]))
					{
						sVotesFixed += sTmpVotes[p];
					}
				}

				m_votes = sVotesFixed.ToInt();
			}
		}
		else
		{
			m_votes = 0;
		}

		/* extract director */
		string sTmpDirector;
		RE directorRE("<h4.*?>\\s*Directors?:.+?\">(.+?)</a", RE_Options(PCRE_CASELESS | PCRE_DOTALL));

		if(directorRE.PartialMatch(sResponse.c_str(), &sTmpDirector))
		{
			m_director = StripHTML(sTmpDirector);
		}

		/* extract tagline */
		string sTmpTagline;
		RE taglineRE("<h4.*?>\\s*Tag\\s*lines?:\\s*</h4>\\s*(.+?)<span", RE_Options(PCRE_CASELESS | PCRE_DOTALL));

		if(taglineRE.PartialMatch(sResponse.c_str(), &sTmpTagline))
		{
			m_tagline = StripHTML(sTmpTagline);
		}

		/* extract genres */
		string sTmpGenreHTML;
		RE genreOuterRE("<h4.*?>\\s*Genres?:</h4>(.+?)</div", RE_Options(PCRE_CASELESS | PCRE_DOTALL));

		if(genreOuterRE.PartialMatch(sResponse.c_str(), &sTmpGenreHTML))
		{
			StringPiece input(sTmpGenreHTML);
			string sTmpGenre;
			RE genreRE("<a href=\"/Sections/Genres/([^\"]+?)/\">", RE_Options(PCRE_CASELESS));

			while(genreRE.FindAndConsume(&input, &sTmpGenre))
			{
				m_genres.push_back(StripHTML(sTmpGenre));
			}
		}

		/* extract runtime */
		string sTmpRuntime;
		RE runtimeRE("<h4.*?>\\s*Runtime:?\\s*</h4>\\s*(.+?)\\s*</div", RE_Options(PCRE_CASELESS | PCRE_DOTALL));

		if(runtimeRE.PartialMatch(sResponse.c_str(), &sTmpRuntime))
		{
			sTmpRuntime = StripHTML(sTmpRuntime);
			string sRuntime;
			RE runtimeCheckRE("(\\d+)\\s*min", RE_Options(PCRE_CASELESS));

			if(runtimeCheckRE.PartialMatch(sTmpRuntime, &sRuntime))
			{
				m_runtime = atoi(sRuntime.c_str());
			}
		}

		/* extract country */
		string sTmpCountry;
		RE countryRE("<h4.*?>\\s*(?:Country|Countries)\\s*.+?<a[^>]+?>(.+?)</a>", RE_Options(PCRE_CASELESS | PCRE_DOTALL));

		if(countryRE.PartialMatch(sResponse.c_str(), &sTmpCountry))
		{
			m_country = sTmpCountry;
			m_country.Trim();
		}

		return true;
	}

	void FormatAndSendInfo()
	{
		const CString sPrefix = "%CL1%[%CL2%iMDB%CL1%]%CLO% ";
		CString sLine;

		sLine = m_title;
		if(m_year > 0 || !m_country.empty()) sLine += " (";
		if(!m_country.empty()) { sLine += m_country; if(m_year > 0) sLine += " "; }
		if(m_year > 0) sLine += CString(m_year);
		if(m_year > 0 || !m_country.empty()) sLine += ")";
		if(!m_tagline.empty()) sLine += " - " + m_tagline;

		m_pMod->SendMessage(m_chan, sPrefix + sLine);
		m_pMod->SendMessage(m_chan, sPrefix + "http://www.imdb.com/title/" + m_imdbID);

		sLine = "";
		if(!m_director.empty()) sLine = "Director: " + m_director + " - ";
		sLine += "Rating: " + (m_rating > 0 ? CString(m_rating, 1) : CString("??")) + "/10 with " + (m_votes > 0 ? CString(m_votes) : CString("awaiting five")) + " votes";

		m_pMod->SendMessage(m_chan, sPrefix + sLine);

		if(m_runtime > 0 || !m_genres.empty())
		{
			sLine = (m_runtime > 0 ? "Runtime: " + CString(m_runtime) + " minutes" + (m_genres.empty() ? CString("") : CString(" - ")) : CString(""));
			if(!m_genres.empty()) sLine += "Genre: ";

			for(VCString::const_iterator it = m_genres.begin(); it != m_genres.end(); it++)
			{
				sLine += (*it) + ((it + 1) != m_genres.end() ? " / " : "");
			}

			m_pMod->SendMessage(m_chan, sPrefix + sLine);
		}
	}

public:
	CImdbComSock(CInfoBotModule *pModInstance, const CString& sImdbID) : CTriggerHTTPSock(pModInstance), m_imdbID(sImdbID) {}

	void Request()
	{
		Get("www.imdb.com", "/title/" + m_imdbID + "/");
	}

	void OnRequestDone(const CString& sResponse)
	{
		if(ParseResponse(sResponse))
		{
			FormatAndSendInfo();
		}
		else
		{
			m_pMod->SendMessage(m_chan, "%CL1%[%CL2%ERROR%CL1%]%CLO% Getting movie info from imdb.com failed, sorry.");
		}
	}
};


class CImdbGoogleSock : public CGoogleSock
{
public:
	CImdbGoogleSock(CInfoBotModule *pModInstance) : CGoogleSock(pModInstance) {}

	void Request()
	{
		Get("www.google.com", "/search?safe=off&num=1&q=" + URLEscape(m_args + " imdb inurl:title"));
	}

	void OnRequestDone(const CString& sResponse)
	{
		const CString sURL = ParseFirstResult(sResponse, true);
		string imdbID;

		RE URLCheckRE("^https?://(?:[\\w+.]+|)imdb\\.\\w+/title/(tt\\d{4,})", RE_Options(PCRE_CASELESS));

		if(URLCheckRE.PartialMatch(sURL.c_str(), &imdbID))
		{
			CImdbComSock *imdbSock = new CImdbComSock(m_pMod, imdbID);
			imdbSock->SetTriggerInfo(m_trigger, m_args, m_chan, m_nick);
			imdbSock->Request();
		}
		else
		{
			m_pMod->SendMessage(m_chan, "%CL1%[%CL2%ERROR%CL1%]%CLO% Movie not found, sorry.");
		}
	}
};


class CDefineGoogleSock : public CGoogleSock
{
public:
	CDefineGoogleSock(CInfoBotModule *pModInstance) : CGoogleSock(pModInstance) {}

	void Request()
	{
		Get("www.google.com", "/search?safe=off&q=" + URLEscape("define:" + m_args));
	}

	void OnRequestDone(const CString& sResponse)
	{
		const CString sResult = ParseDefine(sResponse);

		if(!sResult.empty())
		{
			m_pMod->SendMessage(m_chan, "%CL1%[%CL2%DEFINITION%CL1%]%CLO% " + sResult + " ... www.google.com/search?q=" + URLEscape("define:" + m_args));
		}
		else
		{
			m_pMod->SendMessage(m_chan, "%CL1%[%CL2%ERROR%CL1%]%CLO% It's undefined!");
		}
	}
};


class CCalcGoogleSock : public CGoogleSock
{
public:
	CCalcGoogleSock(CInfoBotModule *pModInstance) : CGoogleSock(pModInstance) {}

	void OnRequestDone(const CString& sResponse)
	{
		const CString sResult = ParseCalc(sResponse);

		if(!sResult.empty())
		{
			m_pMod->SendMessage(m_chan, "%CL1%[%CL2%CALC%CL1%]%CLO% " + sResult);
		}
		else
		{
			m_pMod->SendMessage(m_chan, "%CL1%[%CL2%ERROR%CL1%]%CLO% Google didn't like your calculus!");
		}
	}
};


class CTvRageComSock : public CTriggerHTTPSock
{
protected:
	CString m_tvRageID;
	CString m_title, m_tagline, m_ended, m_latest, m_next, m_rating;

	bool ParseResponse(const CString& sResponse)
	{
		string sTmp;

		m_title.clear(); m_tagline.clear(); m_ended.clear();
		m_latest.clear(); m_next.clear(); m_rating.clear();

		RE titleRE("<h3.+?>(.+?)(?:\\s*\\(\\d+\\s+\\w+\\)|)\\s*</h3", RE_Options(PCRE_CASELESS | PCRE_DOTALL));

		if(titleRE.PartialMatch(sResponse.c_str(), &sTmp))
		{
			m_title = StripHTML(sTmp);
		}
		else
		{
			return false;
		}

		RE taglineRE("</table><center><b><font size=\"2\">(.+?)</b", RE_Options(PCRE_CASELESS | PCRE_DOTALL));

		sTmp.clear();
		if(taglineRE.PartialMatch(sResponse.c_str(), &sTmp))
		{
			m_tagline = StripHTML(sTmp);
		}

		RE endedRE("<b>Ended:.+?<td>(\\w+\\s+\\d+,\\s+\\d{4})</td>", RE_Options(PCRE_CASELESS | PCRE_DOTALL));

		sTmp.clear();
		if(endedRE.PartialMatch(sResponse.c_str(), &sTmp))
		{
			m_ended = StripHTML(sTmp);
		}

		if(m_ended.empty())
		{
			RE latestEpRE("Latest Episode:.+?/episodes/.+?>\\d+:\\s+(.+?)</span", RE_Options(PCRE_CASELESS | PCRE_DOTALL));

			sTmp.clear();
			if(latestEpRE.PartialMatch(sResponse.c_str(), &sTmp))
			{
				m_latest = StripHTML(sTmp);
			}

			RE nextEpRE("Next Episode:.+?/episodes/.+?>\\d+:\\s+(.+?)</span", RE_Options(PCRE_CASELESS | PCRE_DOTALL));

			sTmp.clear();
			if(nextEpRE.PartialMatch(sResponse.c_str(), &sTmp))
			{
				m_next = StripHTML(sTmp);
			}
		}

		RE ratingRE(">([\\d.]{3,4}/10 \\(\\d+ Votes cast\\))<", RE_Options(PCRE_CASELESS));

		sTmp.clear();
		if(ratingRE.PartialMatch(sResponse.c_str(), &sTmp))
		{
			m_rating = StripHTML(sTmp);
		}

		return true;
	}

	void FormatAndSendInfo()
	{
		const CString sPrefix = "%CL1%[%CL2%TvRage%CL1%]%CLO% ";
		CString sLine;

		sLine = m_title + " - ";
		if(!m_tagline.empty()) sLine += m_tagline + " - ";
		sLine += " http://www.tvrage.com/" + m_tvRageID;

		m_pMod->SendMessage(m_chan, sPrefix + sLine);

		if(!m_rating.empty()) m_pMod->SendMessage(m_chan, sPrefix + "Rating: " + m_rating);

		if(!m_ended.empty())
		{
			m_pMod->SendMessage(m_chan, sPrefix + "Show ended: " + m_ended);
		}
		else
		{
			if(!m_latest.empty()) m_pMod->SendMessage(m_chan, sPrefix + "Latest Ep.: " + m_latest);
			if(!m_next.empty()) m_pMod->SendMessage(m_chan, sPrefix + "Next Ep.: " + m_next);
		}
	}

public:
	CTvRageComSock(CInfoBotModule *pModInstance, const CString& sTvRageID) : CTriggerHTTPSock(pModInstance), m_tvRageID(sTvRageID) {}

	void Request()
	{
		Get("www.tvrage.com", "/" + m_tvRageID);
	}

	void OnRequestDone(const CString& sResponse)
	{
		if(ParseResponse(sResponse))
		{
			FormatAndSendInfo();
		}
		else
		{
			m_pMod->SendMessage(m_chan, "%CL1%[%CL2%ERROR%CL1%]%CLO% Getting show info from tvrage.com failed, sorry.");
		}
	}
};


class CTvRageGoogleSock : public CGoogleSock
{
public:
	CTvRageGoogleSock(CInfoBotModule *pModInstance) : CGoogleSock(pModInstance) {}

	void Request()
	{
		Get("www.google.com", "/search?safe=off&num=1&q=" + URLEscape(m_args + " site:tvrage.com"));
	}

	void OnRequestDone(const CString& sResponse)
	{
		const CString sURL = ParseFirstResult(sResponse, true);
		string tvRageID;

		RE URLCheckRE("^https?://(?:www\\.|)tvrage\\.com/((?:shows/)?[^/]+?)(?:/|$)", RE_Options(PCRE_CASELESS));

		if(URLCheckRE.PartialMatch(sURL.c_str(), &tvRageID))
		{
			CTvRageComSock *tvRageSock = new CTvRageComSock(m_pMod, tvRageID);
			tvRageSock->SetTriggerInfo(m_trigger, m_args, m_chan, m_nick);
			tvRageSock->Request();
		}
		else
		{
			m_pMod->SendMessage(m_chan, "%CL1%[%CL2%ERROR%CL1%]%CLO% TV show not found, sorry.");
		}
	}
};


class CWeatherSock : public CTriggerHTTPSock
{
protected:
	VCString m_lines;

	bool ParseResponse(const CString& sResponse)
	{
		// parsing XML via regex is a bad idea since it easily breaks!

		m_lines.clear();

		if(true)
		{
			StringPiece input(sResponse);
			string sTmpTitle, sTmpText;
			RE forecastRE("<forecastday>.*?<title>(.+?)</title>.*?<fcttext>(.+?)</fcttext>.*?</forecastday>", RE_Options(PCRE_CASELESS | PCRE_DOTALL));

			while(forecastRE.FindAndConsume(&input, &sTmpTitle, &sTmpText))
			{
				m_lines.push_back(StripHTML(sTmpTitle) + ": " + StripHTML(sTmpText));

				if(m_lines.size() >= 3) break;
			}
		}

		if(m_lines.size() == 0)
		{
			// fallback.

			StringPiece input(sResponse);
			int iHighF, iHighC, iLowF, iLowC;
			string sTmpWeekday, sTmpConditions;
			RE forecastRE("<forecastday>.*?<weekday>(.+?)</weekday>.*?"
				"<high>.*?<fahrenheit>(\\d+)</fahrenheit>.*?<celsius>(\\d+)</celsius>.*?</high>.*?"
				"<low>.*?<fahrenheit>(\\d+)</fahrenheit>.*?<celsius>(\\d+)</celsius>.*?</low>.*?<conditions>(.+?)</conditions>.*?</forecastday>",
				RE_Options(PCRE_CASELESS | PCRE_DOTALL));

			while(forecastRE.FindAndConsume(&input, &sTmpWeekday, &iHighF, &iHighC, &iLowF, &iLowC, &sTmpConditions))
			{
				m_lines.push_back(StripHTML(sTmpWeekday) + ": " + StripHTML(sTmpConditions) + ", " +
					CString(iLowF) + "°F to " + CString(iHighF) + "°F (" + CString(iLowC) + "°C to " + CString(iHighC) + "°C)");

				if(m_lines.size() >= 2) break;
			}
		}

		return (m_lines.size() > 0);
	}

	void FormatAndSendInfo()
	{
		const CString sPrefix = "%CL1%[%CL2%WEATHER%CL1%]%CLO% ";

		for(VCString::const_iterator it = m_lines.begin(); it != m_lines.end(); it++)
		{
			CString sTmp = *it;

			if((it + 1) == m_lines.end())
			{
				sTmp += " %CL1%[%CL2%www.wunderground.com%CL1%]";
			}

			m_pMod->SendMessage(m_chan, sPrefix + sTmp);
		}
	}

public:
	CWeatherSock(CInfoBotModule *pModInstance) : CTriggerHTTPSock(pModInstance) {}

	void Request()
	{
		Get("api.wunderground.com", "/auto/wui/geo/ForecastXML/index.xml?query=" + URLEscape(m_args));
	}

	void OnRequestDone(const CString& sResponse)
	{
		if(ParseResponse(sResponse))
		{
			FormatAndSendInfo();
		}
		else
		{
			m_pMod->SendMessage(m_chan, "%CL1%[%CL2%ERROR%CL1%]%CLO% Getting weather info from wunderground.com failed, sorry.");
		}
	}
};


class CWetterSock : public CTriggerHTTPSock
{
public:
	CWetterSock(CInfoBotModule *pModInstance) : CTriggerHTTPSock(pModInstance) {}

	void Request()
	{
		Get("www.donnerwetter.de", "/region/suchort.mv?search=" + URLEscape(m_args));
	}

	void OnRequestDone(const CString& sResponse)
	{
		RE sWetterRE("middleblue\"><strong><em>(.+?)</em></strong></span>.*?<strong>(\\w+, \\d{1,2}\\.\\d{1,2}\\.\\d{4})<.+?<tr><td>(.+?)</td></tr></table>", RE_Options(PCRE_CASELESS | PCRE_DOTALL));
		string sTmpCity, sTmpDay, sTmpForecast;

		if(sWetterRE.PartialMatch(sResponse.c_str(), &sTmpCity, &sTmpDay, &sTmpForecast))
		{
			const CString sPrefix = "%CL1%[%CL2%WETTER%CL1%]%CLO% ";

			m_pMod->SendMessage(m_chan, sPrefix + StripHTML(sTmpCity) + ": " + StripHTML(sTmpDay) + ": " + StripHTML(sTmpForecast));
		}
		else if(RE("Es\\s+wurden\\s+folgende\\s+Orte\\s+gefunden").PartialMatch(sResponse.c_str()))
		{
			m_pMod->SendMessage(m_chan, "%CL1%[%CL2%ERROR%CL1%]%CLO% Es wurden mehrere Orte gefunden. Bitte die PLZ verwenden!");
		}
		else
		{
			m_pMod->SendMessage(m_chan, "%CL1%[%CL2%ERROR%CL1%]%CLO% Getting weather info from www.donnerwetter.de failed, sorry.");
		}
	}
};


/***** CInfoBotModule implementation below *****/


bool CInfoBotModule::IsTriggerSupported(const CString& sTriggerName)
{
	for(int i = 0; i < NR_OF_TRIGGERS; i++)
	{
		if(sTriggerName.Equals(TRIGGERS[i]))
			return true;
	}
	return false;
}

bool CInfoBotModule::IsTriggerEnabled(const CString& sChan, const CString& sTriggerName)
{
	const VCString& vEnabled = m_enabledCmds[sChan];

	for(VCString::const_iterator it = vEnabled.begin(); it != vEnabled.end(); it++)
	{
		if(sTriggerName.Equals(*it))
			return true;
	}

	return false;
}

void CInfoBotModule::DisableTrigger(const CString& sChan, const CString& sTriggerName)
{
	VCString& vEnabled = m_enabledCmds[sChan];

	for(VCString::iterator it = vEnabled.begin(); it != vEnabled.end(); it++)
	{
		if(sTriggerName.Equals(*it))
		{
			vEnabled.erase(it);
			break;
		}
	}

	if(vEnabled.empty())
	{
		m_enabledCmds.erase(sChan);
	}
}

void CInfoBotModule::CheckLineForTrigger(const CString& sMessage, const CString& sChannel, const CString& sNick)
{
	if(sMessage.empty() || sMessage[0] != TriggerChar(sChannel))
		return;

	CString sTrigger = sMessage.Token(0).AsLower();
	sTrigger.erase(0, 1);
	const CString sArgs = sMessage.Token(1, true);

	if(!IsTriggerSupported(sTrigger) || !IsTriggerEnabled(sChannel.AsLower(), sTrigger))
		return;

	CTriggerHTTPSock *pTriggerSock = NULL;

	if(sTrigger == "google")
	{
		pTriggerSock = new CGoogleSock(this);
	}
	else if(sTrigger == "imdb")
	{
		pTriggerSock = new CImdbGoogleSock(this);
	}
	else if(sTrigger == "time")
	{
		char buf[100] = {0};
		time_t rawtime;
		tm *timeinfo;

		time(&rawtime);
		timeinfo = localtime(&rawtime);

		if(timeinfo)
		{
			strftime(buf, 100, "%A, %B %d %Y, %H:%M:%S %Z", timeinfo);

			SendMessage(sChannel, "Hey " + sNick + ", it's " + buf + "!");
		}
	}
	else if(sTrigger == "uptime")
	{
		SendMessage(sChannel, "This ZNC has been running for " + CZNC::Get().GetUptime());
	}
	else if(sTrigger == "8ball")
	{
		if(!sArgs.empty())
		{
			SendMessage(sChannel, Do8Ball());
		}
	}
	else if(sTrigger == "calc")
	{
		pTriggerSock = new CCalcGoogleSock(this);
	}
	else if(sTrigger == "define")
	{
		pTriggerSock = new CDefineGoogleSock(this);
	}
	else if(sTrigger == "tvrage")
	{
		pTriggerSock = new CTvRageGoogleSock(this);
	}
	else if(sTrigger == "weather")
	{
		pTriggerSock = new CWeatherSock(this);
	}
	else if(sTrigger == "wetter")
	{
		pTriggerSock = new CWetterSock(this);
	}
	else if(sTrigger == "help")
	{
		CString sCmds = "%CL1%[%CL2%HELP%CL1%]%CLO% ";

		for(VCString::const_iterator itt = m_enabledCmds[sChannel].begin(); itt != m_enabledCmds[sChannel].end(); itt++)
		{
			if(*itt != "help")
			{
				sCmds += TriggerChar(sChannel) + *itt + " ";
			}
		}

		SendMessage(sChannel, sCmds);
	}

	if(pTriggerSock != NULL)
	{
		if(!sArgs.empty() || pTriggerSock->AcceptEmptyArgs())
		{
			pTriggerSock->SetTriggerInfo(sTrigger, sArgs, sChannel, sNick);
		}

		pTriggerSock->Request();
	}
}

bool CInfoBotModule::ColorsEnabled(const CString& sChan)
{
	const CString sChanLower = sChan.AsLower();

	// colors enabled by default:
	return (m_coloredChans.find(sChanLower) == m_coloredChans.end() || m_coloredChans[sChanLower]);
}

char CInfoBotModule::TriggerChar(const CString& sChan)
{
	const CString sChanLower = sChan.AsLower();

	// default is !
	return (m_triggerChars.find(sChanLower) != m_triggerChars.end() ? m_triggerChars[sChanLower] : '!');
}

void CInfoBotModule::SendMessage(const CString& sSendTo, const CString& sMsg)
{
	CString sText = sMsg;

	if(ColorsEnabled(sSendTo))
	{
		sText.Replace("%CL1%", "\x03" + CString(m_colorOne));
		sText.Replace("%CL2%", "\x03" + CString(m_colorTwo));
		sText.Replace("%CLO%", "\x03");

		// forcefully reset colors before applying our own by
		// prepending \x03\x03 to the msg.
		// also fix other double \x03s on the way.

		sText = "\x03" + sText;
		sText.Replace("\x03\x03", "\x03");

		sText = "\x03" + sText;
	}
	else
	{
		sText.Replace("%CL1%", "");
		sText.Replace("%CL2%", "");
		sText.Replace("%CLO%", "");
	}

	m_pUser->PutIRC("PRIVMSG " + sSendTo + " :" + sText);
	m_pUser->PutUser(":" + m_pUser->GetIRCNick().GetNickMask() + " PRIVMSG " + sSendTo + " :" + sText);
}

void CInfoBotModule::OnModCommand(const CString& sCommand)
{
	const CString sCmd = sCommand.Token(0).AsUpper();

	if(sCmd == "ENABLE" || sCmd == "DISABLE")
	{
		bool bEnable = (sCmd == "ENABLE");
		const CString sChan = sCommand.Token(1).AsLower();
		const CString sTrigger = sCommand.Token(2).AsLower();

		if(sChan.empty() || !IsTriggerSupported(sTrigger))
		{
			PutModule("Syntax: " + sCmd + " #chan (" + CString(TRIGGERS_STR) + ")");
		}
		else if(!IsTriggerEnabled(sChan, sTrigger))
		{
			if(bEnable)
			{
				m_enabledCmds[sChan].push_back(sTrigger);
				PutModule("Enabled !" + sTrigger + " on " + sChan + "!");
			}
			else
			{
				PutModule("!" + sTrigger + " is not enabled on " + sChan + "!");
			}
		}
		else
		{
			if(bEnable)
			{
				PutModule("!" + sTrigger + " is already enabled on " + sChan + "!");
			}
			else
			{
				DisableTrigger(sChan, sTrigger);
				PutModule("Disabled !" + sTrigger + " on " + sChan + "!");
			}
		}

		SaveSettings();
	}
	else if(sCmd == "LIST" || sCmd == "SHOW" || sCmd == "CHANS" || sCmd == "CHANNELS")
	{
		CTable ChanTable;

		if(m_enabledCmds.empty())
		{
			PutModule("No channels set up yet! Use the ENABLE command.");
			return;
		}

		ChanTable.AddColumn("Channel");
		ChanTable.AddColumn("Triggers");
		ChanTable.AddColumn("Colors");

		for(TEnabledChanMap::const_iterator itc = m_enabledCmds.begin(); itc != m_enabledCmds.end(); itc++)
		{
			CString sTriggers;

			ChanTable.AddRow();
			ChanTable.SetCell("Channel", itc->first);

			for(VCString::const_iterator itt = itc->second.begin(); itt != itc->second.end(); itt++)
			{
				sTriggers += TriggerChar(itc->first) + *itt + " ";
			}

			ChanTable.SetCell("Triggers", sTriggers);
			ChanTable.SetCell("Colors", CString(ColorsEnabled(itc->first)));
		}

		PutModule(ChanTable);

		PutModule("\x03\x03""Colors: \x03" + CString(m_colorOne) + "[\x03" +  CString(m_colorTwo) + "LikeThis\x03" + CString(m_colorOne) + "]\x03 Example.");
	}
	else if(sCmd == "COLOR" || sCmd == "COLORS" || sCmd == "COLOUR" || sCmd == "COLOURS")
	{
		const CString sAction = sCommand.Token(1).AsLower();
		const CString sParam = sCommand.Token(2).AsLower();

		if((sAction == "one" || sAction == "two") && !sParam.empty())
		{
			int iColor = sParam.ToInt();

			if(iColor >= 0 && iColor <= 15)
			{
				if(sAction == "one")
					m_colorOne = iColor;
				else
					m_colorTwo = iColor;

				PutModule("Changed color " + sAction + " to \x03" + (iColor < 10 ? "0" + CString(iColor) : CString(iColor)) + CString(iColor) + "\x03.");
				SaveSettings();
			}
			else
			{
				PutModule("Invalid color. Needs to be 0 to 15.");
			}
		}
		else if(sParam == "on" || sParam == "off" || sParam == "yes" || sParam == "no" || sParam == "true" || sParam == "false")
		{
			if(sParam == "on" || sParam == "yes" || sParam == "true")
			{
				m_coloredChans.erase(sAction);
				PutModule("Activated color on " + sAction + ".");
			}
			else
			{
				m_coloredChans[sAction] = false;
				PutModule("Deactivated color on " + sAction + ".");
			}

			SaveSettings();
		}
		else
		{
			PutModule("Syntax: COLOR (ONE|TWO) (0-15) / COLOR #chan (ON|OFF)");
		}
	}
	else if(sCmd == "TRIGGERCHAR")
	{
		const CString sChan = sCommand.Token(1).AsLower();
		const CString sParam = sCommand.Token(2);

		if(!sChan.empty() && !sParam.empty())
		{
			m_triggerChars[sChan] = sParam[0];
			PutModule("Set trigger char on " + sChan + " to '" + CString(sParam[0]) + "'.");

			SaveSettings();
		}
		else
		{
			PutModule("Syntax: TRIGGERCHAR #chan C (where C is the char, like ! or .)");
		}
	}
	else if(sCmd == "TRIGGERS")
	{
		CTable TriggerTable;

		TriggerTable.AddColumn("Trigger");
		TriggerTable.AddColumn("Description");

		for(int i = 0; i < NR_OF_TRIGGERS; i++)
		{
			TriggerTable.AddRow();
			TriggerTable.SetCell("Trigger", "!" + CString(TRIGGERS[i]));
			TriggerTable.SetCell("Description", TRIGGER_DESCRIPTIONS[i]);
		}

		PutModule(TriggerTable);
	}
	else if(sCmd == "HELP")
	{
		CTable CmdTable;

		CmdTable.AddColumn("Command");
		CmdTable.AddColumn("Description");

		CmdTable.AddRow();
		CmdTable.SetCell("Command", "ENABLE <#chan> <trigger>");
		CmdTable.SetCell("Description", "Allows everyone on <#chan> to use <trigger>.");

		CmdTable.AddRow();
		CmdTable.SetCell("Command", "DISABLE <#chan> <trigger>");
		CmdTable.SetCell("Description", "Deactivates usage of <trigger> on <#chan>.");

		CmdTable.AddRow();
		CmdTable.SetCell("Command", "TRIGGERS");
		CmdTable.SetCell("Description", "Lists all available triggers.");

		CmdTable.AddRow();
		CmdTable.SetCell("Command", "TRIGGERCHAR <#chan> <C>");
		CmdTable.SetCell("Description", "Sets the trigger char on <#chan> to <C>. Typically '.' or '@'. Default is '!'.");

		CmdTable.AddRow();
		CmdTable.SetCell("Command", "COLOR <#chan> <ON|OFF>");
		CmdTable.SetCell("Description", "(De)activates color codes on <#chan>.");

		CmdTable.AddRow();
		CmdTable.SetCell("Command", "COLOR <ONE|TWO> <1-15>");
		CmdTable.SetCell("Description", "Two colors are used in command results. You can change them to a different color code.");

		CmdTable.AddRow();
		CmdTable.SetCell("Command", "SHOW");
		CmdTable.SetCell("Description", "Shows the active channels and their settings.");

		CmdTable.AddRow();
		CmdTable.SetCell("Command", "HELP");
		CmdTable.SetCell("Description", "This help.");

		PutModule(CmdTable);
	}
	else
	{
		PutModule("Unknown command! Try HELP.");
	}
}

bool CInfoBotModule::OnLoad(const CString& sArgs, CString& sMessage)
{
	LoadSettings();
	return true;
}

void CInfoBotModule::SaveSettings()
{
	ClearNV();

	for(TEnabledChanMap::const_iterator itc = m_enabledCmds.begin(); itc != m_enabledCmds.end(); itc++)
	{
		CString sName = "chan:" + itc->first;
		CString sData;

		for(VCString::const_iterator itt = itc->second.begin(); itt != itc->second.end(); itt++)
		{
			sData += "enable=" + *itt + "\n";
		}

		if(TriggerChar(itc->first) != '!') sData += "trigger=" + CString(TriggerChar(itc->first)) + "\n";
		sData += "colors=" + CString(ColorsEnabled(itc->first) ? 1 : 0);

		SetNV(sName, sData, false);
	}

	SetNV("color:1", CString(m_colorOne), false);
	SetNV("color:2", CString(m_colorTwo), true); // last change writes to disk!
}

void CInfoBotModule::LoadSettings()
{
	for(MCString::iterator it = BeginNV(); it != EndNV(); it++)
	{
		if(it->first.Left(5) == "chan:")
		{
			const CString sChanName = it->first.substr(5);
			pcrecpp::StringPiece input(it->second);

			string left, right;
			pcrecpp::RE re("(\\w+) ?= ?(.+?)\\r?\\n");
			while(re.Consume(&input, &left, &right))
			{
				if(left == "enable")
				{
					m_enabledCmds[sChanName].push_back(right);
				}
				else if(left == "colors")
				{
					m_coloredChans[sChanName] = (right != "0");
				}
				else if(left == "trigger" && !right.empty())
				{
					m_triggerChars[sChanName] = right[0];
				}
			}
		}
		else if(it->first == "color:1")
		{
			m_colorOne = atoi(it->second.c_str());
		}
		else if(it->first == "color:2")
		{
			m_colorTwo = atoi(it->second.c_str());
		}
	}
}

CInfoBotModule::EModRet CInfoBotModule::OnChanMsg(CNick& Nick, CChan& Channel, CString& sMessage)
{
	CheckLineForTrigger(sMessage, Channel.GetName().AsLower(), Nick.GetNick());
	return CONTINUE;
}

CInfoBotModule::EModRet CInfoBotModule::OnUserMsg(CString& sTarget, CString& sMessage)
{
	if(!sTarget.empty() && !isalnum(sTarget[0]))
	{
		CheckLineForTrigger(sMessage, sTarget.AsLower(), m_pUser->GetIRCNick().GetNick());
	}
	return CONTINUE;
}

CInfoBotModule::~CInfoBotModule()
{
}

CString CInfoBotModule::Do8Ball()
{
	static const char *answers[21] = {
		"Are you mad?!",
		"As I see it: yes",
		"Ask again later",
		"Better not tell you now",
		"Cannot predict now",
		"Concentrate and ask again",
		"Don't count on it",
		"It is certain",
		"It is decidedly so",
		"Most likely",
		"My reply is no",
		"My sources say no",
		"No",
		"Outlook good",
		"Outlook not so good",
		"Signs point to yes",
		"Very doubtful",
		"Without a doubt",
		"Yes - definitely",
		"Yes",
		"You may rely on it",
	};

	return answers[rand() % 21];
}

const char* CInfoBotModule::TRIGGERS[NR_OF_TRIGGERS] = {
	"google",
	"imdb",
	"time",
	"uptime",
	"8ball",
	"define",
	"calc",
	"tvrage",
	"weather",
	"wetter",
	"help"
};
const char* CInfoBotModule::TRIGGER_DESCRIPTIONS[NR_OF_TRIGGERS] = {
	"Does a Google search.",
	"Does a search on IMDB.",
	"Shows the current time.",
	"Shows how long ZNC has been running.",
	"Ask the magic 8ball anything!",
	"Looks up a definition.",
	"Calculates simple formulas, using Google.",
	"Looks up a show on tvrage.com.",
	"Shows weather info (US/international)",
	"Shows weather info (Germany)",
	"Shows the available triggers"
};
const char* CInfoBotModule::TRIGGERS_STR = "google|imdb|time|uptime|8ball|define|calc|tvrage|weather|wetter|help";


MODULEDEFS(CInfoBotModule, "Provides commands like !google, !imdb, !weather and !8ball to selected channels")
