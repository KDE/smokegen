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
