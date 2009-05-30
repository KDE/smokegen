/*
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

#include "type.h"

QHash<QString, Class> classes;
QHash<QString, Typedef> typedefs;
QHash<QString, Function> functions;
QHash<QString, Type> types;

const Type Type::Void("void");

Type Typedef::resolve() const {
    bool isRef = false, isConst = false, isVolatile = false;
    QList<bool> pointerDepth;
    const Type* t = this->type();
    for (int i = 0; i < t->pointerDepth(); i++) {
        pointerDepth.append(t->isConstPointer(i));
    }
    while (t->getTypedef()) {
        if (!isRef) isRef = t->isRef();
        if (!isConst) isConst = t->isConst();
        if (!isVolatile) isVolatile = t->isVolatile();
        t = t->getTypedef()->type();
        for (int i = t->pointerDepth() - 1; i >= 0; i--) {
            pointerDepth.prepend(t->isConstPointer(i));
        }
    }
    Type ret = *t;
    if (isRef) ret.setIsRef(true);
    if (isConst) ret.setIsConst(true);
    if (isVolatile) ret.setIsVolatile(true);
    
    ret.setPointerDepth(pointerDepth.count());
    for (int i = 0; i < pointerDepth.count(); i++) {
        ret.setIsConstPointer(i, pointerDepth[i]);
    }
    return ret;
}
