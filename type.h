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
#include <QtDebug>

class Class;
class Typedef;
class Enum;
class GlobalVar;
class Function;
class Type;

extern QHash<QString, Class> classes;
extern QHash<QString, Typedef> typedefs;
extern QHash<QString, Enum> enums;
extern QHash<QString, Function> functions;
extern QHash<QString, GlobalVar> globals;
extern QHash<QString, Type> types;

class Method;
class Field;

enum Access {
    Access_public,
    Access_protected,
    Access_private
};

class Class;

class BasicTypeDeclaration
{
public:
    virtual ~BasicTypeDeclaration() {}
    virtual bool isValid() const { return !m_name.isEmpty(); }
    
    void setName(const QString& name) { m_name = name; }
    QString name() const { return m_name; }
    
    void setNameSpace(const QString& nspace) { m_nspace = nspace; }
    QString nameSpace() const { return m_nspace; }

    void setParent(Class* parent) { m_parent = parent; }
    Class* parent() const { return m_parent; }

    void setFileName(const QString& fileName) { m_file = fileName; }
    QString fileName() const { return m_file; }

    QString toString() const;

protected:
    BasicTypeDeclaration(const QString& name = QString(), const QString& nspace = QString(), Class* parent = 0)
        : m_name(name), m_nspace(nspace), m_parent(parent) {}

    QString m_name;
    QString m_nspace;
    Class* m_parent;
    QString m_file;
};

class Class : public BasicTypeDeclaration
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
    
    Class(const QString& name = QString(), const QString nspace = QString(), Class* parent = 0, Kind kind = Kind_Class, bool isForward = true)
          : BasicTypeDeclaration(name, nspace, parent), m_kind(kind), m_forward(isForward) {}
    virtual ~Class() {}
    
    void setKind(Kind kind) { m_kind = kind; }
    Kind kind() const { return m_kind; } const
    
    void setIsForwardDecl(bool forward) { m_forward = forward; }
    bool isForwardDecl() const { return m_forward; }
    
    const QList<Method>& methods() const { return m_methods; } const
    void appendMethod(const Method& method) { m_methods.append(method); }
    
    const QList<Field>& fields() const { return m_fields; } const
    void appendField(const Field& field) {  m_fields.append(field); }
    
    const QList<BaseClassSpecifier>& baseClasses() const { return m_bases; }
    void appendBaseClass(const BaseClassSpecifier& baseClass) { m_bases.append(baseClass); }
    
    const QList<BasicTypeDeclaration*>& children() const { return m_children; }
    void appendChild(BasicTypeDeclaration* child) { m_children.append(child); }
    
private:
    Kind m_kind;
    bool m_forward;
    QList<Method> m_methods;
    QList<Field> m_fields;
    QList<BaseClassSpecifier> m_bases;
    QList<BasicTypeDeclaration*> m_children;
};

class Typedef : public BasicTypeDeclaration
{
public:
    Typedef(Type* type = 0, const QString& name = QString(), const QString nspace = QString(), Class* parent = 0)
            : BasicTypeDeclaration(name, nspace, parent), m_type(type) {}
    virtual ~Typedef() {}

    virtual bool isValid() const { return (!m_name.isEmpty() && m_type); }

    void setType(Type* type) { m_type = type; }
    Type* type() const { return m_type; }

    Type resolve() const;

private:
    Type* m_type;
};

typedef QPair<QString, QString> EnumMember;

class Enum : public BasicTypeDeclaration
{
public:
    Enum(const QString& name = QString(), const QString nspace = QString(), Class* parent = 0)
         : BasicTypeDeclaration(name, nspace, parent) {}
    virtual ~Enum() {}

    const QList<EnumMember>& members() const { return m_members; }
    void appendMember(const EnumMember& member) { m_members.append(member); }

private:
    QList<EnumMember> m_members;
};

class Member
{
public:
    enum Flag {
        Virtual = 0x1,
        PureVirtual = 0x2,
        Static = 0x4
    };
    typedef QFlags<Flag> Flags;

    Member(Class* klass = 0, const QString& name = QString(), Type* type = 0, Access access = Access_public)
        : m_class(klass), m_name(name), m_type(type), m_access(access) {}
    virtual ~Member() {}

    bool isValid() const { return (!m_name.isEmpty() && m_type && m_class); }

    void setClass(Class* klass) { m_class = klass; }
    Class* getClass() const { return m_class; }

    void setName(const QString& name) { m_name = name; }
    QString name() const { return m_name; }

    void setType(Type* type) { m_type = type; }
    Type* type() const { return m_type; }

    void setAccess(Access access) { m_access = access; }
    Access access() const { return m_access; }

    void setFlag(Flag flag) { m_flags |= flag; }
    Flags flags() const { return m_flags; }

    virtual QString toString(bool withAccess = false) const;

protected:
    Class* m_class;
    QString m_name;
    Type* m_type;
    Access m_access;
    Flags m_flags;
};

class Parameter
{
public:
    Parameter(const QString& name = QString(), Type* type = 0, bool isDefault = false)
        : m_name(name), m_type(type), m_default(isDefault) {}
    virtual ~Parameter() {}

    bool isValid() const { return m_type; }

    void setName(const QString& name) { m_name = name; }
    QString name() const { return m_name; }

    void setType(Type* type) { m_type = type; }
    Type* type() const { return m_type; }

    void setIsDefault(bool isDefault) { m_default = isDefault; }
    bool isDefault() const { return m_default; }

    QString toString() const;

protected:
    QString m_name;
    Type* m_type;
    bool m_default;
};

typedef QList<Parameter> ParameterList;

class Method : public Member
{
public:
    Method(Class* klass = 0, const QString& name = QString(), Type* type = 0, Access access = Access_public, ParameterList params = ParameterList())
        : Member(klass, name, type, access), m_params(params), m_isConstructor(false), m_isDestructor(false), m_isConst(false) {}
    virtual ~Method() {}

    const ParameterList& parameters() const { return m_params; }
    void appendParameter(const Parameter& param) { m_params.append(param); }

    void setIsConstructor(bool isCtor) { m_isConstructor = isCtor; }
    bool isConstructor() const { return m_isConstructor; }

    void setIsDestructor(bool isDtor) { m_isDestructor = isDtor; }
    bool isDestructor() const { return m_isDestructor; }

    void setIsConst(bool isConst) { m_isConst = isConst; }
    bool isConst() const { return m_isConst; }

    virtual QString toString(bool withAccess = false, bool withInitializer = true) const;

protected:
    ParameterList m_params;
    bool m_isConstructor;
    bool m_isDestructor;
    bool m_isConst;
};

class Field : public Member
{
public:
    Field(Class* klass = 0, const QString& name = QString(), Type* type = 0, Access access = Access_public)
        : Member(klass, name, type, access) {}
    virtual ~Field() {}
};

class GlobalVar
{
public:
    GlobalVar(const QString& name = QString(), Type* type = 0) : m_name(name), m_type(type) {}

    bool isValid() const { return (!m_name.isEmpty() && m_type); }

    void setName(const QString& name) { m_name = name; }
    QString name() const { return m_name; }

    void setType(Type* type) { m_type = type; }
    Type* type() const { return m_type; }

    void setFileName(const QString& fileName) { m_file = fileName; }
    QString fileName() { return m_file; }

    virtual QString toString() const;

protected:
    QString m_name;
    Type* m_type;
    
    QString m_file;
};

class Function : public GlobalVar
{
public:
    Function(const QString& name = QString(), Type* type = 0, ParameterList params = ParameterList())
        : GlobalVar(name, type), m_params(params) {}

    const ParameterList& parameters() const { return m_params; }
    void appendParameter(const Parameter& param) { m_params.append(param); }

    virtual QString toString() const;

protected:
    ParameterList m_params;
};

class Type
{
public:
    Type(Class* klass = 0, bool isConst = false, bool isVolatile = false, int pointerDepth = 0, bool isRef = false)
        : m_class(klass), m_typedef(0), m_enum(0), m_isConst(isConst), m_isVolatile(isVolatile), 
          m_pointerDepth(pointerDepth), m_isRef(isRef), m_isIntegral(false), m_isFunctionPointer(false) {}
    Type(Typedef* tdef, bool isConst = false, bool isVolatile = false, int pointerDepth = 0, bool isRef = false)
        : m_class(0), m_typedef(tdef), m_enum(0), m_isConst(isConst), m_isVolatile(isVolatile), 
          m_pointerDepth(pointerDepth), m_isRef(isRef), m_isIntegral(false), m_isFunctionPointer(false) {}
    Type(Enum* e, bool isConst = false, bool isVolatile = false, int pointerDepth = 0, bool isRef = false)
        : m_class(0), m_typedef(0), m_enum(e), m_isConst(isConst), m_isVolatile(isVolatile), 
          m_pointerDepth(pointerDepth), m_isRef(isRef), m_isIntegral(false), m_isFunctionPointer(false) {}    
    Type(const QString& name, bool isConst = false, bool isVolatile = false, int pointerDepth = 0, bool isRef = false)
        : m_class(0), m_typedef(0), m_enum(0), m_name(name), m_isConst(isConst), m_isVolatile(isVolatile), 
          m_pointerDepth(pointerDepth), m_isRef(isRef), m_isIntegral(false), m_isFunctionPointer(false) {}

    void setClass(Class* klass) { m_class = klass; m_typedef = 0; m_enum = 0; }
    Class* getClass() const { return m_class; }
    
    void setTypedef(Typedef* tdef) { m_typedef = tdef; m_class = 0; m_enum = 0; }
    Typedef* getTypedef() const { return m_typedef; }

    void setEnum(Enum* e) { m_enum = e; m_class = 0; m_typedef = 0; }
    Enum* getEnum() const { return m_enum; }

    void setName(const QString& name) { m_name = name; }
    QString name() const {
        if (m_class) {
            return m_class->toString();
        } else if (m_typedef) {
            return m_typedef->toString();
        } else if (m_enum) {
            return m_enum->toString();
        } else {
            return m_name;
        }
    }

    bool isValid() const { return (m_class || m_typedef || !m_name.isEmpty()); }
    
    void setIsConst(bool isConst) { m_isConst = isConst; }
    bool isConst() const { return m_isConst; }
    void setIsVolatile(bool isVolatile) { m_isVolatile = isVolatile; }
    bool isVolatile() const { return m_isVolatile; }
    
    void setPointerDepth(int depth) { m_pointerDepth = depth; }
    int pointerDepth() const { return m_pointerDepth; }
    
    void setIsConstPointer(int depth, bool isConst) { m_constPointer[depth] = isConst; }
    bool isConstPointer(int depth) const { return m_constPointer.value(depth, false); }
    
    void setIsRef(bool isRef) { m_isRef = isRef; }
    bool isRef() const { return m_isRef; }

    void setIsIntegral(bool isIntegral) { m_isIntegral = isIntegral; }
    bool isIntegral() const { return m_isIntegral; }

    const QList<Type>& templateArguments() const { return m_templateArgs; }
    void appendTemplateArgument(const Type& type) { m_templateArgs.append(type); }
    void setTemplateArguments(const QList<Type>& types) { m_templateArgs = types; }

    void setIsFunctionPointer(bool isPtr) { m_isFunctionPointer = isPtr; }
    bool isFunctionPointer() const { return m_isFunctionPointer; }
    const ParameterList& parameters() const { return m_params; }
    void appendParameter(const Parameter& param) { m_params.append(param); }

    QString toString() const;

    static Type* registerType(const Type& type) {
        QString typeString = type.toString();
        if (types.contains(typeString)) {
            // return a reference to the existing type
            return &types[typeString];
        } else {
            QHash<QString, Type>::iterator iter = types.insert(typeString, type);
            return &iter.value();
        }
    }

    static const Type* Void;

protected:
    Class* m_class;
    Typedef* m_typedef;
    Enum* m_enum;
    QString m_name;
    bool m_isConst, m_isVolatile;
    int m_pointerDepth;
    QHash<int, bool> m_constPointer;
    bool m_isRef;
    bool m_isIntegral;
    QList<Type> m_templateArgs;
    bool m_isFunctionPointer;
    ParameterList m_params;
};

#endif // TYPE_H
