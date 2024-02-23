#include <regex>
#include "chordparser.h"


struct ChordParser::Internal {
    std::regex  regex { "([A-G][#b]?)(?:(m)?(6|7|9)?|(maj7|sus2|sus4))(?:/([A-G][#b]?))?" };
};


ChordParser::ChordParser()
{
    internal=new Internal;
}


ChordParser::~ChordParser()
{
    delete internal;
}


std::optional<Chord> ChordParser::operator()(const char* name) const
{
    std::cmatch result;
    if (!std::regex_match(name, result, internal->regex))
        return std::nullopt;

    for (auto m: result)
        printf("  '%s'\n", m.str().c_str());

    Chord chord;
    chord.required=7;

    chord.notes[0]=NoteClass(result[1].str());
    chord.bass=NoteClass(result[5].str());

    if (result[4]=="maj7") {
        chord.quality=Chord::Quality::Major;
        chord.required=11;
        chord.notes[1]=chord.notes[0] + Interval(2, 4);
        chord.notes[2]=chord.notes[0] + Interval(4, 7);
        chord.notes[3]=chord.notes[0] + Interval(6, 11);
    }
    else if (result[4]=="sus2") {
        chord.quality=Chord::Quality::Sus2;
        chord.notes[1]=chord.notes[0] + Interval(1, 2);
        chord.notes[2]=chord.notes[0] + Interval(4, 7);
    }
    else if (result[4]=="sus4") {
        chord.quality=Chord::Quality::Sus4;
        chord.notes[1]=chord.notes[0] + Interval(3, 5);
        chord.notes[2]=chord.notes[0] + Interval(4, 7);
    }
    else if (result[2]=="m") {
        chord.quality=Chord::Quality::Minor;
        chord.notes[1]=chord.notes[0] + Interval(2, 3);
        chord.notes[2]=chord.notes[0] + Interval(4, 7);
    }
    else {
        chord.quality=Chord::Quality::Major;
        chord.notes[1]=chord.notes[0] + Interval(2, 4);
        chord.notes[2]=chord.notes[0] + Interval(4, 7);
    }

    if (result[3]=="6") {
        chord.required=15;
        chord.notes[3]=chord.notes[0] + Interval(5, 9);
    }
    else if (result[3]=="7") {
        chord.required=11;
        chord.notes[3]=chord.notes[0] + Interval(6, 10);
    }
    else if (result[3]=="9") {
        chord.required=19;
        chord.notes[3]=chord.notes[0] + Interval(6, 10);
        chord.notes[4]=chord.notes[0] + Interval(1, 2);
    }

    return chord;
}
