/* This file is part of KDevelop
    Copyright 2002-2005 Roberto Raggi <roberto@kdevelop.org>
    Copyright 2007-2008 David Nolden <david.nolden.kdevelop@art-master.de>

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

#include "lexer.h"
#include "tokens.h"
#include "control.h"
#include "parsesession.h"
#include "rpp/pp-scanner.h"

#include <cctype>
#include "kdevvarlengtharray.h"

/**
 * Returns the character BEHIND the found comment
 * */
void Lexer::skipComment()
{
  ///A nearly exact copy of rpp::pp_skip_comment_or_divop::operator()
  enum {
    MAYBE_BEGIN,
    BEGIN,
    MAYBE_END,
    END,
    IN_COMMENT,
    IN_CXX_COMMENT
  } state (MAYBE_BEGIN);

  while (cursor < endCursor && *cursor) {
    switch (state) {
      case MAYBE_BEGIN:
        if (*cursor != '/')
          return;

        state = BEGIN;
        break;

      case BEGIN:
        if (*cursor == '*')
          state = IN_COMMENT;
        else if (*cursor == '/')
          state = IN_CXX_COMMENT;
        else
          return;
        break;

      case IN_COMMENT:
        if( *cursor == '\n' ) {
          scan_newline();
          continue;
        }
        if (*cursor == '*')
          state = MAYBE_END;
        break;

      case IN_CXX_COMMENT:
        if (*cursor == '\n')
          return;
        break;

      case MAYBE_END:
        if (*cursor == '/')
          state = END;
        else if (*cursor != '*')
          state = IN_COMMENT;
        if( *cursor == '\n' ) {
          scan_newline();
          continue;
        }
        break;

      case END:
        return;
    }

    ++cursor;
  }
  return;
}

IndexedString Token::symbol() const {
  if(size == 1)
    return IndexedString::fromIndex(session->contents()[position]);
  else
    return IndexedString();
}

QByteArray Token::symbolByteArray() const {
  return stringFromContents(session->contentsVector(), position, size);
}

QString Token::symbolString() const {
  return QString::fromUtf8(stringFromContents(session->contentsVector(), position, size));
}

uint Token::symbolLength() const {
  uint ret = 0;
  for(uint a = position; a < position+size; ++a) {
    ret += IndexedString::fromIndex(session->contents()[a]).length();
  }
  return ret;
}

const uint index_size = 200;

KDevVarLengthArray<KDevVarLengthArray<QPair<uint, TOKEN_KIND>, 10 >, index_size > createIndicesForTokens() {
  KDevVarLengthArray<KDevVarLengthArray<QPair<uint, TOKEN_KIND>, 10 >, index_size > ret;
  ret.resize(index_size);
  #define ADD_TOKEN(string) ret[IndexedString(#string).index() % index_size].append(qMakePair(IndexedString(#string).index(), Token_ ## string));
  #define ADD_MAPPED_TOKEN(string, token) ret[IndexedString(#string).index() % index_size].append(qMakePair(IndexedString(#string).index(), token));
  ADD_TOKEN(K_DCOP);
  ADD_TOKEN(Q_OBJECT);
  ADD_MAPPED_TOKEN(Q_SIGNALS, Token_signals);
  ADD_MAPPED_TOKEN(Q_SLOTS, Token_slots);
  ADD_TOKEN(__attribute__);
  ADD_TOKEN(__typeof);
  ADD_TOKEN(and);
  ADD_TOKEN(and_eq);
  ADD_TOKEN(asm);
  ADD_TOKEN(auto);
  ADD_TOKEN(bitand);
  ADD_TOKEN(bitor);
  ADD_TOKEN(bool);
  ADD_TOKEN(break);
  ADD_TOKEN(case);
  ADD_TOKEN(catch);
  ADD_TOKEN(char);
  ADD_TOKEN(class);
  ADD_TOKEN(compl);
  ADD_TOKEN(const);
  ADD_TOKEN(const_cast);
  ADD_TOKEN(continue);
  ADD_TOKEN(default);
  ADD_TOKEN(delete);
  ADD_TOKEN(do);
  ADD_TOKEN(double);
  ADD_TOKEN(dynamic_cast);
  ADD_TOKEN(else);
  ADD_TOKEN(emit);
  ADD_TOKEN(enum);
  ADD_TOKEN(explicit);
  ADD_TOKEN(export);
  ADD_TOKEN(extern);
  ADD_TOKEN(false);
  ADD_TOKEN(float);
  ADD_TOKEN(for);
  ADD_TOKEN(friend);
  ADD_TOKEN(goto);
  ADD_TOKEN(if);
  ADD_TOKEN(incr);
  ADD_TOKEN(inline);
  ADD_TOKEN(int);
  ADD_TOKEN(k_dcop);
  ADD_TOKEN(k_dcop_signals);
  ADD_TOKEN(long);
  ADD_TOKEN(mutable);
  ADD_TOKEN(namespace);
  ADD_TOKEN(new);
  ADD_TOKEN(not);
  ADD_TOKEN(not_eq);
  ADD_TOKEN(operator);
  ADD_TOKEN(or);
  ADD_TOKEN(or_eq);
  ADD_TOKEN(private);
  ADD_TOKEN(protected);
  ADD_TOKEN(public);
  ADD_TOKEN(register);
  ADD_TOKEN(reinterpret_cast);
  ADD_TOKEN(return);
  ADD_TOKEN(short);
  ADD_TOKEN(signals);
  ADD_TOKEN(signed);
  ADD_TOKEN(sizeof);
  ADD_TOKEN(slots);
  ADD_TOKEN(static);
  ADD_TOKEN(static_cast);
  ADD_TOKEN(struct);
  ADD_TOKEN(switch);
  ADD_TOKEN(template);
  ADD_TOKEN(this);
  ADD_TOKEN(throw);
  ADD_TOKEN(true);
  ADD_TOKEN(try);
  ADD_TOKEN(typedef);
  ADD_TOKEN(typeid);
  ADD_TOKEN(typename);
  ADD_TOKEN(union);
  ADD_TOKEN(unsigned);
  ADD_TOKEN(using);
  ADD_TOKEN(virtual);
  ADD_TOKEN(void);
  ADD_TOKEN(volatile);
  ADD_TOKEN(size_t);
  ADD_TOKEN(wchar_t);
  ADD_TOKEN(while);
  ADD_TOKEN(xor);
  ADD_TOKEN(xor_eq);
  ADD_TOKEN(__qt_sig_slot__);
  return ret;
}

//A very simple lookup table: First level contains all pairs grouped by with (index % index_size), then there is a simple list
KDevVarLengthArray<KDevVarLengthArray<QPair<uint, TOKEN_KIND>, 10 >, index_size > indicesForTokens = createIndicesForTokens();

scan_fun_ptr Lexer::s_scan_table[256];
bool Lexer::s_initialized = false;

Lexer::Lexer(Control *c)
  : session(0),
    control(c),
    m_leaveSize(false)
{
}

void Lexer::tokenize(ParseSession* _session)
{
  session = _session;

  if (!s_initialized)
    initialize_scan_table();

  m_canMergeComment = false;
  m_firstInLine = true;
  m_leaveSize = false;
  
  session->token_stream->resize(1024);
  (*session->token_stream)[0].kind = Token_EOF;
  (*session->token_stream)[0].session = session;
  (*session->token_stream)[0].position = 0;
  (*session->token_stream)[0].size = 0;
  index = 1;

  cursor.current = session->contents();
  endCursor = session->contents() + session->contentsVector().size();

  while (cursor < endCursor) {
    size_t previousIndex = index;
    
    if (index == session->token_stream->size())
      session->token_stream->resize(session->token_stream->size() * 2);

    Token *current_token = &(*session->token_stream)[index];
    current_token->session = session;
    current_token->position = cursor.offsetIn( session->contents() );
    current_token->size = 0;
    
    if(cursor.isChar()) {
      (this->*s_scan_table[((uchar)*cursor)])();
    }else{
      //The cursor represents an identifier
      scan_identifier_or_keyword();
    }
    
    if(!m_leaveSize)
      current_token->size = cursor.offsetIn( session->contents() ) - current_token->position;
    
    Q_ASSERT(m_leaveSize || (cursor.current == session->contents() + current_token->position + current_token->size));
    Q_ASSERT(current_token->position + current_token->size <= (uint)session->contentsVector().size());
    Q_ASSERT(previousIndex == index-1 || previousIndex == index); //Never parse more than 1 token, because that won't be initialized correctly

    m_leaveSize = false;
    
    if(previousIndex != index)
      m_firstInLine = false;
    
  }

    if (index == session->token_stream->size())
      session->token_stream->resize(session->token_stream->size() * 2);
  (*session->token_stream)[index].session = session;
  (*session->token_stream)[index].position = cursor.offsetIn(session->contents());
  (*session->token_stream)[index].size = 0;
  (*session->token_stream)[index].kind = Token_EOF;
}

void Lexer::initialize_scan_table()
{
  s_initialized = true;

  for (int i=0; i<256; ++i)
    {
      if (isspace(i))
	s_scan_table[i] = &Lexer::scan_white_spaces;
      else if (isalpha(i) || i == '_')
	s_scan_table[i] = &Lexer::scan_identifier_or_keyword;
      else if (isdigit(i))
	s_scan_table[i] = &Lexer::scan_int_constant;
      else
	s_scan_table[i] = &Lexer::scan_invalid_input;
    }

  s_scan_table[int('L')] = &Lexer::scan_identifier_or_literal;
  s_scan_table[int('\n')] = &Lexer::scan_newline;
  s_scan_table[int('#')] = &Lexer::scan_preprocessor;

  s_scan_table[int('\'')] = &Lexer::scan_char_constant;
  s_scan_table[int('"')]  = &Lexer::scan_string_constant;

  s_scan_table[int('.')] = &Lexer::scan_int_constant;

  s_scan_table[int('!')] = &Lexer::scan_not;
  s_scan_table[int('%')] = &Lexer::scan_remainder;
  s_scan_table[int('&')] = &Lexer::scan_and;
  s_scan_table[int('(')] = &Lexer::scan_left_paren;
  s_scan_table[int(')')] = &Lexer::scan_right_paren;
  s_scan_table[int('*')] = &Lexer::scan_star;
  s_scan_table[int('+')] = &Lexer::scan_plus;
  s_scan_table[int(',')] = &Lexer::scan_comma;
  s_scan_table[int('-')] = &Lexer::scan_minus;
  s_scan_table[int('/')] = &Lexer::scan_divide;
  s_scan_table[int(':')] = &Lexer::scan_colon;
  s_scan_table[int(';')] = &Lexer::scan_semicolon;
  s_scan_table[int('<')] = &Lexer::scan_less;
  s_scan_table[int('=')] = &Lexer::scan_equal;
  s_scan_table[int('>')] = &Lexer::scan_greater;
  s_scan_table[int('?')] = &Lexer::scan_question;
  s_scan_table[int('[')] = &Lexer::scan_left_bracket;
  s_scan_table[int(']')] = &Lexer::scan_right_bracket;
  s_scan_table[int('^')] = &Lexer::scan_xor;
  s_scan_table[int('{')] = &Lexer::scan_left_brace;
  s_scan_table[int('|')] = &Lexer::scan_or;
  s_scan_table[int('}')] = &Lexer::scan_right_brace;
  s_scan_table[int('~')] = &Lexer::scan_tilde;

  s_scan_table[0] = &Lexer::scan_EOF;
}

void Lexer::scan_preprocessor()
{
  while (cursor != endCursor && *cursor && *cursor != '\n')
    ++cursor;

  if (*cursor != '\n')
    {
      Problem *p = createProblem();
      p->description = "expected end of line";
      control->reportProblem(p);
    }
}

void Lexer::scan_char_constant()
{
  //const char *begin = cursor;

  ++cursor;
  while (cursor != endCursor && *cursor && *cursor != '\'')
    {
       if (*cursor == '\n')
        {
          Problem *p = createProblem();
          p->description = "unexpected new line";
          control->reportProblem(p);
          break;
        }

      if (*cursor == '\\')
	++cursor;

      ++cursor;
    }

  if (*cursor != '\'')
    {
      Problem *p = createProblem();
      p->description = "expected '";
      control->reportProblem(p);
    }
  else
    {
      ++cursor;
    }

  //(*session->token_stream)[index].extra.symbol =
    //control->findOrInsertName((const char*) begin, cursor - begin);

  (*session->token_stream)[index++].kind = Token_char_literal;
}

void Lexer::scan_string_constant()
{
  //const char *begin = cursor;

  ++cursor;
  while (cursor != endCursor && *cursor && *cursor != '"')
    {
       if (*cursor == '\n')
        {
          Problem *p = createProblem();
          p->description = "unexpected new line";
          control->reportProblem(p);
          break;
        }

      if (*cursor == '\\')
	++cursor;

      ++cursor;
    }

  if (*cursor != '"')
    {
      Problem *p = createProblem();
      p->description = "expected \"";
      control->reportProblem(p);
    }
  else
    {
      ++cursor;
    }

  //(*session->token_stream)[index].extra.symbol =
    //control->findOrInsertName((const char*) begin, cursor - begin);

  (*session->token_stream)[index++].kind = Token_string_literal;
}

void Lexer::scan_newline()
{
  ++cursor;
  m_firstInLine = true;
}

void Lexer::scan_white_spaces()
{
  while (cursor != endCursor && isspace(*cursor))
    {
      if (*cursor == '\n')
	scan_newline();
      else
	++cursor;
    }
}

void Lexer::scan_identifier_or_literal()
{
  switch (*(cursor + 1))
    {
    case '\'':
      ++cursor;
      scan_char_constant();
      break;

    case '\"':
      ++cursor;
      scan_string_constant();
      break;

    default:
      scan_identifier_or_keyword();
      break;
    }
}

void Lexer::scan_identifier_or_keyword()
{
  if(!(cursor < endCursor))
    return;
  
  //We have to merge symbols tokenized separately, they may have been contracted using ##
  SpecialCursor nextCursor(cursor);
  ++nextCursor;
  
  while(nextCursor < endCursor && (!isCharacter(*(nextCursor.current)) || isLetterOrNumber(*nextCursor.current) || characterFromIndex(*nextCursor.current) == '_')) {
    //Fortunately this shouldn't happen too often, only when ## is used within the preprocessor
    IndexedString mergedSymbol(IndexedString::fromIndex(*(cursor.current)).byteArray() + IndexedString::fromIndex(*(nextCursor.current)).byteArray());
    
    (*cursor.current) = mergedSymbol.index();
    (*nextCursor.current) = 0;
    ++nextCursor;
  }
  
  uint bucket = (*cursor.current) % index_size;
  for(int a = 0; a < indicesForTokens[bucket].size(); ++a) {
    if(indicesForTokens[bucket][a].first == *cursor.current) {
      (*session->token_stream)[index++].kind = indicesForTokens[bucket][a].second;
      ++cursor;
      return;
    }
  }

  m_leaveSize = true; //Since we may have skipped input tokens while mergin, we have to make sure that the size stays 1(the merged tokens will be empty)
  (*session->token_stream)[index].size = 1;
  (*session->token_stream)[index++].kind = Token_identifier;
  
  cursor = nextCursor;
}

void Lexer::scan_int_constant()
{
  if (*cursor == '.' && !std::isdigit(*(cursor + 1)))
    {
      scan_dot();
      return;
    }

  //const char *begin = cursor;

  while (cursor != endCursor &&  (isalnum(*cursor) || *cursor == '.'))
    ++cursor;

  //(*session->token_stream)[index].extra.symbol =
    //control->findOrInsertName((const char*) begin, cursor - begin);

  (*session->token_stream)[index++].kind = Token_number_literal;
}

void Lexer::scan_not()
{
  /*
    '!'		::= not
    '!='		::= not_equal
  */

  ++cursor;

  if (*cursor == '=')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_not_eq;
    }
  else
    {
      (*session->token_stream)[index++].kind = '!';
    }
}

void Lexer::scan_remainder()
{
  /*
    '%'		::= remainder
    '%='		::= remainder_equal
  */

  ++cursor;

  if (*cursor == '=')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_assign;
    }
  else
    {
      (*session->token_stream)[index++].kind = '%';
    }
}

void Lexer::scan_and()
{
  /*
    '&&'		::= and_and
    '&'		::= and
    '&='		::= and_equal
  */

  ++cursor;
  if (*cursor == '=')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_assign;
    }
  else if (*cursor == '&')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_and;
    }
  else
    {
      (*session->token_stream)[index++].kind = '&';
    }
}

void Lexer::scan_left_paren()
{
  ++cursor;
  (*session->token_stream)[index++].kind = '(';
}

void Lexer::scan_right_paren()
{
  ++cursor;
  (*session->token_stream)[index++].kind = ')';
}

void Lexer::scan_star()
{
  /*
    '*'		::= star
    '*='		::= star_equal
  */

  ++cursor;

  if (*cursor == '=')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_assign;
    }
  else
    {
      (*session->token_stream)[index++].kind = '*';
    }
}

void Lexer::scan_plus()
{
  /*
    '+'		::= plus
    '++'		::= incr
    '+='		::= plus_equal
  */

  ++cursor;
  if (*cursor == '=')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_assign;
    }
  else if (*cursor == '+')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_incr;
    }
  else
    {
      (*session->token_stream)[index++].kind = '+';
    }
}

void Lexer::scan_comma()
{
  ++cursor;
  (*session->token_stream)[index++].kind = ',';
}

void Lexer::scan_minus()
{
  /*
    '-'		::= minus
    '--'		::= decr
    '-='		::= minus_equal
    '->'		::= left_arrow
  */

  ++cursor;
  if (*cursor == '=')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_assign;
    }
  else if (*cursor == '-')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_decr;
    }
  else if (*cursor == '>')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_arrow;
    }
  else
    {
      (*session->token_stream)[index++].kind = '-';
    }
}

void Lexer::scan_dot()
{
  /*
    '.'		::= dot
    '...'		::= ellipsis
  */

  ++cursor;
  if (*cursor == '.' && *(cursor + 1) == '.')
    {
      cursor += 2;
      (*session->token_stream)[index++].kind = Token_ellipsis;
    }
  else if (*cursor == '.' && *(cursor + 1) == '*')
    {
      cursor += 2;
      (*session->token_stream)[index++].kind = Token_ptrmem;
    }
  else
    (*session->token_stream)[index++].kind = '.';
}

void Lexer::scan_divide()
{
  /*
    '/'		::= divide
    '/='	::= divide_equal
  */

  ++cursor;

  if (*cursor == '=')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_assign;
    }
  else if( *cursor == '*' || *cursor == '/' )
    {
      ///It is a comment
      --cursor; //Move back to the '/'
      SpecialCursor commentBegin = cursor;
      skipComment();
      if( cursor != commentBegin ) {
        ///Store the comment
        if(!m_canMergeComment || (*session->token_stream)[index-1].kind != Token_comment) {

          //Only allow appending to comments that are behind a newline, because else they may belong to the item on their left side.
          //If index is 1, this comment is the first token, which should be the translation-unit comment. So do not merge following comments.
          if(m_firstInLine && index != 1) 
            m_canMergeComment = true;
          else
            m_canMergeComment = false;
          
          (*session->token_stream)[index++].kind = Token_comment;
          (*session->token_stream)[index-1].size = (size_t)(cursor - commentBegin);
          (*session->token_stream)[index-1].position = commentBegin.offsetIn( session->contents() );
          (*session->token_stream)[index-1].session = session;
        }else{
          //Merge with previous comment
          (*session->token_stream)[index-1].size = cursor.offsetIn(session->contents()) - (*session->token_stream)[index-1].position;
        }
      }
    }
  else
    {
      (*session->token_stream)[index++].kind = '/';
    }
}

void Lexer::scan_colon()
{
  ++cursor;
  if (*cursor == ':')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_scope;
    }
  else
    {
      (*session->token_stream)[index++].kind = ':';
    }
}

void Lexer::scan_semicolon()
{
  ++cursor;
  (*session->token_stream)[index++].kind = ';';
}

void Lexer::scan_less()
{
  /*
    '<'			::= less
    '<<'		::= left_shift
    '<<='		::= left_shift_equal
    '<='		::= less_equal
  */

  ++cursor;
  if (*cursor == '=')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_leq;
    }
  else if (*cursor == '<')
    {
      ++cursor;
      if (*cursor == '=')
	{
	  ++cursor;
	  (*session->token_stream)[index++].kind = Token_assign;
	}
      else
	{
	  (*session->token_stream)[index++].kind = Token_shift;
	}
    }
  else
    {
      (*session->token_stream)[index++].kind = '<';
    }
}

void Lexer::scan_equal()
{
  /*
    '='			::= equal
    '=='		::= equal_equal
  */
  ++cursor;

  if (*cursor == '=')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_eq;
    }
  else
    {
      (*session->token_stream)[index++].kind = '=';
    }
}

void Lexer::scan_greater()
{
  /*
    '>'			::= greater
    '>='		::= greater_equal
    '>>'		::= right_shift
    '>>='		::= right_shift_equal
  */

  ++cursor;
  if (*cursor == '=')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_geq;
    }
  else if (*cursor == '>')
    {
      ++cursor;
      if (*cursor == '=')
	{
	  ++cursor;
	  (*session->token_stream)[index++].kind = Token_assign;
	}
      else
	{
	  (*session->token_stream)[index++].kind = Token_shift;
	}
    }
  else
    {
      (*session->token_stream)[index++].kind = '>';
    }
}

void Lexer::scan_question()
{
  ++cursor;
  (*session->token_stream)[index++].kind = '?';
}

void Lexer::scan_left_bracket()
{
  ++cursor;
  (*session->token_stream)[index++].kind = '[';
}

void Lexer::scan_right_bracket()
{
  ++cursor;
  (*session->token_stream)[index++].kind = ']';
}

void Lexer::scan_xor()
{
  /*
    '^'			::= xor
    '^='		::= xor_equal
  */
  ++cursor;

  if (*cursor == '=')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_assign;
    }
  else
    {
      (*session->token_stream)[index++].kind = '^';
    }
}

void Lexer::scan_left_brace()
{
  ++cursor;
  (*session->token_stream)[index++].kind = '{';
}

void Lexer::scan_or()
{
  /*
    '|'			::= or
    '|='		::= or_equal
    '||'		::= or_or
  */
  ++cursor;
  if (*cursor == '=')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_assign;
    }
  else if (*cursor == '|')
    {
      ++cursor;
      (*session->token_stream)[index++].kind = Token_or;
    }
  else
    {
    (*session->token_stream)[index++].kind = '|';
  }
}

void Lexer::scan_right_brace()
{
  ++cursor;
  (*session->token_stream)[index++].kind = '}';
}

void Lexer::scan_tilde()
{
  ++cursor;
  (*session->token_stream)[index++].kind = '~';
}

void Lexer::scan_EOF()
{
  ++cursor;
  (*session->token_stream)[index++].kind = Token_EOF;
}

void Lexer::scan_invalid_input()
{
  Problem *p = createProblem();
  p->description = "invalid input: %1", IndexedString::fromIndex(*cursor.current).str();
  control->reportProblem(p);

  ++cursor;
}

Problem *Lexer::createProblem() const
{
  Q_ASSERT(index > 0);

  Problem *p = new Problem;

  p->source = Problem::Source_Lexer;
  p->file = session->url().str();
  p->position = session->positionAt(index - 1);
//   p->setFinalLocation(KDevelop::DocumentRange(session->url().str(), KTextEditor::Range(position.textCursor(), 1)));

  return p;
}

