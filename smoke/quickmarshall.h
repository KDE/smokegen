/*
    Copyright (C) 2004  Richard Dale <Richard_Dale@tipitina.demon.co.uk>
    Copyright (C) 2012  Arno Rehn <arno@arnorehn.de>

    This library is free software; you can redistribute and/or modify them under
    the terms of the GNU Lesser General Public License as published by the
    Free Software Foundation; either version 3 of the License, or (at your option)
    any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef QUICKMARSHALL_H
#define QUICKMARSHALL_H

#include <iostream>

#include "smoke.h"
#include "marshall.h"
#include "smokeutils.h"

namespace SmokeUtils {

class BASE_SMOKE_EXPORT QuickMarshall : public Marshall {
private:
    SmokeType _type;
    Smoke::StackItem _input;
    Smoke::StackItem _output;
    Marshall::Action _action;
    bool _smokeDeletesStackObjects;

public:
    QuickMarshall(const SmokeType& type, Smoke::StackItem input, Marshall::Action action, bool smokeDeletesStackObjects)
        : _type(type), _input(input), _action(action), _smokeDeletesStackObjects(smokeDeletesStackObjects) {}

    inline SmokeType type() { return _type; }
    inline Marshall::Action action() { return _action; }
    inline Smoke::StackItem &item() { return _action == FromBinding ? _output : _input; }
    inline Smoke::StackItem &var() { return _action == FromBinding ? _input : _output; }
    inline Smoke *smoke() { return _type.smoke(); }
    inline bool smokeDeletesStackObjects() { return _smokeDeletesStackObjects; }

    inline void unsupported() {
        std::cerr << "QuickMarshall: cannot handle '" << type().name() << '\'' << std::endl;
        std::abort();
    }

    Smoke::StackItem operator()() {
        Marshall::HandlerFn fn = getMarshallFn(type());
        (*fn)(this);
        return _output;
    }

    Smoke::StackItem run() {
        return (*this)();
    }

    inline void next() {}
};

} // namespace SmokeUtils

#endif // QUICKMARSHALL_H
