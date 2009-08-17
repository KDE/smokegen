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

#ifndef CONTROL_H
#define CONTROL_H

#include "cppparser_export.h"
#include "symbol.h"
#include "memorypool.h"
#include "problem.h"

#include <QtCore/QList>

struct Declarator;
struct Type;

/**Parser control information.
Provides problem tracking, context(visibility)-aware symbol name tables,
typedef table and other information about the parsing process.*/
class CPPPARSER_EXPORT Control
{
public:
  Control();
  ~Control();

  const QList<Problem*>& problems() const;

  /**Adds a problem to the list of problems.*/
  void reportProblem(Problem *problem);

private:
  QList<Problem*> _M_problems;
};

#endif // CONTROL_H

