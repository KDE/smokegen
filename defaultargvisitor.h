#ifndef SMOKEGEN_DEFAULTARGVISITOR
#define SMOKEGEN_DEFAULTARGVISITOR

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Sema/Sema.h>
#include <clang/Rewrite/Core/Rewriter.h>

#include "type.h"

// This class attempts to resolve types used in default argument expressions.
// For example:
//
// class QTextCodec {
//     enum ConversionFlag {
//         DefaultConversion,
//         IgnoreHeader = 0x1,
//         FreeFunction = 0x2
//     };
//
//     struct ConverterState {
//         ConverterState(ConversionFlag f = DefaultConversion);
//     };
// };
// In the above constructor for ConverterState, clang will identify the default
// argument for the constructor as the string as written, "DefaultConversion".
// However, when smoke generates bindings for that struct outside of the
// QTextCodec base class, so looking up DefaultConversion will fail.  It needs
// to be fully qualified as QTextCodec::DefaultArgVisitor.
//
// This RecursiveASTVisitor is used to walk the default expression used for a
// parameter, and resolve enums to their fully-qualified versions.

class DefaultArgVisitor : public clang::RecursiveASTVisitor<DefaultArgVisitor> {
public:
    DefaultArgVisitor(clang::CompilerInstance &ci) : ci(ci), containsEnum(false) {
        rewriter.setSourceMgr(ci.getSourceManager(), ci.getLangOpts());
    }

    bool VisitDeclRefExpr(clang::DeclRefExpr* D);

    std::string toString(const clang::Expr* D) const;

private:
    clang::PrintingPolicy pp() const { return ci.getSema().getPrintingPolicy(); }

    clang::CompilerInstance &ci;
    clang::Rewriter rewriter;

    bool containsEnum;
};

#endif
