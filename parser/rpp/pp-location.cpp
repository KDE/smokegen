/*
  Copyright 2007 Hamish Rodda <rodda@kde.org>
  Copyright 2008-2009 David Nolden <david.nolden.kdevelop@art-master.de>

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

#include "pp-location.h"

#include <QtDebug>
#include <QStringList>
#include "../indexedstring.h"
#include "chartools.h"

using namespace rpp;

LocationTable::LocationTable()
{
  anchor(0, Anchor(0,0), 0);
}

LocationTable::LocationTable(const PreprocessedContents& contents)
{
  anchor(0, Anchor(0,0), 0);

  const unsigned int newline = indexFromCharacter('\n');
  int line = 0;

  for (std::size_t i = 0; i < (std::size_t)contents.size(); ++i)
    if (contents.at(i) == newline)
      anchor(i + 1, Anchor(++line, 0), 0);
}

QPair<rpp::Anchor, uint> LocationTable::positionAt(std::size_t offset, const PreprocessedContents& contents, bool collapseIfMacroExpansion) const
{
  AnchorInTable ret = anchorForOffset(offset, collapseIfMacroExpansion);

  if(!ret.anchor.collapsed)
    for(std::size_t a = ret.position; a < offset; ++a)
      ret.anchor.column += IndexedString::fromIndex(contents[a]).length();
  
  uint room = 0;
  if(ret.nextPosition)
    if(ret.nextAnchor.line == ret.anchor.line && ret.nextAnchor.column > ret.anchor.column)
      room = ret.nextAnchor.column - ret.anchor.column;
    
  return qMakePair(ret.anchor, room);
}

void LocationTable::anchor(std::size_t offset, Anchor anchor, const PreprocessedContents* contents)
{
  Q_ASSERT(!offset || !anchor.column || contents);
  
  if (offset && anchor.column) {
    // Check to see if it's different to what we already know
    QPair<rpp::Anchor, uint> known = positionAt(offset, *contents);
    if (known.first == anchor && !anchor.collapsed && known.first.macroExpansion == anchor.macroExpansion)
      return;
  }
  m_currentOffset = m_offsetTable.insert(offset, anchor);
}

LocationTable::AnchorInTable LocationTable::anchorForOffset(std::size_t offset, bool collapseIfMacroExpansion) const
{
  // Look nearby for a match first
  QMap<std::size_t, Anchor>::ConstIterator constEnd = m_offsetTable.constEnd();

  if (m_currentOffset != constEnd) {
    std::size_t current = m_currentOffset.key();
    bool checkForwards = (current < offset);
    // TODO check for optimal number of iterations
    for (int i = 0; i < 5; ++i) {
      if (checkForwards) {
        if (m_currentOffset + 1 == constEnd)
          // Special case
          goto done;

        ++m_currentOffset;
        if (m_currentOffset != constEnd) {
          if (m_currentOffset.key() > offset) {
            // Gone forwards too much, but one back is correct
            --m_currentOffset;
            goto done;
          }

        } else {
          break;
        }

      } else {
        if (m_currentOffset == m_offsetTable.constBegin())
          // Special case
          goto done;

        ++m_currentOffset;
        if (m_currentOffset != constEnd) {
          if (m_currentOffset.key() < offset) {
            // Correct position :)
            goto done;
          }
        } else {
          break;
        }
      }
    }
  }

  m_currentOffset = m_offsetTable.lowerBound(offset);
  //kDebug() << k_funcinfo << offset << "found" << m_currentOffset.key();
  if (m_currentOffset == constEnd)
    --m_currentOffset;
  
  if (m_currentOffset.key() > offset)
    --m_currentOffset;

  done:
  Q_ASSERT(m_currentOffset != constEnd);
  Anchor ret = m_currentOffset.value();
  if(ret.macroExpansion.isValid() && collapseIfMacroExpansion)
    ret.collapsed = true;
  
  AnchorInTable retItem;
  retItem.position = m_currentOffset.key();
  retItem.anchor = ret;
  
  ++m_currentOffset;
  
  if(m_currentOffset == constEnd) {
    retItem.nextPosition = 0;
  }else{
    retItem.nextPosition = m_currentOffset.key();
    retItem.nextAnchor = m_currentOffset.value();
  }
  
  return retItem;
}

void LocationTable::dump() const
{
  QMapIterator<std::size_t, Anchor> it = m_offsetTable;
  qDebug() << "Location Table:";
  while (it.hasNext()) {
    it.next();
    qDebug() << it.key() << " => " << it.value().line << it.value().column;
  }
}

void LocationTable::splitByAnchors(const PreprocessedContents& text, const Anchor& textStartPosition, QList<PreprocessedContents>& strings, QList<Anchor>& anchors) const {

  Anchor currentAnchor = Anchor(textStartPosition);
  size_t currentOffset = 0;
  
  QMapIterator<std::size_t, Anchor> it = m_offsetTable;

  while (currentOffset < (size_t)text.size())
  {
    Anchor nextAnchor(SimpleCursor::invalid());
    size_t nextOffset;
    
    if(it.hasNext()) {
      it.next();
      nextOffset = it.key();
      nextAnchor = it.value();
    }else{
      nextOffset = text.size();
      nextAnchor = Anchor(SimpleCursor::invalid());
    }

    if( nextOffset-currentOffset > 0 ) {
      strings.append(text.mid(currentOffset, nextOffset-currentOffset));
      anchors.append(currentAnchor);
    }
    
    currentOffset = nextOffset;
    currentAnchor = nextAnchor;
  }
}
