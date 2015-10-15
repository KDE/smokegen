#include <clang/Sema/SemaDiagnostic.h>

#include "frontendaction.h"
#include "astconsumer.h"

std::unique_ptr<clang::ASTConsumer>
SmokegenFrontendAction::CreateASTConsumer(clang::CompilerInstance &CI, clang::StringRef file) {
    // Parsing function bodies can cause global template functions to be
    // instantiated unecessarily
    CI.getFrontendOpts().SkipFunctionBodies = true;
    CI.getDiagnostics().setSeverity(clang::diag::warn_undefined_inline, clang::diag::Severity::Ignored, clang::SourceLocation());

    return llvm::make_unique<SmokegenASTConsumer>(CI);
}
