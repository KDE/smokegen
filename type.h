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
#include <QHash>

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

class Type;

class Typedef
{
public:
    Typedef(Type* type, const QString& name) : m_type(type), m_name(name) {}

    void setType(Type* type) { m_type = type; }
    Type* type() { return m_type; }

    void setName(const QString& name) { m_name = name; }
    QString name() { return m_name; }

private:
    Type* m_type;
    QString m_name;
};

class Member
{
public:
    Member(const QString& name = QString(), Type* type = 0, Access access = Access_public) : m_name(name), m_type(type), m_access(access) {}
    virtual ~Member() {}

    void setName(const QString& name) { m_name = name; }
    QString name() { return m_name; }

    void setType(Type* type) { m_type = type; }
    Type* type() { return m_type; }

    void setAccess(Access access) { m_access = access; }
    Access access() { return m_access; }

protected:
    QString m_name;
    Type* m_type;
    Access m_access;
};

class Parameter
{
public:
    Parameter(const QString& name = QString(), Type* type = 0) : m_name(name), m_type(type) {}
    virtual ~Parameter() {}

    void setName(const QString& name) { m_name = name; }
    QString name() { return m_name; }

    void setType(Type* type) { m_type = type; }
    Type* type() { return m_type; }

protected:
    QString m_name;
    Type* m_type;
};

typedef QList<Parameter> ParameterList;

class Method : public Member
{
public:
    Method(const QString& name = QString(), Type* type = 0, Access access = Access_public, ParameterList params = ParameterList())
        : Member(name, type, access), m_params(params), m_isConstructor(false), m_isDestructor(false) {}
    virtual ~Method() {}

    ParameterList parameters() { return m_params; }
    void appendParameter(const Parameter& param) { m_params.append(param); }

    void setIsConstructor(bool isCtor) { m_isConstructor = isCtor; }
    bool isConstructor() { return m_isConstructor; }

    void setIsDestructor(bool isDtor) { m_isDestructor = isDtor; }
    bool isDestructor() { return m_isDestructor; }

protected:
    ParameterList m_params;
    bool m_isConstructor;
    bool m_isDestructor;
};

class Field : public Member
{
public:
    Field(const QString& name = QString(), Type* type = 0, Access access = Access_public) : Member(name, type, access) {}
    virtual ~Field() {}
};

class Type
{
public:
    
};

extern QHash<QString, Class> classes;
extern QHash<QString, Type> types;

#endif // TYPE_H
