#ifndef INCLUDE_CHORD_H
#define INCLUDE_CHORD_H

#include "note.h"

struct Chord {
    enum class Quality:uint8_t {
        Major,
        Minor,
        Sus2,
        Sus4
    };

    Quality     quality;
    uint8_t     required;

    Note        bass;
    Note        notes[6];

    std::string get_name() const;
};


#endif
