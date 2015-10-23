#ifndef SMOKEGEN_ASTVISITOR
#define SMOKEGEN_ASTVISITOR

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Sema/Sema.h>

#include "type.h"

class SmokegenASTVisitor : public clang::RecursiveASTVisitor<SmokegenASTVisitor> {
public:
    SmokegenASTVisitor(clang::CompilerInstance &ci) : ci(ci) {}

    bool VisitCXXRecordDecl(clang::CXXRecordDecl *D);
    bool VisitEnumDecl(clang::EnumDecl *D);
    bool VisitFunctionDecl(clang::FunctionDecl *D);
    bool VisitTypedefNameDecl(clang::TypedefNameDecl *D);

private:
    clang::PrintingPolicy pp() const { return ci.getSema().getPrintingPolicy(); }

    clang::QualType getReturnTypeForFunction(const clang::FunctionDecl* function) const;

    Class* registerClass(const clang::CXXRecordDecl* clangClass) const;
    Enum* registerEnum(const clang::EnumDecl* clangEnum) const;
    Function* registerFunction(const clang::FunctionDecl* clangFunction) const;
    Type* registerType(const clang::QualType clangType) const;
    Typedef* registerTypedef(const clang::TypedefNameDecl* clangTypedef) const;

    Access toAccess(clang::AccessSpecifier clangAccess) const;
    Parameter toParameter(const clang::ParmVarDecl* param) const;

    // The typedef knows what type it aliases.  But places where the typedef is
    // used can add additional pointer depths to the type. eg:
    // typdef double qreal;
    // qreal** somefunc() // returns a double**, but typedef resolves will just return double
    Type* typeFromTypedef(const Typedef* tdef, const Type* sourceType) const;

    void addQPropertyAnnotations(const clang::CXXRecordDecl* D) const;

    clang::CompilerInstance &ci;
};

#endif
