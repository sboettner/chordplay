#include "scale.h"

Scale::Scale()
{
    // C major
    notes[0]=NoteClass(0, 0);
    notes[1]=NoteClass(1, 2);
    notes[2]=NoteClass(2, 4);
    notes[3]=NoteClass(3, 5);
    notes[4]=NoteClass(4, 7);
    notes[5]=NoteClass(5, 9);
    notes[6]=NoteClass(6, 11);
}


Note Scale::project(const Note& in_note) const
{
    int note=in_note.base;
    int value=in_note.value;

    for (int i=0;i<7;i++) {
        if (notes[i].base==note) {
            int deviation=(notes[i].value - value) % 12;
            if (deviation<-6) deviation+=12;
            if (deviation> 6) deviation-=12;
            value+=deviation;
            break;
        }
    }

    return Note(note, value);
}
