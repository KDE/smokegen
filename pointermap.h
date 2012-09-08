/*
    Copyright (C) 2004-2011  Richard Dale <Richard_Dale@tipitina.demon.co.uk>
    Copyright (C) 2012  Arno Rehn <arno@arnorehn.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef POINTERMAP_H
#define POINTERMAP_H

#include <cassert>

#include <unordered_map>

#include <smoke.h>

// TODO: see if we can somehow improve/move these helpers without
//       resorting to subclassing and virtual methods
template <class X>
inline Smoke::ModuleIndex pointermap_get_classid_helper(X *obj) {
    return obj->classId;
}

template <class X>
inline void *pointermap_get_smokeobject_helper(X *obj) {
    return obj->object;
}

template <class MappedType>
class PointerMap
{
public:
    static PointerMap *self() {
        static PointerMap instance;
        return &instance;
    }

    inline void mapPointer(MappedType *instance) { mapPointer(instance, pointermap_get_classid_helper(instance), 0); }
    inline void unmapPointer(MappedType *instance) { unmapPointer(instance, pointermap_get_classid_helper(instance), 0); }
    inline MappedType *getObject(void *ptr) const {
        auto iter = pointerMap.find(ptr);
        return (iter == pointerMap.end()) ? 0 : iter->second;
    }

protected:
    void mapPointer(MappedType *instance, Smoke::ModuleIndex classId, void *lastptr);
    void unmapPointer(MappedType *instance, Smoke::ModuleIndex classId, void *lastptr);

    PointerMap() {}
    PointerMap(const PointerMap& other) {}

    std::unordered_map<void*, MappedType*> pointerMap;
};

// Store pointer in pointerMap hash : "pointer_to_object" => weak ref to associated bound object
// Recurse to store it also as casted to its parent classes.
template <class MappedType>
void PointerMap<MappedType>::mapPointer(MappedType *instance, Smoke::ModuleIndex classId, void *lastptr)
{
    assert(classId);

    Smoke::ModuleIndex instanceIndex = pointermap_get_classid_helper(instance);
    void * ptr = instanceIndex.smoke->cast(pointermap_get_smokeobject_helper(instance), instanceIndex, classId);

    if (ptr != lastptr) {
        lastptr = ptr;

        /*if (Debug::DoDebug & Debug::GC) {
            Object::Instance * instance = Object::Instance::get(obj);
            const char *className = instance->classId.smoke->classes[instance->classId.index].className;
            qWarning("QtRuby::Global::mapPointer (%s*)%p -> %p size: %d", className, ptr, (void*)obj, pointerMap()->size() + 1);
        }*/

        pointerMap[ptr] = instance;
    }

    Smoke * smoke = classId.smoke;

    for (Smoke::Index * parent = smoke->inheritanceList + smoke->classes[classId.index].parents;
         *parent != 0;
         parent++ )
    {
        if (smoke->classes[*parent].flags & Smoke::cf_undefined) {
            Smoke::ModuleIndex mi = Smoke::findClass(smoke->classes[*parent].className);
            if (mi) {
                mapPointer(instance, mi, lastptr);
            }
        } else {
            mapPointer(instance, Smoke::ModuleIndex(smoke, *parent), lastptr);
        }
    }
}

template <class MappedType>
void PointerMap<MappedType>::unmapPointer(MappedType * instance, Smoke::ModuleIndex classId, void *lastptr)
{
    assert(classId);

    Smoke::ModuleIndex instanceIndex = pointermap_get_classid_helper(instance);
    void * ptr = instanceIndex.smoke->cast(pointermap_get_smokeobject_helper(instance), instanceIndex, classId);

    if (ptr != lastptr) {
        lastptr = ptr;
        pointerMap.erase(ptr);
    }

    Smoke * smoke = classId.smoke;

    for (Smoke::Index * parent = smoke->inheritanceList + smoke->classes[classId.index].parents;
         *parent != 0;
         parent++ )
    {
        if (smoke->classes[*parent].flags & Smoke::cf_undefined) {
            Smoke::ModuleIndex mi = Smoke::findClass(smoke->classes[*parent].className);
            if (mi) {
                unmapPointer(instance, mi, lastptr);
            }
        } else {
            unmapPointer(instance, Smoke::ModuleIndex(smoke, *parent), lastptr);
        }
    }
}

#endif // POINTERMAP_H
