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

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QString>
#include <QtDebug>

#include <type.h>

QMap<QString, int> classIndex;
QDir outputDir;
QList<QFileInfo> headerList;
QStringList classList;

QString module = "qt";

void writeSmokeData();

extern "C" Q_DECL_EXPORT
void generate(const QDir& outputDir, const QList<QFileInfo>& headerList, const QStringList& classes)
{
    ::outputDir = outputDir;
    ::headerList = headerList;
    classList = classes;
    int i = 1;
    // build table classname => index
    for (QHash<QString, Class>::const_iterator iter = ::classes.constBegin(); iter != ::classes.constEnd(); iter++) {
//         if (classes.contains(iter.key()))
            classIndex[iter.key()] = i++;
    }
    
    writeSmokeData();
}

void writeSmokeData()
{
    QFile smokedata(outputDir.filePath("smokedata.cpp"));
    smokedata.open(QFile::ReadWrite | QFile::Truncate);
    QTextStream out(&smokedata);
    foreach (const QFileInfo& file, headerList)
        out << "#include <" << file.fileName() << ">\n";
    out << "\n#include <smoke.h>\n";
    out << "#include <" << module << "_smoke.h>\n\n";
    out << "static void *" << module << "_cast(void *xptr, Smoke::Index from, Smoke::Index to) {\n";
    out << "  switch(from) {\n";
    for (QMap<QString, int>::const_iterator iter = classIndex.constBegin(); iter != classIndex.constEnd(); iter++) {
        out << "    case " << iter.value() << ":   //" << iter.key() << "\n";
        out << "      switch(to) {\n";
        const Class& klass = classes[iter.key()];
        foreach (const Class::BaseClassSpecifier& baseClass, klass.baseClasses()) {
            QString baseClassName = baseClass.baseClass->toString();
            out << "        case " << classIndex[baseClassName] << ": return (void*)(" << baseClassName << "*)(" << klass.toString() << "*)xptr;\n";
        }
        out << "      }\n";
    }
    out << "  }\n";
    out << "}\n";
    smokedata.close();
}
