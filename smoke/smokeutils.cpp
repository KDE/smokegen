/*
    Copyright (C) 2003-2011  Richard Dale <Richard_Dale@tipitina.demon.co.uk>
    Copyright (C) 2012  Arno Rehn <arno@arnorehn.de>

    Based on the PerlQt marshalling code by Ashley Winters

    This library is free software; you can redistribute and/or modify them under
    the terms of the GNU Lesser General Public License as published by the
    Free Software Foundation; either version 3 of the License, or (at your option)
    any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "smokeutils.h"
#include "smokemanager.h"

namespace SmokeUtils {

bool SmokeType::isUnsigned() const {
    if (!typeId()) return false;
    if (_unsigned > -1) return _unsigned;

    switch (elem()) {
        case Smoke::t_uchar:
        case Smoke::t_ushort:
        case Smoke::t_uint:
        case Smoke::t_ulong:
            return true;
    }

    _unsigned = (!strncmp(isConst() ? name() + static_strlen("const ") : name(), "unsigned ", static_strlen("unsigned ")));
    return _unsigned;
}

char SmokeType::pointerDepth() const {
    if (!typeId()) return 0;
    if (_pointerDepth > -1) return _pointerDepth;

    const char *n = name();
    signed char depth = 0;
    signed char templateDepth = 0;

    while (*n) {
        if (*n == '<') {
            ++templateDepth;
            depth = 0;
        }
        if (*n == '>')
            --templateDepth;
        if (templateDepth == 0 && *n == '*')
            ++depth;
        ++n;
    }
    _pointerDepth = depth;
    return _pointerDepth;
}

std::string SmokeType::plainName() const {
    if (!typeId()) return "void";
    if (isClass())
        return smoke()->classes[classId()].className;
    if (!_plainName.empty()) return _plainName;

    char offset = 0;
    if (isConst()) offset += static_strlen("const ");
    if (isUnsigned()) offset += static_strlen("unsigned ");

    const char *start = name() + offset;
    const char *n = start;

    signed char templateDepth = 0;

    while (*n) {
        if (*n == '<')
            ++templateDepth;
        if (*n == '>')
            --templateDepth;
        if (templateDepth == 0 && (*n == '*' || *n == '&')) {
            _plainName = std::string(start, static_cast<std::size_t>(n - start));
            return _plainName;
        }
        ++n;
    }

    _plainName = std::string(start);
    return _plainName;
}

// not cached, expensive
std::vector<SmokeType> SmokeType::templateArguments() const {
    std::vector<SmokeType> ret;
    if (!typeId()) return ret;

    int depth = 0;

    const char *begin = 0;
    const char *n = name();

    while (*n) {
        if (*n == '<') {
            if (depth == 0) {
                ret.clear();
                begin = n + 1;
            }

            ++depth;
        }

        if (depth == 1 && (*n == '>' || *n == ',')) {
            assert(begin);
            std::string str(begin, static_cast<std::size_t>((*(n-1) == ' ' ? n-1 : n)  - begin));

            Smoke::Index typeId = smoke()->idType(str.c_str());
            assert(typeId);
            ret.push_back(SmokeType(smoke(), typeId));

            begin = *n == ',' ? n + 1 : 0;
        }

        if (*n == '>')
            --depth;

        ++n;
    }

    return ret;
}

void *SmokeClass::constructCopy(void *obj, SmokeBinding *binding) const
{
    std::string ccSig = unqualifiedName();
    ccSig.append("#");

    std::string ccArg; ccArg.reserve(ccSig.size() + static_strlen("const &"));
    ccArg.append("const ").append(className()).push_back('&');

    std::vector<std::string> argTypes; argTypes.push_back(ccArg);

    SmokeMethod ccMeth = SmokeManager::self()->findMethod(moduleIndex(), ccSig, argTypes);

    if(!ccMeth) {
        return 0;
    }

    // Okay, ccMeth is the copy constructor. Time to call it.
    Smoke::StackItem args[2];
    args[0].s_voidp = 0;
    args[1].s_voidp = obj;
    ccMeth.call(args);

    // Initialize the binding for the new instance
    setBindingForObject(args[0].s_voidp, binding);

    return args[0].s_voidp;
}

bool SmokeClass::resolve() {
    if (!isExternal()) return true;
    Smoke::ModuleIndex newId = SmokeManager::self()->findClass(_c->className);
    if (!newId) return false;
    set(newId);
    return true;
}

std::string SmokeClass::unqualifiedName() const
{
    std::string name = className();
    int pos = name.find_last_of("::");

    if (pos != name.npos)
        name = name.substr(pos + static_strlen("::") - 1);

    return name;
}

} // namespace SmokeUtils
