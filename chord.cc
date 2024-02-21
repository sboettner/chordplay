#include "chord.h"


std::string Chord::get_name() const
{
    std::string name=notes[0].get_name();

    const static char* quality_names[]={ "", "m", "sus2", "sus4" };
    name+=quality_names[int(quality)];

    return name;
}
