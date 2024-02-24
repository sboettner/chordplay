#ifndef INCLUDE_SCALE_H
#define INCLUDE_SCALE_H

#include "chord.h"

class Scale {
    NoteClass   notes[7];

public:
    Scale();
    Scale(const Scale&, const Chord&);

    Note project(const Note&) const;
};

#endif
