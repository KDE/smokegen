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

#ifndef PP_MACRO_EXPANDER_H
#define PP_MACRO_EXPANDER_H

#include <QtCore/QList>
#include <QtCore/QHash>


#include "pp-macro.h"
#include "pp-stream.h"
#include "pp-scanner.h"
#include "anchor.h"
#include "../simplecursor.h"

namespace KDevelop {
  class IndexedString;
}

namespace rpp {

class pp;

//The value of a preprocessor function-like macro parameter
class pp_actual {
public:
  pp_actual() : forceValid(false) {
  }
  QList<PreprocessedContents> text;
  QList<Anchor> inputPosition; //Each inputPosition marks the beginning of one item in the text list
  bool forceValid;

  bool isValid() const {
    return !text.isEmpty() || forceValid;
  }
  void clear() {
    forceValid = false;
    text.clear();
    inputPosition.clear();
  }

  PreprocessedContents mergeText() const {
    if(text.count() == 1)
      return text.at(0);
    
    PreprocessedContents ret;
    
    foreach(const PreprocessedContents& t, text)
      ret += t;
    return ret;
  }
};

class pp_frame
{
public:
  pp_frame (pp_macro* __expandingMacro, const QList<pp_actual>& __actuals);

  int depth;
  pp_macro* expandingMacro;
  QList<pp_actual> actuals;
};

class pp_macro_expander
{
public:
  explicit pp_macro_expander(pp* engine, pp_frame* frame = 0, bool inHeaderSection = false);

  pp_actual resolve_formal(IndexedString name, Stream& input);

  /// Expands text with the known macros. Continues until it finds a new text line
  /// beginning with #, at which point control is returned.
  void operator()(Stream& input, Stream& output);

  void skip_argument_variadics (const QList<pp_actual>& __actuals, pp_macro *__macro,
                                Stream& input, Stream& output);

  bool in_header_section() const {
    return m_in_header_section;
  }
  
  bool foundSignificantContent() const {
    return m_found_significant_content;
  }
  
  void startSignificantContentSearch() {
    m_search_significant_content = true;
  }
  
private:
  pp* m_engine;
  pp_frame* m_frame;

  pp_skip_number skip_number;
  pp_skip_identifier skip_identifier;
  pp_skip_string_literal skip_string_literal;
  pp_skip_char_literal skip_char_literal;
  pp_skip_argument skip_argument;
  pp_skip_comment_or_divop skip_comment_or_divop;
  pp_skip_blanks skip_blanks;
  pp_skip_whitespaces skip_whitespaces;

  bool m_in_header_section;
  bool m_search_significant_content, m_found_significant_content;
};

}

#endif // PP_MACRO_EXPANDER_H

