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

#ifndef GENERATORPREPROCESSOR_H
#define GENERATORPREPROCESSOR_H

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QList>
#include <QString>

#include <rpp/pp-engine.h>
#include <rpp/preprocessor.h>

class Preprocessor : public rpp::Preprocessor
{
public:
    Preprocessor(QList<QDir> includeDirs = QList<QDir>(), QStringList defines = QStringList(),
                 const QFileInfo& file = QFileInfo());
    virtual ~Preprocessor();
    
    virtual rpp::Stream* sourceNeeded(QString& fileName, rpp::Preprocessor::IncludeType type, int sourceLine, bool skipCurrentPath);

    void setFile(const QFileInfo& file);
    QFileInfo file();
    
    void setIncludeDirs(QList<QDir> dirs);
    QList<QDir> includeDirs();
    
    void setDefines(QStringList defines);
    QStringList defines();
    
    PreprocessedContents preprocess();
    PreprocessedContents lastContents();

private:
    rpp::pp *pp;
    QList<QDir> m_includeDirs;
    QStringList m_defines;
    QFileInfo m_file;
    PreprocessedContents m_contents;
    QHash<QString, PreprocessedContents> m_cache;
};

#endif // GENERATORPREPROCESSOR_H
