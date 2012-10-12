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

#ifndef _WIN32
    #include <dlfcn.h>
#else
    #include <windows.h>
#endif

static const int SMOKE_VERSION = 3;

namespace SmokeUtils {

#ifndef _WIN32
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
#ifndef _WIN32
        dlclose(*iter);
#else
        FreeLibrary(*iter);
#endif
    }

    delete d;
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

const SmokeManager::StringSmokeMap& SmokeManager::loadedModules() const
{
    return d->moduleNameMap;
}

Smoke* SmokeManager::load(const std::string& moduleName)
{
    std::ostringstream libName;
#ifndef _WIN32
    libName << "lib";
#endif
    libName << "smoke" << moduleName;

#ifdef __APPLE__
    libName << '.' << SMOKE_VERSION << ".dylib";
#elif _WIN32
    libName << ".dll";
#else
    libName << ".so." << SMOKE_VERSION;
#endif

    std::string libNameString = libName.str();

#ifndef _WIN32
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

#ifndef _WIN32
    initFn = (SmokeInitFn) dlsym(smokeLib, initSymbolName.c_str());
#else
    initFn = (SmokeInitFn) GetProcAddress(smokeLib, initSymbolName.c_str());
#endif

    if (!initFn) {
        std::cerr << "ERROR: Couldn't resolve symbol: " << initSymbolName << std::endl;
        return 0;
    }

    Smoke *smoke = initFn();

    // legacy support, this is normally called by init_foo_Smoke();
    manage(smoke);

    return smoke;
}

Smoke::ModuleIndex SmokeManager::findClass(const char* className)
{
    assert(className);

    // Try to do an 'intelligent' lookup.
    for (StringSmokeMap::iterator iter = d->moduleNameMap.begin();
         iter != d->moduleNameMap.end();
         ++iter)
    {
        Smoke *smoke = iter->second;
        int left = strcmp(className, smoke->classes[1].className);
        int right = strcmp(className, smoke->classes[smoke->numClasses].className);
        if (left > 0 && right < 0) {
            Smoke::ModuleIndex id = smoke->idClass(className);
            if (id) {
                return id;
            }
        } else if (left == 0) {
            return Smoke::ModuleIndex(smoke, 1);
        } else if (right == 0) {
            return Smoke::ModuleIndex(smoke, smoke->numClasses);
        }
    }

    return Smoke::NullModuleIndex;
}

Smoke::ModuleIndex SmokeManager::findMethodName(const char* methodName)
{
    assert(methodName);

    for (StringSmokeMap::iterator iter = d->moduleNameMap.begin();
         iter != d->moduleNameMap.end();
         ++iter)
    {
        Smoke::ModuleIndex mi = iter->second->idMethodName(methodName);
        if (mi)
            return mi;
    }

    return Smoke::NullModuleIndex;
}

Smoke::ModuleIndex SmokeManager::findMethodNameImpl(const Smoke::ModuleIndex& classId, const char *methodName, Smoke *last) {
    assert(methodName);

    Smoke *smoke = classId.smoke;

    // first try to look up 'methodName' in classId's smoke module...
    Smoke::ModuleIndex mi;
    if (last != smoke) {
        mi = smoke->idMethodName(methodName);
        if (mi)
            return mi;
    }

    // if not found, look it up in the smoke modules of the superclasses
    for (Smoke::Index *p = smoke->inheritanceList + smoke->classes[classId.index].parents; *p; ++p) {
        if (smoke->classes[*p].flags & Smoke::cf_undefined) {
            mi = findClass(smoke->classes[*p].className);
        } else {
            mi = Smoke::ModuleIndex(classId.smoke, *p);
        }
        mi = findMethodNameImpl(mi, methodName, smoke);
        if (mi)
            return mi;
    }

    return Smoke::NullModuleIndex;
}

Smoke::ModuleIndex SmokeManager::findMethodMap(const Smoke::ModuleIndex& classId, const Smoke::ModuleIndex& mungedName)
{
    if (!classId || !mungedName)
        return Smoke::NullModuleIndex;

    Smoke::ModuleIndex tmp;

    // Look if the class or any of its superclasses have a method with the given name.
    // If classId and methodName are in different modules, we have to walk the inheritance list until
    // we reach methodName's module.
    if (classId.smoke != mungedName.smoke) {
        for (Smoke::Index *p = classId.smoke->inheritanceList + classId.smoke->classes[classId.index].parents;
             *p; ++p)
        {
            const Smoke::Class& parent = classId.smoke->classes[*p];
            if (parent.flags & Smoke::cf_undefined) {
                tmp = findClass(parent.className);
            } else {
                tmp.index = Smoke::ModuleIndex(classId.smoke, *p);
            }

            tmp = findMethodMap(tmp, mungedName);
            if (tmp)
                return tmp;
        }
    } else {
        tmp = classId.smoke->idMethod(classId.index, mungedName.index);
        if (tmp)
            return tmp;

        for (Smoke::Index *p = classId.smoke->inheritanceList + classId.smoke->classes[classId.index].parents;
             *p; ++p)
        {
            if (classId.smoke->classes[*p].flags & Smoke::cf_undefined)
                continue;  // smoke modules of class and name are the same; the method can't be somewhere else

            tmp = findMethodMap(Smoke::ModuleIndex(classId.smoke, *p), mungedName);
            if (tmp)
                return tmp;
        }
    }

    return Smoke::NullModuleIndex;
}

inline static bool matchesArgs(Smoke *smoke, Smoke::Index id, const char **argTypes)
{
    Smoke::Index *argId = smoke->argumentList + smoke->methods[id].args;

    do {
        const Smoke::Type& type = smoke->types[*argId];
        if (strcmp(type.name, *argTypes))
            return false;
        ++argId; ++argTypes;
    } while (*argId && *argTypes);

    return !*argId && !*argTypes;
}

inline static bool matchesArgs(Smoke *smoke, Smoke::Index id, const std::vector<std::string>& argTypes)
{
    Smoke::Index *argId = smoke->argumentList + smoke->methods[id].args;
    std::vector<std::string>::const_iterator iter = argTypes.begin();

    do {
        const Smoke::Type& type = smoke->types[*argId];
        if (strcmp(type.name, iter->c_str()))
            return false;
        ++argId; ++iter;
    } while (*argId && iter != argTypes.end());

    return !*argId && iter == argTypes.end();
}

#define findMethodImpl(argsAssertion) \
    Smoke::ModuleIndex mi = findMethodMap(classId, mungedName); \
    if (!mi) \
        return mi; \
    const Smoke::MethodMap& map = mi.smoke->methodMaps[mi.index]; \
    assert(map.method != 0); \
    if (map.method > 0) { \
        return Smoke::ModuleIndex(mi.smoke, map.method); \
    } else { \
        assert(argsAssertion); \
        Smoke::Index *candidateId = mi.smoke->ambiguousMethodList - map.method; \
        do { \
            if (matchesArgs(mi.smoke, *candidateId, args)) { \
                return Smoke::ModuleIndex(mi.smoke, *candidateId); \
            } \
        } while (*++candidateId); \
    } \
    return Smoke::NullModuleIndex;


Smoke::ModuleIndex SmokeManager::findMethod(const Smoke::ModuleIndex& classId, const Smoke::ModuleIndex& mungedName,
                                            const char** args)
{
    findMethodImpl(args);
}

Smoke::ModuleIndex SmokeManager::findMethod(const Smoke::ModuleIndex& classId, const Smoke::ModuleIndex& mungedName,
                                            const std::vector<std::string>& args)
{
    findMethodImpl(!args.empty());
}

bool SmokeManager::isDerivedFrom(const Smoke::ModuleIndex& classId, const Smoke::ModuleIndex& baseClassId)
{
    if (classId == baseClassId)
        return true;

    Smoke *smoke = classId.smoke;

    for (Smoke::Index *p = smoke->inheritanceList + smoke->classes[classId.index].parents; *p; ++p) {
        Smoke::ModuleIndex parent;
        if (smoke->classes[*p].flags & Smoke::cf_undefined) {
            parent = findClass(smoke->classes[*p].className);
        } else {
            parent = Smoke::ModuleIndex(smoke, *p);
        }

        if (isDerivedFrom(parent, baseClassId))
            return true;
    }

    return false;
}

} // namespace SmokeUtils
