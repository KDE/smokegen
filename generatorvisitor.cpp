/*
    Copyright (C) 2009  Arno Rehn <arno@arnorehn.de>

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

#include <ast.h>
#include <tokens.h>
#include "name_compiler.h"
#include "type_compiler.h"

#include "generatorvisitor.h"
#include "options.h"

#include <QtDebug>

GeneratorVisitor::GeneratorVisitor(ParseSession *session, const QString& header) 
    : m_session(session), m_header(header), createType(false), createTypedef(false),
      inClass(0), isStatic(false), isVirtual(false), hasInitializer(false), currentTypeRef(0), inMethod(false)
{
    nc = new NameCompiler(m_session, this);
    tc = new TypeCompiler(m_session, this);
    
    usingTypes.push(QStringList());
    usingNamespaces.push(QStringList());
}

GeneratorVisitor::~GeneratorVisitor()
{
    delete nc;
    delete tc;
}

QPair<bool, bool> GeneratorVisitor::parseCv(const ListNode<std::size_t> *cv)
{
    QPair<bool, bool> ret(false, false);
    if (!cv) return ret;
    const ListNode<std::size_t> *it = cv->toFront(), *end = it;
    do {
        if (it->element) {
            int _kind = m_session->token_stream->kind(it->element);
            if (_kind == Token_const)
                ret.first = true;
            else if (_kind == Token_volatile)
                ret.second = true;
        }
        it = it->next;
    } while (end != it);
    return ret;
}

#define returnOnExistence(name) \
        if (classes.contains(name)) { \
            return &classes[name]; \
        } else if (typedefs.contains(name)) { \
            return &typedefs[name]; \
        } else if (enums.contains(name)) { \
            return &enums[name]; \
        }

BasicTypeDeclaration* GeneratorVisitor::resolveTypeInSuperClasses(const Class* klass, const QString& name)
{
    foreach (const Class::BaseClassSpecifier& bclass, klass->baseClasses()) {
        QString _name = bclass.baseClass->toString() + "::" + name;
        returnOnExistence(_name);
        if (!bclass.baseClass->baseClasses().count())
            continue;
        BasicTypeDeclaration* decl = resolveTypeInSuperClasses(bclass.baseClass, name);
        if (decl)
            return decl;
    }
    return 0;
}

BasicTypeDeclaration* GeneratorVisitor::resolveType(const QString & name)
{
    QString _name = name;
    return resolveType(_name);
}

// TODO: this might have to be improved for cases like 'Typedef::Nested foo'
BasicTypeDeclaration* GeneratorVisitor::resolveType(QString & name)
{
    // check for nested classes
    for (int i = klass.count() - 1; i >= 0; i--) {
        QString _name = klass[i]->toString() + "::" + name;
        returnOnExistence(_name);
        BasicTypeDeclaration* decl = resolveTypeInSuperClasses(klass[i], name);
        if (decl)
            return decl;
    }
    
    // check for 'using type;'
    // if we use 'type', we can also access type::nested, take care of that
    int index = name.indexOf("::");
    QString first = name, rest;
    if (index > -1) {
        first = name.mid(0, index);
        rest = name.mid(index);
    }
    foreach (const QStringList& list, usingTypes) {
        foreach (const QString& string, list) {
            QString complete = string + rest;
            if (string.endsWith(first)) {
                returnOnExistence(complete);
            }
        }
    }

    // check for the name in parent namespaces
    QStringList nspace = this->nspace;
    do {
        nspace.push_back(name);
        QString n = nspace.join("::");
        returnOnExistence(n);
        nspace.pop_back();
        if (!nspace.isEmpty())
            nspace.pop_back();
    } while (!nspace.isEmpty());

    // check for the name in any of the namespaces included by 'using namespace'
    foreach (const QStringList& list, usingNamespaces) {
        foreach (const QString& string, list) {
            QString cname = string + "::" + name;
            returnOnExistence(cname);
        }
    }

    QStringList parts = name.split("::");
    if (parts.count() > 1) {
        BasicTypeDeclaration* decl = resolveType(parts.takeFirst());
        if (!decl)
            return 0;
        parts.prepend(decl->toString());
        name = parts.join("::");
        returnOnExistence(name);
    }

    return 0;
}

#undef returnOnExistence

void GeneratorVisitor::visitAccessSpecifier(AccessSpecifierAST* node)
{
    if (!inClass) {
        DefaultVisitor::visitAccessSpecifier(node);
        return;
    }
    
    const ListNode<std::size_t> *it = node->specs->toFront(), *end = it;
    do {
        if (it->element) {
            const Token& t = token(it->element);
            if (t.kind == Token_public)
                access.top() = Access_public;
            else if (t.kind == Token_protected)
                access.top() = Access_protected;
            else if (t.kind == Token_private)
                access.top() = Access_private;
        }
        it = it->next;
    } while (end != it);
    DefaultVisitor::visitAccessSpecifier(node);
}

void GeneratorVisitor::visitBaseSpecifier(BaseSpecifierAST* node)
{
    Class::BaseClassSpecifier baseClass;
    int _kind = token(node->access_specifier).kind;
    if (_kind == Token_public) {
        baseClass.access = Access_public;
    } else if (_kind == Token_protected) {
        baseClass.access = Access_protected;
    } else {
        baseClass.access = Access_private;
    }
    baseClass.isVirtual = (node->virt > 0);
    nc->run(node->name);
    BasicTypeDeclaration* base = resolveType(nc->qualifiedName().join("::"));
    if (!base || !dynamic_cast<Class*>(base))
        return;
    baseClass.baseClass = static_cast<Class*>(base);
    klass.top()->appendBaseClass(baseClass);
}

void GeneratorVisitor::visitClassSpecifier(ClassSpecifierAST* node)
{
    nc->run(node->name);
    if (klass.isEmpty())
        return;
    
    if (kind == Class::Kind_Class)
        access.push(Access_private);
    else
        access.push(Access_public);
    inClass++;
    if (!klass.isEmpty()) {
        klass.top()->setFileName(m_header);
        klass.top()->setIsForwardDecl(false);
        if (klass.count() > 1) {
            // get the element before the last element, which is the parent
            Class* parent = klass[klass.count() - 2];
            parent->appendChild(klass.top());
        }
    }
    DefaultVisitor::visitClassSpecifier(node);
    access.pop();
    inClass--;
}

void GeneratorVisitor::visitDeclarator(DeclaratorAST* node)
{
    // TODO: get rid of this and add a proper typdef
    bool typeCreated = false;
    if (createType) {
        // run it again on the list of pointer operators to add them to the type
        tc->run(node);
        currentType = tc->type();
        currentTypeRef = Type::registerType(currentType);
        createType = false;
        typeCreated = true;
    }
    
    if (currentType.isFunctionPointer() && node->sub_declarator)
        nc->run(node->sub_declarator->id);
    else
        nc->run(node->id);
    const QString declName = nc->name();
    
    if (createTypedef) {
        if (!typeCreated)
            return;
        // we've just created the type that the typedef points to
        // so we just need to get the new name and store it
        Class* parent = klass.isEmpty() ? 0 : klass.top();
        Typedef tdef = Typedef(currentTypeRef, declName, nspace.join("::"), parent);
        tdef.setFileName(m_header);
        QString name = tdef.toString();
        if (!typedefs.contains(name)) {
            QHash<QString, Typedef>::iterator it = typedefs.insert(name, tdef);
            if (parent)
                parent->appendChild(&it.value());
            
            if (ParserOptions::qtMode && currentTypeRef->name() == "QFlags" &&
                !currentTypeRef->templateArguments().isEmpty())
            {
                // in Qt-mode, store that this is a QFlags-typedef
                flagTypes.insert(&it.value());
            }
        }
        createTypedef = false;
        return;
    }

    // we don't care about methods with ellipsis paramaters (i.e. 'foo(const char*, ...)') for now..
    if (node->parameter_declaration_clause && node->parameter_declaration_clause->ellipsis)
        return;

    // only run this if we're not in a method. only checking for parameter_declaration_clause
    // won't be enough because function pointer types also have that.
    if (node->parameter_declaration_clause && !inMethod && inClass) {
        bool isConstructor = (declName == klass.top()->name());
        bool isDestructor = (declName == "~" + klass.top()->name());
        Type* returnType = currentTypeRef;
        if (isConstructor) {
            // constructors return a pointer to the class they create
            Type t(klass.top()); t.setPointerDepth(1);
            returnType = Type::registerType(t);
        } else if (isDestructor) {
            // destructors don't have a return type.. so return void
            returnType = const_cast<Type*>(Type::Void);
        } else if (nc->isCastOperator()) {
            returnType = Type::registerType(nc->castType());
        }
        currentMethod = Method(klass.top(), declName, returnType, access.top());
        currentMethod.setIsConstructor(isConstructor);
        currentMethod.setIsDestructor(isDestructor);
        // build parameter list
        inMethod = true;
        visit(node->parameter_declaration_clause);
        inMethod = false;
        QPair<bool, bool> cv = parseCv(node->fun_cv);
        // const & volatile modifiers
        currentMethod.setIsConst(cv.first);
        if (isVirtual) currentMethod.setFlag(Method::Virtual);
        if (hasInitializer) currentMethod.setFlag(Method::PureVirtual);
        if (isStatic) currentMethod.setFlag(Method::Static);
        klass.top()->appendMethod(currentMethod);
        return;
    }
    
    // global function
    if (node->parameter_declaration_clause && !inMethod && !inClass && !currentTypeRef->isFunctionPointer()) {
        if (!declName.contains("::")) {
            Type* returnType = currentTypeRef;
            currentFunction = Function(declName, nspace.join("::"), returnType);
            currentFunction.setFileName(m_header);
            // build parameter list
            inMethod = true;
            visit(node->parameter_declaration_clause);
            inMethod = false;
            QString name = currentFunction.toString();
            if (!functions.contains(name)) {
                functions[name] = currentFunction;
            }
        }
        return;
    }
    
    // field
    if (!inMethod && !klass.isEmpty() && inClass) {
        Field field = Field(klass.top(), declName, currentTypeRef, access.top());
        if (isStatic) field.setFlag(Field::Static);
        klass.top()->appendField(field);
        return;
    } else if (!inMethod && !inClass) {
        // global variable
        if (!globals.contains(declName)) {
            GlobalVar var = GlobalVar(declName, nspace.join("::"), currentTypeRef);
            var.setFileName(m_header);
            globals[var.name()] = var;
        }
        return;
    }
}

void GeneratorVisitor::visitEnumSpecifier(EnumSpecifierAST* node)
{
    nc->run(node->name);
    Class* parent = klass.isEmpty() ? 0 : klass.top();
    currentEnum = Enum(nc->name(), nspace.join("::"), parent);
    Access a = (access.isEmpty() ? Access_public : access.top());
    currentEnum.setAccess(a);
    currentEnum.setFileName(m_header);
    QHash<QString, Enum>::iterator it = enums.insert(currentEnum.toString(), currentEnum);
    currentEnumRef = &it.value();
    visitNodes(this, node->enumerators);
    if (parent)
        parent->appendChild(currentEnumRef);
}

void GeneratorVisitor::visitEnumerator(EnumeratorAST* node)
{
    currentEnumRef->appendMember(EnumMember(currentEnumRef, token(node->id).symbolString(), QString()));
//     DefaultVisitor::visitEnumerator(node);
}

void GeneratorVisitor::visitFunctionDefinition(FunctionDefinitionAST* node)
{
    visit(node->type_specifier);
    if (node->function_specifiers) {
        const ListNode<std::size_t> *it = node->function_specifiers->toFront(), *end = it;
        do {
            if (it->element && m_session->token_stream->kind(it->element) == Token_virtual) {
                // found virtual token
                isVirtual = true;
                break;
            }
            it = it->next;
        } while (end != it);
    }
    if (node->storage_specifiers) {
        const ListNode<std::size_t> *it = node->storage_specifiers->toFront(), *end = it;
        do {
            if (it->element && m_session->token_stream->kind(it->element) == Token_static) {
                isStatic = true;
            } else if (it->element && m_session->token_stream->kind(it->element) == Token_friend) {
                // we're not interested in who's the friend of whom ;)
                return;
            }
            it = it->next;
        } while (end != it);
    }
    visit(node->init_declarator);
    isStatic = isVirtual = hasInitializer = false;
}

void GeneratorVisitor::visitInitializerClause(InitializerClauseAST *)
{
    // we don't care about initializers
    return;
}

void GeneratorVisitor::visitNamespace(NamespaceAST* node)
{
    usingTypes.push(QStringList());
    usingNamespaces.push(QStringList());

    QString name = token(node->namespace_name).symbolString();
    nspace.push_back(name);
    DefaultVisitor::visitNamespace(node);
    nspace.pop_back();
    // using directives in that namespace aren't interesting anymore :)
    usingTypes.pop();
    usingNamespaces.pop();
}

void GeneratorVisitor::visitParameterDeclaration(ParameterDeclarationAST* node)
{
    tc->run(node->type_specifier);
    QString name;
    if (node->declarator) {
        tc->run(node->declarator);
        if (currentType.isFunctionPointer() && node->declarator->sub_declarator)
            nc->run(node->declarator->sub_declarator->id);
        else
            nc->run(node->declarator->id);
        name = nc->name();
    }
    currentType = tc->type();
    currentTypeRef = Type::registerType(currentType);
    
    // foo(void) is the same as foo()
    if (currentTypeRef == Type::Void)
        return;
    
    QString defaultValue;
    if (node->expression) {
        // this parameter has a default value
        for (int i = node->expression->start_token; i < node->expression->end_token; i++)
            defaultValue.append(token(i).symbolByteArray());
    }
    if (inClass)
        currentMethod.appendParameter(Parameter(name, currentTypeRef, defaultValue));
    else
        currentFunction.appendParameter(Parameter(name, currentTypeRef, defaultValue));
}

void GeneratorVisitor::visitSimpleDeclaration(SimpleDeclarationAST* node)
{
    bool popKlass = false;
    int _kind = token(node->start_token).kind;
    if (_kind == Token_class) {
        kind = Class::Kind_Class;
    } else if (_kind == Token_struct) {
        kind = Class::Kind_Struct;
    }
    if (_kind == Token_class || _kind == Token_struct) {
        tc->run(node->type_specifier);
        if (tc->qualifiedName().isEmpty()) return;
        // for nested classes
        Class* parent = klass.isEmpty() ? 0 : klass.top();
        Class _class = Class(tc->qualifiedName().last(), nspace.join("::"), parent, kind);
        Access a = (access.isEmpty() ? Access_public : access.top());
        _class.setAccess(a);
        QString name = _class.toString();
        // This class has already been parsed.
        if (classes.contains(name) && !classes[name].isForwardDecl())
            return;
        QHash<QString, Class>::iterator item = classes.insert(name, _class);
        klass.push(&item.value());
        popKlass = true;
    }
    
    if (node->function_specifiers) {
        const ListNode<std::size_t> *it = node->function_specifiers->toFront(), *end = it;
        do {
            if (it->element && m_session->token_stream->kind(it->element) == Token_virtual) {
                // found virtual token
                isVirtual = true;
                break;
            }
            it = it->next;
        } while (end != it);
    }
    // look for initializers - if we find one, the method is pure virtual
    if (node->init_declarators) {
        const ListNode<InitDeclaratorAST*> *it = node->init_declarators->toFront(), *end = it;
        do {
            if (it->element && it->element->initializer) {
                hasInitializer = true;
                break;
            }
            it = it->next;
        } while (end != it);
    }
    if (node->storage_specifiers) {
        const ListNode<std::size_t> *it = node->storage_specifiers->toFront(), *end = it;
        do {
            if (it->element && m_session->token_stream->kind(it->element) == Token_static) {
                isStatic = true;
            } else if (it->element && m_session->token_stream->kind(it->element) == Token_friend) {
                // we're not interested in who's the friend of whom ;)
                if (popKlass)
                    klass.pop();
                isStatic = isVirtual = hasInitializer = false;
                return;
            }
            it = it->next;
        } while (end != it);
    }
    DefaultVisitor::visitSimpleDeclaration(node);
    if (popKlass)
        klass.pop();
    isStatic = isVirtual = hasInitializer = false;
}

void GeneratorVisitor::visitSimpleTypeSpecifier(SimpleTypeSpecifierAST* node)
{
    // first run on the type specifier to get the name of the type
    tc->run(node);
    createType = true;
    DefaultVisitor::visitSimpleTypeSpecifier(node);
}

void GeneratorVisitor::visitTemplateDeclaration(TemplateDeclarationAST* node)
{
    // skip template declarations
    return;
}

void GeneratorVisitor::visitTemplateArgument(TemplateArgumentAST* node)
{
    // skip template arguments - they're handled by TypeCompiler
    return;
}

void GeneratorVisitor::visitTypedef(TypedefAST* node)
{
    createTypedef = true;
    DefaultVisitor::visitTypedef(node);
}

void GeneratorVisitor::visitUsing(UsingAST* node)
{
    nc->run(node->name);
    usingTypes.top() << nc->name();
    DefaultVisitor::visitUsing(node);
}

void GeneratorVisitor::visitUsingDirective(UsingDirectiveAST* node)
{
    nc->run(node->name);
    usingNamespaces.top() << nc->name();
    DefaultVisitor::visitUsingDirective(node);
}
