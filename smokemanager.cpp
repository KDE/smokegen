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


#include "smokemanager.h"

#include <cassert>

#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#include <smoke.h>

#ifndef WIN32
    #include <dlfcn.h>
#else
    #include <windows.h>
#endif

static const int SMOKE_VERSION = 3;

typedef std::map<std::string, Smoke*> StringSmokeMap;

#ifndef WIN32
typedef std::vector<void*> HandleList;
#else
typedef std::vector<HMODULE> HandleList;
#endif

struct SmokeManager::Data {
    StringSmokeMap moduleNameMap;
    HandleList libraryHandles;
};

SmokeManager::SmokeManager()
    : d(new Data)
{
}

SmokeManager::SmokeManager(const SmokeManager& other)
{
}

SmokeManager::~SmokeManager()
{
    for (StringSmokeMap::iterator iter = d->moduleNameMap.begin();
         iter != d->moduleNameMap.end();
         ++iter)
    {
        delete iter->second;
    }

    for (HandleList::iterator iter = d->libraryHandles.begin();
         iter != d->libraryHandles.end();
         ++iter)
    {
#ifndef WIN32
        dlclose(*iter);
#else
        FreeLibrary(*iter);
#endif
    }
}

SmokeManager* SmokeManager::self()
{
    static SmokeManager inst;
    return &inst;
}

void SmokeManager::manage(Smoke* smoke)
{
    assert(smoke);
    d->moduleNameMap[smoke->moduleName()] = smoke;
}

Smoke* SmokeManager::get(const std::string& moduleName, SmokeManager::LoadOptions options)
{
    Smoke *smoke = d->moduleNameMap[moduleName];

    if (!smoke && options == LoadIfRequired) {
        smoke = load(moduleName);
    }

    return smoke;
}

Smoke* SmokeManager::load(const std::string& moduleName)
{
    std::ostringstream libName;
#ifndef WIN32
    libName << "lib";
#endif
    libName << "smoke" << moduleName;

#ifdef __APPLE__
    libName << '.' << SMOKE_VERSION << ".dylib";
#elif WIN32
    libName << ".dll";
#else
    libName << ".so." << SMOKE_VERSION;
#endif

    std::string libNameString = libName.str();

#ifndef WIN32
    void *smokeLib = dlopen(libNameString.c_str(), RTLD_LAZY);
#else
    HMODULE smokeLib = LoadLibrary(libNameString.c_str());
#endif

    if (!smokeLib) {
        std::cerr << "ERROR: Couldn't load smoke module " << moduleName << std::endl;
        return 0;
    }

    d->libraryHandles.push_back(smokeLib);

    std::ostringstream initName;
    initName << "init_" << moduleName << "_Smoke";
    std::string initSymbolName = initName.str();

    typedef Smoke* (*SmokeInitFn)();

    SmokeInitFn initFn;

#ifndef WIN32
    initFn = (SmokeInitFn) dlsym(smokeLib, initSymbolName.c_str());
#else
    initFn = (SmokeInitFn) GetProcAddress(smokeLib, initSymbolName.c_str());
#endif

    if (!initFn) {
        std::cerr << "ERROR: Couldn't resolve symbol: " << initSymbolName << std::endl;
        return 0;
    }

    Smoke *smoke = initFn();

    manage(smoke);

    return smoke;
}
