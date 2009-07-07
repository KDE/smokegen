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
#include "options.h"
#include "type.h"

typedef int (*GenerateFn)(const QDir& outputDir, const QList<QFileInfo>& headerList, const QStringList& classes);

static void showUsage()
{
    std::cout << 
    "Usage: generator [options] -- <header files>" << std::endl <<
    "Possible command line options are:" << std::endl <<
    "    -I <include dir>" << std::endl <<
    "    -c <path to file containing a list of classes>" << std::endl <<
    "    -d <path to file containing #defines>" << std::endl <<
    "    -g <generator to use>" << std::endl <<
    "    -n <comma-seperated list of namespaces that should be parsed as classes>" << std::endl <<
    "    -qt enables Qt-mode (special treatment of QFlags)" << std::endl <<
    "    -t resolve typedefs" << std::endl <<
    "    -o <output dir>" << std::endl <<
    "    --config <config file>" << std::endl <<
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

    QFileInfo configFile;
    QDir output;
    QString generator;
    bool addHeaders = false;

    for (int i = 1; i < args.count(); i++) {
        if ((args[i] == "-I" || args[i] == "-c" || args[i] == "-d" || args[i] == "-o"||
             args[i] == "-n" || args[i] == "--config") && i + 1 >= args.count())
        {
            qCritical() << "not enough parameters for option" << args[i];
            return EXIT_FAILURE;
        }
        if (args[i] == "-I") {
            ParserOptions::includeDirs << QDir(args[++i]);
        } else if (args[i] == "--config") {
            configFile = QFileInfo(args[++i]);
        } else if (args[i] == "-c") {
            ParserOptions::classList = QFileInfo(args[++i]);
        } else if (args[i] == "-d") {
            ParserOptions::definesList = QFileInfo(args[++i]);
        } else if (args[i] == "-o") {
            output = QDir(args[++i]);
        } else if (args[i] == "-g") {
            generator = args[++i];
        } else if (args[i] == "-n") {
            ParserOptions::namespacesAsClasses = args[++i].split(",");
        } else if ((args[i] == "-h" || args[i] == "--help") && argc == 2) {
            showUsage();
            return EXIT_SUCCESS;
        } else if (args[i] == "-t") {
            ParserOptions::resolveTypedefs = true;
        } else if (args[i] == "-qt") {
            ParserOptions::qtMode = true;
        } else if (args[i] == "--") {
            addHeaders = true;
        } else if (addHeaders) {
            ParserOptions::headerList << QFileInfo(args[i]);
        }
    }
    
    if (!output.exists()) {
        qWarning() << "output directoy" << output.path() << "doesn't exist; creating it...";
        QDir::current().mkpath(output.path());
    }
    
    foreach (QDir dir, ParserOptions::includeDirs) {
        if (!dir.exists()) {
            qWarning() << "include directory" << dir.path() << "doesn't exist";
            ParserOptions::includeDirs.removeAll(dir);
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
    if (ParserOptions::classList.exists()) {
        QFile file(ParserOptions::classList.filePath());
        file.open(QIODevice::ReadOnly);
        while (!file.atEnd()) {
            QByteArray array = file.readLine();
            if (!array.isEmpty()) classes << array.trimmed();
        }
        file.close();
    }
    
    QStringList defines;
    if (ParserOptions::definesList.exists()) {
        QFile file(ParserOptions::definesList.filePath());
        file.open(QIODevice::ReadOnly);
        while (!file.atEnd()) {
            QByteArray array = file.readLine();
            if (!array.isEmpty())
                defines << array.trimmed();
        }
        file.close();
    }
    
    foreach (QFileInfo file, ParserOptions::headerList) {
        qDebug() << "parsing" << file.absoluteFilePath();
        // this has already been parsed because it was included by some header
        if (parsedHeaders.contains(file.absoluteFilePath()))
            continue;
        Preprocessor pp(ParserOptions::includeDirs, defines, file);
        Control c;
        Parser parser(&c);
        ParseSession session;
        session.setContentsAndGenerateLocationTable(pp.preprocess());
        TranslationUnitAST* ast = parser.parse(&session);
        // TODO: improve 'header => class' association
        GeneratorVisitor visitor(&session, file.fileName());
        visitor.visit(ast);
    }
    
    return generate(output, ParserOptions::headerList, classes);
}
