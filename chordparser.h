#ifndef INCLUDE_CHORDPARSER_H
#define INCLUDE_CHORDPARSER_H

#include <optional>
#include "chord.h"


class ChordParser {
    struct Internal;

    Internal*   internal;

public:
    ChordParser();
    ~ChordParser();
    
    std::optional<Chord> operator()(const char*) const;
};
 

#endif
