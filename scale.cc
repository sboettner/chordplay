#include "scale.h"

Scale::Scale()
{
    // C major
    root=NoteClass(NoteName::C, 0);
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
    root=chord.notes[0];

    const static int8_t semitones_major[]={ 0, 2, 4, 5, 7, 9, 11 };
    const static int8_t semitones_minor[]={ 0, 2, 3, 5, 7, 8, 10 };

    const int8_t* semitones=chord.quality==Chord::Quality::Minor ? semitones_minor : semitones_major;

    for (int i=0;i<7;i++) {
        NoteName name=NoteName((int8_t(root.base) + i) % 7);

        notes[i]=chord[name].value;
        if (notes[i]<0)
            notes[i]=notes[0] + semitones[i];
        else if (i && notes[i]<notes[i-1])
            notes[i]+=12;
    }
}


Scale::Scale(const NoteClass& root, Mode mode):root(root), mode(mode)
{
    notes[0]=root.value;

    const int8_t semitones[]={ 0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23 };

    for (int i=1;i<7;i++) {
        NoteClass scaledegree=root + Interval(i, semitones[i+int(mode)]-semitones[int(mode)]);
        notes[i]=scaledegree.value;
        if (notes[i]<notes[i-1])
            notes[i]+=12;
    }
}


std::string Scale::get_name() const
{
    const static char* modenames[]={ "ionian", "dorian", "phrygian", "lydian", "mixolydian", "aeolian", "locrian" };
    return root.get_name() + '-' + modenames[int(mode)];
}


NoteClass Scale::operator[](int8_t degree) const
{
    int8_t notename=int8_t(root.base) + degree;
    if (notename>6) notename-=7;

    return NoteClass(NoteName(notename), notes[degree]%12);
}


Note Scale::operator()(int8_t degree) const
{
    int8_t octave=degree/7;
    degree%=7;

    if (degree<0) {
        degree+=7;
        octave--;
    }

    int8_t notename=int8_t(root.base) + degree;
    if (notename>6) notename-=7;

    return Note(NoteName(notename), notes[degree]+octave*12);
}


int8_t Scale::to_scale(const Note& note) const
{
    int8_t degree=int8_t(note.base) - int8_t(root.base);
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


bool Scale::contains(const NoteClass& note) const
{
    int8_t degree=int8_t(note.base) - int8_t(root.base);
    if (degree<0) degree+=7;

    return (notes[degree]-note.value)%12==0;
}


int Scale::distance(const Scale& scale1, const Scale& scale2)
{
    int dist=0;

    for (int i=0;i<7;i++) {
        int d1=i - int(scale1.root.base);
        if (d1<0) d1+=7;

        int d2=i - int(scale2.root.base);
        if (d2<0) d2+=7;

        int v=scale1.notes[d1] - scale2.notes[d2];
        if (v<-6) v+=12;
        if (v> 6) v-=12;

        dist+=v*v;
    }

    return dist;
}
