#include "astvisitor.h"

bool SmokegenASTVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl *D) {
    // We can't make bindings for things that don't have names.
    if (!D->getDeclName())
        return true;

    llvm::outs() << D->getQualifiedNameAsString() << "\n";
    //generator.addClass(D);

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

