#ifndef INCLUDE_NOTE_H
#define INCLUDE_NOTE_H

#include <cstdint>
#include <string>

enum class NoteName:int8_t {
    Invalid=-1,
    C=0,
    D=1,
    E=2,
    F=3,
    G=4,
    A=5,
    B=6
};


class alignas(2) Interval {
    friend class NoteClass;

    int8_t  notes;
    int8_t  semitones;

public:
    Interval(int8_t notes, int8_t semitones):notes(notes), semitones(semitones) {}

    int get_notes() const
    {
        return notes;
    }

    int get_semitones() const
    {
        return semitones;
    }
};


class Note;

class alignas(2) NoteClass {
    friend class Note;
    friend class Scale;

    NoteName    base;
    int8_t      value;

public:
    NoteClass()
    {
        base=NoteName::Invalid;
        value=-1;
    }

    NoteClass(NoteName base, int8_t value):base(base), value(value) {}

    explicit NoteClass(const std::string&);

    operator bool() const
    {
        return base>NoteName::Invalid;
    }

    std::string get_name() const;

    NoteClass operator+(const Interval&) const;

    bool operator==(NoteName name) const
    {
        return base==name;
    }

    bool operator==(const NoteClass& rhs) const
    {
        return base==rhs.base && value==rhs.value;
    }
};


class Note {
    friend class Scale;

    NoteName    base;
    int8_t      value;

    Note(NoteName base, int8_t value):base(base), value(value) {}

public:
    Note() {}

    Note(const NoteClass& note, int8_t octave)
    {
        base=note.base;
        value=note.value + octave*12;
    }

    explicit Note(const std::string&);

    operator NoteClass() const
    {
        return NoteClass(base, value%12);
    }

    std::string get_name() const;

    uint8_t get_midi_note() const
    {
        return (uint8_t) value;
    }

    Note operator+(const Interval&) const;
    Interval operator-(const Note& rhs) const;

    bool operator<(const Note& rhs) const
    {
        return value<rhs.value;
    }

    bool operator>(const Note& rhs) const
    {
        return value>rhs.value;
    }

    bool operator<=(const Note& rhs) const
    {
        return value<=rhs.value;
    }

    bool operator>=(const Note& rhs) const
    {
        return value>=rhs.value;
    }
};

#endif
