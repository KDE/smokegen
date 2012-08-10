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

#ifndef SMOKEUTILS_H
#define SMOKEUTILS_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_POSIX
#define __USE_POSIX
#endif
#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <smoke.h>

#include <cassert>

#include <string>
#include <sstream>
#include <vector>

class SmokeType {
    Smoke::Type *_t;    // derived from _mi, but cached
    Smoke::ModuleIndex _mi;

    signed char _pointerDepth;
    signed char _unsigned;
    std::string _plainName;

public:
    SmokeType() : _t(0), _mi(Smoke::NullModuleIndex), _pointerDepth(-1), _unsigned(-1) {}
    SmokeType(const Smoke::ModuleIndex& mi) { set(mi); }
    SmokeType(Smoke *s, Smoke::Index i) { set(s, i); }

    // mutators
    void set(const Smoke::ModuleIndex& mi) {
        _mi = mi;
        _pointerDepth = -1;
        _unsigned = -1;

        if(_mi.index < 0 || _mi.index > _mi.smoke->numTypes) _mi.index = 0;
        _t = _mi.smoke->types + _mi.index;
    }

    void set(Smoke *s, Smoke::Index i) {
        set(Smoke::ModuleIndex(s, i));
    }

    void preparse() {
        if (!typeId()) {
            return;
        }

        isUnsigned();
        pointerDepth();

        if (!isClass())
            plainName();
    }

    // accessors
    Smoke *smoke() const { return _mi.smoke; }
    Smoke::Index typeId() const { return _mi.index; }
    Smoke::ModuleIndex moduleIndex() const { return _mi; }
    const Smoke::Type &type() const { return *_t; }
    unsigned short flags() const { return _t->flags; }
    unsigned short elem() const { return _t->flags & Smoke::tf_elem; }
    const char *name() const { return _t->name ? _t->name : "void"; }
    Smoke::Index classId() const { return _t->classId; }

    // tests
    operator bool() const { return _mi != Smoke::NullModuleIndex; }

    bool isStack() const { return ((flags() & Smoke::tf_ref) == Smoke::tf_stack); }
    bool isPtr() const { return ((flags() & Smoke::tf_ref) == Smoke::tf_ptr); }
    bool isRef() const { return ((flags() & Smoke::tf_ref) == Smoke::tf_ref); }
    bool isConst() const { return (flags() & Smoke::tf_const); }
    bool isClass() const {
        if(elem() == Smoke::t_class)
            return classId() ? true : false;
        return false;
    }

    bool isUnsigned() const {
        if (!typeId()) return false;
        if (_unsigned > -1) return _unsigned;

        switch (elem()) {
            case Smoke::t_uchar:
            case Smoke::t_ushort:
            case Smoke::t_uint:
            case Smoke::t_ulong:
                return true;
        }

        return (!strncmp(isConst() ? name() + 6 : name(), "unsigned ", 9));
    }

    // cached
    bool isUnsigned() {
        if (_unsigned < 0)
            _unsigned = const_cast<const SmokeType*>(this)->isUnsigned();

        return _unsigned;
    }

    char pointerDepth() const {
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
        return depth;
    }

    // cached
    char pointerDepth() {
        if (_pointerDepth < 0)
            _pointerDepth = const_cast<const SmokeType*>(this)->pointerDepth();

        return _pointerDepth;
    }

    std::string plainName() const {
        if (!typeId()) return "void";
        if (isClass())
            return smoke()->classes[classId()].className;
        if (!_plainName.empty()) return _plainName;

        char offset = 0;
        if (isConst()) offset += 6;
        if (isUnsigned()) offset += 9;

        const char *start = name() + offset;
        const char *n = start;

        // unsigned long long&
        signed char templateDepth = 0;

        while (*n) {
            if (*n == '<')
                ++templateDepth;
            if (*n == '>')
                --templateDepth;
            if (templateDepth == 0 && (*n == '*' || *n == '&'))
                return std::string(start, static_cast<std::size_t>(n - start));
            ++n;
        }

        return std::string(start);
    }

    // cached
    std::string plainName() {
        if (isClass())
            return smoke()->classes[classId()].className;

        if (_plainName.empty())
            _plainName = const_cast<const SmokeType*>(this)->plainName();

        return _plainName;
    }

    // not cached, expensive
    std::vector<SmokeType> templateArguments() const {
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

    bool operator ==(const SmokeType &b) const {
        const SmokeType &a = *this;
        if(a.name() == b.name()) return true;
        if(a.name() && b.name() && !strcmp(a.name(), b.name()))
            return true;
        return false;
    }
    bool operator !=(const SmokeType &b) const {
        const SmokeType &a = *this;
        return !(a == b);
    }

};

class SmokeClass {
    Smoke::Class *_c;
    Smoke::ModuleIndex _mi;
public:
    SmokeClass() : _c(0), _mi(Smoke::NullModuleIndex) {}
    SmokeClass(const SmokeType &t) {
        _mi = Smoke::ModuleIndex(t.smoke(), t.classId());
        _c = _mi.smoke->classes + _mi.index;
    }
    SmokeClass(Smoke *smoke, Smoke::Index id) : _mi(Smoke::ModuleIndex(smoke, id)) {
        _c = _mi.smoke->classes + _mi.index;
    }
    SmokeClass(const Smoke::ModuleIndex& mi) : _mi(mi) {
        _c = _mi.smoke->classes + _mi.index;
    }

    operator bool() const { return _c && _c->className != 0; }

    Smoke::ModuleIndex moduleIndex() const { return _mi; }
    Smoke *smoke() const { return _mi.smoke; }
    const Smoke::Class &c() const { return *_c; }
    Smoke::Index classId() const { return _mi.index; }
    const char *className() const { return _c->className; }
    Smoke::ClassFn classFn() const { return _c->classFn; }
    Smoke::EnumFn enumFn() const { return _c->enumFn; }
    bool operator ==(const SmokeClass &b) const {
        const SmokeClass &a = *this;
        if(a.className() == b.className()) return true;
        if(a.className() && b.className() && !strcmp(a.className(), b.className()))
            return true;
        return false;
    }
    bool operator !=(const SmokeClass &b) const {
        const SmokeClass &a = *this;
        return !(a == b);
    }
    bool isa(const SmokeClass &sc) const {
        return Smoke::isDerivedFrom(_mi, sc._mi);
    }

    std::vector<SmokeClass> parents() const {
        std::vector<SmokeClass> ret;

        Smoke::Index *parentId = _mi.smoke->inheritanceList + _c->parents;
        while (*parentId) {
            ret.push_back(SmokeClass(_mi.smoke, *parentId));
            ++parentId;
        }

        return ret;
    }

    unsigned short flags() const { return _c->flags; }
    bool hasConstructor() const { return flags() & Smoke::cf_constructor; }
    bool hasCopy() const { return flags() & Smoke::cf_deepcopy; }
    bool hasVirtual() const { return flags() & Smoke::cf_virtual; }
    bool isExternal() const { return _c->external; }
};

class SmokeMethod {
    Smoke::Method *_m;
    Smoke::ModuleIndex _mi;
public:
    SmokeMethod(const Smoke::ModuleIndex& mi) : _mi(mi) {}
    SmokeMethod(Smoke *smoke, Smoke::Index id) : _mi(Smoke::ModuleIndex(smoke, id)) {
        _m = smoke->methods + id;
    }

    Smoke *smoke() const { return _mi.smoke; }
    const Smoke::Method &m() const { return *_m; }
    SmokeClass c() const { return SmokeClass(_mi.smoke, _m->classId); }
    const char *name() const { return _mi.smoke->methodNames[_m->name]; }
    int numArgs() const { return _m->numArgs; }
    unsigned short flags() const { return _m->flags; }
    SmokeType arg(int i) const {
        if(i >= numArgs()) return SmokeType();
        return SmokeType(_mi.smoke, _mi.smoke->argumentList[_m->args + i]);
    }
    SmokeType ret() const { return SmokeType(_mi.smoke, _m->ret); }
    Smoke::Index methodId() const { return _mi.index; }
    Smoke::ModuleIndex moduleMethodId() const { return _mi; }
    Smoke::Index method() const { return _m->method; }

    bool isAttribute() const { return flags() & Smoke::mf_attribute; }
    bool isProperty() const { return flags() & Smoke::mf_property; }
    bool isSignal() const { return flags() & Smoke::mf_signal; }
    bool isSlot() const { return flags() & Smoke::mf_slot; }

    bool isVirtual() const { return flags() & Smoke::mf_virtual; }
    bool isPureVirtual() const { return flags() & Smoke::mf_purevirtual; }

    bool isProtected() const { return flags() & Smoke::mf_protected; }
    bool isStatic() const { return flags() & Smoke::mf_static; }
    bool isConst() const { return flags() & Smoke::mf_const; }

    bool isConstructor() const { return flags() & Smoke::mf_ctor; }
    bool isCopyConstructor() const { return flags() & Smoke::mf_copyctor; }
    bool isExplicit() const { return flags() & Smoke::mf_explicit; }
    bool isDestructor() const { return flags() & Smoke::mf_dtor; }

    void call(Smoke::Stack args, void *ptr = 0) const {
        Smoke::ClassFn fn = c().classFn();
        (*fn)(method(), ptr, args);
    }

    template<typename OStream>
    void prettyPrint(OStream& stream) const {
        stream << ret().name() << " " << c().className() << "::" << name() << '(';
        for (int i = 0; i < numArgs(); ++i) {
            if (i != 0) stream << ", ";
            stream << arg(i).name();
        }
        stream << ')';
        if (isConst()) {
            stream << " const";
        }
    }

    std::string prettyPrint() const {
        std::ostringstream tmp;
        prettyPrint(tmp);
        return tmp.str();
    }
};

/*
 * Type handling by moc is simple.
 *
 * If the type name matches /^(?:const\s+)?\Q$types\E&?$/, use the
 * static_QUType, where $types is join('|', qw(bool int double char* QString);
 *
 * Everything else is passed as a pointer! There are types which aren't
 * Smoke::tf_ptr but will have to be passed as a pointer. Make sure to keep
 * track of what's what.
 */

/*
 * Simply using typeids isn't enough for signals/slots. It will be possible
 * to declare signals and slots which use arguments which can't all be
 * found in a single smoke object. Instead, we need to store smoke => typeid
 * pairs. We also need additional informatation, such as whether we're passing
 * a pointer to the union element.
 */

enum MocArgumentType {
    xmoc_ptr,
    xmoc_bool,
    xmoc_int,
    xmoc_uint,
    xmoc_long,
    xmoc_ulong,
    xmoc_double,
    xmoc_charstar,
    xmoc_QString,
    xmoc_void
};

struct MocArgument {
    // smoke object and associated typeid
    SmokeType st;
    MocArgumentType argType;
};

#endif // SMOKEUTILS_H
