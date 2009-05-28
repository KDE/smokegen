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

GeneratorVisitor::GeneratorVisitor(ParseSession *session) : m_session(session), inClass(0), pointerDepth(0), isRef(0)
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

Class* GeneratorVisitor::resolveClassName(const QString & name)
{
    // check for 'using type;'
    foreach (const QStringList& list, usingTypes) {
        foreach (const QString& string, list) {
            if (string.endsWith(name) && classes.contains(string))
                return &classes[string];
        }
    }

    // check for the name in parent namespaces
    QStringList nspace = this->nspace;
    do {
        nspace.push_back(name);
        QString n = nspace.join("::");
        if (classes.contains(n))
            return &classes[n];
        nspace.pop_back();
        if (!nspace.isEmpty())
            nspace.pop_back();
    } while (!nspace.isEmpty());

    // check for the name in any of the namespaces included by 'using namespace'
    foreach (const QStringList& list, usingNamespaces) {
        foreach (const QString& string, list) {
            QString cname = string + "::" + name;
            if (classes.contains(cname))
                return &classes[cname];
        }
    }

    return 0;
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
    // finish currentType
    QVector<bool> pointerDepth;
    bool isRef = false;
    this->pointerDepth = &pointerDepth;
    this->isRef = &isRef;
    visitNodes(this, node->ptr_ops);
    this->pointerDepth = 0;
    this->isRef = 0;
    
    currentType.setPointerDepth(pointerDepth.count());
    for (int i = 0; i < pointerDepth.count(); i++) {
        if (pointerDepth[i])
            currentType.setIsConstPointer(i, true);
    }
    currentType.setIsRef(isRef);
    
    QString typeStr = currentType.toString();
    if (!typeStr.isEmpty())
        types[typeStr] = currentType;
    
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
    usingTypes.pop();
    usingNamespaces.pop();
    nspace.pop_back();
}

void GeneratorVisitor::visitParameterDeclaration(ParameterDeclarationAST* node)
{
    DefaultVisitor::visitParameterDeclaration(node);
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
    DefaultVisitor::visitSimpleDeclaration(node);
}

void GeneratorVisitor::visitSimpleTypeSpecifier(SimpleTypeSpecifierAST* node)
{
    tc->run(node);
    Class* klass = resolveClassName(tc->qualifiedName().last());
    if (klass)
        currentType = Type(klass, tc->isConstant(), tc->isVolatile());
    else
        currentType = Type(tc->qualifiedName().join("::"), tc->isConstant(), tc->isVolatile());
    DefaultVisitor::visitSimpleTypeSpecifier(node);
}

void GeneratorVisitor::visitTemplateDeclaration(TemplateDeclarationAST* node)
{
    DefaultVisitor::visitTemplateDeclaration(node);
}

void GeneratorVisitor::visitTypedef(TypedefAST* node)
{
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
