// Copyright © 2008-2011 Rick Parrish

#include "../Stream/Stream.h"
#include <tchar.h>
#include <vector>
#include <string>

#pragma once

namespace XML
{

class Writer
{
	struct entry
	{
		std::string Element;
		bool Children;
		size_t Skipped;

		entry() : Children(false), Skipped(0) { };
		entry(const entry &copy) : Children(copy.Children), Element(copy.Element), Skipped(copy.Skipped) { };
	};

	IOutputStream *_pStream;
	std::vector<entry> _stack;

	void adopt();
	void writeString(std::string &strText);
	void writeString(std::wstring &strText);
	void writeString(const wchar_t *strText);
	void writeString(const char *strText);
	// does not process entities
	bool writeAttributeRaw(const char *strAttribute, const char *strValue);

public:
	bool writeStartElement(const char *strElement);
	bool writeAttribute(const char *strAttribute, const TCHAR *strValue);
	bool writeAttribute(const char *strAttribute, short iValue);
	bool writeAttribute(const char *strAttribute, int iValue);
	bool writeAttribute(const char *strAttribute, long iValue);
	bool writeAttribute(const char *strAttribute, unsigned char iValue);
	bool writeAttribute(const char *strAttribute, unsigned short iValue);
	bool writeAttribute(const char *strAttribute, unsigned long iValue);
	bool writeAttribute(const char *strAttribute, bool bValue);
	bool writeAttribute(const char *strAttribute, time_t iValue);
	bool writeAttribute(const char *strAttribute, const char *strFormat, double fValue);
	bool writeAttribute(const char *strAttribute, const char *strFormat, long iValue);
	bool writeAttribute(const char *strAttribute, const char *strFormat, unsigned long iValue);
	bool writeEndElement();
	bool writeStringElement(const char *strElement, const TCHAR *strValue);
	bool writePCData(const TCHAR *strPCData);
	bool open(IOutputStream *);
	void close();
	Writer();
};

};