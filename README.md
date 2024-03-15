# ChordPlay

ChordPlay is an algorithmic composition tool for exploring chord progressions via a simple command line interface.
Its key features are:
* Find and play a minimal voice leading
* Improvise a simple melody over the chord sequence
* Loop the given chord sequence endlessly so you can improvise over it with your own instrument

## Prerequisites
* cmake (for building from source)
* RtMidi
* libpopt

## Basic Usage
Run Chordplay from the command line with a chord sequence
as arguments, e.g.
```
chordplay C F G7 C
```
Chordplay will automatically compute and print a minimal voice leading.
To play back the chord sequence, you need to have a General MIDI
synthesizer running and pass the `-p`  argument to Chordplay:
```
chordplay -p C F G7 C
```
To let Chordplay additionally improvise a random melody over the given
chord sequence, additionally pass the `-i` argument:
```
chordplay -p -i C F G7 C
```
