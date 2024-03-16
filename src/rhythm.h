#ifndef INCLUDE_RHYTHM_H
#define INCLUDE_RHYTHM_H

#include <string>
#include <vector>

class Rhythm {
public:
    struct Voice {
        int8_t  midi_channel;
        int8_t  midi_program;
        int8_t  midi_note;
        int8_t  midi_velocity_onbeat;
        int8_t  midi_velocity_offbeat;

        std::string pattern;
    };

    void add_voice(const Voice&);

private:
    std::vector<Voice>  voices;
};

#endif
