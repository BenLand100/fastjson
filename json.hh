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

#ifndef _JSON
#define _JSON

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <string>

namespace json {
    
    class Value;
    class Reader;
    class Writer;

    typedef int TInteger;
    typedef unsigned int TUInteger;
    typedef double TReal;
    typedef std::string TString;
    typedef std::map<TString,Value*> TObject;
    typedef std::vector<Value*> TArray;

    enum Type {
        TINTEGER,
        TUINTEGER,
        TREAL,
        TSTRING,
        TOBJECT,
        TARRAY,
        TNULL
    };
    
    class Value {
        friend Reader;
        friend Writer;
        public:
            inline Value() : refcount(NULL), type(TNULL) { }
            inline Value(TInteger integer) : refcount(new TUInteger(0)), type(TINTEGER) { data.integer = integer; }
            inline Value(TUInteger uinteger) : refcount(new TUInteger(0)), type(TUINTEGER) { data.uinteger = uinteger; }
            inline Value(TReal real) : refcount(new TUInteger(0)), type(TREAL) { data.real = real; }
            inline Value(TString string) : refcount(new TUInteger(0)), type(TSTRING) { init(); (*data.string) = string; }
            inline Value(Value &other) { decref(); type = other.type; data = other.data; refcount = other.refcount; (*refcount)++; }
            inline ~Value() { decref(); }
            
            inline Value& operator=(const Value& other) { decref(); data = other.data; refcount = other.refcount; (*refcount)++; }
            
            void reset(Type type);
            inline void reset() { reset(TNULL); }
            inline Type getType() { return type; }
            
            inline TInteger getInteger() { checkType(TINTEGER); return data.integer; }
            inline TUInteger getUInteger() { checkType(TUINTEGER); return data.uinteger; }
            inline TReal getReal() { checkType(TREAL); return data.real; }
            inline TString getString() { checkType(TSTRING); return *data.string; }
            
            inline Value* getMember(TString key) { checkType(TOBJECT); return (*data.object)[key]; }
            inline TUInteger getArraySize() { checkType(TARRAY); return data.array->size(); }
            inline Value* getIndex(TUInteger index) {  checkType(TARRAY); return (*data.array)[index]; }
    
            inline void setInteger(TInteger integer)  { checkTypeReset(TINTEGER); data.integer = integer; }
            inline void setUINteger(TUInteger uinteger) { checkTypeReset(TUINTEGER); data.uinteger = uinteger; }
            inline void setReal(TReal real) { checkTypeReset(TREAL); data.real = real; }
            inline void setString(TString string) { checkTypeReset(TSTRING); data.string = new TString(string); }
            
            inline void setMember(TString key, Value *value) { checkTypeReset(TOBJECT); (*data.object)[key] = value; }
            inline void setArraySize(TUInteger size) { checkTypeReset(TARRAY); data.array->resize(size); }
            inline void setIndex(TUInteger index, Value *value) { checkTypeReset(TARRAY); (*data.array)[index] = value; }
            
        protected:
            inline void checkType(Type type) { if (this->type != type) { /** Make bad happen **/ } }
            inline void checkTypeReset(Type type) { if (this->type != type) reset(TOBJECT); }
            inline void decref() { if (refcount && !((*refcount)--)) clean(); }
            void clean();
            void init();
        
            TUInteger *refcount;
            Type type;
            union Data {
                TInteger integer;
                TUInteger uinteger;
                TReal real;
                TString *string;
                TObject *object;
                TArray *array;
            } data;
    };
    
	class parser_error : public std::runtime_error {
		public:
			const int line, pos;
			inline explicit parser_error(const int line_, const int pos_, const std::string& desc) : std::runtime_error(desc), line(line_), pos(pos_)  { }
			inline explicit parser_error(const int line_, const int pos_, const char* desc) : std::runtime_error(desc), line(line_), pos(pos_) { }
	};
	
    class Reader {
        public:
            //Reads the entire stream immediately
            Reader(std::istream &stream);
            ~Reader();
            
            Value* getValue();
            
        protected:
            char *data,*cur,*lastbr;
			int line;
            
            std::string unescapeString(std::string string);
            
            Value* readNumber();
            Value* readString();
            Value* readObject();
            Value* readArray();
    
    };
    
    class Writer {
        public:
            //Only writes to the stream when requested
            Writer(std::ostream &stream);
            ~Writer();
            
            void putValue(Value *value);
            
        protected:
            std::ostream &out;
            
            std::string escapeString(std::string string);
            
            void writeValue(Value *value);
    
    };  
    
}

#endif
