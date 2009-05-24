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
    
    bool isValid() { return !m_name.isEmpty(); }
    
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

    bool isValid() { return (!m_name.isEmpty() && m_type); }

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
    enum Flag {
        Virtual = 0x1,
        Static = 0x2
    };
    typedef QFlags<Flag> Flags;

    Member(const QString& name = QString(), Type* type = 0, Access access = Access_public) : m_name(name), m_type(type), m_access(access) {}
    virtual ~Member() {}

    bool isValid() { return (!m_name.isEmpty() && m_type); }

    void setName(const QString& name) { m_name = name; }
    QString name() { return m_name; }

    void setType(Type* type) { m_type = type; }
    Type* type() { return m_type; }

    void setAccess(Access access) { m_access = access; }
    Access access() { return m_access; }

    void setFlag(Flag flag) { m_flags |= flag; }
    Flags flags() { return m_flags; }

protected:
    QString m_name;
    Type* m_type;
    Access m_access;
    Flags m_flags;
};

class Parameter
{
public:
    Parameter(const QString& name = QString(), Type* type = 0) : m_name(name), m_type(type) {}
    virtual ~Parameter() {}

    bool isValid() { return (!m_name.isEmpty() && m_type); }

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
        : Member(name, type, access), m_params(params), m_isConstructor(false), m_isDestructor(false), m_isConst(false) {}
    virtual ~Method() {}

    ParameterList parameters() { return m_params; }
    void appendParameter(const Parameter& param) { m_params.append(param); }

    void setIsConstructor(bool isCtor) { m_isConstructor = isCtor; }
    bool isConstructor() { return m_isConstructor; }

    void setIsDestructor(bool isDtor) { m_isDestructor = isDtor; }
    bool isDestructor() { return m_isDestructor; }

    void setIsConst(bool isConst) { m_isConst = isConst; }
    bool isConst() { return m_isConst; }

protected:
    ParameterList m_params;
    bool m_isConstructor;
    bool m_isDestructor;
    bool m_isConst;
};

class Field : public Member
{
public:
    Field(const QString& name = QString(), Type* type = 0, Access access = Access_public) : Member(name, type, access) {}
    virtual ~Field() {}
};

class Function
{
public:
    Function(const QString& name = QString(), Type* type = 0, ParameterList params = ParameterList())
        : m_name(name), m_type(type), m_params(params) {}

    bool isValid() { return (!m_name.isEmpty() && m_type); }

    void setName(const QString& name) { m_name = name; }
    QString name() { return m_name; }

    void setType(Type* type) { m_type = type; }
    Type* type() { return m_type; }

    ParameterList parameters() { return m_params; }
    void appendParameter(const Parameter& param) { m_params.append(param); }

    void setIsStatic(bool isStatic) { m_isStatic = isStatic; }
    bool isStatic() { return m_isStatic; }

private:
    QString m_name;
    Type* m_type;
    ParameterList m_params;
    bool m_isStatic;
};

class Type
{
public:
    Type(Class* klass = 0, bool isConst = false, bool isVolatile = false, int pointerDepth = 0, bool isRef = false)
        : m_class(klass), m_typedef(0), m_isConst(isConst), m_isVolatile(isVolatile), m_pointerDepth(pointerDepth), m_isRef(isRef) {}
    Type(Typedef* tdef = 0, bool isConst = false, bool isVolatile = false, int pointerDepth = 0, bool isRef = false)
        : m_class(0), m_typedef(tdef), m_isConst(isConst), m_isVolatile(isVolatile), m_pointerDepth(pointerDepth), m_isRef(isRef) {}

    void setClass(Class* klass) { m_class = klass; m_typedef = 0; }
    Class* getClass() { return m_class; }
    
    void setTypedef(Typedef* tdef) { m_typedef = tdef; m_class = 0; }
    Typedef* getTypedef() { return m_typedef; }
    
    bool isValid() { return (m_class || m_typedef); }
    
    void setIsConst(bool isConst) { m_isConst = isConst; }
    bool isConst() { return m_isConst; }
    void setIsVolatile(bool isVolatile) { m_isVolatile = isVolatile; }
    bool isVolatile() { return m_isVolatile; }
    
    void setPointerDepth(int depth) { m_pointerDepth = depth; }
    int pointerDepth() { return m_pointerDepth; }
    
    void setIsConstPointer(int depth, bool isConst) { m_constPointer[depth] = isConst; }
    bool isConstPointer(int depth) { return m_constPointer.value(depth, false); }
    
    void setIsRef(bool isRef) { m_isRef = isRef; }
    bool isRef() { return m_isRef; }

protected:
    Class* m_class;
    Typedef* m_typedef;
    bool m_isConst, m_isVolatile;
    int m_pointerDepth;
    QHash<int, bool> m_constPointer;
    bool m_isRef;
};

extern QHash<QString, Class> classes;
extern QHash<QString, Typedef> typedefs;
extern QHash<QString, Function> functions;
extern QHash<QString, Type> types;

#endif // TYPE_H
