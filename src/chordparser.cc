#include <sstream>
#include <regex>
#include "chordparser.h"


struct ChordParser::Internal {
    std::regex  regex { "([A-G][#b]?)(?:(m|dim|aug)?(6|7|9)?|(maj7|sus2|sus4))((?:\\+[0-9]+)*)(?:/([A-G][#b]?))?" };
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

    Chord chord;
    chord.required=7;

    chord.notes[0]=NoteClass(result[1].str());
    chord.bass=NoteClass(result[6].str());

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
    else if (result[2]=="dim") {
        chord.quality=Chord::Quality::Diminished;
        chord.notes[1]=chord.notes[0] + Interval(2, 3);
        chord.notes[2]=chord.notes[0] + Interval(4, 6);
    }
    else if (result[2]=="aug") {
        chord.quality=Chord::Quality::Augmented;
        chord.notes[1]=chord.notes[0] + Interval(2, 4);
        chord.notes[2]=chord.notes[0] + Interval(4, 8);
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

    // parse tensions
    std::stringstream tensions(result[5].str());
    std::string token;
    while (getline(tensions, token, '+'))
        if (token.size()) {
            int step=stoi(token) - 1;
            if (step<0)
                return std::nullopt;
            
            int octaves=step/7;
            step%=7;

            const static int8_t semitones[]={ 0, 2, 4, 5, 7, 9, 11 };
            if (!chord.append(chord.notes[0] + Interval(step, semitones[step] + octaves*12)))
                return std::nullopt;
        }

    return chord;
}
