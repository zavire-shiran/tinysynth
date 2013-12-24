import curses
import curses.ascii

notenames = ['C ', 'C#', 'D ', 'D#', 'E ', 'F ', 'F#', 'G ', 'G#', 'A ', 'A#', 'B ']
notevals = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']

oscillatortypes = ['square', 'sawtooth', 'triangle', 'sine'] # leaving out FM for now because UI is complicated.

def pitch2text(pitch):
    octave = pitch / 12
    note = pitch % 12
    return '%s%2i' % (notenames[note], octave)

class note(object):
    def __init__(self, pitch, gain):
        self.pitch = pitch
        self.gain = gain


class instrument(object):
    def __init__(self, instr_type, fm_numerator = None, fm_denominator = None, fm_gain = None):
        self.instr_type = instr_type
        self.fm_numerator = fm_numerator
        self.fm_denominator = fm_denominator
        self.fm_gain = fm_gain
        self.notes = []

    @property
    def num_notes(self):
        return len(self.notes)

    def add_note(self):
        self.notes.append(note(0, 20))

    def del_note(self):
        del self.notes[-1]


class section(object):
    def __init__(self):
        self.tempo = 60
        self.instruments = []

    def add_instrument(self):
        self.instruments.append(instrument(0))
        while self.instruments[-1].num_notes < self.num_notes:
            self.instruments[-1].add_note()

    def del_instrument(self, instr_num):
        if instr_num < len(self.instruments):
            del self.instruments[instr_num]

    def add_note(self):
        for inst in self.instruments:
            inst.add_note()

    def del_note(self):
        if self.num_notes > 0:
            for instr in self.instruments:
                instr.del_note()

    @property
    def num_instruments(self):
        return len(self.instruments)

    @property
    def num_notes(self):
        # all instruments must have the same number of notes
        # this is harder to ensure in practice than you might imagine
        if self.num_instruments > 0:
            return self.instruments[0].num_notes
        else:
            return 0


class composition(object):
    def __init__(self):
        self.play_order = []
        self.sections = []

    def add_section(self):
        self.sections.append(section())

    @property
    def num_sections(self):
        return len(self.sections)

    @property
    def play_order_length(self):
        return len(self.play_order)


keybindings = {ord(k): v for k, v in {
    'q' : 'C',
    '2' : 'C#',
    'w' : 'D',
    '3' : 'D#',
    'e' : 'E',
    'r' : 'F',
    '5' : 'F#',
    't' : 'G',
    '6' : 'G#',
    'y' : 'A',
    '7' : 'A#',
    'u' : 'B'}.items() }


class section_edit(object):
    def __init__(self, section):
        if section.num_instruments == 0:
            section.add_instrument()
            section.add_instrument()

        while section.num_notes < 8:
            section.add_note()

        self.section = section
        self.note_selection = 0
        self.instr_selection = 0

        self.octave = 5

    def draw(self, window):
        window.clear()
        window.addstr(2, 0, 'Octave: %i' % self.octave)
        window.addstr(2, 15, 'Tempo: %i' % self.section.tempo)
        for instr_n, instr in enumerate(self.section.instruments):
            x = instr_n * 10 + 2
            window.addstr(3, x, oscillatortypes[instr.instr_type])

            for note_n, note in enumerate(instr.notes):
                y = note_n + 4

                if instr_n == 0:
                    window.addstr(y, 0, str(note_n + 1))

                if self.note_selection == note_n and self.instr_selection == instr_n:
                    window.attron(curses.A_REVERSE)

                if note.pitch == 0:
                    window.addstr(y, x, '---- ---')
                elif note.pitch == 255:
                    window.addstr(y, x, '|||| %3i' % note.gain)
                else:
                    window.addstr(y, x, '%4s %3i' % (pitch2text(note.pitch), note.gain))

                if self.note_selection == note_n and self.instr_selection == instr_n:
                    window.attroff(curses.A_REVERSE)

    def set_current_note_pitch(self, note, octave):
        pitch = notevals.index(note) + (octave * 12)
        self.section.instruments[self.instr_selection].notes[self.note_selection].pitch = pitch

    def set_current_note_sustain(self):
        self.section.instruments[self.instr_selection].notes[self.note_selection].pitch = 255

    def set_current_note_off(self):
        self.section.instruments[self.instr_selection].notes[self.note_selection].pitch = 0

    def modify_current_note_gain(self, amount):
        gain = self.section.instruments[self.instr_selection].notes[self.note_selection].gain
        gain = max(min(gain + amount, 250), 0)
        self.section.instruments[self.instr_selection].notes[self.note_selection].gain = gain

    def oninput(self, key):
        if key == curses.KEY_DOWN:
            self.note_selection = min(self.section.num_notes - 1, self.note_selection + 1)
        elif key == curses.KEY_UP:
            self.note_selection = max(0, self.note_selection - 1)
        elif key == curses.KEY_RIGHT:
            self.instr_selection = min(self.section.num_instruments - 1, self.instr_selection + 1)
        elif key == curses.KEY_LEFT:
            self.instr_selection = max(0, self.instr_selection - 1)
        elif key == ord('-'):
            self.octave = max(self.octave - 1, 0)
        elif key == ord('='):
            self.octave = min(self.octave + 1, 15)
        elif key == ord('['):
            self.modify_current_note_gain(-1)
        elif key == ord(']'):
            self.modify_current_note_gain(1)
        elif key == ord('{'):
            self.modify_current_note_gain(-5)
        elif key == ord('}'):
            self.modify_current_note_gain(5)
        elif key == ord('s'):
            self.set_current_note_sustain()
        elif key == ord('x'):
            self.set_current_note_off()
        elif key == ord(','):
            self.section.tempo = max(20, self.section.tempo - 1)
        elif key == ord('.'):
            self.section.tempo = min(500, self.section.tempo + 1)
        elif key == ord('m'):
            self.section.add_note()
            self.note_selection = max(0, self.note_selection)
        elif key == ord('n'):
            self.section.del_note()
            self.note_selection = min(self.section.num_notes - 1, self.note_selection)
        elif key == ord('M'):
            self.section.add_instrument()
        elif key == ord('D'):
            self.section.del_instrument(self.instr_selection)
            self.instr_selection = min(self.section.num_instruments - 1, self.instr_selection)
        elif key == ord('I'):
            self.section.instruments[self.instr_selection].instr_type = (self.section.instruments[self.instr_selection].instr_type + 1) % len(oscillatortypes)
        elif key in keybindings:
            self.set_current_note_pitch(keybindings[key], self.octave)
        elif key == ord('Q'):
            return composition_overview(cur_comp)
        return self


class composition_overview(object):
    def __init__(self, comp):
        self.composition = comp
        self.selected_section = 0
        self.selected_column = 0
        self.grabbed_selection = False

    def draw(self, window):
        window.erase()

        window.addstr(0, 0, 'Sections: %4i   Play order length: %4i' % (self.composition.num_sections,
                                                                        self.composition.play_order_length))
        window.addstr(1, 0, 'Sections')
        window.addstr(1, 42, '|')
        window.addstr(1, 45, 'Play order')

        for n, sec in enumerate(self.composition.sections):
            if self.selected_column == 0 and n == self.selected_section:
                window.attron(curses.A_REVERSE)
            window.addstr(2+n, 0, '%03i Section  %5i notes  %2i instruments' % (n, sec.num_notes, sec.num_instruments))
            if self.selected_column == 0 and n == self.selected_section:
                window.attroff(curses.A_REVERSE)

        for n, secn in enumerate(self.composition.play_order):
            if self.selected_column == 1 and n == self.selected_section:
                window.attron(curses.A_REVERSE)
            sec = self.composition.sections[secn]
            window.addstr(2+n, 45, '%03i Section  %5i notes  %2i instruments' % (secn, sec.num_notes, sec.num_instruments))
            if self.selected_column == 1 and n == self.selected_section:
                window.attroff(curses.A_REVERSE)


        for n in xrange(max(self.composition.num_sections, self.composition.play_order_length)):
            window.addstr(2+n, 42, '|')

    def oninput(self, key):
        if key == ord('n'):
            cur_comp.add_section()
        if key == ord('l'):
            if self.selected_column == 0:
                self.composition.play_order.append(self.selected_section)
                self.selected_column = 1
                self.selected_section = self.composition.play_order_length - 1
            elif self.selected_column == 1:
                if self.composition.play_order_length > 0:
                    del self.composition.play_order[self.selected_section]
                    self.selected_section = min(self.selected_section, self.composition.play_order_length - 1)
                    if self.selected_section == -1:
                        self.selected_column = 0
                        self.selected_section = 0
        elif key == curses.KEY_DOWN:
            if self.selected_column == 1:
                if self.grabbed_selection:
                    if self.selected_section < self.composition.play_order_length - 1:
                        temp = self.composition.play_order[self.selected_section]
                        self.composition.play_order[self.selected_section] = self.composition.play_order[self.selected_section + 1]
                        self.composition.play_order[self.selected_section + 1] = temp
                        self.selected_section += 1
                else:
                    self.selected_section = min(self.composition.play_order_length - 1, self.selected_section + 1)
            elif self.selected_column == 0:
                self.selected_section = min(self.composition.num_sections - 1, self.selected_section + 1)
        elif key == curses.KEY_UP:
            if self.selected_column == 1:
                if self.grabbed_selection:
                    if self.selected_section > 0:
                        temp = self.composition.play_order[self.selected_section]
                        self.composition.play_order[self.selected_section] = self.composition.play_order[self.selected_section - 1]
                        self.composition.play_order[self.selected_section - 1] = temp
                        self.selected_section -= 1
                else:
                    self.selected_section = max(0, self.selected_section - 1)
            elif self.selected_column == 0:
                self.selected_section = max(0, self.selected_section - 1)
        elif key == curses.KEY_LEFT or key == curses.KEY_RIGHT:
            if self.composition.play_order_length > 0:
                self.selected_column = (self.selected_column + 1) % 2
                self.selected_section = 0
        elif key == curses.ascii.NL:
            if self.selected_column == 0:
                return section_edit(self.composition.sections[self.selected_section])
            if self.selected_column == 1:
                self.grabbed_selection = not self.grabbed_selection
        elif key == ord('Q'):
            return None
        return self

cur_comp = composition()
interface_state = composition_overview(cur_comp)

try:
    rootwindow = curses.initscr()
    curses.start_color()
    curses.noecho()
    curses.cbreak()
    rootwindow.keypad(1)
    rootwindow.leaveok(1)

    running = True

    while interface_state != None:
        interface_state.draw(rootwindow)
        key = rootwindow.getch()
        interface_state = interface_state.oninput(key)

finally:
    curses.endwin()

