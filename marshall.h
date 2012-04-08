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

#include <smoke.h>

class SmokeType;

class Marshall {
public:

    typedef void (*HandlerFn)(Marshall *);

    struct TypeHandler {
        const char *name;
        HandlerFn fn;
    };

    static HandlerFn getMarshallFn(const SmokeType& type);

    /**
     * FromCrack is used for virtual function return values and regular
     * method arguments.
     *
     * ToCrack is used for method return-values and virtual function
     * arguments.
     */
    enum Action { FromCrack, ToCrack };
    virtual SmokeType type() = 0;
    virtual Action action() = 0;
    virtual Smoke::StackItem &item() = 0;
    virtual Smoke::StackItem &var() = 0;
    virtual void unsupported() = 0;
    virtual Smoke *smoke() = 0;
    /**
     * For return-values, next() does nothing.
     * For FromObject, next() calls the method and returns.
     * For ToObject, next() calls the virtual function and returns.
     *
     * Required to reset Marshall object to the state it was
     * before being called when it returns.
     */
    virtual void next() = 0;
    /**
     * For FromObject, cleanup() returns false when the handler should free
     * any allocated memory after next().
     *
     * For ToObject, cleanup() returns true when the handler should delete
     * the pointer passed to it.
     */
    virtual bool cleanup() = 0;

    virtual ~Marshall() {}
};    
#endif
