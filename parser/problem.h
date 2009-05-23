#ifndef PROBLEM_H
#define PROBLEM_H

#include "simplecursor.h"

struct Problem {
    enum Source {
        Source_Preprocessor,
        Source_Lexer,
        Source_Parser
    };
    
    Source source;
    QString description;
    QString explanation;
    QString file;
    SimpleCursor position;
};

#endif
