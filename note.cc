#include <stdexcept>
#include "note.h"

const static int8_t notevalues[]={ 0, 2, 4, 5, 7, 9, 11 };
const static char notenames[]="CDEFGAB";

NoteClass::NoteClass(const std::string& name)
{
    if (name.size()>2)
        throw std::range_error("invalid note");

    if (!name.size()) {
        base=NoteName::Invalid;
        value=-1;
        return;
    }

    switch (name[0]) {
    case 'C':
        base=NoteName::C;
        break;
    case 'D':
        base=NoteName::D;
        break;
    case 'E':
        base=NoteName::E;
        break;
    case 'F':
        base=NoteName::F;
        break;
    case 'G':
        base=NoteName::G;
        break;
    case 'A':
        base=NoteName::A;
        break;
    case 'B':
        base=NoteName::B;
        break;
    default:
        throw std::range_error("invalid note");
    }

    value=notevalues[(int8_t) base];

    if (name.size()<2) return;

    switch (name[1]) {
    case '#':
        if (++value==12) value=0;
        break;
    case 'b':
        if (--value<0) value=11;
        break;
    default:
        throw std::range_error("invalid note");
    }
}


std::string NoteClass::get_name() const
{
    std::string name(1, notenames[(int8_t) base]);

    int8_t tmp=value - notevalues[(int8_t) base];
    if (tmp<-6) tmp+=12;
    if (tmp> 6) tmp-=12;

    while (tmp<0) {
        name+='b';
        tmp++;
    }

    while (tmp>0) {
        name+='#';
        tmp--;
    }

    return name;
}


NoteClass NoteClass::operator+(const Interval& iv) const
{
    if (base<=NoteName::Invalid)
        return *this;

    NoteClass result;

    int8_t tmp=int8_t(base) + iv.notes;
    while (tmp<0) tmp+=7;
    while (tmp>6) tmp-=7;
    
    result.base=NoteName(tmp);
    result.value=value + iv.semitones;
    if (result.value>=12) result.value-=12;

    return result;
}


Note::Note(const std::string& str)
{
    int len=str.length();

    NoteClass nc(str.substr(0, len-1));

    base=nc.base;
    value=nc.value + stoi(str.substr(len-1))*12;
}


std::string Note::get_name() const
{
    std::string name(1, notenames[(int8_t) base]);

    int8_t tmp=value%12 - notevalues[(int8_t) base];
    if (tmp<-6) tmp+=12;
    if (tmp> 6) tmp-=12;

    if (!tmp) name+='-';

    while (tmp<0) {
        name+='b';
        tmp++;
    }

    while (tmp>0) {
        name+='#';
        tmp--;
    }

    name+='0' + (value/12);

    return name;
}


Note Note::operator+(const Interval& iv) const
{
    int8_t result_base=(int8_t(base) + iv.get_notes()) % 7;
    if (result_base<0) result_base+=7;

    return Note(NoteName(result_base), value + iv.get_semitones());
}


Interval Note::operator-(const Note& rhs) const
{
    int steps=int8_t(base) - int8_t(rhs.base);
    if (steps<0) steps+=7;

    int semitones=value - rhs.value;

    int expected=notevalues[steps];

    for (;;) {
        if (expected-semitones>6) {
            expected-=12;
            steps-=7;
        }
        else if (semitones-expected>6) {
            expected+=12;
            steps+=7;
        }
        else
            break;
    }

    return Interval(steps, semitones);
}
