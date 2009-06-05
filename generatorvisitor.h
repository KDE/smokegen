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

#ifndef PARSERVISITOR_H
#define PARSERVISITOR_H

#include <QStringList>
#include <QStack>

#include <default_visitor.h>
#include <parsesession.h>
#include <lexer.h>

#include "type.h"

class NameCompiler;
class TypeCompiler;

class GeneratorVisitor : public DefaultVisitor
{
public:
    GeneratorVisitor(ParseSession *session, bool resolveTypedefs = false);
    virtual ~GeneratorVisitor();
    QPair<Class*, Typedef*> resolveType(const QString& name);
    QPair<bool, bool> parseCv(const ListNode<std::size_t> *cv);
    inline bool resolveTypdefs() const { return m_resolveTypedefs; }

protected:
    inline const Token& token(std::size_t token) { return m_session->token_stream->token(token); }

    virtual void visitAccessSpecifier(AccessSpecifierAST* node);
    virtual void visitBaseSpecifier(BaseSpecifierAST* node);
    virtual void visitClassSpecifier(ClassSpecifierAST* node);
    virtual void visitDeclarator(DeclaratorAST* node);
    virtual void visitEnumSpecifier(EnumSpecifierAST *);
    virtual void visitEnumerator(EnumeratorAST *);
    virtual void visitInitializerClause(InitializerClauseAST *);
    virtual void visitNamespace(NamespaceAST* node);
    virtual void visitParameterDeclaration(ParameterDeclarationAST* node);
    virtual void visitSimpleDeclaration(SimpleDeclarationAST* node);
    virtual void visitSimpleTypeSpecifier(SimpleTypeSpecifierAST* node);
    virtual void visitTemplateDeclaration(TemplateDeclarationAST* node);
    virtual void visitTemplateArgument(TemplateArgumentAST* node);
    virtual void visitTypedef(TypedefAST* node);
    virtual void visitUsing(UsingAST* node);
    virtual void visitUsingDirective(UsingDirectiveAST* node);

private:
    NameCompiler *nc;
    TypeCompiler *tc;
    
    ParseSession *m_session;
    bool m_resolveTypedefs;
    
    bool createType;
    bool createTypedef;
    short inClass;
    
    bool isStatic;
    bool isVirtual;
    bool isPureVirtual;
    
    Type currentType;
    Type* currentTypeRef;
    
    bool inMethod;
    bool inParameter;
    Method currentMethod;
    
    Function currentFunction;
    
    Enum currentEnum;
    
    Class::Kind kind;
    QStack<Class*> klass;
    QStack<Access> access;
    
    QStack<QStringList> usingTypes;
    QStack<QStringList> usingNamespaces;
    
    QStringList nspace;
};

#endif // PARSERVISITOR_H
