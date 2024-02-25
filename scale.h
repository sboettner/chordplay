#ifndef INCLUDE_SCALE_H
#define INCLUDE_SCALE_H

#include "chord.h"

class Scale {
public:
    NoteName    rootname;
    int8_t      notes[7];

public:
    Scale();
    Scale(const Chord&);

    Note operator()(int8_t) const;

    int8_t to_scale(const Note&) const;
};

#endif
