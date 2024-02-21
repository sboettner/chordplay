#ifndef INCLUDE_NOTE_H
#define INCLUDE_NOTE_H

#include <cstdint>
#include <string>

class alignas(2) Interval {
    friend class Note;

    int8_t  notes;
    int8_t  semitones;

public:
    Interval(int8_t notes, int8_t semitones):notes(notes), semitones(semitones) {}
};


class MidiNote;

class alignas(2) Note {
    friend class MidiNote;

    int8_t  base;
    int8_t  value;

public:
    Note()
    {
        base=value=-1;
    }

    explicit Note(const std::string&);

    operator bool() const
    {
        return base>=0;
    }

    std::string get_name() const;

    Note operator+(const Interval&) const;

    friend bool operator==(const Note& lhs, const MidiNote& rhs);
};


class MidiNote {
    uint8_t value;

public:
    MidiNote() {}

    MidiNote(const Note& note, uint8_t octave)
    {
        value=note.value + octave*12;
    }

    operator uint8_t() const
    {
        return value;
    }

    friend bool operator==(const Note& lhs, const MidiNote& rhs);
};

#endif
