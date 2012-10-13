// Copyright © 2008-2011 Rick Parrish

#include "Writer.h"

namespace XML
{

static void append(std::string &strValue, TCHAR ch, mbstate_t &state)
{
#ifdef UNICODE
	char dest[4] = {0};
	size_t used = 0;
	errno_t err = wcrtomb_s(&used, dest, sizeof dest, ch, &state);
	if (err == 0)
		strValue.append(dest, used);
#else
	state;
	strValue += (char)ch;
#endif
}

static void insertEntities(const TCHAR *strText, std::string &strEntity)
{
	mbstate_t state = {0};
	strEntity.reserve(64);
	while (*strText != 0)
	{
		TCHAR ch = *strText++;
		switch (ch)
		{
			case _T('\''):
				strEntity += "&apos;";
				break;
			case _T('&'):
				strEntity += "&amp;";
				break;
			case _T('<'):
				strEntity += "&lt;";
				break;
			case _T('>'):
				strEntity += "&gt;";
				break;
			case _T('"'):
				strEntity += "&quot;";
				break;
			default:
				append(strEntity, ch, state);
				break;
		}
	}
}

Writer::Writer() :
	_pStream(NULL)
{
}

// call this for any action that will cause the current element on the 
// stack to become a parent node (eg. starting a new child element).
void Writer::adopt()
{
	if (_stack.size() && !_stack.back().Children )
	{
		_stack.back().Children = true;
		writeString(">");
	}
}

bool Writer::writePCData(const TCHAR *strPCData)
{
	std::string strEntity;
	insertEntities(strPCData, strEntity);
	adopt();
	writeString(strEntity.c_str());
	return true;
}

bool Writer::writeStartElement(const char *strElement)
{
	entry e;
	e.Element = strElement;
	// call before pushing new element onto stack.
	adopt();
	_stack.push_back(e);
	writeString("<");
	writeString(strElement);
	return true;
}

bool Writer::writeAttributeRaw(const char *strAttribute, const char *strValue)
{
	writeString(" ");
	writeString(strAttribute);
	writeString("=\"");
	writeString(strValue);
	writeString("\"");
	return true;
}

bool Writer::writeAttribute(const char *strAttribute, const TCHAR *strValue)
{
	std::string strEntity;
	insertEntities(strValue, strEntity);
	return writeAttributeRaw(strAttribute, strEntity.c_str());
}

bool Writer::writeAttribute(const char *strAttribute, int iValue)
{
	char strValue[32] = {0};
	sprintf_s(strValue, 32, "%d", iValue);
	return writeAttributeRaw(strAttribute, strValue);
}

bool Writer::writeAttribute(const char *strAttribute, long iValue)
{
	char strValue[32] = {0};
	sprintf_s(strValue, 32, "%ld", iValue);
	return writeAttributeRaw(strAttribute, strValue);
}

bool Writer::writeAttribute(const char *strAttribute, unsigned long iValue)
{
	char strValue[32] = {0};
	sprintf_s(strValue, 32, "%ld", iValue);
	return writeAttributeRaw(strAttribute, strValue);
}

bool Writer::writeAttribute(const char *strAttribute, short iValue)
{
	char strValue[32] = {0};
	sprintf_s(strValue, 32, "%hd", iValue);
	return writeAttributeRaw(strAttribute, strValue);
}

bool Writer::writeAttribute(const char *strAttribute, unsigned short iValue)
{
	char strValue[32] = {0};
	sprintf_s(strValue, 32, "%hu", iValue);
	return writeAttributeRaw(strAttribute, strValue);
}

bool Writer::writeAttribute(const char *strAttribute, unsigned char iValue)
{
	char strValue[32] = {0};
	sprintf_s(strValue, 32, "%u", iValue);
	return writeAttributeRaw(strAttribute, strValue);
}

bool Writer::writeAttribute(const char *strAttribute, time_t iValue)
{
	char strValue[32] = {0};
	sprintf_s(strValue, 32, "%ld", iValue);
	return writeAttributeRaw(strAttribute, strValue);
}

bool Writer::writeAttribute(const char *strAttribute, bool bValue)
{
	return writeAttributeRaw(strAttribute, bValue ? "1" : "0");
}

bool Writer::writeAttribute(const char *strAttribute, const char *strFormat, double fValue)
{
	char strValue[32] = {0};
	sprintf_s(strValue, 32, strFormat, fValue);
	return writeAttributeRaw(strAttribute, strValue);
}

bool Writer::writeAttribute(const char *strAttribute, const char *strFormat, long iValue)
{
	char strValue[32] = {0};
	sprintf_s(strValue, 32, strFormat, iValue);
	return writeAttributeRaw(strAttribute, strValue);
}

bool Writer::writeAttribute(const char *strAttribute, const char *strFormat, unsigned long iValue)
{
	char strValue[32] = {0};
	sprintf_s(strValue, 32, strFormat, iValue);
	return writeAttributeRaw(strAttribute, strValue);
}

bool Writer::writeEndElement()
{
	if (_stack.size())
	{
		if (_stack.back().Children)
		{
			writeString("</");
			writeString(_stack.back().Element.c_str());
			writeString(">\n");
		}
		else
			writeString(" />\n");
		_stack.pop_back();
		return true;
	}
	return false;
}

bool Writer::writeStringElement(const char *strElement, const TCHAR *strValue)
{
	std::string strEntity;
	insertEntities(strValue, strEntity);

	adopt();
	writeString("<");
	writeString(strElement);
	writeString(">");
	writeString(strEntity.c_str());
	writeString("</");
	writeString(strElement);
	writeString(">");
	return true;
}

bool Writer::open(IOutputStream *pStream)
{
	const char strPreamble[] = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n";
	if (_pStream != NULL)
		_pStream->Close();
	_pStream = pStream;
	size_t iWrote = 0;
	return _pStream->Write( (unsigned char *)strPreamble, _countof(strPreamble) - 1, iWrote );
}

void Writer::close()
{
	_pStream->Close();
	_pStream = NULL;
}

void Writer::writeString(const char *strText)
{
	size_t iLen = strlen(strText);
	size_t iWrote = 0;
	_pStream->Write((unsigned char *)strText, iLen, iWrote);
}

void Writer::writeString(const wchar_t *strText)
{
	size_t iLen = wcslen(strText);

	std::string strTran;
	strTran.resize(iLen * 2 + 1);
	wcstombs_s(&iLen, &strTran[0], strTran.size(), strText, iLen);
	strTran.resize(iLen);
	writeString(strTran.c_str());
}

void Writer::writeString(std::string &strText)
{
	writeString(strText.c_str());
}

void Writer::writeString(std::wstring &strText)
{
	writeString(strText.c_str());
}

};