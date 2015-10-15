#ifndef SMOKEGEN_FRONTENDACTION
#define SMOKEGEN_FRONTENDACTION

#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>

struct Options;

// For each source file provided to the tool, a new FrontendAction is created.
class SmokegenFrontendAction : public clang::ASTFrontendAction {
public:
    std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(clang::CompilerInstance &CI, clang::StringRef file) override;
};

#endif
