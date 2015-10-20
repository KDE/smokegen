#include <clang/AST/ASTContext.h>

#include "astvisitor.h"

bool SmokegenASTVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl *D) {
    registerClass(D);

    return true;
}

bool SmokegenASTVisitor::VisitEnumDecl(clang::EnumDecl *D) {
    registerEnum(D);

    return true;
}

bool SmokegenASTVisitor::VisitNamespaceDecl(clang::NamespaceDecl *D) {
    if (!D->getDeclName())
        return true;

    registerNamespace(D);

    return true;
}

bool SmokegenASTVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
    if (clang::isa<clang::CXXMethodDecl>(D)){
        return true;
    }

    if (!D->getDeclName())
        return true;

    if (D->isDependentContext()) {
        return true;
    }

    registerFunction(D);

    return true;
}

bool SmokegenASTVisitor::VisitTypedefNameDecl(clang::TypedefNameDecl *D) {

    registerTypedef(D);

    return true;
}

clang::QualType SmokegenASTVisitor::getReturnTypeForFunction(const clang::FunctionDecl* function) const {
    if (const auto ctor = clang::dyn_cast<clang::CXXConstructorDecl>(function)) {
        return ci.getASTContext().getPointerType(clang::QualType(ctor->getParent()->getTypeForDecl(), 0));
    }
    else {
        return function->getReturnType();
    }
}

Access SmokegenASTVisitor::toAccess(clang::AccessSpecifier clangAccess) const {
    Access access;
    switch (clangAccess) {
        case clang::AS_public:
        case clang::AS_none:
            access = Access_public;
            break;
        case clang::AS_protected:
            access = Access_protected;
            break;
        case clang::AS_private:
            access = Access_private;
            break;
    }
    return access;
}

Parameter SmokegenASTVisitor::toParameter(const clang::ParmVarDecl* param) const {
    Parameter parameter(
        QString::fromStdString(param->getNameAsString()),
        registerType(param->getType())
    );

    if (const clang::Expr* defaultArgExpr = param->getDefaultArg()) {
        std::string defaultArgStr;
        llvm::raw_string_ostream s(defaultArgStr);
        defaultArgExpr->printPretty(s, nullptr, pp());
        parameter.setDefaultValue(QString::fromStdString(s.str()));
    }

    return parameter;
}

Class* SmokegenASTVisitor::registerClass(const clang::CXXRecordDecl* clangClass) const {
    // We can't make bindings for things that don't have names.
    if (!clangClass->getDeclName())
        return nullptr;

    clangClass = clangClass->hasDefinition() ? clangClass->getDefinition() : clangClass->getCanonicalDecl();

    QString qualifiedName = QString::fromStdString(clangClass->getQualifiedNameAsString());
    if (classes.contains(qualifiedName) and not classes[qualifiedName].isForwardDecl()) {
        // We already have this class
        return &classes[qualifiedName];
    }

    QString name = QString::fromStdString(clangClass->getNameAsString());
    QString nspace;
    Class* parent = nullptr;
    if (const auto clangParent = clang::dyn_cast<clang::NamespaceDecl>(clangClass->getParent())) {
        nspace = QString::fromStdString(clangParent->getQualifiedNameAsString());
    }
    else if (const auto clangParent = clang::dyn_cast<clang::CXXRecordDecl>(clangClass->getParent())) {
        parent = registerClass(clangParent);
    }
    Class::Kind kind;
    switch (clangClass->getTagKind()) {
        case clang::TTK_Class:
            kind = Class::Kind_Class;
            break;
        case clang::TTK_Struct:
            kind = Class::Kind_Struct;
            break;
        case clang::TTK_Union:
            kind = Class::Kind_Union;
            break;
        default:
            break;
    }

    bool isForward = !clangClass->hasDefinition();

    Class localClass(name, nspace, parent, kind, isForward);
    classes[qualifiedName] = localClass;
    Class* klass = &classes[qualifiedName];

    klass->setAccess(toAccess(clangClass->getAccess()));

    if (clangClass->getTypeForDecl()->isDependentType() || clang::isa<clang::ClassTemplateSpecializationDecl>(clangClass)) {
        klass->setIsTemplate(true);
    }

    if (!isForward && !clangClass->getTypeForDecl()->isDependentType()) {
        // Set base classes
        for (const clang::CXXBaseSpecifier& base : clangClass->bases()) {
            const clang::CXXRecordDecl* baseRecordDecl = base.getType()->getAsCXXRecordDecl();

            if (!baseRecordDecl) {
                // Ignore template specializations
                continue;
            }

            Class::BaseClassSpecifier baseClass = Class::BaseClassSpecifier {
                &classes[QString::fromStdString(baseRecordDecl->getQualifiedNameAsString())],
                toAccess(base.getAccessSpecifier()),
                base.isVirtual()
            };

            klass->appendBaseClass(baseClass);
        }

        // Set methods
        for (const clang::CXXMethodDecl* method : clangClass->methods()) {
            if (method->isImplicit()) {
                continue;
            }
            Method newMethod = Method(
                klass,
                QString::fromStdString(method->getNameAsString()),
                registerType(getReturnTypeForFunction(method)),
                toAccess(method->getAccess())
            );
            if (const clang::CXXConstructorDecl* ctor = clang::dyn_cast<clang::CXXConstructorDecl>(method)) {
                newMethod.setIsConstructor(true);
                if (ctor->isExplicitSpecified()) {
                    newMethod.setFlag(Member::Explicit);
                }
            }
            else if (clang::isa<clang::CXXDestructorDecl>(method)) {
                newMethod.setIsDestructor(true);
            }
            newMethod.setIsConst(method->isConst());
            if (method->isVirtual()) {
                newMethod.setFlag(Member::Virtual);
                if (method->isPure()) {
                    newMethod.setFlag(Member::PureVirtual);
                }
            }
            if (method->isStatic()) {
                newMethod.setFlag(Member::Static);
            }

            for (const clang::ParmVarDecl* param : method->parameters()) {
                newMethod.appendParameter(toParameter(param));
            }

            klass->appendMethod(newMethod);
        }

        for (const clang::Decl* decl : clangClass->decls()) {
            const clang::VarDecl* varDecl = clang::dyn_cast<clang::VarDecl>(decl);
            const clang::FieldDecl* fieldDecl = clang::dyn_cast<clang::FieldDecl>(decl);
            if (!varDecl && !fieldDecl) {
                continue;
            }
            const clang::DeclaratorDecl* declaratorDecl = clang::dyn_cast<clang::DeclaratorDecl>(decl);
            Field field(
                klass,
                QString::fromStdString(declaratorDecl->getNameAsString()),
                registerType(declaratorDecl->getType()),
                toAccess(declaratorDecl->getAccess())
            );
            if (varDecl) {
                field.setFlag(Member::Static);
            }
            klass->appendField(field);
        }
    }
    return klass;
}

Enum* SmokegenASTVisitor::registerEnum(const clang::EnumDecl* clangEnum) const {
    clangEnum = clangEnum->getDefinition();
    if (!clangEnum) {
        return nullptr;
    }

    QString qualifiedName = QString::fromStdString(clangEnum->getQualifiedNameAsString());
    if (enums.contains(qualifiedName)) {
        // We already have this class
        return &enums[qualifiedName];
    }

    QString name = QString::fromStdString(clangEnum->getNameAsString());
    QString nspace;
    Class* parent = nullptr;
    if (const auto clangParent = clang::dyn_cast<clang::NamespaceDecl>(clangEnum->getParent())) {
        nspace = QString::fromStdString(clangParent->getQualifiedNameAsString());
    }
    else if (const auto clangParent = clang::dyn_cast<clang::CXXRecordDecl>(clangEnum->getParent())) {
        parent = registerClass(clangParent);
    }

    Enum localE(
        name,
        nspace,
        parent
    );

    enums[qualifiedName] = localE;
    Enum* e = &enums[qualifiedName];
    e->setAccess(toAccess(clangEnum->getAccess()));

    if (parent) {
        parent->appendChild(e);
    }

    for (const clang::EnumConstantDecl* enumVal : clangEnum->enumerators()) {
        EnumMember member(
            e,
            QString::fromStdString(enumVal->getNameAsString())
        );
        // The existing parser doesn't set the values for enums.
        //if (const clang::Expr* initExpr = enumVal->getInitExpr()) {
        //    std::string initExprStr;
        //    llvm::raw_string_ostream s(initExprStr);
        //    initExpr->printPretty(s, nullptr, pp());
        //    member.setValue(QString::fromStdString(s.str()));
        //}
        e->appendMember(member);
    }
    return e;
}

Function* SmokegenASTVisitor::registerFunction(const clang::FunctionDecl* clangFunction) const {
    clangFunction = clangFunction->getCanonicalDecl();

    std::string signatureStr = clangFunction->getQualifiedNameAsString();
    clangFunction->getType().getAsStringInternal(signatureStr, pp());
    QString signature = QString::fromStdString(signatureStr);

    if (functions.contains(signature)) {
        // We already have this function
        return &functions[signature];
    }

    QString name = QString::fromStdString(clangFunction->getNameAsString());
    QString nspace;
    if (const auto clangParent = clang::dyn_cast<clang::NamespaceDecl>(clangFunction->getParent())) {
        nspace = QString::fromStdString(clangParent->getQualifiedNameAsString());
    }

    Function newFunction(
        name,
        nspace,
        registerType(getReturnTypeForFunction(clangFunction))
    );

    for (const clang::ParmVarDecl* param : clangFunction->parameters()) {
        newFunction.appendParameter(toParameter(param));
    }

    functions[signature] = newFunction;
    return &functions[signature];
}

Class* SmokegenASTVisitor::registerNamespace(const clang::NamespaceDecl* clangNamespace) const {
    clangNamespace = clangNamespace->getCanonicalDecl();

    QString qualifiedName = QString::fromStdString(clangNamespace->getQualifiedNameAsString());
    if (classes.contains(qualifiedName)) {
        // We already have this class
        return &classes[qualifiedName];
    }

    QString name = QString::fromStdString(clangNamespace->getNameAsString());
    QString nspace;
    if (const auto clangParent = clang::dyn_cast<clang::NamespaceDecl>(clangNamespace->getParent())) {
        nspace = QString::fromStdString(clangParent->getQualifiedNameAsString());
    }
    classes[qualifiedName] = Class(name, nspace, nullptr, Class::Kind_Class, false);
    classes[qualifiedName].setIsNameSpace(true);
    return &classes[qualifiedName];
}

Type* SmokegenASTVisitor::registerType(clang::QualType clangType) const {
    clang::QualType orig = clang::QualType(clangType);

    Type type;

    if (clangType->isReferenceType()) {
        type.setIsRef(true);

        clangType = clangType->getPointeeType();
    }
    while (clangType->isPointerType()) {
        if (clangType->isFunctionPointerType()) {
            type.setIsFunctionPointer(true);
            const clang::FunctionType* fnType = clangType->getPointeeType()->getAs<clang::FunctionType>();
            clangType = fnType->getReturnType();

            if (const clang::FunctionProtoType* proto = clang::dyn_cast<clang::FunctionProtoType>(fnType)) {
                for (const clang::QualType param : proto->param_types()) {
                    type.appendParameter(Parameter("", registerType(param)));
                }
            }

            if (clangType->isReferenceType()) {
                type.setIsRef(true);
                clangType = clangType->getPointeeType();
            }
        }
        else {
            type.setIsConstPointer(type.pointerDepth(), clangType.isConstQualified());
            type.setPointerDepth(type.pointerDepth() + 1);

            clangType = clangType->getPointeeType();
        }
    }

    type.setIsConst(clangType.isConstQualified());
    type.setIsVolatile(clangType.isVolatileQualified());

    // We've got all the qualifier info we need.  Remove it so that the
    // qualifiers don't appear in the type name.
    clangType = clangType.getUnqualifiedType();

    type.setName(QString::fromStdString(clangType.getAsString(pp())));
    type.setIsIntegral(clangType->isBuiltinType());

    if (const clang::CXXRecordDecl* clangClass = clangType->getAsCXXRecordDecl()) {
        type.setClass(registerClass(clangClass));

        const auto templateSpecializationDecl = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(clangClass);
        if (templateSpecializationDecl) {
            const auto & args = templateSpecializationDecl->getTemplateArgs();
            for (int i=0; i < args.size(); ++i) {
                type.appendTemplateArgument(*registerType(args[i].getAsType()));
            }
        }
    }
    else if (const clang::EnumDecl* clangEnum = clang::dyn_cast_or_null<clang::EnumDecl>(clangType->getAsTagDecl())) {
        type.setEnum(registerEnum(clangEnum));
    }
    else if (const clang::TypedefType* typedefType = clang::dyn_cast<clang::TypedefType>(clangType)) {
        clang::TypedefNameDecl* typedefDecl = typedefType->getDecl();
        type.setTypedef(registerTypedef(typedefDecl));
    }
    return Type::registerType(type);
}

Typedef* SmokegenASTVisitor::registerTypedef(const clang::TypedefNameDecl* clangTypedef) const {
    clangTypedef = clangTypedef->getCanonicalDecl();

    QString qualifiedName = QString::fromStdString(clangTypedef->getQualifiedNameAsString());
    if (typedefs.contains(qualifiedName)) {
        // We already have this typedef
        return &typedefs[qualifiedName];
    }
    if (clangTypedef->getUnderlyingType().getCanonicalType()->isDependentType()) {
        return nullptr;
    }

    QString name = QString::fromStdString(clangTypedef->getNameAsString());
    QString nspace;
    Class* parent = nullptr;
    if (const auto clangParent = clang::dyn_cast_or_null<clang::NamespaceDecl>(clangTypedef->getDeclContext())) {
        nspace = QString::fromStdString(clangParent->getQualifiedNameAsString());
    }
    else if (const auto clangParent = clang::dyn_cast_or_null<clang::CXXRecordDecl>(clangTypedef->getDeclContext())) {
        parent = registerClass(clangParent);
    }

    Typedef tdef(
        registerType(clangTypedef->getUnderlyingType().getCanonicalType()),
        name,
        nspace,
        parent
    );

    typedefs[qualifiedName] = tdef;
    return &typedefs[qualifiedName];
}
