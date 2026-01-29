import gdb

class LoadModule(gdb.Command):
    def __init__(self):
        super().__init__("loom_gdb_module_load", gdb.COMMAND_USER)
        self.tab = None
        self.syms = 'build/mods.syms'

    def invoke(self, arg, tty):
        mod = gdb.parse_and_eval(arg)
        rawhash = mod['hash']
        modhash = bytes(int(rawhash[i]) for i in range(20)).hex()

        if self.tab is None:
            try:
                f = open(self.syms, 'r')
                lines = f.readlines()
                self.tab = {}

                for line in lines:
                    pair = line.strip().split(' ', maxsplit=1)
                    if len(pair) < 2:
                        self.tab = None
                        print(f'Invalid {self.syms}')
                        return
                    if pair[0] in self.tab:
                        print(f'WARNING: duplicate symbol hash key {pair[0]}')
                    self.tab[pair[0]] = pair[1]
            except:
                self.tab = None
                print(f'Could not read {self.syms}.')
                return

        if not modhash in self.tab:
            print(f"Symbol file for module {mod['name']} not found.")
            return

        symbol_file = self.tab[modhash]
        section = mod['sections']
        command = f'add-symbol-file {symbol_file}'

        while section:
            name = section['name'].string()
            if name == '.text':
                command += f" {section['p']}"
            else:
                command += f" -s {name} {section['p']}"
            section = section['next']

        gdb.execute(command)

LoadModule()