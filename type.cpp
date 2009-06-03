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
QHash<QString, Enum> enums;
QHash<QString, Function> functions;
QHash<QString, GlobalVar> globals;
QHash<QString, Type> types;

QString Class::toString() const
{
    QString ret;
    Class* parent = m_parent;
    while (parent) {
        ret.prepend(parent->name() + "::");
        parent = parent->parent();
    }
    if (!m_nspace.isEmpty())
        ret.prepend(m_nspace + "::");
    ret += m_name;
    return ret;
}

QString Enum::toString() const
{
    QString ret;
    Class* parent = m_parent;
    while (parent) {
        ret.prepend(parent->name() + "::");
        parent = parent->parent();
    }
    if (!m_nspace.isEmpty())
        ret.prepend(m_nspace + "::");
    ret += m_name;
    return ret;
}

QString Member::toString(bool withAccess) const
{
    QString ret;
    if (withAccess) {
        if (m_access == Access_public)
            ret += "public ";
        else if (m_access == Access_protected)
            ret += "protected ";
        else if (m_access == Access_private)
            ret += "private ";
    }
    if (m_flags & Static)
        ret += "static ";
    if (m_flags & Virtual)
        ret += "virtual ";
    ret += m_type->toString() + " " + m_name;
    return ret;
}

QString Parameter::toString() const
{
    return m_type->toString() + " " + m_name;
}

QString Method::toString(bool withAccess) const
{
    QString ret = Member::toString(withAccess);
    ret += "(";
    for (int i = 0; i < m_params.count(); i++) {
        ret += m_params[i].toString();
        if (i < m_params.count() - 1) ret += ", ";
    }
    ret += ")";
    if (m_isConst) ret += " const";
    if (m_flags & Member::PureVirtual) ret += " = 0";
    return ret;
}

QString Typedef::toString() const
{
    QString ret;
    Class* parent = m_parent;
    while (parent) {
        ret.prepend(parent->name() + "::");
        parent = parent->parent();
    }
    if (!m_nspace.isEmpty())
        ret.prepend(m_nspace + "::");
    ret += m_name;
    return ret;
}

const Type* Type::Void = Type::registerType(Type("void"));

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

QString GlobalVar::toString() const
{
    return m_type->toString() + " " + m_name;
}

QString Function::toString() const
{
    QString ret = GlobalVar::toString();
    ret += "(";
    for (int i = 0; i < m_params.count(); i++) {
        ret += m_params[i].type()->toString();
        if (i < m_params.count() - 1) ret += ", ";
    }
    ret += ")";
    return ret;
}

QString Type::toString() const
{
    QString ret;
    if (m_isVolatile) ret += "volatile ";
    if (m_isConst) ret += "const ";
    ret += name();
    if (!m_templateArgs.isEmpty()) {
        ret += "< ";
        for (int i = 0; i < m_templateArgs.count(); i++) {
            if (i > 0) ret += ',';
            ret += m_templateArgs[i].toString();
        }
        ret += " >";
    }
    for (int i = 0; i < m_pointerDepth; i++) {
        ret += "*";
        if (isConstPointer(i)) ret += " const ";
    }
    ret = ret.trimmed();
    if (m_isRef) ret += "&";
    return ret;
}
