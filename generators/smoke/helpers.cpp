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

QList<const Class*> superClassList(const Class* klass)
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

QList<const Class*> descendantsList(const Class* klass)
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

void collectTypes(const QList<QString>& keys)
{
    foreach (const QString& key, keys) {
        const Class& klass = classes[key];
        foreach (const Method& m, klass.methods()) {
            if (m.access() == Access_private)
                continue;
            usedTypes << m.type();
            foreach (const Parameter& param, m.parameters())
                usedTypes << param.type();
        }
        foreach (const Field& f, klass.fields()) {
            if (f.access() == Access_private)
                continue;
            usedTypes << f.type();
        }
    }
}

bool isClassUsed(const Class* klass)
{
    for (QSet<Type*>::const_iterator it = usedTypes.constBegin(); it != usedTypes.constEnd(); it++) {
        if ((*it)->getClass() == klass)
            return true;
    }
    return false;
}

bool canClassBeInstanciated(const Class* klass)
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

bool canClassBeCopied(const Class* klass)
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

bool hasClassVirtualDestructor(const Class* klass)
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
