import gdb
import os.path

class LoadModule(gdb.Command):
    def __init__(self):
        super().__init__("loom_gdb_module_load", gdb.COMMAND_USER)

    def invoke(self, arg, tty):
        mod = gdb.parse_and_eval(arg)

        if mod['name'] is None:
            print('Module name not found.')
            return

        mod_name = mod['name'].string()

        mod_dir = gdb.convenience_variable('moddir')
        if mod_dir is None:
            print('Set $moddir to load symbols.')
            return

        mod_dir = mod_dir.string()

        symbol_file = f'{mod_dir}/{mod_name}.lib'

        if not os.path.isfile(symbol_file):
            print(f'Could not find symbol file: \'{symbol_file}\'')
            return

        section = mod['sections']
        command = f'add-symbol-file {symbol_file}'

        try:
            while section:
                name = section['name'].string()
                if name == '.text':
                    command += f" {section['p']}"
                else:
                    command += f" -s {name} {section['p']}"
                section = section['next']
        except gdb.error:
            print('Bootloader not compiled with debug support enabled.')
            return

        gdb.execute(command)

LoadModule()