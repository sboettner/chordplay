#ifndef INCLUDE_RHYTHM_H
#define INCLUDE_RHYTHM_H

#include <string>
#include <vector>

class Rhythm {
public:
    struct Voice {
        enum class Role:uint8_t {
            Percussion,
            Bass
        };

        Role    role;
        int8_t  midi_channel;
        int8_t  midi_program;
        int8_t  midi_note;
        int8_t  midi_velocity_strong;
        int8_t  midi_velocity_weak;

        std::string pattern;
        std::string loop_end_pattern;
    };

    void add_voice(const Voice&);

    int get_voice_count() const
    {
        return voices.size();
    }

    const Voice& get_voice(int i) const
    {
        return voices[i];
    }

private:
    std::vector<Voice>  voices;
};

#endif
