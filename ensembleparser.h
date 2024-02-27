#ifndef INCLUDE_ENSEMBLEPARSER_H
#define INCLUDE_ENSEMBLEPARSER_H

#include <iostream>
#include "ensemble.h"

class EnsembleParser {
    struct Internal;

    Internal*   internal;

    Ensemble::Voice parse_voice(const std::string&) const;

public:
    EnsembleParser();
    ~EnsembleParser();

    Ensemble operator()(std::istream&) const;
};

#endif
