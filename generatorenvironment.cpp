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

#include "generatorenvironment.h"

#include <rpp/pp-macro.h>

GeneratorEnvironment::GeneratorEnvironment(rpp::pp * preprocessor) : rpp::Environment(preprocessor)
{
}

void GeneratorEnvironment::setMacro(rpp::pp_macro* macro)
{
    if (macro->name.str() != "signals" && macro->name.str() != "slots")
        rpp::Environment::setMacro(macro);
    else
        delete macro;
}
