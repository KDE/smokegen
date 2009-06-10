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

class Class;
class Type;

extern QMap<QString, int> classIndex;
extern QDir outputDir;
extern QList<QFileInfo> headerList;
extern QStringList classList;

extern int parts;
extern QString module;

extern QSet<Class*> externalClasses;
extern QSet<Type*> usedTypes;

void writeClassFiles(const QList<QString>& keys);
void writeSmokeData();

QList<const Class*> superClassList(const Class* klass);
QList<const Class*> descendantsList(const Class* klass);

void collectTypes(const QList<QString>& keys);
bool isClassUsed(const Class* klass);

bool canClassBeInstanciated(const Class* klass);
bool canClassBeCopied(const Class* klass);
bool hasClassVirtualDestructor(const Class* klass);

#endif
