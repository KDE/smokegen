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

#include "smokemanager.h"

template <typename CharType, std::size_t N>
constexpr std::size_t static_strlen(const CharType (&) [N]) {
    // subtract trailing zero
    return N - 1;
}

class SmokeType {
    Smoke::Type *_t;    // derived from _mi, but cached
    Smoke::ModuleIndex _mi;

    mutable signed char _pointerDepth;
    mutable signed char _unsigned;
    mutable std::string _plainName;

public:
    SmokeType() : _t(0), _mi(Smoke::NullModuleIndex), _pointerDepth(-1), _unsigned(-1) {}
    SmokeType(const Smoke::ModuleIndex& mi) { set(mi); }
    SmokeType(Smoke *s, Smoke::Index i) { set(s, i); }

    // mutators
    void set(const Smoke::ModuleIndex& mi) {
        _mi = mi;
        _pointerDepth = -1;
        _unsigned = -1;

        if (!mi.smoke) {
            _t = 0;
            return;
        }

        if(_mi.index < 0 || _mi.index > _mi.smoke->numTypes) _mi.index = 0;
        _t = _mi.smoke->types + _mi.index;
    }

    void set(Smoke *s, Smoke::Index i) {
        set(Smoke::ModuleIndex(s, i));
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

    char pointerDepth() const;
    std::string plainName() const;

    // not cached, expensive
    std::vector<SmokeType> templateArguments() const;

    // tests
    operator bool() const { return _mi.smoke; }

    bool isStack() const { return ((flags() & Smoke::tf_ref) == Smoke::tf_stack); }
    bool isPtr() const { return ((flags() & Smoke::tf_ref) == Smoke::tf_ptr); }
    bool isRef() const { return ((flags() & Smoke::tf_ref) == Smoke::tf_ref); }
    bool isConst() const { return (flags() & Smoke::tf_const); }
    bool isClass() const { return classId(); }
    bool isVoid() const { return !typeId(); }
    bool isFunctionPointer() const { return name() && strstr(name(), "(*)"); }
    bool isUnsigned() const;

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
    SmokeClass(Smoke *smoke, Smoke::Index id) {
        set(smoke, id);
    }
    SmokeClass(const Smoke::ModuleIndex& mi) {
        set(mi);
    }

    // mutators
    void set(const Smoke::ModuleIndex& mi) {
        _mi = mi;
        if (!mi.smoke) {
            _c = 0;
            return;
        }
        _c = _mi.smoke->classes + _mi.index;
    }

    void set(Smoke *s, Smoke::Index id) {
        set(Smoke::ModuleIndex(s, id));
    }

    // resolves external classes, returns true on success
    bool resolve();

    void setBindingForObject(void *obj, SmokeBinding *binding) const {
        Smoke::StackItem stack[2];
        stack[1].s_voidp = binding;
        classFn()(0, obj, stack);
    }

    operator bool() const { return _c && _c->className != 0; }

    Smoke::ModuleIndex moduleIndex() const { return _mi; }
    Smoke *smoke() const { return _mi.smoke; }
    const Smoke::Class &c() const { return *_c; }
    Smoke::Index classId() const { return _mi.index; }
    const char *className() const { return _c->className; }
    std::string unqualifiedName() const;
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
        return SmokeManager::self()->isDerivedFrom(_mi, sc._mi);
    }

#define iterateAncestorsImpl \
    for (Smoke::Index *p = _mi.smoke->inheritanceList + _c->parents; \
            *p; ++p) \
    { \
        SmokeClass klass(smoke(), *p); \
        klass.resolve(); \
        if (func(klass) || (!parentsOnly && klass.iterateAncestors(func))) { \
            return true; \
        } \
    } \
    return false;

    // need both overloads, otherwise lambdas might cause a compilation
    // error
    /// Func has to accept a SmokeClass argument and return a bool.
    /// If it returns true, the iteration is aborted.
    template <typename Func>
    bool iterateAncestors(const Func& func, bool parentsOnly = false) const
    { iterateAncestorsImpl }

    template <typename Func>
    bool iterateAncestors(Func& func, bool parentsOnly = false) const
    { iterateAncestorsImpl }
#undef iterateAncestorsImpl

    std::vector<SmokeClass> parents() const {
        struct Collector {
            std::vector<SmokeClass> val;
            bool operator()(const SmokeClass& klass) {
                val.push_back(klass);
                return false;
            }
        };
        Collector c; iterateAncestors(c, true);
        return c.val;
    }

    unsigned short flags() const { return _c->flags; }
    bool hasConstructor() const { return flags() & Smoke::cf_constructor; }
    bool hasCopy() const { return flags() & Smoke::cf_deepcopy; }
    bool hasVirtual() const { return flags() & Smoke::cf_virtual; }
    bool isExternal() const { return flags() & Smoke::cf_undefined; }
};

class SmokeMethod {
    Smoke::Method *_m;
    Smoke::ModuleIndex _mi;
public:
    enum CallMethod {
        DynamicDispatch = 0,
        Direct = 1
    };

    SmokeMethod() : _m(0), _mi(Smoke::NullModuleIndex) {}
    SmokeMethod(const Smoke::ModuleIndex& mi) {
        set(mi);
    }
    SmokeMethod(Smoke *smoke, Smoke::Index id) {
        set(smoke, id);
    }

    void set(const Smoke::ModuleIndex& mi) {
        _mi = mi;
        if (!mi.smoke) {
            _m = 0;
            return;
        }
        _m = mi.smoke->methods + mi.index;
    }
    void set(Smoke *s, Smoke::Index id) {
        set(Smoke::ModuleIndex(s, id));
    }

    operator bool() const { return _mi; }

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

    void call(Smoke::Stack args, void *ptr = 0, CallMethod callType = DynamicDispatch) const {
        Smoke::ClassFn fn = c().classFn();
        unsigned int offset = static_cast<unsigned int>(callType == Direct && !isPureVirtual() && isVirtual());
        (*fn)(method() + offset, ptr, args);
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

inline std::ostream& operator<<(std::ostream& stream, const SmokeMethod& method) {
    method.prettyPrint(stream);
    return stream;
}

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
