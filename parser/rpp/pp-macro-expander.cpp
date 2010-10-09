/*
  Copyright 2005 Roberto Raggi <roberto@kdevelop.org>
  Copyright 2006 Hamish Rodda <rodda@kde.org>
  Copyright 2007-2009 David Nolden <david.nolden.kdevelop@art-master.de>

  Permission to use, copy, modify, distribute, and sell this software and its
  documentation for any purpose is hereby granted without fee, provided that
  the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  KDEVELOP TEAM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "pp-macro-expander.h"

#include <QDate>
#include <QTime>
#include <QtDebug>

#include "../problem.h"
#include "../indexedstring.h"

#include "pp-internal.h"
#include "pp-engine.h"
#include "pp-environment.h"
#include "pp-location.h"
#include "preprocessor.h"
#include "chartools.h"

const int maxMacroExpansionDepth = 50;

QString joinIndexVector(const uint* arrays, uint size, QString between) {
  QString ret;
  FOREACH_CUSTOM(uint item, arrays, size) {
    if(!ret.isEmpty())
      ret += between;
    ret += IndexedString::fromIndex(item).str();
  }
  return ret;
}

QString joinIndexVector(const QVector<IndexedString>& list, QString between) {
  QString ret;
  foreach(const IndexedString& item, list) {
    if(!ret.isEmpty())
      ret += between;
    ret += item.str();
  }
  return ret;
}

void trim(QVector<uint>& array) {
  int lastValid = array.size()-1;
  for(; lastValid >= 0; --lastValid)
    if(array[lastValid] != indexFromCharacter(' '))
      break;
  
  array.resize(lastValid+1);
  
  int firstValid = 0;
  for(; firstValid < array.size(); ++firstValid)
    if(array[firstValid] != indexFromCharacter(' '))
      break;
  array = array.mid(firstValid);
}

using namespace rpp;              

pp_frame::pp_frame(pp_macro* __expandingMacro, const QList<pp_actual>& __actuals)
  : depth(0)
  , expandingMacro(__expandingMacro)
  , actuals(__actuals)
{
}

pp_actual pp_macro_expander::resolve_formal(IndexedString name, Stream& input)
{
  if (!m_frame)
    return pp_actual();

  Q_ASSERT(m_frame->expandingMacro != 0);

  const QVector<IndexedString>& formals = m_frame->expandingMacro->formals;
  uint formalsSize = formals.size();

  if(name.isEmpty()) {
    Problem *problem = new Problem;
    problem->file = m_engine->currentFileNameString();
    problem->position = input.originalInputPosition();
    problem->description = "Macro error";
    m_engine->problemEncountered(problem);
    return pp_actual();
  }
  
  for (uint index = 0; index < formalsSize; ++index) {
    if (name.index() == formals[index].index()) {
      if (index < (uint)m_frame->actuals.size()) {
        return m_frame->actuals[index];
      }
      else {
        Problem *problem = new Problem;
        problem->file = m_engine->currentFileNameString();
        problem->position = input.originalInputPosition();
        problem->description = QString("Call to macro %1 missing argument number %2").arg(name.str()).arg(index);
        problem->explanation = QString("Formals: %1").arg(joinIndexVector(formals, ", "));
        m_engine->problemEncountered(problem);
      }
    }
  }

  return pp_actual();
}

#define RETURN_IF_INPUT_BROKEN    if(input.atEnd()) { qDebug() << "too early end while expanding" << macro->name.str(); return; }


pp_macro_expander::pp_macro_expander(pp* engine, pp_frame* frame, bool inHeaderSection)
  : m_engine(engine)
  , m_frame(frame)
  , m_in_header_section(inHeaderSection)
  , m_search_significant_content(false)
  , m_found_significant_content(false)
{
  if(m_in_header_section)
    m_search_significant_content = true; //Find the end of the header section
}

//A header-section ends when the first non-directive and non-comment occurs
#define check_header_section \
  if( m_search_significant_content ) \
  { \
     \
    if(m_in_header_section) { \
      m_in_header_section = false; \
      m_engine->preprocessor()->headerSectionEnded(input); \
    } \
    m_found_significant_content = true;  \
    m_search_significant_content = false; \
    if( input.atEnd() ) \
      continue; \
  } \

struct EnableMacroExpansion {
  EnableMacroExpansion(Stream& _input, const SimpleCursor& expansionPosition) : input(_input), hadMacroExpansion(_input.macroExpansion().isValid()) {
    
    if(!hadMacroExpansion)
      _input.setMacroExpansion(expansionPosition);
  }
  ~EnableMacroExpansion() {
    if(!hadMacroExpansion)
      input.setMacroExpansion(SimpleCursor::invalid());
  }
  Stream& input;
  bool hadMacroExpansion;
};

IndexedString definedIndex = IndexedString("defined");
IndexedString lineIndex = IndexedString("__LINE__");
IndexedString fileIndex = IndexedString("__FILE__");
IndexedString dateIndex = IndexedString("__DATE__");
IndexedString timeIndex= IndexedString("__TIME__");

void pp_macro_expander::operator()(Stream& input, Stream& output)
{
  skip_blanks(input, output);

  while (!input.atEnd())
  {
    if (isComment(input))
    {
      skip_comment_or_divop(input, output, true);
    }else{
      if (input == '\n')
      {
        output << input;

        skip_blanks(++input, output);

        if (!input.atEnd() && input == '#')
          break;
      }
      else if (input == '#')
      {
        Q_ASSERT(isCharacter(input.current()));
        Q_ASSERT(IndexedString::fromIndex(input.current()).str() == "#");
        
        ++input;
        
        // search for the paste token
        if(input == '#') {
          ++input;
          skip_blanks (input, devnull());
          
          IndexedString previous = IndexedString::fromIndex(output.popLastOutput()); //Previous already has been expanded
          while(output.offset() > 0 && isCharacter(previous.index()) && characterFromIndex(previous.index()) == ' ')
            previous = IndexedString::fromIndex(output.popLastOutput());   
          
          IndexedString add = IndexedString::fromIndex(skip_identifier (input));
          
          PreprocessedContents newExpanded;
          
          {
            //Expand "add", so it is eventually replaced by an actual
            PreprocessedContents actualText;
            actualText.append(add.index());

            {
              Stream as(&actualText);
              pp_macro_expander expand_actual(m_engine, m_frame);
              Stream nas(&newExpanded);
              expand_actual(as, nas);
            }
          }
          
          if(!newExpanded.isEmpty()) {
            IndexedString first = IndexedString::fromIndex(newExpanded.first());
            if(!isCharacter(first.index()) || QChar(characterFromIndex(first.index())).isLetterOrNumber() || characterFromIndex(first.index()) == '_') {
              //Merge the tokens
              newExpanded.first() = IndexedString(previous.byteArray() + first.byteArray()).index();
            }else{
              //Cannot merge, prepend the previous text
              newExpanded.prepend(previous.index());
            }
          }else{
            newExpanded.append(previous.index());
          }
          
          //Now expand the merged text completely into the output.
          pp_macro_expander final_expansion(m_engine, m_frame);
          Stream nas(&newExpanded, output.currentOutputAnchor());
          final_expansion(nas, output);
          continue;
        }
        
        skip_blanks(input, output);

        IndexedString identifier = IndexedString::fromIndex( skip_identifier(input) );

        Anchor inputPosition = input.inputPosition();
        SimpleCursor originalInputPosition = input.originalInputPosition();
        PreprocessedContents formal = resolve_formal(identifier, input).mergeText();
        
        //Escape so we don't break on '"'
        for(int a = formal.count()-1; a >= 0; --a) {
          if(formal[a] == indexFromCharacter('\"') || formal[a] == indexFromCharacter('\\'))
            formal.insert(a, indexFromCharacter('\\'));
            else if(formal[a] == indexFromCharacter('\n')) {
              //Replace newlines with "\n"
              formal[a] = indexFromCharacter('n');
              formal.insert(a, indexFromCharacter('\\'));
            }
              
        }

        Stream is(&formal, inputPosition);
        is.setOriginalInputPosition(originalInputPosition);
        skip_whitespaces(is, devnull());

        output << '\"';

        while (!is.atEnd()) {
          if (is == '"') {
            output << '\\' << is;

          } else if (is == '\n') {
            output << '"' << is << '"';

          } else {
            output << is;
          }

          skip_whitespaces(++is, output);
        }

        output << '\"';
      }
      else if (input == '\"')
      {
        check_header_section
        
        skip_string_literal(input, output);
      }
      else if (input == '\'')
      {
        check_header_section
        
        skip_char_literal(input, output);
      }
      else if (isSpace(input.current()))
      {
        do {
          if (input == '\n' || !isSpace(input.current()))
            break;

          output << input;

        } while (!(++input).atEnd());
      }
      else if (isNumber(input.current()))
      {
        check_header_section
        
        skip_number (input, output);
      }
      else if (isLetter(input.current()) || input == '_' || !isCharacter(input.current()))
      {
        check_header_section
        
        Anchor inputPosition = input.inputPosition();
        IndexedString name = IndexedString::fromIndex(skip_identifier (input));
        
        Anchor inputPosition2 = input.inputPosition();
        pp_actual actual = resolve_formal(name, input);
        if (actual.isValid()) {
          Q_ASSERT(actual.text.size() == actual.inputPosition.size());
          
          QList<PreprocessedContents>::const_iterator textIt = actual.text.constBegin();
          QList<Anchor>::const_iterator cursorIt = actual.inputPosition.constBegin();

          for( ; textIt != actual.text.constEnd(); ++textIt, ++cursorIt )
          {
            output.appendString(*cursorIt, *textIt);
          }
          output << ' '; //Insert a whitespace to omit implicit token merging
          output.mark(input.inputPosition());
          
          if(actual.text.isEmpty()) {
            int start = input.offset();
            
            skip_blanks(input, devnull());
            //Omit paste tokens behind empty used actuals, else we will merge with the previous text
            if(!input.atEnd() && input == '#' && !(++input).atEnd() && input == '#') {
              ++input;
              //We have skipped a paste token
            }else{
              input.seek(start);
            }
          }
          
          continue;
        }

        // TODO handle inbuilt "defined" etc functions

        pp_macro* macro = m_engine->environment()->retrieveMacro(name, false);
        
        if (!macro || !macro->defined || macro->hidden || macro->function_like || m_engine->hideNextMacro())
        {
          m_engine->setHideNextMacro(name == definedIndex);

          if (name == lineIndex)
            output.appendString(inputPosition, convertFromByteArray(QString::number(input.inputPosition().line).toUtf8()));
          else if (name == fileIndex)
            output.appendString(inputPosition, convertFromByteArray(QString("\"%1\"").arg(m_engine->currentFileNameString()).toUtf8()));
          else if (name == dateIndex)
            output.appendString(inputPosition, convertFromByteArray(QDate::currentDate().toString("MMM dd yyyy").toUtf8()));
          else if (name == timeIndex)
            output.appendString(inputPosition, convertFromByteArray(QTime::currentTime().toString("hh:mm:ss").toUtf8()));
          else
            output.appendString(inputPosition, name);
          continue;
        }
        
        EnableMacroExpansion enable(output, input.inputPosition()); //Configure the output-stream so it marks all stored input-positions as transformed through a macro

          if (macro->definition.size()) {
            macro->hidden = true;

            pp_macro_expander expand_macro(m_engine);
            ///@todo UGLY conversion
            Stream ms((uint*)macro->definition.constData(), macro->definition.size(), Anchor(input.inputPosition(), true));
            ms.setOriginalInputPosition(input.originalInputPosition());
            PreprocessedContents expanded;
            {
              Stream es(&expanded);
              expand_macro(ms, es);
            }

            if (!expanded.isEmpty())
            {
              Stream es(&expanded, Anchor(input.inputPosition(), true));
              es.setOriginalInputPosition(input.originalInputPosition());
              skip_whitespaces(es, devnull());
              IndexedString identifier = IndexedString::fromIndex( skip_identifier(es) );

              output.appendString(Anchor(input.inputPosition(), true), expanded);
              output << ' '; //Prevent implicit token merging
            }

            macro->hidden = false;
          }
        }else if(input == '(') {

        //Eventually execute a function-macro
          
        IndexedString previous = IndexedString::fromIndex(indexFromCharacter(' ')); //Previous already has been expanded
        uint stepsBack = 0;
        while(isCharacter(previous.index()) && characterFromIndex(previous.index()) == ' ' && output.peekLastOutput(stepsBack)) {
          previous = IndexedString::fromIndex(output.peekLastOutput(stepsBack));
          ++stepsBack;
        }
        
        pp_macro* macro = m_engine->environment()->retrieveMacro(previous, false);
        
        if(!macro || !macro->function_like || !macro->defined || macro->hidden) {
          output << input;
          ++input;
          continue;
        }
        
        //In case expansion fails, we can skip back to this position
        int openingPosition = input.offset();
        Anchor openingPositionCursor = input.inputPosition();
        
        QList<pp_actual> actuals;
        ++input; // skip '('
        
        RETURN_IF_INPUT_BROKEN

        pp_macro_expander expand_actual(m_engine, m_frame);

        {
          PreprocessedContents actualText;
          skip_whitespaces(input, devnull());
          Anchor actualStart = input.inputPosition();
          {
            Stream as(&actualText);
            skip_argument_variadics(actuals, macro, input, as);
          }
          trim(actualText);

          pp_actual newActual;
          {
            PreprocessedContents newActualText;
            Stream as(&actualText, actualStart);
            as.setOriginalInputPosition(input.originalInputPosition());

            rpp::LocationTable table;
            table.anchor(0, actualStart, 0);
            Stream nas(&newActualText, actualStart, &table);
            expand_actual(as, nas);
            
            table.splitByAnchors(newActualText, actualStart, newActual.text, newActual.inputPosition);
          }
          newActual.forceValid = true;
          
          actuals.append(newActual);
        }

        // TODO: why separate from the above?
        while (!input.atEnd() && input == ',')
        {
          ++input; // skip ','
          
          RETURN_IF_INPUT_BROKEN

          {
            PreprocessedContents actualText;
            skip_whitespaces(input, devnull());
            Anchor actualStart = input.inputPosition();
            {
              Stream as(&actualText);
              skip_argument_variadics(actuals, macro, input, as);
            }
            trim(actualText);

            pp_actual newActual;
            {
              PreprocessedContents newActualText;
              Stream as(&actualText, actualStart);
              as.setOriginalInputPosition(input.originalInputPosition());

              PreprocessedContents actualText;
              rpp::LocationTable table;
              table.anchor(0, actualStart, 0);
              Stream nas(&newActualText, actualStart, &table);
              expand_actual(as, nas);
              
              table.splitByAnchors(newActualText, actualStart, newActual.text, newActual.inputPosition);
            }
            newActual.forceValid = true;
            actuals.append(newActual);
          }
        }

        if( input != ')' ) {
          //Failed to expand the macro. Output the macro name and continue normal
          //processing behind it.(Code completion depends on this behavior when expanding
          //incomplete input-lines)
          input.seek(openingPosition);
          input.setInputPosition(openingPositionCursor);
          //Move one character into the output, so we don't get an endless loop
          output << input;
          ++input;
          
          continue;
        }
        
        //Remove the name of the called macro
        while(stepsBack) {
          --stepsBack;
          output.popLastOutput();
        }
        
        //Q_ASSERT(!input.atEnd() && input == ')');

        ++input; // skip ')'
        
#if 0 // ### enable me
        assert ((macro->variadics && macro->formals.size () >= actuals.size ())
                    || macro->formals.size() == actuals.size());
#endif
        EnableMacroExpansion enable(output, input.inputPosition()); //Configure the output-stream so it marks all stored input-positions as transformed through a macro

        pp_frame frame(macro, actuals);
        if(m_frame)
          frame.depth = m_frame->depth + 1;
        
        if(frame.depth >= maxMacroExpansionDepth) 
        {
          qDebug() << "reached maximum macro-expansion depth while expanding" << macro->name.str();
          RETURN_IF_INPUT_BROKEN
          
          output << input;
          ++input;
        }else{
          pp_macro_expander expand_macro(m_engine, &frame);
          macro->hidden = true;
          ///@todo UGLY conversion
          Stream ms((uint*)macro->definition.constData(), macro->definition.size(), Anchor(input.inputPosition(), true));
          ms.setOriginalInputPosition(input.originalInputPosition());
          expand_macro(ms, output);
          output << ' '; //Prevent implicit token merging
          macro->hidden = false;
        }
      } else {
        output << input;
        ++input;
      }
    }

  }
}

void pp_macro_expander::skip_argument_variadics (const QList<pp_actual>& __actuals, pp_macro *__macro, Stream& input, Stream& output)
{
  int first;

  do {
    first = input.offset();
    skip_argument(input, output);

  } while ( __macro->variadics
            && first != input.offset()
            && !input.atEnd()
            && input == '.'
            && (__actuals.size() + 1) == (int)__macro->formals.size());
}

