#include <stdlib.h>
#include <regex>
#include "ensembleparser.h"


struct EnsembleParser::Internal {
    std::regex regex {
        "[[:space:]]*"
        "([[:lower:]]+)"    // role
        "[[:space:]]+"
        "([[:digit:]]+)"    // MIDI channel
        "[[:space:]]+"
        "([[:digit:]]+)"    // MIDI program (instrument)
        "[[:space:]]+"
        "([A-G][#b]?[0-9])" // low end
        "\\.\\.\\."
        "([A-G][#b]?[0-9])" // high end
        "[[:space:]]+"
        "([0-7])"           // color
        "[[:space:]]*"
        "(?:;.*)"           // comment
    };
};


EnsembleParser::EnsembleParser()
{
    internal=new Internal;
}


EnsembleParser::~EnsembleParser()
{
    delete internal;
}


Ensemble::Voice EnsembleParser::parse_voice(const std::string& line) const
{
    Ensemble::Voice voice;

    std::smatch result;
    if (!std::regex_match(line, result, internal->regex)) {
        std::cerr << "Error parsing ensemble definition: Invalid voice definition\n" << line << std::endl;
        exit(1);
    }

    if (result[1]=="bass")
        voice.role=Ensemble::Voice::Role::Bass;
    else if (result[1]=="harmony")
        voice.role=Ensemble::Voice::Role::Harmony;
    else if (result[1]=="melody")
        voice.role=Ensemble::Voice::Role::Melody;
    else {
        std::cerr << "Error parsing ensemble definition: Invalid voice role " << result[1] << std::endl;
        exit(1);
    }
    
    voice.midi_channel=stoi(result[2]);
    voice.midi_program=stoi(result[3]);
    voice.range_low=Note(result[4]);
    voice.range_high=Note(result[5]);
    voice.color=stoi(result[6]);

    return voice;
}


Ensemble EnsembleParser::operator()(std::istream& istr) const
{
    Ensemble ensemble;

    std::string line;
    while (getline(istr, line))
        ensemble.add_harmony_voice(parse_voice(line));

    return ensemble;
}
