/***************************************************************************
   Copyright 2008 David Nolden <david.nolden.kdevelop@art-master.de>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef INDEXED_STRING_H
#define INDEXED_STRING_H

//krazy:excludeall=dpointer,inline

#include "cppparser_export.h"
#include <QtCore/QString>

class QDataStream;
class QUrl;

class IndexedString;

///This string does "disk reference-counting", which means that reference-counts are maintainted, but
///only when the string is in a disk-stored location. The file referencecounting.h is used to manage this condition.
///Whenever reference-counting is enabled for a range that contains the IndexedString, it will manipulate the reference-counts.
///The duchain storage mechanisms automatically are about correctly managing that condition, so you don't need to care, and can
///just use this class in every duchain data type without restrictions.
///
///@warning Do not use IndexedString after QCoreApplication::aboutToQuit() has been emitted, items that are not disk-referenced will be invalid at that point
///
///Empty strings have an index of zero.
///Strings of length one are not put into the repository, but are encoded directly within the index:
///They are encoded like 0xffff00bb where bb is the byte of the character.
class CPPPARSER_EXPORT IndexedString {
 public:
  IndexedString();
  ///@param str must be a utf8 encoded string, does not need to be 0-terminated.
  ///@param length must be its length in bytes.
  ///@param hash must be a hash as constructed with the here defined hash functions. If it is zero, it will be computed.
  explicit IndexedString( const char* str, unsigned short length, unsigned int hash = 0 );

  ///Needs a zero terminated string. When the information is already available, try using the other constructor.
  explicit IndexedString( const char* str );

  explicit IndexedString( char c );
  
  ///When the information is already available, try using the other constructor. This is expensive.
  explicit IndexedString( const QString& str );

  ///When the information is already available, try using the other constructor. This is expensive.
  explicit IndexedString( const QByteArray& str );

  ///Returns a not reference-counted IndexedString that represents the given index
  ///@warning It is dangerous dealing with indices directly, because it may break the reference counting logic
  ///         never stay pure indices to disk
  static IndexedString fromIndex( unsigned int index ) {
    IndexedString ret;
    ret.m_index = index;
    return ret;
  }

  IndexedString( const IndexedString& );

  ~IndexedString();

  ///Creates an indexed string from a KUrl, this is expensive.
  explicit IndexedString( const QUrl& url );

  ///Re-construct a KUrl from this indexed string, the result can be used with the
  ///KUrl-using constructor. This is expensive.
  QUrl toUrl() const;

  inline unsigned int hash() const {
    return m_index;
  }

  ///The string is uniquely identified by this index. You can use it for comparison.
  ///@warning It is dangerous dealing with indices directly, because it may break the reference counting logic
  ///         never stay pure indices to disk
  inline unsigned int index() const {
    return m_index;
  }

  bool isEmpty() const {
    return m_index == 0;
  }

  //This is relatively expensive(needs a mutex lock, hash lookups, and eventual loading), so avoid it when possible.
  int length() const;

  ///Convenience function, avoid using it, it's relatively expensive
  QString str() const;

  ///Convenience function, avoid using it, it's relatively expensive(les expensive then str() though)
  QByteArray byteArray() const;

  IndexedString& operator=(const IndexedString&);
  
  bool operator == ( const IndexedString& rhs ) const {
    return m_index == rhs.m_index;
  }

  bool operator != ( const IndexedString& rhs ) const {
    return m_index != rhs.m_index;
  }

  ///Does not compare alphabetically, uses the index  for ordering.
  bool operator < ( const IndexedString& rhs ) const {
    return m_index < rhs.m_index;
  }

  //Use this to construct a hash-value on-the-fly
  //To read it, just use the hash member, and when a new string is started, call clear(.
  //This needs very fast performance(per character operation), so it must stay inlined.
  struct RunningHash {
    enum {
      HashInitialValue = 5381
    };

    RunningHash() : hash(HashInitialValue) { //We initialize the hash with zero, because we want empty strings to create a zero hash(invalid)
    }
    inline void append(const char c) {
      hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    inline void clear() {
      hash = HashInitialValue;
    }
    unsigned int hash;
  };

  static unsigned int hashString(const char* str, unsigned short length);

 private:
   explicit IndexedString(bool);
   uint m_index;
};

CPPPARSER_EXPORT inline uint qHash( const IndexedString& str ) {
  return str.index();
}


#endif
