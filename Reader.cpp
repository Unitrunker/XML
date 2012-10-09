// Copyright © 2008-2011 Rick Parrish

#include "Reader.h"
#include <tchar.h>

namespace XML
{

// handles transcoding as needed for appending byte characters to a string.
static void append(std::wstring &strValue, char ch, mbstate_t &state)
{
	wchar_t dest[4] = {0};
	if ( mbrtowc(dest, &ch, 1, &state) > 0)
		strValue += dest;
}

// handles transcoding as needed for appending byte characters to a string.
static void append(std::string &strValue, char ch)
{
	strValue += (TCHAR)ch;
}

// Expand the five standard XML entities to their text values.
// &amp; &lt; &gt; &apos; &quot;
// unrecognized entities are preserved.
static void readEntities(const std::string &strValue, std::string &strResult)
{
	bool bEntity = false;
	std::string strEntity;

	strResult.reserve(strValue.size()<<1);
	strEntity.reserve(64);
	const char *strCursor = strValue.c_str();
	const char *strEnd = strValue.c_str() + strValue.size();
	while (strCursor != strEnd)
	{
		char ch = *strCursor++;
		switch (ch)
		{
			case '&':
				bEntity = true;
				break;
			case ';':
				if (bEntity)
				{
					// Content containing brackets collide with tag bracket delimiting 
					// characters. These must be replaced with entity references 
					// &lt; and &gt; when writing XML.
					// The ampersand character collides with the XML entity references 
					// content containing ampersands must be converted to an &amp; 
					// entity reference when writing XML.

					// ampersand
					if (strEntity.compare("amp") == 0)
						strResult += '&';
					// less-than
					else if (strEntity.compare("lt") == 0)
						strResult += '<';
					// greater-than
					else if (strEntity.compare("gt") == 0)
						strResult += '>';
					else if (strEntity.compare("quot") == 0)
						strResult += '"';
					else if (strEntity.compare("apos") == 0)
						strResult += '\'';
					else
					{
						strResult += '&';
						strResult += strEntity;
						strResult += ';';
					}
					strEntity.resize(0);
					bEntity = false;
					break;
				}
				// deliberate fall through
			default:
				if (bEntity)
					append(strEntity, ch);
				else
					append(strResult, ch);
				break;
		}
	}
}

static void readEntities(const std::string &strValue, std::wstring &strResult)
{
	size_t size = 0;
	std::string strMulti;
	
	readEntities(strValue, strMulti);
	strResult.resize(strMulti.size());
	mbstowcs_s(&size, &strResult[0], strResult.size(), strMulti.c_str(), strMulti.size());
	strResult.resize(size);
}

Reader::Reader() : _bStart(false), _iSkipped(0)
{
}

// connect parser to an input stream.
bool Reader::open(IInputStream *pStream)
{
	_bStart = false;
	_iSkipped = 0;
	return _parser.open(pStream);
}

// close parsing
void Reader::close()
{
	_parser.close();
	_bStart = false;
}

// True if parser has reached end of stream
// because of the read-ahead buffer, this is not the same
// as EOF on the underlying stream.
// EOF true here means the parser has reached the end of the stream.
bool Reader::eof()
{
	return _parser.eof();
}

// parse a token
bool Reader::parseToken(std::string &strToken)
{
	strToken.reserve(64);
	int ch = _parser.peek();
	while ( !(ch < 0) )
	{
		if ( ch != '=' && ch!= '/' && ch != '>' && ch != '"' && !isspace(ch) )
		{
			strToken += (char)ch;
			_parser.consume(1);
		}
		else break;
		ch = _parser.peek();
	}
	return strToken.size() > 0;
}

// returns true if start of an element.
bool Reader::isStartElement()
{
	if ( _parser.eof() ) return false;
	if (_bStart)
		return true;
	skipspace(false);
	_bStart = _parser.peekMatch('<') && !_parser.peekMatch("</");
	if (_bStart) _parser.consume(1);
	return _bStart;
}

// returns true if start of the named element.
bool Reader::isStartElement(const char *strElement)
{
	return isStartElement() && _parser.peekMatch(strElement);
}

// returns true start of an element is successfully consumed.
// if true, the element's attributes are available below through getAttribute.
bool Reader::readStartElement()
{
	entry element;
	if ( isStartElement() && parseToken(element.Element) )
	{
		_attributes.clear();
		while ( parseAttribute() ) ;
		element.Children = _parser.parseMatch('>');
		_stack.push_back(element);
		_bStart = false;
		return true;
	}
	return false;
}

// returns true if start of named element is successfully consumed.
// if true, the element's attributes are available below through getAttribute.
bool Reader::readStartElement(const char *strElement)
{
	entry element;
	size_t iLen = 0;
	if ( isStartElement() && _parser.parseMatch(strElement, iLen) )
	{
		element.Element.assign(strElement, iLen);
		_attributes.clear();
		while ( parseAttribute() ) ;
		element.Children = _parser.parseMatch('>');
		_stack.push_back(element);
		_bStart = false;
		return true;
	}
	return false;
}

// returns true if current element is self-closing OR no more nested content remains
// eg. cursor is positioned at the closing tag.
bool Reader::isEndElement()
{
	if (_stack.size() > 0)
	{
		bool bChildren = _stack.back().Children;
		skipspace(!bChildren);
		if (bChildren)
			return _parser.peekMatch("</");
		return _parser.peekMatch("/>");
	}
	return false;
}

// conclude self-closing element OR consume closing element.
bool Reader::readEndElement(bool bSkip)
{
	bool bOK = false;
	if (_stack.size() > 0)
	{
		bool bChildren = _stack.back().Children;
		skipspace(!bChildren);
		if (bChildren)
		{
			//bool bSkip = false;
			std::string strTail;
			strTail.reserve(_stack.back().Element.size() + 3);
			strTail = "</";
			strTail += _stack.back().Element;
			strTail += ">";
			do
			{
				bOK = _parser.parseMatch(strTail.c_str());
				if (!bOK && bSkip)
				{
					if ( readStartElement() )
					{
						_stack.back().Skipped++;
						_iSkipped++;
						readEndElement(bSkip); // recursive
					}
					else break;
				}
				else break;
			}
			while (!bOK);
		}
		else
			bOK = _parser.parseMatch("/>");
		if (bOK)
			_stack.pop_back();
	}
	return bOK;
}

// conclude named self-closing element OR consume named closing element.
// return false if the top of the element stack does not match our expected element.
bool Reader::readEndElement(bool bSkip, const char *element)
{
	return _stack.size() && 
		_stack.back().Element.compare(element) == 0 &&
		readEndElement(bSkip);
}

// read text content: assumes element with text only - no child elements.
bool Reader::readStringElement(const char *strElement, std::string &strValue)
{
	if ( readStartElement(strElement) )
	{
		readPCData(strValue);
		return readEndElement(false, strElement);
	}
	return false;
}

// read text content: assumes element with text only - no child elements.
bool Reader::readStringElement(const char *strElement, std::wstring &strValue)
{
	if ( readStartElement(strElement) )
	{
		readPCData(strValue);
		return readEndElement(false, strElement);
	}
	return false;
}

// retrieve PC Data (free text nodes under an element).
bool Reader::readPCData(std::string &strData)
{
	bool bOK = false;
	if (_stack.size() > 0 && _stack.back().Children)
	{
		std::string strText;
		strText.reserve(128);
		bOK = _parser.readText(strText, '<');
		if (bOK)
			readEntities(strText, strData);
	}
	return bOK;
}

// retrieve PC Data (free text nodes under an element).
bool Reader::readPCData(std::wstring &strData)
{
	bool bOK = false;
	if (_stack.size() > 0 && _stack.back().Children)
	{
		std::string strText;
		strText.reserve(128);
		bOK = _parser.readText(strText, '<');
		if (bOK)
			readEntities(strText, strData);
	}
	return bOK;
}

// skips / consumes whitespace.
// bInside - true if inside an element declaration eg. between '<' and '>'.
bool Reader::skipspace(bool bInside)
{
	if (bInside)
	{
		while (true)
		{
			_parser.skipspace();
			if ( _parser.peekMatch("--") ) 
			{
				_parser.consume(2);
				while ( !_parser.eof() && !_parser.peekMatch("--") ) _parser.consume(1);
				if ( _parser.peekMatch("--") )
					_parser.consume(2);
			}
			else break;
		}
	}
	else
	{
		while (true)
		{
			_parser.skipspace();
			if ( _parser.peekMatch("<!--") ) 
			{
				_parser.consume(4);
				while ( !_parser.eof() && !_parser.peekMatch("-->") ) _parser.consume(1);
				if ( _parser.peekMatch("-->") ) _parser.consume(3);
			}
			else if ( _parser.peekMatch("<?") ) 
			{
				_parser.consume(2);
				while ( !_parser.eof() && !_parser.peekMatch("?>") ) _parser.consume(1);
				if ( _parser.peekMatch("?>") ) _parser.consume(2);
			}
			else break;
		}
	}
	return true;
}

// parse attribute=quoted-value sequence.
bool Reader::parseAttribute()
{
	std::pair<std::string, std::string> pair;
	bool bOK = skipspace(true) && parseToken(pair.first);
	if (bOK)
	{
		bOK = skipspace(true) && _parser.parseMatch('=') && skipspace(true) &&
			_parser.parseMatch('"') &&
			_parser.readText(pair.second, '"') &&
			_parser.parseMatch('"');
		if (bOK)
		{
			_attributes.push_back(pair);
		}
	}
	return bOK;
}

// retrieve text for the named attribute.
bool Reader::getAttribute(const char *strAttribute, std::string &strValue)
{
	strValue.resize(0);
	std::list< std::pair<std::string, std::string> >::const_iterator it = _attributes.begin();
	while (it != _attributes.end() )
	{
		if ( (*it).first.compare(strAttribute) == 0)
		{
			// expand entities only when value is requested.
			readEntities((*it).second, strValue);
			return true;
		}
		it++;
	}
	return false;
}

// retrieve text for the named attribute.
bool Reader::getAttribute(const char *strAttribute, std::wstring &strValue)
{
	strValue.resize(0);
	std::list< std::pair<std::string, std::string> >::const_iterator it = _attributes.begin();
	while (it != _attributes.end() )
	{
		if ( (*it).first.compare(strAttribute) == 0)
		{
			// expand entities only when value is requested.
			readEntities((*it).second, strValue);
			return true;
		}
		it++;
	}
	return false;
}

// begin/end iterators for current element's attributes.
bool Reader::enumAttributes(std::list< std::pair<std::string, std::string> >::iterator &itBegin, std::list< std::pair<std::string, std::string> >::iterator &itEnd)
{
	itBegin = _attributes.begin();
	itEnd = _attributes.end();
	return _attributes.size() > 0;
}

// get current element's name
bool Reader::getElementName(std::string &strElement)
{
	bool bOK = _stack.size() > 0;
	if (bOK)
		strElement = _stack.back().Element;
	return bOK;
}

// number of child elements skipped.
size_t Reader::getSkipped(bool bDocument) const
{
	if (bDocument || _stack.size() == 0)
	{
		return _iSkipped;
	}
	return _stack.back().Skipped;
}

};