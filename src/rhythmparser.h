#ifndef INCLUDE_RHYTHMPARSER_H
#define INCLUDE_RHYTHMPARSER_H

#include <iostream>
#include "rhythm.h"

class RhythmParser {
    struct Internal;

    Internal*   internal;

    Rhythm::Voice parse_voice(const std::string&) const;

public:
    RhythmParser();
    ~RhythmParser();

    Rhythm operator()(std::istream&) const;
};

#endif
