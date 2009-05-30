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
#include <name_compiler.h>
#include <tokens.h>
#include <type_compiler.h>

#include "generatorvisitor.h"

#include <QtDebug>

GeneratorVisitor::GeneratorVisitor(ParseSession *session, bool resolveTypedefs) 
    : m_session(session), m_resolveTypedefs(resolveTypedefs), createType(false), createTypedef(false),
      inClass(0), pointerDepth(0), isRef(0), isStatic(false), isVirtual(false), isPureVirtual(false), 
      currentTypeRef(0), inMethod(false), inParameter(false)
{
    nc = new NameCompiler(m_session);
    tc = new TypeCompiler(m_session);
    
    usingTypes.push(QStringList());
    usingNamespaces.push(QStringList());
}


inline
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

QPair<Class*, Typedef*> GeneratorVisitor::resolveType(const QString & name)
{
    // check for 'using type;'
    foreach (const QStringList& list, usingTypes) {
        foreach (const QString& string, list) {
            if (string.endsWith(name)) {
                if (classes.contains(string))
                    return qMakePair<Class*, Typedef*>(&classes[string], 0);
                else if (typedefs.contains(string))
                    return qMakePair<Class*, Typedef*>(0, &typedefs[string]);
            }
        }
    }

    // check for the name in parent namespaces
    QStringList nspace = this->nspace;
    do {
        nspace.push_back(name);
        QString n = nspace.join("::");
        if (classes.contains(n))
            return qMakePair<Class*, Typedef*>(&classes[n], 0);
        else if (typedefs.contains(n))
            return qMakePair<Class*, Typedef*>(0, &typedefs[n]);
        nspace.pop_back();
        if (!nspace.isEmpty())
            nspace.pop_back();
    } while (!nspace.isEmpty());

    // check for the name in any of the namespaces included by 'using namespace'
    foreach (const QStringList& list, usingNamespaces) {
        foreach (const QString& string, list) {
            QString cname = string + "::" + name;
            if (classes.contains(cname))
                return qMakePair<Class*, Typedef*>(&classes[cname], 0);
            else if (typedefs.contains(cname))
                return qMakePair<Class*, Typedef*>(0, &typedefs[cname]);
        }
    }

    return qMakePair<Class*, Typedef*>(0, 0);
}

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
    DefaultVisitor::visitBaseSpecifier(node);
}

void GeneratorVisitor::visitClassSpecifier(ClassSpecifierAST* node)
{
    if (kind == Class::Kind_Class)
        access.push(Access_private);
    else
        access.push(Access_public);
    inClass++;
    DefaultVisitor::visitClassSpecifier(node);
    inClass--;
}

void GeneratorVisitor::visitDeclarator(DeclaratorAST* node)
{
    if (createType) {
        // finish currentType
        QVector<bool> pointerDepth;
        bool isRef = false;
        // set global pointers to these vars so visitPtrOperator() can access them
        this->pointerDepth = &pointerDepth;
        this->isRef = &isRef;
        visitNodes(this, node->ptr_ops);
        // set them to 0 again so nothing bad will happen
        this->pointerDepth = 0;
        this->isRef = 0;
        
        int offset = currentType.pointerDepth();
        currentType.setPointerDepth(offset + pointerDepth.count());
        for (int i = 0; i < pointerDepth.count(); i++) {
            if (pointerDepth[i])
                currentType.setIsConstPointer(offset + i, true);
        }
        if (isRef) currentType.setIsRef(true);
        
        currentTypeRef = Type::registerType(currentType);
        
        // done with creating the type
        createType = false;
    }
    if (createTypedef) {
        // we've just created the type that the typedef points to
        // so we just need to get the new name and store it
        nc->run(node->id);
        QStringList name = nspace;
        name.append(nc->name());
        QString joined = name.join("::");
        if (!typedefs.contains(joined))
            typedefs[joined] = Typedef(currentTypeRef, nc->name(), nspace.join("::"));
        createTypedef = false;
    }
    
    if (node->parameter_declaration_clause && !inMethod) {
        nc->run(node->id);
        bool isConstructor = (nc->name() == klass.top()->name());
        bool isDestructor = (nc->name() == "~" + klass.top()->name());
        Type* returnType = currentTypeRef;
        if (isConstructor) {
            Type t(klass.top()); t.setPointerDepth(1);
            returnType = Type::registerType(t);
        } else if (isDestructor) {
            returnType = const_cast<Type*>(&Type::Void);
        }
        currentMethod = Method(klass.top(), nc->name(), returnType, access.top());
        inMethod = true;
        visit(node->parameter_declaration_clause);
        inMethod = false;
        QPair<bool, bool> cv = parseCv(node->fun_cv);
        currentMethod.setIsConst(cv.first);
        if (isVirtual) currentMethod.setFlag(Method::Virtual);
        if (isPureVirtual) currentMethod.setFlag(Method::PureVirtual);
        if (isStatic) currentMethod.setFlag(Method::Static);
        klass.top()->appendMethod(currentMethod);
    }
    if (inParameter) {
        nc->run(node->id);
        currentMethod.appendParameter(Parameter(nc->name(), currentTypeRef));
    }
    DefaultVisitor::visitDeclarator(node);
}

void GeneratorVisitor::visitFunctionDefinition(FunctionDefinitionAST* node)
{
    DefaultVisitor::visitFunctionDefinition(node);
}

void GeneratorVisitor::visitNamespace(NamespaceAST* node)
{
    nspace.push_back(token(node->namespace_name).symbolString());
    usingTypes.push(QStringList());
    usingNamespaces.push(QStringList());
    DefaultVisitor::visitNamespace(node);
    // using directives in that namespace aren't interesting anymore :)
    usingTypes.pop();
    usingNamespaces.pop();
    nspace.pop_back();
}

void GeneratorVisitor::visitParameterDeclaration(ParameterDeclarationAST* node)
{
    inParameter = true;
    DefaultVisitor::visitParameterDeclaration(node);
    inParameter = false;
}

void GeneratorVisitor::visitPtrOperator(PtrOperatorAST* node)
{
    if (pointerDepth && token_text(m_session->token_stream->kind(node->op))[0] == '*') {
        QPair<bool, bool> cv = parseCv(node->cv);
        pointerDepth->append(cv.first);
    } else if (isRef && token_text(m_session->token_stream->kind(node->op))[0] == '&') {
        *isRef = true;
    }
    DefaultVisitor::visitPtrOperator(node);
}

void GeneratorVisitor::visitSimpleDeclaration(SimpleDeclarationAST* node)
{
    int _kind = token(node->start_token).kind;
    if (_kind == Token_class) {
        kind = Class::Kind_Class;
    } else if (_kind == Token_struct) {
        kind = Class::Kind_Struct;
    }
    if (_kind == Token_class || _kind == Token_struct) {
        tc->run(node->type_specifier);
        QString name;
        if (nspace.count() > 0) name = nspace.join("::").append("::");
        name.append(tc->qualifiedName().last());
        QHash<QString, Class>::iterator item = classes.insert(name, Class(tc->qualifiedName().last(), nspace.join("::"), kind));
        klass.push(&item.value());
    }
    
    if (node->function_specifiers) {
        const ListNode<std::size_t> *it = node->function_specifiers->toFront(), *end = it;
        do {
            if (it->element && m_session->token_stream->kind(it->element) == Token_virtual) {
                // found virtual token
                isVirtual = true;
                
                // look for initializers - if we find one, the method is pure virtual
                if (node->init_declarators) {
                    const ListNode<InitDeclaratorAST*> *it = node->init_declarators->toFront(), *end = it;
                    do {
                        if (it->element && it->element->initializer) {
                            isPureVirtual = true;
                            break;
                        }
                        it = it->next;
                    } while (end != it);
                }
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
                break;
            }
            it = it->next;
        } while (end != it);
    }
    DefaultVisitor::visitSimpleDeclaration(node);
    isStatic = isVirtual = isPureVirtual = false;
}

void GeneratorVisitor::visitSimpleTypeSpecifier(SimpleTypeSpecifierAST* node)
{
    tc->run(node);
    // the method returns void - we don't need to create a new type for that
    if (tc->qualifiedName().join("::") == "void") {
        currentTypeRef = const_cast<Type*>(&Type::Void);
        DefaultVisitor::visitSimpleTypeSpecifier(node);
        return;
    }
    // resolve the type in the current context
    QPair<Class*, Typedef*> klass = resolveType(tc->qualifiedName().last());
    if (klass.first) {
        // it's a class
        currentType = Type(klass.first, tc->isConstant(), tc->isVolatile());
    } else if (klass.second) {
        // it's a typedef, resolve it or not?
        if (!m_resolveTypedefs) {
            currentType = Type(klass.second, tc->isConstant(), tc->isVolatile());
        } else {
            currentType = klass.second->resolve();
            if (tc->isConstant()) currentType.setIsConst(true);
            if (tc->isVolatile()) currentType.setIsVolatile(true);
        }
    } else {
        // type not known (e.g. integral type)
        currentType = Type(tc->qualifiedName().join("::"), tc->isConstant(), tc->isVolatile());
    }
    createType = true;
    DefaultVisitor::visitSimpleTypeSpecifier(node);
}

void GeneratorVisitor::visitTemplateDeclaration(TemplateDeclarationAST* node)
{
    DefaultVisitor::visitTemplateDeclaration(node);
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
