/* This file is part of KDevelop
    Copyright 2002-2005 Roberto Raggi <roberto@kdevelop.org>

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

#ifndef NAME_COMPILER_H
#define NAME_COMPILER_H

#include <QString>
#include <QStringList>

#include "default_visitor.h"
#include "type.h"

class ParseSession;
class OperatorAST;
class GeneratorVisitor;

class Q_DECL_EXPORT NameCompiler: protected DefaultVisitor
{
public:
  NameCompiler(ParseSession* session, GeneratorVisitor* visitor);

  void run(NameAST *node);
  void run(UnqualifiedNameAST *node) { m_typeSpecifier = 0; internal_run(node); }

  QString name() const { return m_name.join("::"); }
  const QStringList& qualifiedName() const { return m_name; }
  const QList<Type>& templateArguments() const { return m_templateArgs; }

  /**
   * When the name contains one type-specifier, this should contain that specifier after the run.
   * Especially this is the type-specifier of a conversion-operator like "operator int()"
   * */
  TypeSpecifierAST* lastTypeSpecifier() const;

protected:
  virtual void visitUnqualifiedName(UnqualifiedNameAST *node);
  virtual void visitTemplateArgument(TemplateArgumentAST *node);

  void internal_run(AST *node);

private:
  ParseSession* m_session;
  TypeSpecifierAST* m_typeSpecifier;
  QStringList m_name;
  QString m_currentIdentifier;
  QList<Type> m_templateArgs;
  GeneratorVisitor* m_visitor;
};

//Extracts a type-identifier from a template argument
// KDevelop::IndexedTypeIdentifier Q_DECL_EXPORT typeIdentifierFromTemplateArgument(ParseSession* session, TemplateArgumentAST *node);
uint Q_DECL_EXPORT parseConstVolatile(ParseSession* session, const ListNode<std::size_t> *cv);

#endif // NAME_COMPILER_H

