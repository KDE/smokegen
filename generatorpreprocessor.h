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

#include <QString>

#include <rpp/pp-engine.h>
#include <rpp/preprocessor.h>

class Preprocessor : public rpp::Preprocessor
{
public:
    Preprocessor(const QString& name = QString());
    virtual ~Preprocessor();
    
    virtual rpp::Stream* sourceNeeded(QString& fileName, rpp::Preprocessor::IncludeType type, int sourceLine, bool skipCurrentPath);

    void setFileName(const QString& name);
    QString fileName();
    
    PreprocessedContents preprocess();
    PreprocessedContents lastContents();

private:
    rpp::pp *pp;
    QString m_fileName;
    PreprocessedContents m_contents;
};

#endif // GENERATORPREPROCESSOR_H
