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

//krazy:excludeall=cpp

#include "name_compiler.h"
#include "type_compiler.h"
#include <lexer.h>
#include <symbol.h>
#include <parsesession.h>
#include <tokens.h>
#include "type_compiler.h"

#include <QtCore/qdebug.h>

///@todo this is very expensive
QString decode(ParseSession* session, AST* ast, bool without_spaces = false)
{
  QString ret;
  if( without_spaces ) {
    //Decode operator-names without spaces for now, since we rely on it in other places.
    ///@todo change this, here and in all the places that rely on it. Operators should then by written like "operator [ ]"(space between each token)
    for( size_t a = ast->start_token; a < ast->end_token; a++ ) {
      ret += session->token_stream->token(a).symbolString();
    }
  } else {
    for( size_t a = ast->start_token; a < ast->end_token; a++ ) {
      ret += session->token_stream->token(a).symbolString() + ' ';
    }
  }
  return ret;
}

/*uint parseConstVolatile(ParseSession* session, const ListNode<std::size_t> *cv)
{
  uint ret = AbstractType::NoModifiers;

  if (cv) {
    const ListNode<std::size_t> *it = cv->toFront();
    const ListNode<std::size_t> *end = it;
    do {
      int kind = session->token_stream->kind(it->element);
      if (kind == Token_const)
        ret |= AbstractType::ConstModifier;
      else if (kind == Token_volatile)
        ret |= AbstractType::VolatileModifier;

      it = it->next;
    } while (it != end);
  }

  return ret;
}*/

/*IndexedTypeIdentifier typeIdentifierFromTemplateArgument(ParseSession* session, TemplateArgumentAST *node)
{
  IndexedTypeIdentifier id;
  if(node->expression) {
    id = IndexedTypeIdentifier(decode(session, node), true);
  }else if(node->type_id) {
    //Parse the pointer operators
    TypeCompiler tc(session);
    tc.run(node->type_id->type_specifier);
    id = IndexedTypeIdentifier(tc.identifier());
    //node->type_id->type_specifier->cv
    
    if(node->type_id->type_specifier)
      id.setIsConstant(parseConstVolatile(session, node->type_id->type_specifier->cv) & AbstractType::ConstModifier);
    
    if(node->type_id->declarator && node->type_id->declarator->ptr_ops)
    {
      const ListNode<PtrOperatorAST*> *it = node->type_id->declarator->ptr_ops->toFront();
      const ListNode<PtrOperatorAST*> *end = it; ///@todo check ordering, eventually walk the list in reversed order
      do
        {
          if(it->element && it->element->op) { ///@todo What about ptr-to-member?
            static IndexedString ref('&');
            if( session->token_stream->token(it->element->op).symbol() == ref) {
              //We're handling a 'reference'
              id.setIsReference(true);
            } else {
              //We're handling a real pointer
              id.setPointerDepth(id.pointerDepth()+1);

              if(it->element->cv) {
                id.setIsConstPointer(id.pointerDepth()-1, parseConstVolatile(session, it->element->cv) & AbstractType::ConstModifier);
              }
            }
          }
          it = it->next;
        }
      while (it != end);
    }
  }
  return id;
}*/

NameCompiler::NameCompiler(ParseSession* session, GeneratorVisitor* visitor)
  : m_session(session), m_visitor(visitor)
{
}

void NameCompiler::internal_run(AST *node)
{
  m_name.clear();
  m_castType = Type();
  m_templateArgs.clear();
  visit(node);
}

void NameCompiler::visitUnqualifiedName(UnqualifiedNameAST *node)
{
  IndexedString tmp_name;

  if (node->id)
    tmp_name = m_session->token_stream->token(node->id).symbol();

  if (node->tilde)
    tmp_name = IndexedString('~' + tmp_name.byteArray());

  if (OperatorFunctionIdAST *op_id = node->operator_id)
    {
#if defined(__GNUC__)
#warning "NameCompiler::visitUnqualifiedName() -- implement me"
#endif
      static QString operatorString("operator");
      QString tmp = operatorString;

      if (op_id->op && op_id->op->op) {
        tmp +=  decode(m_session, op_id->op, true);
      } else {
        TypeCompiler tc(m_session, m_visitor);
        tc.run(op_id->type_specifier);
        tc.run(op_id->ptr_ops);
        m_castType = tc.type();
        tmp += " " + tc.type().toString();
      }

      tmp_name = IndexedString(tmp);
      m_typeSpecifier = op_id->type_specifier;
    }

  m_currentIdentifier = tmp_name.str();

  if (node->template_arguments)
    {
      visitNodes(this, node->template_arguments);
    }/*else if(node->end_token == node->start_token + 3 && node->id == node->start_token && m_session->token_stream->token(node->id+1).symbol() == KDevelop::IndexedString('<')) {
      ///@todo Represent this nicer in the AST
      ///It's probably a type-specifier with instantiation of the default-parameter, like "Bla<>".
      m_currentIdentifier.appendTemplateIdentifier( IndexedTypeIdentifier() );
    }*/

  m_name.push_back(m_currentIdentifier);
}

TypeSpecifierAST* NameCompiler::lastTypeSpecifier() const {
  return m_typeSpecifier;
}

void NameCompiler::visitTemplateArgument(TemplateArgumentAST *node)
{
    if (!node->type_id) {
        QString ret;
        for (int i = node->expression->start_token; i < node->expression->end_token; i++) {
            ret.append(m_session->token_stream->token(i).symbolString());
        }
        // TODO: Better use a new struct here - expressions aren't really types.
        m_templateArgs[m_name.count()] << Type(ret);
        return;
    }
    TypeCompiler tc(m_session, m_visitor);
    tc.run(node->type_id->type_specifier, node->type_id->declarator ? node->type_id->declarator : 0);
    m_templateArgs[m_name.count()] << tc.type();
}

/*const QualifiedIdentifier& NameCompiler::identifier() const
{
  return *_M_name;
}*/

void NameCompiler::run(NameAST *node/*, QualifiedIdentifier* target*/)
{
//   if(target)
//     _M_name = target;
//   else
//     _M_name = &m_localName;
    
  m_typeSpecifier = 0; internal_run(node); /*if(node && node->global) _M_name->setExplicitlyGlobal( node->global );*/
}
