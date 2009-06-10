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

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QString>
#include <QtDebug>

#include <type.h>

#include "globals.h"

QDir outputDir;
QList<QFileInfo> headerList;
QStringList classList;

int parts = 20;
QString module = "qt";

QMap<QString, int> classIndex;
QSet<Class*> externalClasses;
QSet<Type*> usedTypes;

extern "C" Q_DECL_EXPORT
void generate(const QDir& outputDir, const QList<QFileInfo>& headerList, const QStringList& classes)
{
    qDebug() << "Generating SMOKE sources...";
    ::outputDir = outputDir;
    ::headerList = headerList;
    classList = classes;
    
    for (QHash<QString, Class>::const_iterator iter = ::classes.constBegin(); iter != ::classes.constEnd(); iter++) {
        if (classList.contains(iter.key()) && !iter.value().isForwardDecl()) {
            classIndex[iter.key()] = 1;
        }
    }
    
    QStringList wantedClasses = classIndex.keys();
    collectTypes(wantedClasses);  // collect all used types
    
    // if a class is used somewhere but not listed in the class list, mark it external
    for (QHash<QString, Class>::iterator iter = ::classes.begin(); iter != ::classes.end(); iter++) {
        if (isClassUsed(&iter.value())) {
            classIndex[iter.key()] = 1;
            if (!classes.contains(iter.key()))
                externalClasses << &iter.value();
            else    
                wantedClasses << iter.key();
        }
    }
    
    // build class index here because the list needs to be sorted
    int i = 1;
    for (QMap<QString, int>::iterator iter = classIndex.begin(); iter != classIndex.end(); iter++) {
        iter.value() = i++;
    }
    
    writeSmokeData();
    writeClassFiles(wantedClasses);
}
