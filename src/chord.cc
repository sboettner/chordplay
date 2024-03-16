#include "chord.h"


bool Chord::operator==(const Chord& other) const
{
    if (bass!=other.bass)
        return false;

    for (int i=0;i<6;i++)
        if (notes[i]!=other.notes[i])
            return false;

    return true;
}


std::string Chord::get_name() const
{
    std::string name=notes[0].get_name();

    const static char* quality_names[]={ "", "m", "dim", "aug", "sus2", "sus4" };
    name+=quality_names[int(quality)];

    if (notes[3]) {
        Interval fourth=notes[3]-notes[0];
        if (fourth==Interval(6, 10))
            name+="7";
        else if (quality==Quality::Major && fourth==Interval(6, 11))
            name+="maj7";
    }

    if (bass) {
        name+='/';
        name+=bass.get_name();
    }

    return name;
}


bool Chord::append(const NoteClass& note)
{
    for (int i=0;i<6;i++) {
        if (!notes[i]) {
            notes[i]=note;
            return true;
        }

        if (notes[i]==note)
            break;
    }

    return false;
}


NoteClass Chord::operator[](NoteName name) const
{
    for (int i=0;i<6 && notes[i];i++)
        if (notes[i]==name)
            return notes[i];

    return NoteClass();
}


Chord& Chord::operator+=(const Interval& ival)
{
    if (bass)
        bass+=ival;

    for (int i=0;i<6 && notes[i];i++)
        notes[i]+=ival;
    
    return *this;
}
