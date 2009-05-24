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

Preprocessor::Preprocessor(const QString& name)
    : m_fileName(name)
{
    pp = new rpp::pp(this);
    pp->setEnvironment(new GeneratorEnvironment(pp));
}


Preprocessor::~Preprocessor()
{
    delete pp;
}

void Preprocessor::setFileName(const QString& name)
{
    m_fileName = name;
}

QString Preprocessor::fileName()
{
    return m_fileName;
}


PreprocessedContents Preprocessor::lastContents()
{
    return m_contents;
}


PreprocessedContents Preprocessor::preprocess()
{
    m_contents = pp->processFile(m_fileName);
    return m_contents;
}

rpp::Stream* Preprocessor::sourceNeeded(QString& fileName, rpp::Preprocessor::IncludeType type, int sourceLine, bool skipCurrentPath)
{
    return rpp::Preprocessor::sourceNeeded(fileName, type, sourceLine, skipCurrentPath);
}

