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
#include <smoke.h>

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

    /// find class id for 'className'
    Smoke::ModuleIndex findClass(const char* className);
    inline Smoke::ModuleIndex findClass(const std::string& className) { return findClass(className.c_str()); }

    /// find method name id for 'methodName' (lookups performed in all managed modules)
    Smoke::ModuleIndex findMethodName(const char *methodName);
    inline Smoke::ModuleIndex findMethodName(const std::string& methodName)
        { return findMethodName(methodName.c_str()); }

    /// find method name id for 'methodName' in class denoted by 'classId'
    inline Smoke::ModuleIndex findMethodName(const Smoke::ModuleIndex& classId, const char *methodName)
        { return findMethodNameImpl(classId, methodName); }
    inline Smoke::ModuleIndex findMethodName(const Smoke::ModuleIndex& classId, const std::string& methodName)
        { return findMethodName(classId, methodName.c_str()); }

    /// find method map id for method 'methodName' in class 'classId'
    Smoke::ModuleIndex findMethod(const Smoke::ModuleIndex& classId, const Smoke::ModuleIndex& methodName);
    inline Smoke::ModuleIndex findMethod(const Smoke::ModuleIndex& classId, const char *methodName)
        { return findMethod(classId, findMethodName(classId, methodName)); }
    inline Smoke::ModuleIndex findMethod(const Smoke::ModuleIndex& classId, const std::string& methodName)
        { return findMethod(classId, methodName.c_str()); }
    inline Smoke::ModuleIndex findMethod(const char *className, const char *methodName)
        { return findMethod(findClass(className), methodName); }
    inline Smoke::ModuleIndex findMethod(const std::string& className, const std::string& methodName)
        { return findMethod(className.c_str(), methodName.c_str()); }

    bool isDerivedFrom(const Smoke::ModuleIndex& classId, const Smoke::ModuleIndex& baseClassId);
    inline bool isDerivedFrom(const char *klass, const char *baseClass)
        { return isDerivedFrom(findClass(klass), findClass(baseClass)); }
    inline bool isDerivedFrom(const std::string& klass, const std::string& baseClass)
        { return isDerivedFrom(klass.c_str(), baseClass.c_str()); }

protected:
    Smoke::ModuleIndex findMethodNameImpl(const Smoke::ModuleIndex& classId, const char *methodName, Smoke *last = 0);

    SmokeManager();
    SmokeManager(const SmokeManager& other);
    ~SmokeManager();

    struct Data;
    Data *d;
};

#endif // SMOKEMANAGER_H
