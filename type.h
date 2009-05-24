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

#ifndef TYPE_H
#define TYPE_H

#include <QString>
#include <QList>

class Method;
class Field;

enum Access {
    Access_public,
    Access_protected,
    Access_private
};

class Class
{
public:
    enum Kind {
        Kind_Class,
        Kind_Struct,
        Kind_Union
    };
    
    struct BaseClassSpecifier {
        Class *baseClass;
        Access access;
        bool isVirtual;
    };
    
    Class(const QString& name = QString(), Kind kind = Kind_Class) : m_name(name), m_kind(kind) {}
    ~Class() {}
    
    void setName(const QString& name) { m_name = name; }
    QString name() { return m_name; } const
    
    void setKind(Kind kind) { m_kind = kind; }
    Kind kind() { return m_kind; } const
    
    QList<Method> methods() { return m_methods; } const
    void appendMethod(const Method& method) { m_methods.append(method); }
    
    QList<Field> fields() { return m_fields; } const
    void appendField(const Field& field) {  m_fields.append(field); }
    
    QList<BaseClassSpecifier> baseClasses() { return m_bases; }
    void appendBaseClass(const BaseClassSpecifier& baseClass) { m_bases.append(baseClass); }
    
private:
    QString m_name;
    Kind m_kind;
    QList<Method> m_methods;
    QList<Field> m_fields;
    QList<BaseClassSpecifier> m_bases;
};

class Member
{
};

class Method : public Member
{
};

class Field : public Member
{
};

class Type
{
public:
    
};

extern QList<Class> classes;
extern QList<Type> types;

#endif // TYPE_H
