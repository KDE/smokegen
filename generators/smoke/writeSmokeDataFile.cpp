/*
    Generator for the SMOKE sources
    Copyright (C) 2009 Arno Rehn <arno@arnorehn.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QTextStream>

#include <type.h>

#include "globals.h"

uint qHash(const QVector<int> intList)
{
    const char *byteArray = (const char*) intList.constData();
    int length = sizeof(int) * intList.count();
    return qHash(QByteArray::fromRawData(byteArray, length));
}

void writeSmokeData()
{
    QFile smokedata(outputDir.filePath("smokedata.cpp"));
    smokedata.open(QFile::ReadWrite | QFile::Truncate);
    QTextStream out(&smokedata);
    foreach (const QFileInfo& file, headerList)
        out << "#include <" << file.fileName() << ">\n";
    out << "\n#include <smoke.h>\n";
    out << "#include <" << module << "_smoke.h>\n\n";
    
    // write out module_cast() function
    out << "static void *" << module << "_cast(void *xptr, Smoke::Index from, Smoke::Index to) {\n";
    out << "  switch(from) {\n";
    for (QMap<QString, int>::const_iterator iter = classIndex.constBegin(); iter != classIndex.constEnd(); iter++) {
        out << "    case " << iter.value() << ":   //" << iter.key() << "\n";
        out << "      switch(to) {\n";
        const Class& klass = classes[iter.key()];
        foreach (const Class* base, superClassList(&klass)) {
            QString className = base->toString();
            out << QString("        case %1: return (void*)(%2*)(%3*)xptr;\n")
                .arg(classIndex[className]).arg(className).arg(klass.toString());
        }
        out << QString("        case %1: return (void*)(%2*)xptr;\n").arg(iter.value()).arg(klass.toString());
        foreach (const Class* desc, descendantsList(&klass)) {
            QString className = desc->toString();
            out << QString("        case %1: return (void*)(%2*)(%3*)xptr;\n")
                .arg(classIndex[className]).arg(className).arg(klass.toString());
        }
        out << "        default: return xptr;\n";
        out << "      }\n";
    }
    out << "    default: return xptr;\n";
    out << "  }\n";
    out << "}\n\n";
    
    // write out the inheritance list
    QHash<QString, int> inheritanceList;
    QHash<const Class*, int> inheritanceIndex;
    out << "// Group of Indexes (0 separated) used as super class lists.\n";
    out << "// Classes with super classes have an index into this array.\n";
    out << "static Smoke::Index " << module << "_inheritanceList[] = {\n";
    out << "    0,\t// 0: (no super class)\n";
    
    int currentIdx = 1;
    for (QMap<QString, int>::const_iterator iter = classIndex.constBegin(); iter != classIndex.constEnd(); iter++) {
        const Class& klass = classes[iter.key()];
        if (!klass.baseClasses().count())
            continue;
        QList<int> indices;
        QStringList comment;
        foreach (const Class::BaseClassSpecifier& base, klass.baseClasses()) {
            QString className = base.baseClass->toString();
            indices << classIndex[className];
            comment << className;
        }
        int idx = 0;
        
        // FIXME:
        // Storing the index for a given list of parent classes with a string as a key
        // is ugly. Can anyone come up with a hash algorithm for integer lists?
        QString commentString = comment.join(", ");
        if (!inheritanceList.contains(commentString)) {
            idx = currentIdx;
            inheritanceList[commentString] = idx;
            out << "    ";
            for (int i = 0; i < indices.count(); i++) {
                if (i > 0) out << ", ";
                out << indices[i];
                currentIdx++;
            }
            currentIdx++;
            out << ", 0,\t// " << idx << ": " << commentString << "\n";
        } else {
            idx = inheritanceList[commentString];
        }
        
        // store the index into inheritanceList for the class
        inheritanceIndex[&klass] = idx;
    }
    out << "};\n\n";
    
    // xenum functions
    out << "// These are the xenum functions for manipulating enum pointers\n";
    QSet<QString> enumClassesHandled;
    for (QHash<QString, Enum>::const_iterator it = enums.constBegin(); it != enums.constEnd(); it++) {
        if (it.value().parent() && !externalClasses.contains(it.value().parent())) {
            QString smokeClassName = it.value().parent()->toString();
            if (enumClassesHandled.contains(smokeClassName))
                continue;
            enumClassesHandled << smokeClassName;
            smokeClassName.replace("::", "__");
            out << "void xenum_" << smokeClassName << "(Smoke::EnumOperation, Smoke::Index, void*&, long&);\n";
        } else if (!it.value().parent()) {
            if (enumClassesHandled.contains("QGlobalSpace"))
                continue;
            out << "void xenum_QGlobalSpace(Smoke::EnumOperation, Smoke::Index, void*&, long&);\n";
            enumClassesHandled << "QGlobalSpace";
        }
    }
    
    // xcall functions
    out << "\n// Those are the xcall functions defined in each x_*.cpp file, for dispatching method calls\n";
    for (QMap<QString, int>::const_iterator iter = classIndex.constBegin(); iter != classIndex.constEnd(); iter++) {
        Class& klass = classes[iter.key()];
        if (externalClasses.contains(&klass))
            continue;
        QString smokeClassName = QString(klass.toString()).replace("::", "__");
        out << "xcall_" << smokeClassName << "(Smoke::Index, void*, Smoke::Stack);\n";
    }
    
    // classes table
    out << "\n// List of all classes\n";
    out << "// Name, external, index into inheritanceList, method dispatcher, enum dispatcher, class flags\n";
    out << "static Smoke::Class " << module << "_classes[] = {\n";
    out << "    { 0L, false, 0, 0, 0, 0 },\t// 0 (no class)\n";
    for (QMap<QString, int>::const_iterator iter = classIndex.constBegin(); iter != classIndex.constEnd(); iter++) {
        Class* klass = &classes[iter.key()];
        if (externalClasses.contains(klass)) {
            out << "    { \""  << iter.key() << "\", true, 0, 0, 0, 0 },\t//" << iter.value() << "\n";
        } else {
            QString smokeClassName = QString(iter.key()).replace("::", "__");
            out << "    { \"" << iter.key() << "\", false" << ", "
                << inheritanceIndex.value(klass, 0) << ", xcall_" << smokeClassName << ", "
                << (enumClassesHandled.contains(iter.key()) ? QString("xenum_").append(smokeClassName) : "0") << ", ";
            QString flags = "0";
            if (canClassBeInstanciated(klass)) flags += "|cf_constructor";
            if (canClassBeCopied(klass)) flags += "|cf_deepcopy";
            if (hasClassVirtualDestructor(klass)) flags += "|cf_virtual";
            flags.replace("0|", ""); // beautify
            out << flags;
            out << " },\t//" << iter.value() << "\n";
        }
    }
    out << "};\n\n";
    
    out << "// List of all types needed by the methods (arguments and return values)\n"
        << "// Name, class ID if arg is a class, and TypeId\n";
    out << "static Smoke::Type " << module << "_types[] = {\n";
    out << "    { 0, 0, 0 },\t//0 (no type)\n";
    QMap<QString, Type*> sortedTypes;
    for (QSet<Type*>::const_iterator it = usedTypes.constBegin(); it != usedTypes.constEnd(); it++) {
        sortedTypes.insert((*it)->toString(), *it);
    }
    QHash<Type*, int> typeIndex;
    int i = 1;
    for (QMap<QString, Type*>::const_iterator it = sortedTypes.constBegin(); it != sortedTypes.constEnd(); it++) {
        Type* t = it.value();
        int classIdx = 0;
        QString flags = "0";
        if (t->getClass()) {
            flags += "|Smoke::t_class";
            classIdx = classIndex.value(t->getClass()->toString(), 0);
        } else if (t->isIntegral() && t->name() != "void") {
            flags += "|Smoke::t_";
            QString typeName = t->name();
            typeName.replace("unsigned ", "u");
            typeName.replace("signed ", "");
            typeName.replace("long long", "long");
            typeName.replace("long double", "double");
            flags += typeName;
        } else if (t->getEnum()) {
            flags += "|Smoke::t_enum";
            if (t->getEnum()->parent())
                classIdx = classIndex.value(t->getEnum()->parent()->toString(), 0);
        } else {
            flags += "|Smoke::t_voidp";
        }
        
        if (t->isRef())
            flags += "|Smoke::tf_ref";
        if (t->pointerDepth() > 0)
            flags += "|Smoke::tf_ptr";
        if (!t->isRef() && t->pointerDepth() == 0)
            flags += "|Smoke::tf_stack";
        if (t->isConst())
            flags += "|Smoke::tf_const";
        flags.replace("0|", "");
        typeIndex[t] = i;
        out << "    { \"" << it.key() << "\", " << classIdx << ", " << flags << " },\t//" << i++ << "\n";
    }
    out << "};\n\n";
    
    out << "static Smoke::Index " << module << "_argumentList[] = {\n";
    out << "    0,\t//0  (void)\n";
    
    QHash<QVector<int>, int> parameterList;
    QHash<const Method*, int> parameterIndices;
    
    // munged name => index
    QMap<QString, int> methodNames;
    // class => list of munged names with possible methods
    QHash<const Class*, QMap<QString, QList<const Method*> > > classMungedNames;
    
    currentIdx = 1;
    for (QMap<QString, int>::const_iterator iter = classIndex.constBegin(); iter != classIndex.constEnd(); iter++) {
        Class* klass = &classes[iter.key()];
        if (externalClasses.contains(klass))
            continue;
        foreach (const Method& meth, klass->methods()) {
            if (meth.access() == Access_private)
                continue;
            
            methodNames[meth.name()] = 1;
            QString mungedName = ::mungedName(meth);
            methodNames[mungedName] = 1;
            QMap<QString, QList<const Method*> >& map = classMungedNames[klass];
            map[mungedName].append(&meth);
            
            if (!meth.parameters().count()) {
                parameterIndices[&meth] = 0;
                continue;
            }
            QVector<int> indices(meth.parameters().count());
            QStringList comment;
            for (int i = 0; i < indices.size(); i++) {
                Type* t = meth.parameters()[i].type();
                indices[i] = typeIndex[t];
                comment << t->toString();
            }
            int idx = 0;
            if ((idx = parameterList.value(indices, -1)) == -1) {
                idx = currentIdx;
                parameterList[indices] = idx;
                out << "    ";
                for (int i = 0; i < indices.count(); i++) {
                    if (i > 0) out << ", ";
                    out << indices[i];
                }
                out << ", 0,\t//" << idx << "  " << comment.join(", ") << "\n";
                currentIdx += indices.count() + 1;
            }
            parameterIndices[&meth] = idx;
        }
        foreach (BasicTypeDeclaration* decl, klass->children()) {
            const Enum* e = 0;
            if ((e = dynamic_cast<Enum*>(decl))) {
                foreach (const EnumMember& member, e->members())
                    methodNames[member.first] = 1;
            }
        }
    }
    for (QHash<QString, Enum>::const_iterator it = ::enums.constBegin(); it != ::enums.constEnd(); it++) {
        if (!it.value().parent()) {
            foreach (const EnumMember& member, it.value().members())
                methodNames[member.first] = 1;
        }
    }
    
    out << "};\n\n";
    
    out << "// Raw list of all methods, using munged names\n";
    out << "static const char *" << module << "_methodNames[] {\n";
    i = 1;
    for (QMap<QString, int>::iterator it = methodNames.begin(); it != methodNames.end(); it++, i++) {
        it.value() = i;
        out << "    \"" << it.key() << "\",\t//" << i << "\n";
    }
    out << "};\n\n";
    
    out << "// (classId, name (index in methodNames), argumentList index, number of args, method flags, "
        << "return type (index in types), xcall() index)\n";
    out << "static Smoke::Method " << module << "_methods[] = {\n";
    
    QHash<const Method*, int> methodIdx;
    i = 0;
    for (QMap<QString, int>::const_iterator iter = classIndex.constBegin(); iter != classIndex.constEnd(); iter++) {
        Class* klass = &classes[iter.key()];
        if (externalClasses.contains(klass))
            continue;
        int xcall_index = 1;
        foreach (const Method& meth, klass->methods()) {
            if (meth.access() == Access_private)
                continue;
            out << "    {" << iter.value() << ", " << methodNames[meth.name()] << ", ";
            int numArgs = meth.parameters().count();
            if (numArgs) {
                out << parameterIndices[&meth] << ", " << numArgs << ", ";
            } else {
                out << "0, 0, ";
            }
            QString flags = "0";
            if (meth.isConst())
                flags += "|Smoke::mf_const";
            if (meth.flags() & Method::Static)
                flags += "|Smoke::mf_static";
            if (meth.isConstructor())
                flags += "|Smoke::mf_ctor";
            if (meth.isDestructor())
                flags += "|Smoke::mf_dtor";
            if (meth.access() == Access_protected)
                flags += "|Smoke::mf_protected";
            if (meth.isConstructor() &&
                meth.parameters().count() == 1 &&
                meth.parameters()[0].type()->isConst() &&
                meth.parameters()[0].type()->getClass() == klass)
                flags += "|Smoke::mf_copyctor";
            flags.replace("0|", "");
            out << flags;
            out << ", " << typeIndex[meth.type()];
            out << ", " << xcall_index << "},";
            
            // comment
            out << "\t//" << i << " " << klass->toString() << "::";
            out << meth.name() << '(';
            for (int j = 0; j < meth.parameters().count(); j++) {
                if (j > 0) out << ", ";
                out << meth.parameters()[j].toString();
            }
            out << ')';
            if (meth.isConst())
                out << " const";
            if (meth.flags() & Method::PureVirtual)
                out << " [pure virtual]";
            out << "\n";
            methodIdx[&meth] = i;
            xcall_index++;
            i++;
        }
        // enums
        foreach (BasicTypeDeclaration* decl, klass->children()) {
            const Enum* e = 0;
            if ((e = dynamic_cast<Enum*>(decl))) {
                foreach (const EnumMember& member, e->members()) {
                    out << "    {" << iter.value() << ", " << methodNames[member.first]
                        << ", 0, 0, Smoke::mf_static|Smoke::mf_enum, " << typeIndex[&types[e->toString()]]
                        << ", " << xcall_index << "},";
                    
                    // comment
                    out << "\t//" << i << " " << klass->toString() << "::" << member.first;
                    out << "\n";
                    xcall_index++;
                    i++;
                }
            }
        }
    }
    
    out << "};\n\n";

    out << "static Smoke::Index " << module << "_ambiguousMethodList[] = {\n";
    out << "    0,\n";
    
    QHash<const Class*, QHash<QString, int> > ambigiousIds;
    i = 1;
    // ambigious method list
    for (QHash<const Class*, QMap<QString, QList<const Method*> > >::const_iterator iter = classMungedNames.constBegin();
         iter != classMungedNames.constEnd(); iter++)
    {
        const Class* klass = iter.key();
        const QMap<QString, QList<const Method*> >& map = iter.value();
        
        for (QMap<QString, QList<const Method*> >::const_iterator munged_it = map.constBegin();
             munged_it != map.constEnd(); munged_it++)
        {
            if (munged_it.value().size() < 2)
                continue;
            foreach (const Method* meth, munged_it.value()) {
                out << "    " << methodIdx[meth] << ',';
                
                // comment
                out << "  // " << klass->toString() << "::" << meth->name() << '(';
                for (int j = 0; j < meth->parameters().count(); j++) {
                    if (j > 0) out << ", ";
                    out << meth->parameters()[j].type()->toString();
                }
                out << ')';
                if (meth->isConst()) out << " const";
                out << "\n";
            }
            out << "    0,\n";
            ambigiousIds[klass][munged_it.key()] = i;
            i += munged_it.value().size() + 1;
        }
    }

    out << "};\n\n";

    out << "// Class ID, munged name ID (index into methodNames), method def (see methods) if >0 or number of overloads if <0\n";
    out << "static Smoke::MethodMap " << module << "_methodMaps[] = {\n";
    out << "    {0, 0, 0},\t//0 (no method)\n";

    for (QMap<QString, int>::const_iterator iter = classIndex.constBegin(); iter != classIndex.constEnd(); iter++) {
        Class* klass = &classes[iter.key()];
        if (externalClasses.contains(klass))
            continue;
        
        QMap<QString, QList<const Method*> >& map = classMungedNames[klass];
        for (QMap<QString, QList<const Method*> >::const_iterator munged_it = map.constBegin(); munged_it != map.constEnd(); munged_it++) {
            
            // class index, munged name index
            out << "    {" << classIndex[iter.key()] << ", " << methodNames[munged_it.key()] << ", ";
            
            // if there's only one matching method for this class and the munged name, insert the index into methodss
            if (munged_it.value().size() == 1) {
                out << methodIdx[munged_it.value().first()];
            } else {
                // negative index into ambigious methods list
                out << '-' << ambigiousIds[klass][munged_it.key()];
            }
            out << "},";
            // comment
            out << "\t// " << klass->toString() << "::" << munged_it.key();
            out << "\n";
        }
    }

    out << "};\n\n";

    smokedata.close();
}
