#include "astconsumer.h"
#include "ppcallbacks.h"

void SmokegenASTConsumer::Initialize(clang::ASTContext &ctx) {
    ppCallbacks = new SmokegenPPCallbacks(ci.getPreprocessor());
    ci.getPreprocessor().addPPCallbacks(std::unique_ptr<SmokegenPPCallbacks>(ppCallbacks));
}

bool SmokegenASTConsumer::HandleTopLevelDecl(clang::DeclGroupRef DR) {

    for (clang::DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
        // Traverse the declaration using our AST visitor.
        Visitor.TraverseDecl(*b);
    }
    return true;
}
