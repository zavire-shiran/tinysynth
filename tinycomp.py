import curses


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


class section(object):
    def __init__(self):
        self.tempo = 60
        self.instruments = []

    @property
    def num_instruments(self):
        return len(self.instruments)

    @property
    def num_notes(self):
        # all instruments must have the same number of notes
        # this is harder to ensure in practice than you might imagine
        if self.num_instruments > 0:
            return self.instruments[0].num_notes


class composition(object):
    def __init__(self):
        self.play_order = []
        self.sections = []

    def add_section(self):
        self.sections.append(section())


class interface_state(object):
    def __init__(self):
        self.current_section = -1

cur_comp = composition()
int_state = interface_state()

def draw_summary_line(window, comp):
    global int_state

    window.move(0, 0)
    window.clrtoeol()
    window.addstr('Section: ')
    if int_state.current_section == -1:
        window.addstr('N/A')
    else:
        window.addstr(str(int_state.current_section))

try:
    rootwindow = curses.initscr()

    draw_summary_line(rootwindow, cur_comp)

    rootwindow.getch()
finally:
    curses.endwin()

