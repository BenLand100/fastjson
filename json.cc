/**
 *  Copyright 2014 by Benjamin Land (a.k.a. BenLand100)
 *
 *  fastjson is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  fastjson is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with fastjson. If not, see <http://www.gnu.org/licenses/>.
 */


#include "json.hh"

#include <cstdlib>
#include <cstring>
#include <sstream>

namespace json {
    
    void Value::reset(Type type) {
        if (refcount) decref();
        refcount = new TUInteger(0);
        this->type = type;
        init();
    }
    
    void Value::init() {
        switch (type) {
            case TSTRING:
                data.string = new TString();
                return;
            case TOBJECT:
                data.object = new TObject();
                return;
            case TARRAY:
                data.array = new TArray();
                return;
        }
    }
    
    void Value::clean() {
        if (refcount) delete refcount;
        switch (type) {
            case TSTRING:
                delete data.string;
                break;
            case TOBJECT: {
                    TObject::iterator it = data.object->begin();
                    TObject::iterator end = data.object->end();
                    for ( ; it != end; it++) {
                        delete it->second;
                    }
                }
                delete data.object;
                break;   
            case TARRAY: {
                    TArray::iterator it = data.array->begin();
                    TArray::iterator end = data.array->end();
                    for ( ; it != end; it++) {
                        delete *it;
                    }
                }
                delete data.array;
                break;
        }
        type = TNULL;
        refcount = NULL;
    }

    Reader::Reader(std::istream &in) {
        std::string ret;
        char buffer[4096];
        while (in.read(buffer, sizeof(buffer)))
            ret.append(buffer, sizeof(buffer));
        ret.append(buffer, in.gcount());
        data = new char[ret.length()+1];
        cur = data;
        memcpy(data,ret.c_str(),ret.length());
        data[ret.length()] = '\0';
		line = 1;
		lastbr = cur;
    }
    
    Reader::~Reader() {
        delete data;
    }

    Value *Reader::getValue() {
        while (*cur) {
            switch (*cur) {
                case '\n':
					line++;
                case '\r':
					lastbr = cur+1;
                case ' ':
                case '\t':
                    cur++;
                    break;
                case '-':
                case '+':
                case '.':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    return readNumber();
                case '{':
                    return readObject();
                case '[':
                    return readArray();
                case '"':
                    return readString();
                case 'N':
                    if (cur[1] == 'U' && cur[2] == 'L' && cur[3] == 'L') {
                        cur+=4;
                        return new Value();
                    }
                    throw parser_error(line,cur-lastbr,"Unexpected character " + *cur);
                case '/':
                    if (cur[1] == '/') {
						cur++;
                        while (*(cur++) != '\n') { }
                        break;
                    }
                    throw parser_error(line,cur-lastbr,"Malformed comment");
                default:
                    throw parser_error(line,cur-lastbr,"Unexpected character " + *cur);
            }
        }
        return NULL; //Really EOF - no more values to get
    }
    
    Value* Reader::readNumber() {
        bool real = false;
		bool exp = false;
        char *start = cur;
        while (*cur) {
            switch (*cur) {
				case 'e':
					exp = true;
					break;
                case 'u':
                    *cur = '\0';
                    cur++;
                    return new Value((TUInteger)atoi(start));
                case 'd':
                    *cur = '\0';
                    cur++;
                    return new Value((TReal)atof(start));
                case '.':
                    real = true;
                case '+':
                case '-':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    cur++;
                    break;
                default: {
                    char next = *cur;
                    *cur = '\0';
                    Value *val;
                    if (real || exp) {
                        val = new Value((TReal)atof(start));
                    } else {
                        val = new Value((TInteger)atoi(start));
                    }
                    *cur = next;
                    return val;
                }
            }
        }
		throw parser_error(line,cur-lastbr,"Reached EOF while parsing numeric");
    }
    
    Value* Reader::readString() {
        char *start = ++cur;
        while (*cur) {
            switch (*(cur++)) {
                case '\\':
                    cur++;
                    break;
                case '\"':
                    cur[-1] = '\0';
                    return new Value(unescapeString(std::string(start)));
            }
        }
		throw parser_error(line,cur-lastbr,"Reached EOF while parsing string");
    }
    
    Value* Reader::readObject() {
        Value *object = new Value();
        object->reset(TOBJECT);
        char *key = NULL;
        bool keyfound = false;
        Value *val = NULL;
        cur++;
        while (*cur) {
            switch (*cur) {
                case ' ':
                case '\n':
                case '\r':
                case '\t':
                    if (key && !keyfound) {
                        *cur = '\0';
                        keyfound = true;
                    }
                    cur++;
                    break;
                case '}':
                    cur++;
                    if (key) {
						throw parser_error(line,cur-lastbr,"} found where value expected");
                    }
                    return object;
                case ',':
                    cur++;
                    if (key) {
						throw parser_error(line,cur-lastbr,", found where value expected");
                    }
                    break;
                case ':':
                    if (!key) {
						throw parser_error(line,cur-lastbr,": found where field expected");
                    }
                    if (key && !keyfound) *cur = '\0';
                    cur++;
                    val = getValue();
                    object->setMember(std::string(key),val);
                    key = NULL;
                    val = NULL;
                    keyfound = false;
                    break;
                case '\"':
                    cur++;
                    key = cur;
                    delete readString();
                    keyfound = true;
                    break;
                default:
                    if (keyfound) {
						throw parser_error(line,cur-lastbr,*cur + " found where value expected");
                    }
                    if (!key) key = cur;
                    cur++;
            }
        }
		throw parser_error(line,cur-lastbr,"Reached EOF while parsing object");
    }
    
    Value* Reader::readArray() {
        Value *array = new Value();
        array->reset(TARRAY);
        Value *next = NULL;
        cur++;
        while (*cur) {
            switch (*cur) {
                case ' ':
                case '\n':
                case '\r':
                case '\t':
                case ',':
                    cur++;
                    break;
                case ']':
                    cur++;
                    return array;
                default:
                    next = getValue();
                    if (!next) {
						throw parser_error(line,cur-lastbr,"Malformed array elements");
                    }
                    array->data.array->push_back(next);
            }
        }
		throw parser_error(line,cur-lastbr,"Reached EOF while parsing array");
    }

    Writer::Writer(std::ostream &stream) : out(stream) {
        
    }
    
    Writer::~Writer() {
        
    }

    void Writer::putValue(Value *value) {
        writeValue(value);
        out << '\n';
    }
    
	//This could make prettier output
    void Writer::writeValue(Value *value) {
        switch (value->type) {
            case TINTEGER:
                out << value->data.integer;
                break;
            case TUINTEGER:
                out << value->data.uinteger << 'u';
                break;
            case TREAL:
                out << value->data.real;
                break;
            case TSTRING:
                out << escapeString(*(value->data.string));
                break;
            case TOBJECT: {
                    TObject::iterator it = value->data.object->begin();
                    TObject::iterator end = value->data.object->end();
                    out << "{\n";
                    for ( ; it != end; it++) {
                        out << '\"' << it->first << "\" : ";
                        writeValue(it->second);
                        out << ",\n";
                    }
                    out << '}';
                }
                break;   
            case TARRAY: {
                    TArray::iterator it = value->data.array->begin();
                    TArray::iterator end = value->data.array->end();
                    out << '[';
                    for ( ; it != end; it++) {
                        writeValue(*it);
                        out << ", ";
                    }
                    out << ']';
                }
                break;
            case TNULL:
                out << "NULL";
                break;
        }
    }

	//https://tools.ietf.org/rfc/rfc7159.txt
    std::string Writer::escapeString(std::string unescaped) {
		std::stringstream escaped;
		size_t last = 0, pos = 0, len = unescaped.length();
		while (pos < len) {
			switch (unescaped[pos]) {
				case '"':
				case '\\':
				case '/':
					escaped << unescaped.substr(last,pos-last) << '\\' << unescaped[pos];
					last = pos+1;
					break;
				case '\b':
					escaped << unescaped.substr(last,pos-last) << "\\b";
					last = pos+1;
					break;
				case '\f':
					escaped << unescaped.substr(last,pos-last) << "\\f";
					last = pos+1;
					break;
				case '\n':
					escaped << unescaped.substr(last,pos-last) << "\\n";
					last = pos+1;
					break;
				case '\r': 
					escaped << unescaped.substr(last,pos-last) << "\\r";
					last = pos+1;
					break;
				case '\t':
					escaped << unescaped.substr(last,pos-last) << "\\t";
					last = pos+1;
					break;
				default:
					if (unescaped[pos] < 0x20) throw parser_error(0,0,"Arbitrary unicode escapes not yet supported"); //FIXME
			}
			pos++;
		}
		escaped << unescaped.substr(last,pos-last);
		return escaped.str();
    }
	
	//https://tools.ietf.org/rfc/rfc7159.txt
    std::string Reader::unescapeString(std::string escaped) {
		if (escaped.find("\\") != std::string::npos) {
			size_t last = 0, pos = 0;
			std::stringstream unescaped;
			while ((pos = escaped.find("\\",pos)) != std::string::npos) {
				unescaped << escaped.substr(last,pos-last);
				switch (escaped[pos+1]) {
					case '"':
					case '\\':
					case '/':
						unescaped << escaped[pos+1];
						last = pos = pos+2;
						break;
					case 'b':
						unescaped << '\b';
						last = pos = pos+2;
						break;
					case 'f':
						unescaped << '\f';
						last = pos = pos+2;
						break;
					case 'n':
						unescaped << '\n';
						last = pos = pos+2;
						break;
					case 'r':
						unescaped << '\r';
						last = pos = pos+2;
						break;
					case 't':
						unescaped << '\t';
						last = pos = pos+2;
						break;
					case 'u':
						throw parser_error(line,cur-lastbr,"Arbitrary unicode escapes not yet supported"); //FIXME
					default:
						throw parser_error(line,cur-lastbr,"Invalid escape sequence in string");
				}
			}
			unescaped << escaped.substr(last);
			return unescaped.str();
		} else {
			return escaped;
		}
    }
    
}

