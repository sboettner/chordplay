#ifndef INCLUDE_SCALE_H
#define INCLUDE_SCALE_H

#include "note.h"

class Scale {
    NoteClass   notes[7];

public:
    Scale();

    Note project(const Note&) const;
};

#endif
