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
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QTextStream>

#include <type.h>

#include "globals.h"

void writeSmokeData()
{
    QFile smokedata(outputDir.filePath("smokedata.cpp"));
    smokedata.open(QFile::ReadWrite | QFile::Truncate);
    QTextStream out(&smokedata);
    foreach (const QFileInfo& file, headerList)
        out << "#include <" << file.fileName() << ">\n";
    out << "\n#include <smoke.h>\n";
    out << "#include <" << module << "_smoke.h>\n\n";
    
    // write out module_cast() function
    out << "static void *" << module << "_cast(void *xptr, Smoke::Index from, Smoke::Index to) {\n";
    out << "  switch(from) {\n";
    for (QMap<QString, int>::const_iterator iter = classIndex.constBegin(); iter != classIndex.constEnd(); iter++) {
        out << "    case " << iter.value() << ":   //" << iter.key() << "\n";
        out << "      switch(to) {\n";
        const Class& klass = classes[iter.key()];
        foreach (const Class* base, superClassList(&klass)) {
            QString className = base->toString();
            out << QString("        case %1: return (void*)(%2*)(%3*)xptr;\n")
                .arg(classIndex[className]).arg(className).arg(klass.toString());
        }
        out << QString("        case %1: return (void*)(%2*)xptr;\n").arg(iter.value()).arg(klass.toString());
        foreach (const Class* desc, descendantsList(&klass)) {
            QString className = desc->toString();
            out << QString("        case %1: return (void*)(%2*)(%3*)xptr;\n")
                .arg(classIndex[className]).arg(className).arg(klass.toString());
        }
        out << "        default: return xptr;\n";
        out << "      }\n";
    }
    out << "    default: return xptr;\n";
    out << "  }\n";
    out << "}\n\n";
    
    // write out the inheritance list
    out << "// Group of Indexes (0 separated) used as super class lists.\n";
    out << "// Classes with super classes have an index into this array.\n";
    out << "static Smoke::Index " << module << "_inheritanceList[] = {\n";
    out << "    0,\t// 0: (no super class)\n";
    out << "}\n\n";
    smokedata.close();
}
