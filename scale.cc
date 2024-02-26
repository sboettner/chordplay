#include "scale.h"

Scale::Scale()
{
    // C major
    rootname=NoteName::C;
    notes[0]=0;
    notes[1]=2;
    notes[2]=4;
    notes[3]=5;
    notes[4]=7;
    notes[5]=9;
    notes[6]=11;
}


Scale::Scale(const Chord& chord)
{
    rootname=NoteName(chord.notes[0].base);

    const static int8_t semitones_major[]={ 0, 2, 4, 5, 7, 9, 11 };
    const static int8_t semitones_minor[]={ 0, 2, 3, 5, 7, 8, 10 };

    const int8_t* semitones=chord.quality==Chord::Quality::Minor ? semitones_minor : semitones_major;

    for (int i=0;i<7;i++) {
        NoteName name=NoteName((int8_t(rootname) + i) % 7);

        notes[i]=chord[name].value;
        if (notes[i]<0)
            notes[i]=notes[0] + semitones[i];
        else if (i && notes[i]<notes[i-1])
            notes[i]+=12;
    }
}


Note Scale::operator()(int8_t degree) const
{
    int8_t octave=degree/7;
    degree%=7;

    if (degree<0) {
        degree+=7;
        octave--;
    }

    int8_t notename=int8_t(rootname) + degree;
    if (notename>6) notename-=7;

    return Note(NoteName(notename), notes[degree]+octave*12);
}


int8_t Scale::to_scale(const Note& note) const
{
    int8_t degree=int8_t(note.base) - int8_t(rootname);
    if (degree<0) degree+=7;

    int8_t offset=note.value - notes[degree];
    while (offset<-6) {
        offset+=12;
        degree-=7;
    }

    while (offset>6) {
        offset-=12;
        degree+=7;
    }

    return degree;
}
