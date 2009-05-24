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

#include "generatorvisitor.h"

#include <ast.h>

void GeneratorVisitor::visitAccessSpecifier(AccessSpecifierAST* node)
{
    DefaultVisitor::visitAccessSpecifier(node);
}

void GeneratorVisitor::visitBaseClause(BaseClauseAST* node)
{
    DefaultVisitor::visitBaseClause(node);
}

void GeneratorVisitor::visitBaseSpecifier(BaseSpecifierAST* node)
{
    DefaultVisitor::visitBaseSpecifier(node);
}

void GeneratorVisitor::visitClassSpecifier(ClassSpecifierAST* node)
{
    DefaultVisitor::visitClassSpecifier(node);
}

void GeneratorVisitor::visitFunctionDefinition(FunctionDefinitionAST* node)
{
    DefaultVisitor::visitFunctionDefinition(node);
}

void GeneratorVisitor::visitNamespace(NamespaceAST* node)
{
    DefaultVisitor::visitNamespace(node);
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
