#ifndef INCLUDE_CHORD_H
#define INCLUDE_CHORD_H

#include "note.h"

struct Chord {
    enum class Quality:uint8_t {
        Major,
        Minor,
        Diminished,
        Augmented,
        Sus2,
        Sus4
    };

    Quality     quality;
    uint8_t     required;

    NoteClass   bass;
    NoteClass   notes[6];

    std::string get_name() const;

    bool append(const NoteClass&);

    NoteClass operator[](NoteName) const;

    bool operator==(const Chord&) const;
    bool operator!=(const Chord& rhs) const
    {
        return !(*this==rhs);
    }

    Chord& operator+=(const Interval&);
};


#endif
