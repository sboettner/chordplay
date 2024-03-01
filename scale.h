#ifndef INCLUDE_SCALE_H
#define INCLUDE_SCALE_H

#include "chord.h"

class Scale {
public:
    enum class Mode {
        Ionian=0,
        Dorian=1,
        Phrygian=2,
        Lydian=3,
        Mixolydian=4,
        Aeolian=5,
        Locrian=6
    };

    Scale();
    Scale(const Chord&);
    Scale(const NoteClass& root, Mode mode);

    std::string get_name() const;

    NoteClass operator[](int8_t) const;
    Note operator()(int8_t) const;

    int8_t to_scale(const Note&) const;
    bool contains(const NoteClass&) const;

    static int distance(const Scale&, const Scale&);

private:
    NoteClass   root;
    Mode        mode;

    int8_t      notes[7];
};

#endif
