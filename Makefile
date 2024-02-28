OBJS=chordplay.o midi.o chord.o chordparser.o note.o scale.o ensemble.o ensembleparser.o

chordplay: $(OBJS)
	g++ -o chordplay $(OBJS) -lpopt `pkg-config --libs rtmidi`

$(OBJS): %.o: %.cc
	g++ -std=c++17 -g -O -c -o $@ $< `pkg-config --cflags rtmidi`
