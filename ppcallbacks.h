#ifndef SMOKEGEN_PPCALLBACKS
#define SMOKEGEN_PPCALLBACKS

#include <clang/Lex/Preprocessor.h>

class SmokegenPPCallbacks : public clang::PPCallbacks {
public:
    SmokegenPPCallbacks(clang::Preprocessor &pp) : pp(pp) {}

    void FileChanged(clang::SourceLocation Loc, FileChangeReason Reason,
            clang::SrcMgr::CharacteristicKind FileType, clang::FileID PrevFID) override;

private:
    void InjectQObjectDefs(clang::SourceLocation Loc);

    clang::Preprocessor &pp;
};

#endif
