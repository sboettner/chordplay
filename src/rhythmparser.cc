#include <regex>
#include "rhythmparser.h"


struct RhythmParser::Internal {
    std::regex regex {
        "[[:space:]]*"
        "([[:lower:]]+)"    // role
        "[[:space:]]+"
        "([[:digit:]]+)"    // MIDI channel
        "[[:space:]]+"
        "([[:digit:]]+)"    // MIDI program (instrument/drum kit)
        "[[:space:]]+"
        "([[:digit:]]+)"    // MIDI note
        "[[:space:]]+"
        "([[:digit:]]+)"    // MIDI velocity (on-beat)
        "[[:space:]]+"
        "([[:digit:]]+)"    // MIDI velocity (off-beat)
        "[[:space:]]+"
        "(\\.*(?:[Xx]_*\\.*)+)"
        "[[:space:]]*"
        "(?:;.*)"           // comment
    };
};


RhythmParser::RhythmParser()
{
    internal=new Internal;
}


RhythmParser::~RhythmParser()
{
    delete internal;
}


Rhythm::Voice RhythmParser::parse_voice(const std::string& line) const
{
    std::smatch result;
    if (!std::regex_match(line, result, internal->regex)) {
        std::cerr << "Error parsing ensemble definition: Invalid voice definition\n" << line << std::endl;
        exit(1);
    }

    Rhythm::Voice voice;

    voice.midi_channel=std::stoi(result[2]);
    voice.midi_program=std::stoi(result[3]);
    voice.midi_note   =std::stoi(result[4]);
    voice.midi_velocity_onbeat =std::stoi(result[5]);
    voice.midi_velocity_offbeat=std::stoi(result[6]);
    voice.pattern=result[7];

    return voice;
}


Rhythm RhythmParser::operator()(std::istream& istr) const
{
    Rhythm rhythm;

    std::string line;
    while (getline(istr, line))
        rhythm.add_voice(parse_voice(line));

    return rhythm;
}
