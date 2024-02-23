#include <stdexcept>
#include "note.h"

const static int8_t notevalues[]={ 0, 2, 4, 5, 7, 9, 11 };

NoteClass::NoteClass(const std::string& name)
{
    if (name.size()>2)
        throw std::range_error("invalid note");

    if (!name.size()) {
        base=value=-1;
        return;
    }

    switch (name[0]) {
    case 'C':
        base=0;
        break;
    case 'D':
        base=1;
        break;
    case 'E':
        base=2;
        break;
    case 'F':
        base=3;
        break;
    case 'G':
        base=4;
        break;
    case 'A':
        base=5;
        break;
    case 'B':
        base=6;
        break;
    default:
        throw std::range_error("invalid note");
    }

    value=notevalues[base];

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
    const static char names[]="CDEFGAB";

    std::string name(1, names[base]);

    int8_t tmp=value - notevalues[base];
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
    if (base<0)
        return *this;

    NoteClass result;

    result.base=base + iv.notes;
    if (result.base>=7) result.base-=7;

    result.value=value + iv.semitones;
    if (result.value>=12) result.value-=12;

    return result;
}


std::string Note::get_name() const
{
    const static char names[]="CDEFGAB";

    std::string name(1, names[base]);

    int8_t tmp=value%12 - notevalues[base];
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
