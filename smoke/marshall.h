/*
    Copyright (C) 2003-2011  Richard Dale <Richard_Dale@tipitina.demon.co.uk>
    Copyright (C) 2012  Arno Rehn <arno@arnorehn.de>

    Based on the PerlQt marshalling code by Ashley Winters

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

#ifndef MARSHALL_H
#define MARSHALL_H

#include "smoke.h"

namespace SmokeUtils {

class SmokeType;

class BASE_SMOKE_EXPORT Marshall {
public:

    typedef void (*HandlerFn)(Marshall *);

    struct TypeHandler {
        const char *name;
        HandlerFn fn;
    };

    static HandlerFn getMarshallFn(const SmokeType& type);

    /**
     * FromBinding is used for virtual function return values and regular
     * method arguments.
     *
     * ToBinding is used for method return-values and virtual function
     * arguments.
     */
    enum Action { FromBinding, ToBinding };
    virtual SmokeType type() = 0;
    virtual Action action() = 0;

    /// contains the StackItem for SMOKE
    virtual Smoke::StackItem &item() = 0;
    /// contains the StackItem for the binding
    virtual Smoke::StackItem &var() = 0;

    virtual void unsupported() = 0;
    virtual Smoke *smoke() = 0;
    /**
     * For return-values, next() does nothing.
     * For FromBinding, next() calls the method and returns.
     * For ToBinding, next() calls the virtual function and returns.
     *
     * Required to reset Marshall object to the state it was
     * before being called when it returns.
     */
    virtual void next() = 0;
    /**
     * Indicates whether objects on the stack are deleted by SMOKE or if
     * the handler has to do it after next().
     * For example, stack objects from virtual method callbacks are deleted
     * by SMOKE, stack objects for ordinary method calls are not.
     */
    virtual bool smokeDeletesStackObjects() = 0;

    virtual ~Marshall() {}
};

} // namespace SmokeUtils

#endif
