/*
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
#include <QList>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibrary>

#include <QtDebug>

#include <control.h>
#include <parsesession.h>
#include <parser.h>
#include <ast.h>

#include <iostream>

#include "generatorpreprocessor.h"
#include "generatorvisitor.h"
#include "type.h"

typedef void (*GenerateFn)(const QDir& outputDir, const QList<QFileInfo>& headerList, const QStringList& classes);

void showUsage()
{
    std::cout << 
    "Usage: smokegenerator [options] <header files>" << std::endl <<
    "Possible command line options are:" << std::endl <<
    "    -I <include dir>" << std::endl <<
    "    -c <path to file containing a list of classes>" << std::endl <<
    "    -d <path to file containing #defines>" << std::endl <<
    "    -g <generator to use>" << std::endl <<
    "    -t resolve typedefs" << std::endl <<
    "    -o <output dir>" << std::endl <<
    "    -h shows this message" << std::endl;
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        showUsage();
        return EXIT_SUCCESS;
    }

    QCoreApplication app(argc, argv);
    const QStringList& args = app.arguments();

    QList<QDir> includeDirs;
    QList<QFileInfo> headerList;
    QFileInfo classList, definesList;
    QDir output;
    QString generator;
    bool resolveTypdefs = false;

    for (int i = 1; i < args.count(); i++) {
        if ((args[i] == "-I" || args[i] == "-c" || args[i] == "-d" || args[i] == "-o") && i + 1 >= args.count()) {
            qCritical() << "not enough parameters for option" << args[i];
            return EXIT_FAILURE;
        }
        if (args[i] == "-I") {
            includeDirs << QDir(args[++i]);
        } else if (args[i] == "-c") {
            classList = QFileInfo(args[++i]);
        } else if (args[i] == "-d") {
            definesList = QFileInfo(args[++i]);
        } else if (args[i] == "-o") {
            output = QDir(args[++i]);
        } else if (args[i] == "-g") {
            generator = args[++i];
        } else if (args[i] == "-h") {
            showUsage();
            return EXIT_SUCCESS;
        } else if (args[i] == "-t") {
            resolveTypdefs = true;
        } else {
            headerList << QFileInfo(args[i]);
        }
    }
    
    if (!output.exists()) {
        qWarning() << "output directoy" << output.path() << "doesn't exist; creating it...";
        QDir::current().mkpath(output.path());
    }
    
    foreach (QDir dir, includeDirs) {
        if (!dir.exists()) {
            qWarning() << "include directory" << dir.path() << "doesn't exist";
            includeDirs.removeAll(dir);
        }
    }
    
    QLibrary lib("generator_" + generator);
    lib.load();
    if (!lib.isLoaded()) {
        qCritical() << lib.errorString();
        return EXIT_FAILURE;
    }
    qDebug() << "using generator" << lib.fileName();
    GenerateFn generate = (GenerateFn) lib.resolve("generate");
    if (!generate) {
        qCritical() << "couldn't resolve symbol 'generate', aborting";
        return EXIT_FAILURE;
    }
    
    QStringList classes;
    if (classList.exists()) {
        QFile file(classList.filePath());
        file.open(QIODevice::ReadOnly);
        while (!file.atEnd()) {
            QByteArray array = file.readLine();
            if (!array.isEmpty()) classes << array.trimmed();
        }
        file.close();
    }
    
    QStringList defines;
    if (definesList.exists()) {
        QFile file(definesList.filePath());
        file.open(QIODevice::ReadOnly);
        while (!file.atEnd()) {
            QByteArray array = file.readLine();
            if (!array.isEmpty()) defines << array.trimmed();
        }
        file.close();
    }
        
    foreach (QFileInfo file, headerList) {
        qDebug() << "parsing" << file.absoluteFilePath();
        // this has already been parsed because it was included by some header
        if (parsedHeaders.contains(file.absoluteFilePath()))
            continue;
        Preprocessor pp(includeDirs, defines, file);
        Control c;
        Parser parser(&c);
        ParseSession session;
        session.setContentsAndGenerateLocationTable(pp.preprocess());
        TranslationUnitAST* ast = parser.parse(&session);
        // TODO: improve 'header => class' association
        GeneratorVisitor visitor(&session, resolveTypdefs, file.fileName());
        visitor.visit(ast);
    }
    
    generate(output, headerList, classes);
}
