/* This file is part of KDevelop
    Copyright 2002-2005 Roberto Raggi <roberto@kdevelop.org>
    Copyright 2009 Arno Rehn <arno@arnorehn.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "type_compiler.h"
#include "name_compiler.h"
#include "generatorvisitor.h"
#include <lexer.h>
#include <symbol.h>
#include <tokens.h>
#include <parsesession.h>

#include <QtCore/QString>

TypeCompiler::TypeCompiler(ParseSession* session, GeneratorVisitor* visitor)
  : m_session(session), m_visitor(visitor)
{
}

void TypeCompiler::run(TypeSpecifierAST* node, const ListNode< PtrOperatorAST* > *ptr_ops)
{
  m_type.clear();
  m_realType = Type();
  m_templateArgs.clear();
  isRef = false;
  pointerDepth.clear();
  _M_cv.clear();

  if (node && node->cv)
    {
      const ListNode<std::size_t> *it = node->cv->toFront();
      const ListNode<std::size_t> *end = it;
      do
        {
          int kind = m_session->token_stream->kind(it->element);
          if (! _M_cv.contains(kind))
            _M_cv.append(kind);

          it = it->next;
        }
      while (it != end);
    }
  visit(node);
  
  if (ptr_ops)
    run(ptr_ops);
}

void TypeCompiler::run(const ListNode< PtrOperatorAST* > *ptr_ops)
{
    visitNodes(this, ptr_ops);
    if (isRef) m_realType.setIsRef(true);
    int offset = m_realType.pointerDepth();
    m_realType.setPointerDepth(offset + pointerDepth.count());
    for (int i = 0; i < pointerDepth.count(); i++) {
        if (pointerDepth[i])
        m_realType.setIsConstPointer(offset + i, true);
    }
}

void TypeCompiler::visitClassSpecifier(ClassSpecifierAST *node)
{
  visit(node->name);
}


void TypeCompiler::visitEnumSpecifier(EnumSpecifierAST *node)
{
  visit(node->name);
}

void TypeCompiler::visitElaboratedTypeSpecifier(ElaboratedTypeSpecifierAST *node)
{
  visit(node->name);
}

void TypeCompiler::visitPtrOperator(PtrOperatorAST* node)
{
    if (token_text(m_session->token_stream->kind(node->op))[0] == '*') {
        QPair<bool, bool> cv = m_visitor->parseCv(node->cv);
        pointerDepth.append(cv.first);
    } else if (token_text(m_session->token_stream->kind(node->op))[0] == '&') {
        isRef = true;
    }
}

void TypeCompiler::visitSimpleTypeSpecifier(SimpleTypeSpecifierAST *node)
{
  if (const ListNode<std::size_t> *it = node->integrals)
    {
      it = it->toFront();
      const ListNode<std::size_t> *end = it;
      do
        {
          std::size_t token = it->element;
          // FIXME
          m_type.push_back(token_name(m_session->token_stream->kind(token)));
          it = it->next;
        }
      while (it != end);
    }
  else if (node->type_of)
    {
      // ### implement me
      m_type.push_back("typeof<...>");
    }

  visit(node->name);
  
  if (node->integrals) {
    m_realType = Type(m_type.join(" "), isConstant(), isVolatile());
  } else {
    QPair<Class*, Typedef*> type = m_visitor->resolveType(m_type.join("::"));
    if (type.first) {
        m_realType = Type(type.first, isConstant(), isVolatile());
    } else if (type.second) {
        if (m_visitor->resolveTypdefs())
            m_realType = type.second->resolve();
        else
            m_realType = Type(type.second);
        if (isConstant()) m_realType.setIsConst(true);
        if (isVolatile()) m_realType.setIsVolatile(true);
    } else {
        m_realType = Type(m_type.join("::"), isConstant(), isVolatile());
    }
    
    m_realType.setTemplateArguments(m_templateArgs);
  }
}

void TypeCompiler::visitName(NameAST *node)
{

  NameCompiler name_cc(m_session, m_visitor);
  name_cc.run(node);
  m_type = name_cc.qualifiedName();
  m_templateArgs = name_cc.templateArguments();
}

QStringList TypeCompiler::cvString() const
{
  QStringList lst;

  foreach (int q, cv())
    {
      if (q == Token_const)
        lst.append(QLatin1String("const"));
      else if (q == Token_volatile)
        lst.append(QLatin1String("volatile"));
    }

  return lst;
}

bool TypeCompiler::isConstant() const
{
    return _M_cv.contains(Token_const);
}

bool TypeCompiler::isVolatile() const
{
    return _M_cv.contains(Token_volatile);
}

/*QualifiedIdentifier TypeCompiler::identifier() const
{
  return _M_type;
}*/

