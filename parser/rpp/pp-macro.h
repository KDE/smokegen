/*
  Copyright 2005 Roberto Raggi <roberto@kdevelop.org>

  Permission to use, copy, modify, distribute, and sell this software and its
  documentation for any purpose is hereby granted without fee, provided that
  the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  KDEVELOP TEAM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef PP_MACRO_H
#define PP_MACRO_H

//krazy:excludeall=inline

#include <QtCore/QStringList>
#include <QVector>
#include "../indexedstring.h"
#include "../cppparser_export.h"
// #include "appendedlist.h"
// #include <language/editor/hashedstring.h>
// #include <language/duchain/appendedlist.h>

#define FOREACH_CUSTOM(item, container, size) for(int a = 0, mustDo = 1; a < (int)size; ++a) if((mustDo = 1)) for(item(container[a]); mustDo; mustDo = 0)

namespace rpp {

// Q_DECL_EXPORT DECLARE_LIST_MEMBER_HASH(pp_macro, definition, IndexedString)
// Q_DECL_EXPORT DECLARE_LIST_MEMBER_HASH(pp_macro, formals, IndexedString)

  //This contains the data of a macro that can be marshalled by directly copying the memory
struct CPPPARSER_EXPORT pp_macro
{ ///@todo enable structure packing
  pp_macro(const IndexedString& name = IndexedString());
  pp_macro(const char* name);
  pp_macro(const pp_macro& rhs, bool dynamic = true);
  ~pp_macro();
  
  uint classSize() const {
    return sizeof(pp_macro);
  }
  
  /*uint itemSize() const {
    return dynamicSize();
  }*/
  
  typedef uint HashType;

  IndexedString name;
  IndexedString file;
  
  int sourceLine; //line

  bool defined: 1; // !isUndefMacro
  bool hidden: 1;
  bool function_like: 1; // hasArguments
  bool variadics: 1;
  bool fixed : 1; //If this is set, the macro can not be overridden or undefined.
  mutable bool m_valueHashValid : 1;
  
  //The valueHash is not necessarily valid
  mutable HashType m_valueHash; //Hash that represents the values of all macros
  
  bool operator==(const pp_macro& rhs) const;
  
  bool isUndef() const  {
    return !defined;
  }
  
  unsigned int hash() const {
    return completeHash();
  }
  
  HashType idHash() const {
    return name.hash();
  }
  
  QString toString() const;
  
  struct NameCompare {
    bool operator () ( const pp_macro& lhs, const pp_macro& rhs ) const {
      return lhs.name.index() < rhs.name.index();
    }
    #ifdef Q_CC_MSVC
    
    HashType operator () ( const pp_macro& macro ) const
    {
        return macro.idHash();
    }
    
    enum
    { // parameters for hash table
    bucket_size = 4,  // 0 < bucket_size
    min_buckets = 8}; // min_buckets = 2 ^^ N, 0 < N
    #endif
  };

  //Hash over id and value
  struct CompleteHash {
    HashType operator () ( const pp_macro& lhs ) const {
        return lhs.completeHash();
    }
    
    #ifdef Q_CC_MSVC
    
    bool operator () ( const pp_macro& lhs, const pp_macro& rhs ) const {
        HashType lhash = lhs.valueHash()+lhs.idHash();
        HashType rhash = rhs.valueHash()+rhs.idHash();
        if( lhash < rhash ) return true;
        else if( lhash > rhash ) return false;

      int df = lhs.name.str().compare( rhs.name.str() );
      return df < 0;
    }
    
    enum
    { // parameters for hash table
    bucket_size = 4,  // 0 < bucket_size
    min_buckets = 8}; // min_buckets = 2 ^^ N, 0 < N
    #endif
  };
  
  HashType valueHash() const {
    if( !m_valueHashValid ) computeHash();
    return m_valueHash;
  }

  ///Hash that identifies all of this macro, the value and the identity
  HashType completeHash() const {
    return valueHash() + idHash() * 3777;
  }
  
  void invalidateHash();
  
  ///Convenient way of setting the definition, it is tokenized automatically
  ///@param definition utf-8 representation of the definition text
  void setDefinitionText(QByteArray definition);
  
  ///More convenient overload
  void setDefinitionText(QString definition);
  
  void setDefinitionText(const char* definition) {
    setDefinitionText(QByteArray(definition));
  }
  
//   START_APPENDED_LISTS(pp_macro)
//   APPENDED_LIST_FIRST(pp_macro, IndexedString, definition)
//   APPENDED_LIST(pp_macro, IndexedString, formals, definition)
//   END_APPENDED_LISTS(pp_macro, formals)

  QVector<IndexedString> definition;
  QVector<IndexedString> formals;

  private:
    pp_macro& operator=(const pp_macro& rhs);
    void computeHash() const;
};

}

inline uint qHash( const rpp::pp_macro& m ) {
  return (uint)m.idHash();
}

#endif // PP_MACRO_H

