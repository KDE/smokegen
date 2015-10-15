#include <clang/AST/ASTContext.h>

#include "astvisitor.h"

bool SmokegenASTVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl *D) {
    // We can't make bindings for things that don't have names.
    if (!D->getDeclName())
        return true;

    registerClass(D);

    return true;
}

bool SmokegenASTVisitor::VisitEnumDecl(clang::EnumDecl *D) {
    // We can't make bindings for things that don't have names.
    if (!D->getDeclName())
        return true;

    //generator.addEnum(D);

    return true;
}

bool SmokegenASTVisitor::VisitNamespaceDecl(clang::NamespaceDecl *D) {
    if (!D->getDeclName())
        return true;

    //generator.addNamespace(D);

    return true;
}

bool SmokegenASTVisitor::VisitFunctionDecl(clang::FunctionDecl *D) {
    if (clang::isa<clang::CXXMethodDecl>(D)){
        return true;
    }

    if (!D->getDeclName())
        return true;

    //generator.addFunction(D);

    return true;
}

clang::QualType SmokegenASTVisitor::getReturnTypeForMethod(const clang::CXXMethodDecl* method) const {
    if (clang::isa<clang::CXXConstructorDecl>(method)) {
        return ci.getASTContext().getPointerType(clang::QualType(method->getParent()->getTypeForDecl(), 0));
    }
    else {
        return method->getReturnType();
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

Class* SmokegenASTVisitor::registerClass(const clang::CXXRecordDecl* clangClass) const {
    clangClass = clangClass->hasDefinition() ? clangClass->getDefinition() : clangClass->getCanonicalDecl();

    QString qualifiedName = QString::fromStdString(clangClass->getQualifiedNameAsString());
    if (classes.contains(qualifiedName) and not classes[qualifiedName].isForwardDecl()) {
        // We already have this class
        return &classes[qualifiedName];
    }

    QString name = QString::fromStdString(clangClass->getNameAsString());
    QString nspace;
    if (const auto parent = clang::dyn_cast<clang::NamespaceDecl>(clangClass->getParent())) {
        nspace = QString::fromStdString(parent->getQualifiedNameAsString());
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

    Class* parent = nullptr;
    if (const auto clangParent = clang::dyn_cast<clang::CXXRecordDecl>(clangClass->getParent())) {
        parent = registerClass(clangParent);
    }
    Class localClass(name, nspace, parent, kind, isForward);
    classes[qualifiedName] = localClass;
    Class* klass = &classes[qualifiedName];

    klass->setAccess(toAccess(clangClass->getAccess()));

    if (clangClass->getTypeForDecl()->isDependentType() || clang::isa<clang::ClassTemplateSpecializationDecl>(clangClass)) {
        klass->setIsTemplate(true);
    }

    if (!isForward) {
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

    }
    return klass;
}

Type* SmokegenASTVisitor::registerType(clang::QualType clangType) const {
    clang::QualType orig = clang::QualType(clangType);

    Type type;

    if (clangType->isReferenceType()) {
        type.setIsRef(true);

        clangType = clangType->getPointeeType();
    }
    while (clangType->isPointerType()) {
        type.setIsConstPointer(type.pointerDepth(), clangType.isConstQualified());
        type.setPointerDepth(type.pointerDepth() + 1);

        clangType = clangType->getPointeeType();
    }

    type.setIsConst(clangType.isConstQualified());
    type.setIsVolatile(clangType.isVolatileQualified());

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
    return Type::registerType(type);
}
