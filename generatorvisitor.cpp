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

GeneratorVisitor::GeneratorVisitor(ParseSession *session) : m_session(session), createClass(0), inClass(0)
{
    nc = new NameCompiler(m_session);
    tc = new TypeCompiler(m_session);
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
    if (createClass) {
        nc->run(node->id);
        QString name;
        if (nspace.count() > 0) name = nspace.join("::").append("::");
        name.append(nc->name());
        QHash<QString, Class>::iterator item = classes.insert(name, Class(nc->name(), kind));
        klass.push(&item.value());
        createClass--;
        DefaultVisitor::visitDeclarator(node);
        return;
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
    DefaultVisitor::visitNamespace(node);
    nspace.pop_back();
}

void GeneratorVisitor::visitParameterDeclaration(ParameterDeclarationAST* node)
{
    DefaultVisitor::visitParameterDeclaration(node);
}

void GeneratorVisitor::visitParameterDeclarationClause(ParameterDeclarationClauseAST* node)
{
    DefaultVisitor::visitParameterDeclarationClause(node);
}

void GeneratorVisitor::visitPtrOperator(PtrOperatorAST* node)
{
    DefaultVisitor::visitPtrOperator(node);
}

void GeneratorVisitor::visitSimpleDeclaration(SimpleDeclarationAST* node)
{
    int _kind = token(node->start_token).kind;
    if (_kind == Token_class) {
        kind = Class::Kind_Class;
        createClass++;
    } else if (_kind == Token_struct) {
        kind = Class::Kind_Struct;
        createClass++;
    }
    DefaultVisitor::visitSimpleDeclaration(node);
}

void GeneratorVisitor::visitSimpleTypeSpecifier(SimpleTypeSpecifierAST* node)
{
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
    DefaultVisitor::visitUsing(node);
}

void GeneratorVisitor::visitUsingDirective(UsingDirectiveAST* node)
{
    DefaultVisitor::visitUsingDirective(node);
}
