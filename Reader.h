// Copyright © 2008-2011 Rick Parrish

#include <map>
#include <hash_map>
#include <vector>
#include <list>
#include "../Stream/Parser.h"

#pragma once

namespace XML
{

// XML parser, reads from an IInputStream.
// The byte stream must be UTF-8 or ASCII. UTF-16 is not supported.
// UTF-16 applications are supported by transcoding text from UTF-8 to UTF-16.
// The following convention was adopted for performance:
// a. Element & attribute names are always specified as char, not wchar_t.
// This tactic avoids repetitively transcoding to compare against incoming text. 
class Reader
{
	struct entry
	{
		std::string Element;
		bool Children;
		size_t Skipped;

		entry() : Children(false), Skipped(0) { };
		entry(const entry &copy) : Children(copy.Children), Element(copy.Element), Skipped(copy.Skipped) { };
	};

	Parser _parser;
	// list of attributes for most recent XML element.
	std::list< std::pair<std::string, std::string> > _attributes;
	// stack of nested elements.
	std::list<entry> _stack;
	// true when consumed the opening '<' bracket of an element.
	bool _bStart;
	// number of skipped elements.
	size_t _iSkipped;

	// recursive descent parsing functions:

	// parse a token
	bool parseToken(std::string &strToken);
	// parse attribute=quoted-value sequence.
	bool parseAttribute();
	// skips / consumes whitespace.
	// bInside - true if inside an element declaration eg. between '<' and '>'.
	bool skipspace(bool bInside);

public:
	Reader();
	// connect parser to an input stream.
	bool open(IInputStream *pStream);
	// close parsing
	void close();
	// True if parser has reached end of stream
	// because of the read-ahead buffer, this is not the same
	// as EOF on the underlying stream.
	// EOF true here means the parser has reached the end of the stream.
	bool eof();

	// returns true if start of an element.
	bool isStartElement();
	// returns true if start of the named element.
	bool isStartElement(const char *strElement);
	// returns true start of an element is successfully consumed.
	// if true, the element's attributes are available below through getAttribute.
	bool readStartElement();
	// returns true if start of named element is successfully consumed.
	// if true, the element's attributes are available below through getAttribute.
	bool readStartElement(const char *strElement);
	// returns true if current element is self-closing OR no more nested content remains
	// eg. cursor is positioned at the closing tag.
	bool isEndElement();
	// conclude self-closing element OR consume closing element.
	bool readEndElement(bool bSkip);
	// conclude named self-closing element OR consume named closing element.
	bool readEndElement(bool bSkip, const char *strElement);
	// read text content: assumes element with text only - no child elements.
	bool readStringElement(const char *strElement, std::string &strValue);
	bool readStringElement(const char *strElement, std::wstring &strValue);
	// retrieve text for the named attribute.
	bool getAttribute(const char *strAttribute, std::string &strValue);
	bool getAttribute(const char *strAttribute, std::wstring &strValue);
	// retrieve PC Data (free text nodes under an element).
	bool readPCData(std::string &strData);
	bool readPCData(std::wstring &strData);
	// begin/end iterators for current element's attributes.
	bool enumAttributes(std::list< std::pair<std::string, std::string> >::iterator &itBegin, 
		std::list< std::pair<std::string, std::string> >::iterator &itEnd);
	// get current element's name
	bool getElementName(std::string &strElement);
	// number of child elements skipped.
	size_t getSkipped(bool bDocument) const;
};

};