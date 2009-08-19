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

#include "cppparser_export.h"
#include "default_visitor.h"
// #include <language/duchain/identifier.h>
// #include <language/duchain/declaration.h>

class ParseSession;
class OperatorAST;

class CPPPARSER_EXPORT NameCompiler: protected DefaultVisitor
{
public:
  NameCompiler(ParseSession* session);

  ///@param target If this is nonzero, the identifier will be written to that place without any copying.
  void run(NameAST *node/*, KDevelop::QualifiedIdentifier* target = 0*/);
  void run(UnqualifiedNameAST *node) { m_typeSpecifier = 0; internal_run(node); }

  QString name() const { return m_name.join("::"); }
  const QStringList& qualifiedName() const { return m_name; }
  const QStringList& templateArgs() const { return m_templateArgs; }

//   const KDevelop::QualifiedIdentifier& identifier() const;

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
  QStringList m_templateArgs;
  QString m_currentIdentifier;
//   KDevelop::QualifiedIdentifier m_base;
//   KDevelop::Identifier m_currentIdentifier;
//   KDevelop::QualifiedIdentifier* _M_name;
//   KDevelop::QualifiedIdentifier m_localName;
};

//Extracts a type-identifier from a template argument
// KDevelop::IndexedTypeIdentifier Q_DECL_EXPORT typeIdentifierFromTemplateArgument(ParseSession* session, TemplateArgumentAST *node);
uint CPPPARSER_EXPORT parseConstVolatile(ParseSession* session, const ListNode<std::size_t> *cv);

#endif // NAME_COMPILER_H

