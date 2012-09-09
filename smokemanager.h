/*
    Copyright (C) 2012  Arno Rehn <arno@arnorehn.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef SMOKEMANAGER_H
#define SMOKEMANAGER_H

#include <string>

class Smoke;

class SmokeManager
{
public:
    enum LoadOptions {
        DoNotLoad = 0,
        LoadIfRequired = 1,
    };

    static SmokeManager *self();

    void manage(Smoke *smoke);
    Smoke *load(const std::string& moduleName);
    Smoke *get(const std::string& moduleName, LoadOptions options = DoNotLoad);

protected:
    SmokeManager();
    SmokeManager(const SmokeManager& other);
    ~SmokeManager();

    struct Data;
    Data *d;
};

#endif // SMOKEMANAGER_H
