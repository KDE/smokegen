/*
    Generator for the SMOKE sources
    Copyright (C) 2009 Arno Rehn <arno@arnorehn.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef GLOBALS_H
#define GLOBALS_H

#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>

template<class Key, class Value>
class QMap;

template<class T>
class QList;
template<class T>
class QSet;

class QDir;
class QFileInfo;
class QString;
class QStringList;
class QTextStream;

class Class;
class Method;
class Type;

struct Options
{
    static QDir outputDir;
    static int parts;
    static QString module;
    static QStringList parentModules;
    static QList<QFileInfo> headerList;
    static QStringList classList;
};

struct SmokeDataFile
{
    SmokeDataFile();

    void write();
    bool isClassUsed(const Class* klass);

    QMap<QString, int> classIndex;
    QHash<const Method*, int> methodIdx;
    QSet<Class*> externalClasses;
    QSet<Type*> usedTypes;
    QStringList includedClasses;
};

struct SmokeClassFiles
{
    SmokeClassFiles(SmokeDataFile *data);
    void write();
    void write(const QList<QString>& keys);

private:
    void generateMethod(QTextStream& out, const QString& className, const QString& smokeClassName, const Method& meth, int index);
    void generateEnumMemberCall(QTextStream& out, const QString& className, const QString& member, int index);
    void generateVirtualMethod(QTextStream& out, const QString& className, const Method& meth);
    
    void writeClass(QTextStream& out, const Class* klass, const QString& className);
    
    SmokeDataFile *m_smokeData;
};
    
struct Util
{
    static QList<const Class*> superClassList(const Class* klass);
    static QList<const Class*> descendantsList(const Class* klass);

    static void preparse(QSet<Type*> *usedTypes, const QList<QString>& keys);

    static bool canClassBeInstanciated(const Class* klass);
    static bool canClassBeCopied(const Class* klass);
    static bool hasClassVirtualDestructor(const Class* klass);

    static void addDefaultConstructor(Class* klass);
    static void addCopyConstructor(Class* klass);

    static QString mungedName(const Method&);
    
    static QString stackItemField(const Type* type);
    static QString assignmentString(const Type* type, const QString& var);
    static QList<const Method*> collectVirtualMethods(const Class* klass);
};

#endif
