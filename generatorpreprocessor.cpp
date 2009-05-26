/*
    Copyright (C) 2009  Arno Rehn <arno@arnorehn.de>

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

#include "generatorpreprocessor.h"
#include "generatorenvironment.h"

#include <rpp/pp-engine.h>
#include <rpp/pp-stream.h>

#include <QtDebug>

Preprocessor::Preprocessor(QList<QDir> includeDirs, QStringList defines, const QFileInfo& file)
    : m_includeDirs(includeDirs), m_defines(defines), m_file(file)
{
    pp = new rpp::pp(this);
    pp->setEnvironment(new GeneratorEnvironment(pp));
}


Preprocessor::~Preprocessor()
{
    delete pp;
}

void Preprocessor::setFile(const QFileInfo& file)
{
    m_file = file;
}

QFileInfo Preprocessor::file()
{
    return m_file;
}


void Preprocessor::setIncludeDirs(QList< QDir > dirs)
{
    m_includeDirs = dirs;
}


QList< QDir > Preprocessor::includeDirs()
{
    return m_includeDirs;
}


void Preprocessor::setDefines(QStringList defines)
{
    m_defines = defines;
}


QStringList Preprocessor::defines()
{
    return m_defines;
}


PreprocessedContents Preprocessor::lastContents()
{
    return m_contents;
}


PreprocessedContents Preprocessor::preprocess()
{
    m_contents = pp->processFile(m_file.absoluteFilePath());
    return m_contents;
}

rpp::Stream* Preprocessor::sourceNeeded(QString& fileName, rpp::Preprocessor::IncludeType type, int sourceLine, bool skipCurrentPath)
{
    QString path;
    if (type == rpp::Preprocessor::IncludeLocal) {
        if (m_file.absoluteDir().exists(fileName))
            path = m_file.absoluteDir().filePath(fileName);
    } else {
        foreach (QDir dir, m_includeDirs) {
            if (dir.exists(fileName)) {
                path = dir.filePath(fileName);
                break;
            }
        }
    }
    if (path.isEmpty())
        return 0;
    QFile file(path);
    file.open(QFile::ReadOnly);
    QByteArray array = file.readAll();
    file.close();
    QHash<QString, PreprocessedContents>::iterator iter = m_cache.insert(fileName, convertFromByteArray(array));
    return new rpp::Stream(&iter.value());
}

