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

QDir Options::outputDir;
QList<QFileInfo> Options::headerList;
QStringList Options::classList;

int Options::parts = 20;
QString Options::module = "qt";
QStringList Options::parentModules;

extern "C" Q_DECL_EXPORT
void generate(const QDir& outputDir, const QList<QFileInfo>& headerList, const QStringList& classes)
{
    qDebug() << "Generating SMOKE sources...";
    Options::outputDir = outputDir;
    Options::headerList = headerList;
    Options::classList = classes;
    
    SmokeDataFile smokeData;
    smokeData.write();
    SmokeClassFiles classFiles(&smokeData);
    classFiles.write();
}
