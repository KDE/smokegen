#include "defaultargvisitor.h"

bool DefaultArgVisitor::VisitDeclRefExpr(clang::DeclRefExpr* D) {
    if (clang::EnumConstantDecl* enumConstant = clang::dyn_cast<clang::EnumConstantDecl>(D->getDecl())) {
        ops.push_back(
            clang::cast<clang::NamedDecl>(enumConstant->getDeclContext()->getParent())->getQualifiedNameAsString() + "::" +
            enumConstant->getNameAsString()
        );
    }
    return true;
}

bool DefaultArgVisitor::VisitBinaryOperator(clang::BinaryOperator* D) {
    isSimple = false;
    return true;
}

bool DefaultArgVisitor::VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr* D) {
    isSimple = false;
    return true;
}

std::string DefaultArgVisitor::toString() const {
    std::string ret;
    if (!isSimple) {
        return ret;
    }

    for (const std::string& op : ops) {
        ret += op;
    }
    return ret;
}
