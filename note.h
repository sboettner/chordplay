#ifndef INCLUDE_NOTE_H
#define INCLUDE_NOTE_H

#include <cstdint>
#include <string>

class alignas(2) Interval {
    friend class NoteClass;

    int8_t  notes;
    int8_t  semitones;

public:
    Interval(int8_t notes, int8_t semitones):notes(notes), semitones(semitones) {}
};


class Note;

class alignas(2) NoteClass {
    friend class Note;

    int8_t  base;
    int8_t  value;

    NoteClass(int8_t base, int8_t value):base(base), value(value) {}

public:
    NoteClass()
    {
        base=value=-1;
    }

    explicit NoteClass(const std::string&);

    operator bool() const
    {
        return base>=0;
    }

    std::string get_name() const;

    NoteClass operator+(const Interval&) const;

    bool operator==(const NoteClass& rhs) const
    {
        return base==rhs.base && value==rhs.value;
    }
};


class Note {
    int8_t  base;
    int8_t  value;

public:
    Note() {}

    Note(const NoteClass& note, int8_t octave)
    {
        base=note.base;
        value=note.value + octave*12;
    }

    operator NoteClass() const
    {
        return NoteClass(base, value%12);
    }

    std::string get_name() const;

    uint8_t get_midi_note() const
    {
        return (uint8_t) value;
    }

    bool operator<(const Note& rhs) const
    {
        return value<rhs.value;
    }

    bool operator>=(const Note& rhs) const
    {
        return value>=rhs.value;
    }
};

#endif
