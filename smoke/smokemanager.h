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

#include "smoke.h"

#include <map>
#include <string>
#include <vector>

class Smoke;

namespace SmokeUtils {

class BASE_SMOKE_EXPORT SmokeManager
{
public:
    typedef std::map<std::string, Smoke*> StringSmokeMap;

    enum LoadOptions {
        DoNotLoad = 0,
        LoadIfRequired = 1,
    };

    static SmokeManager *self();

    void manage(Smoke *smoke);
    Smoke *load(const std::string& moduleName);
    Smoke *get(const std::string& moduleName, LoadOptions options = DoNotLoad);

    const StringSmokeMap& loadedModules() const;

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

    /// find method map id for method 'mungedName' in class 'classId'
    Smoke::ModuleIndex findMethodMap(const Smoke::ModuleIndex& classId, const Smoke::ModuleIndex& mungedName);
    inline Smoke::ModuleIndex findMethodMap(const Smoke::ModuleIndex& classId, const char *mungedName)
        { return findMethodMap(classId, findMethodName(classId, mungedName)); }
    inline Smoke::ModuleIndex findMethodMap(const Smoke::ModuleIndex& classId, const std::string& mungedName)
        { return findMethodMap(classId, mungedName.c_str()); }
    inline Smoke::ModuleIndex findMethodMap(const char *className, const char *mungedName)
        { return findMethodMap(findClass(className), mungedName); }
    inline Smoke::ModuleIndex findMethodMap(const std::string& className, const std::string& mungedName)
        { return findMethodMap(className.c_str(), mungedName.c_str()); }

    /// find method id for method 'mungedName' in 'classId' with 'args' used for ambigious overload resolution
    Smoke::ModuleIndex findMethod(const Smoke::ModuleIndex& classId, const Smoke::ModuleIndex& mungedName,
                                  const char **args);
    inline Smoke::ModuleIndex findMethod(const Smoke::ModuleIndex& classId, const char *mungedName,
                                         const char **args)
        { return findMethod(classId, findMethodName(classId, mungedName), args); }
    inline Smoke::ModuleIndex findMethod(const char *className, const char *mungedName, const char **args)
        { return findMethod(findClass(className), mungedName, args); }
    Smoke::ModuleIndex findMethod(const Smoke::ModuleIndex& classId, const Smoke::ModuleIndex& mungedName,
                                  const std::vector<std::string>& args);
    inline Smoke::ModuleIndex findMethod(const Smoke::ModuleIndex& classId, const std::string& mungedName,
                                  const std::vector<std::string>& args)
        { return findMethod(classId, findMethodName(classId, mungedName.c_str()), args); }
    inline Smoke::ModuleIndex findMethod(const std::string& className, const std::string& mungedName,
                                         const std::vector<std::string>& args)
        { return findMethod(findClass(className.c_str()), mungedName, args); }

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

} // namespace SmokeUtils

#endif // SMOKEMANAGER_H
