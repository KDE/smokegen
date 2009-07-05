/*
    Generator for the SMOKE sources
    Copyright (C) 2009 Arno Rehn <arno@arnorehn.de>

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

#include <QHash>
#include <QList>

#include <type.h>

#include "globals.h"

QHash<QString, QString> Util::typeMap;
QHash<const Method*, const Function*> Util::globalFunctionMap;
QHash<const Method*, QStringList> Util::defaultParameterValues;

QList<const Class*> Util::superClassList(const Class* klass)
{
    static QHash<const Class*, QList<const Class*> > superClassCache;

    QList<const Class*> ret;
    if (superClassCache.contains(klass))
        return superClassCache[klass];
    foreach (const Class::BaseClassSpecifier& base, klass->baseClasses()) {
        ret << base.baseClass;
        ret.append(superClassList(base.baseClass));
    }
    // cache
    superClassCache[klass] = ret;
    return ret;
}

QList<const Class*> Util::descendantsList(const Class* klass)
{
    static QHash<const Class*, QList<const Class*> > descendantsClassCache;

    QList<const Class*> ret;
    if (descendantsClassCache.contains(klass))
        return descendantsClassCache[klass];
    for (QHash<QString, Class>::const_iterator iter = classes.constBegin(); iter != classes.constEnd(); iter++) {
        if (superClassList(&iter.value()).contains(klass))
            ret << &iter.value();
    }
    // cache
    descendantsClassCache[klass] = ret;
    return ret;
}

void Util::preparse(QSet<Type*> *usedTypes, const QList<QString>& keys)
{
    Class& globalSpace = classes["QGlobalSpace"];
    globalSpace.setName("QGlobalSpace");
    globalSpace.setKind(Class::Kind_Class);
    
    // add all functions as methods to a class called 'QGlobalSpace'
    for (QHash<QString, Function>::const_iterator it = functions.constBegin(); it != functions.constEnd(); it++) {
        const Function& fn = it.value();
        
        // gcc doesn't like this function... for whatever reason
        if (fn.name() == "_IO_ftrylockfile")
            continue;
        
        Method meth = Method(&globalSpace, fn.name(), fn.type(), Access_public, fn.parameters());
        meth.setFlag(Method::Static);
        globalSpace.appendMethod(meth);
        // map this method to the function, so we can later retrieve the header it was defined in
        globalFunctionMap[&globalSpace.methods().last()] = &fn;
        
        (*usedTypes) << meth.type();
        foreach (const Parameter& param, meth.parameters())
            (*usedTypes) << param.type();
    }
    
    // all enums that don't have a parent are put under QGlobalSpace, too
    for (QHash<QString, Enum>::iterator it = enums.begin(); it != enums.end(); it++) {
        if (!it.value().parent()) {
            Type *t = Type::registerType(Type(&it.value()));
            (*usedTypes) << t;
            globalSpace.appendChild(&it.value());
        }
    }
    
    foreach (const QString& key, keys) {
        Class& klass = classes[key];
        addCopyConstructor(&klass);
        addDefaultConstructor(&klass);
        addDestructor(&klass);
        foreach (const Method& m, klass.methods()) {
            if (m.access() == Access_private)
                continue;
            if (m.type()->getClass() && m.type()->getClass()->access() == Access_private) {
                klass.methodsRef().removeOne(m);
                continue;
            }
            addOverloads(m);
            (*usedTypes) << m.type();
            foreach (const Parameter& param, m.parameters())
                (*usedTypes) << param.type();
        }
        foreach (const Field& f, klass.fields()) {
            if (f.access() == Access_private)
                continue;
            (*usedTypes) << f.type();
        }
        foreach (BasicTypeDeclaration* decl, klass.children()) {
            Enum* e = 0;
            if ((e = dynamic_cast<Enum*>(decl))) {
                Type *t = Type::registerType(Type(e));
                (*usedTypes) << t;
            }
        }
    }
}

bool Util::canClassBeInstanciated(const Class* klass)
{
    static QHash<const Class*, bool> cache;
    if (cache.contains(klass))
        return cache[klass];
    
    bool ctorFound = false, publicCtorFound = false;
    foreach (const Method& meth, klass->methods()) {
        if (meth.isConstructor()) {
            ctorFound = true;
            if (meth.access() != Access_private) {
                publicCtorFound = true;
                // this class can be instanciated, break the loop
                break;
            }
        }
    }
    
    // The class can be instanciated if it has a public constructor or no constructor at all
    // because then it has a default one generated by the compiler.
    bool ret = (publicCtorFound || !ctorFound);
    cache[klass] = ret;
    return ret;
}

bool Util::canClassBeCopied(const Class* klass)
{
    static QHash<const Class*, bool> cache;
    if (cache.contains(klass))
        return cache[klass];

    bool privateCopyCtorFound = false;
    foreach (const Method& meth, klass->methods()) {
        if (meth.access() != Access_private)
            continue;
        if (meth.isConstructor() && meth.parameters().count() == 1) {
            const Type* type = meth.parameters()[0].type();
            // c'tor should be Foo(const Foo& copy)
            if (type->isConst() && type->isRef() && type->getClass() == klass) {
                privateCopyCtorFound = true;
                break;
            }
        }
    }
    
    bool parentCanBeCopied = true;
    foreach (const Class::BaseClassSpecifier& base, klass->baseClasses()) {
        if (!canClassBeCopied(base.baseClass)) {
            parentCanBeCopied = false;
            break;
        }
    }
    
    // if the parent can be copied and we didn't find a private copy c'tor, the class is copiable
    bool ret = (parentCanBeCopied && !privateCopyCtorFound);
    cache[klass] = ret;
    return ret;
}

bool Util::hasClassVirtualDestructor(const Class* klass)
{
    static QHash<const Class*, bool> cache;
    if (cache.contains(klass))
        return cache[klass];

    bool virtualDtorFound = false;
    foreach (const Method& meth, klass->methods()) {
        if (meth.isDestructor() && meth.flags() & Method::Virtual) {
            virtualDtorFound = true;
            break;
        }
    }
    
    cache[klass] = virtualDtorFound;
    return virtualDtorFound;
}

bool Util::hasClassPublicDestructor(const Class* klass)
{
    static QHash<const Class*, bool> cache;
    if (cache.contains(klass))
        return cache[klass];

    bool publicDtorFound = true;
    foreach (const Method& meth, klass->methods()) {
        if (meth.isDestructor()) {
            if (meth.access() != Access_public)
                publicDtorFound = false;
            // a class has only one destructor, so break here
            break;
        }
    }
    
    cache[klass] = publicDtorFound;
    return publicDtorFound;
}

void Util::addDefaultConstructor(Class* klass)
{
    foreach (const Method& meth, klass->methods()) {
        // if the class already has a constructor or if it has pure virtuals, there's nothing to do for us
        if (meth.isConstructor() || meth.flags() & Method::PureVirtual)
            return;
    }
    
    Type t = Type(klass);
    t.setPointerDepth(1);
    Method meth = Method(klass, klass->name(), Type::registerType(t));
    meth.setIsConstructor(true);
    klass->appendMethod(meth);
}

void Util::addCopyConstructor(Class* klass)
{
    foreach (const Method& meth, klass->methods()) {
        if (meth.isConstructor() && meth.parameters().count() == 1) {
            const Type* type = meth.parameters()[0].type();
            // found a copy c'tor? then there's nothing to do
            if (type->isConst() && type->isRef() && type->getClass() == klass)
                return;
        }
    }
    
    // if the parent can't be copied, a copy c'tor is of no use
    foreach (const Class::BaseClassSpecifier& base, klass->baseClasses()) {
        if (!canClassBeCopied(base.baseClass))
            return;
    }
    
    Type t = Type(klass);
    t.setPointerDepth(1);
    Method meth = Method(klass, klass->name(), Type::registerType(t));
    meth.setIsConstructor(true);
    // parameter is a constant reference to another object of the same types
    Type paramType = Type(klass, true); paramType.setIsRef(true);
    meth.appendParameter(Parameter("copy", Type::registerType(paramType)));
    klass->appendMethod(meth);
}

void Util::addDestructor(Class* klass)
{
    foreach (const Method& meth, klass->methods()) {
        // we already have a destructor
        if (meth.isDestructor())
            return;
    }
    
    Method meth = Method(klass, "~" + klass->name(), const_cast<Type*>(Type::Void));
    meth.setIsDestructor(true);
    klass->appendMethod(meth);
}

QString Util::mungedName(const Method& meth) {
    QString ret = meth.name();
    foreach (const Parameter& param, meth.parameters()) {
        const Type* type = param.type();
        if (type->pointerDepth() > 1) {
            // reference to array or hash
            ret += "?";
        } else if (type->isIntegral() || type->getEnum() || Options::stringTypes.contains(type->name())) {
            // plain scalar
            ret += "$";
        } else if (type->getClass()) {
            // object
            ret += "#";
        } else {
            // unknown
            ret += "?";
        }
    }
    return ret;
}

QString Util::stackItemField(const Type* type)
{
    if (type->pointerDepth() > 0 || type->isRef() || type->isFunctionPointer() || (!type->isIntegral() && !type->getEnum()))
        return "s_class";
    if (type->getEnum())
        return "s_enum";
    
    QString typeName = type->name();
    // replace the unsigned stuff, look the type up in Util::typeMap and if
    // necessary, add a 'u' for unsigned types at the beginning again
    bool _unsigned = false;
    if (typeName.startsWith("unsigned ")) {
        typeName.replace("unsigned ", "");
        _unsigned = true;
    }
    typeName.replace("signed ", "");
    typeName = Util::typeMap.value(typeName, typeName);
    if (_unsigned)
        typeName.prepend('u');
    return "s_" + typeName;
}

QString Util::assignmentString(const Type* type, const QString& var)
{
    if (type->pointerDepth() > 0 || type->isFunctionPointer()) {
        return "(void*)" + var;
    } else if (type->isRef()) {
        return "(void*)&" + var;
    } else if (type->isIntegral()) {
        return var;
    } else if (type->getEnum()) {
        return var;
    } else {
        QString ret = "(void*)new ";
        if (Class* retClass = type->getClass())
            ret += retClass->toString();
        else if (Typedef* retTdef = type->getTypedef())
            ret += retTdef->toString(); 
        else
            ret += type->toString();
        ret += '(' + var + ')';
        return ret;
    }
    return QString();
}

QList<const Method*> Util::collectVirtualMethods(const Class* klass)
{
    QList<const Method*> methods;
    foreach (const Method& meth, klass->methods()) {
        if ((meth.flags() & Method::Virtual || meth.flags() & Method::PureVirtual) && !meth.isDestructor())
            methods << &meth;
    }
    foreach (const Class::BaseClassSpecifier& baseClass, klass->baseClasses()) {
        methods.append(collectVirtualMethods(baseClass.baseClass));
    }
    return methods;
}

// don't make this public - it's just a utility function for the next method and probably not what you would expect it to be
static bool operator==(const Method& rhs, const Method& lhs)
{
    // these have to be equal for methods to be the same
    bool ok = (rhs.name() == lhs.name() && rhs.isConst() == lhs.isConst() &&
               rhs.parameters().count() == lhs.parameters().count() && rhs.type() == lhs.type());
    if (!ok)
        return false;
    
    // now check the parameter types for equality
    for (int i = 0; i < rhs.parameters().count(); i++) {
        if (rhs.parameters()[i].type() != lhs.parameters()[i].type())
            return false;
    }
    
    return true;
}

void Util::addOverloads(const Method& meth)
{
    ParameterList params;
    Class* klass = meth.getClass();
    
    for (int i = 0; i < meth.parameters().count(); i++) {
        const Parameter& param = meth.parameters()[i];
        if (!param.isDefault()) {
            params << param;
            continue;
        }
        Method overload = meth;
        overload.setParameterList(params);
        klass->appendMethod(overload);
        
        QStringList remainingDefaultValues;
        for (int j = i; j < meth.parameters().count(); j++) {
            const Parameter defParam = meth.parameters()[j];
            QString cast = "(";
            cast += defParam.type()->toString() + ')';
            cast += defParam.defaultValue();
            remainingDefaultValues << cast;
        }
        defaultParameterValues[&klass->methods().last()] = remainingDefaultValues;
        
        params << param;
    }
}

// checks if method meth is overriden in class klass or any of its superclasses
const Method* Util::isVirtualOverriden(const Method& meth, const Class* klass)
{
    // is the method virtual at all?
    if (!(meth.flags() & Method::Virtual) && !(meth.flags() & Method::PureVirtual))
        return 0;
    
    // if the method is defined in klass, it can't be overriden there or in any parent class
    if (meth.getClass() == klass)
        return 0;
    
    foreach (const Method& m, klass->methods()) {
        if (!(m.flags() & Method::Static) && m == meth)
            // the method m overrides meth
            return &m;
    }
    
    foreach (const Class::BaseClassSpecifier& base, klass->baseClasses()) {
        // we reached the class in which meth was defined and we still didn't find any overrides => return
        if (base.baseClass == meth.getClass())
            return 0;
        
        // recurse into the base classes
        const Method* m = 0;
        if ((m = isVirtualOverriden(meth, base.baseClass)))
            return m;
    }
    
    return 0;
}
