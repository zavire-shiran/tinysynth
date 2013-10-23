import curses
import curses.ascii

notenames = ['C ', 'C#', 'D ', 'D#', 'E ', 'F ', 'F#', 'G ', 'G#', 'A', 'A#', 'B']
notevals = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']

def noteval2text(noteval):
    octave = noteval / 12
    note = noteval % 12
    return notenames[note] + str(octave)

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


class section(object):
    def __init__(self):
        self.tempo = 60
        self.instruments = []

    def add_instrument(self):
        self.instruments.append(instrument('sine'))
        while self.instruments[-1].num_notes < self.num_notes:
            self.instruments[-1].add_note()

    def add_note(self):
        for inst in self.instruments:
            inst.add_note()

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

        for instr_n, instr in enumerate(self.section.instruments):
            x = instr_n * 10 + 2
            window.addstr(3, x, str(instr.instr_type))

            for note_n, note in enumerate(instr.notes):
                if self.note_selection == note_n and self.instr_selection == instr_n:
                    window.attron(curses.A_REVERSE)
                y = note_n + 4
                if note.pitch == 0:
                    window.addstr(y, x, '---- ---')
                elif note.pitch == 255:
                    window.addstr(y, x, '|||| %3i' % note.gain)
                else:
                    window.addstr(y, x, '%4s %3i' % (noteval2text(note.pitch), note.gain))
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
        elif key in keybindings:
            self.set_current_note_pitch(keybindings[key], self.octave)
        return self


class composition_overview(object):
    def __init__(self, comp):
        self.composition = comp
        self.selected_section = 0

    def draw(self, window):
        window.erase()

        window.addstr(0, 0, 'Sections: %4i   Play order length: %4i' % (self.composition.num_sections,
                                                                        self.composition.play_order_length))

        for n, sec in enumerate(self.composition.sections):
            if n == self.selected_section:
                window.attron(curses.A_REVERSE)
            window.addstr(2+n, 0, '%03i Section  %5i notes  %2i instruments' % (n, sec.num_notes, sec.num_instruments))
            if n == self.selected_section:
                window.attroff(curses.A_REVERSE)

    def oninput(self, key):
        if key == ord('n'):
            cur_comp.add_section()
        elif key == curses.KEY_DOWN:
            self.selected_section = min(self.composition.num_sections - 1, self.selected_section + 1)
        elif key == curses.KEY_UP:
            self.selected_section = max(0, self.selected_section - 1)
        elif key == curses.ascii.NL:
            return section_edit(self.composition.sections[self.selected_section])
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

    interface_state.draw(rootwindow)

    running = True

    while running:
        key = rootwindow.getch()
        if key == ord('Q'):
            running = False
        else:
            interface_state = interface_state.oninput(key)

        interface_state.draw(rootwindow)

finally:
    curses.endwin()

