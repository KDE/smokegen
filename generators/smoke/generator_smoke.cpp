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

#include <iostream>

#include <type.h>

#include "globals.h"

QDir Options::outputDir;
QList<QFileInfo> Options::headerList;
QStringList Options::classList;

int Options::parts = 20;
QString Options::module = "qt";
QStringList Options::parentModules;
QStringList Options::stringTypes;

static void showUsage()
{
    std::cout <<
    "Usage: generator -g smoke [smoke generator options] [other generator options] -- <headers>" << std::endl <<
    "    -m <module name> (default: 'qt')" << std::endl <<
    "    -p <parts> (default: 20)" << std::endl <<
    "    -pm <comma-seperated list of parent modules>" << std::endl;
    "    -st <comma-seperated list of types storing strings>" << std::endl;
}

extern "C" Q_DECL_EXPORT
int generate(const QDir& outputDir, const QList<QFileInfo>& headerList, const QStringList& classes)
{
    Options::outputDir = outputDir;
    Options::headerList = headerList;
    Options::classList = classes;
    
    const QStringList& args = QCoreApplication::arguments();
    for (int i = 0; i < args.count(); i++) {
        if ((args[i] == "-m" || args[i] == "-p" || args[i] == "-pm") && i + 1 >= args.count()) {
            qCritical() << "generator_smoke: not enough parameters for option" << args[i];
            return EXIT_FAILURE;
        } else if (args[i] == "-m") {
            Options::module = args[++i];
        } else if (args[i] == "-p") {
            bool ok = false;
            Options::parts = args[++i].toInt(&ok);
            if (!ok) {
                qCritical() << "generator_smoke: couldn't parse argument for option" << args[i - 1];
                return EXIT_FAILURE;
            }
        } else if (args[i] == "-pm") {
            Options::parentModules = args[++i].split(',');
        } else if (args[i] == "-st") {
            Options::stringTypes = args[++i].split(',');
        } else if (args[i] == "-h" || args[i] == "--help") {
            showUsage();
            return EXIT_SUCCESS;
        }
    }
    
    // Fill the type map. It maps some long integral types to shorter forms as used in SMOKE.
    Util::typeMap["long long"] = "long";
    Util::typeMap["long long int"] = "long";
    Util::typeMap["long int"] = "long";
    Util::typeMap["short int"] = "short";
    Util::typeMap["long double"] = "double";
    Util::typeMap["wchar_t"] = "int";   // correct?
    
    qDebug() << "Generating SMOKE sources...";
    
    SmokeDataFile smokeData;
    smokeData.write();
    SmokeClassFiles classFiles(&smokeData);
    classFiles.write();
    
    return EXIT_SUCCESS;
}
