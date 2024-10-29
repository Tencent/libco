import gdb

class PrintCoroutines(gdb.Command):
    def __init__(self):
        super(PrintCoroutines, self).__init__("print_coroutines", gdb.COMMAND_STACK)

    def invoke(self, arg, from_tty):
        coctx_array_type = gdb.lookup_type("void").pointer().pointer()
        coctx_array = gdb.parse_and_eval("coctx_array").cast(coctx_array_type)

        coctx_array_length = int(gdb.parse_and_eval("coctx_array_length"))
        print("Coroutine count: %d" % coctx_array_length)

        print("backup registers...\n")
        original_registers1 = {}
        try:
            for reg in ['rbp', 'rsp', 'rip', 'rbx']:
                original_registers1[reg] = int(gdb.parse_and_eval("${}".format(reg)))
        except gdb.error:
            print("get original_registers1 error\n")
            original_registers1 = {}

        original_registers2 = {}
        try:
            for reg in ['rcx', 'rdx', 'rsi', 'rdi', 'r8', 'r9', 'r12', 'r13', 'r14', 'r15']:
                original_registers2[reg] = int(gdb.parse_and_eval("${}".format(reg)))
        except gdb.error:
            print("get original_registers2 error\n")
            original_registers2 = {}

        print("print callstacks...\n")
        gdb.execute("set backtrace limit 24")
        for idx in range(coctx_array_length):
            coctx_ptr = coctx_array[idx].cast(gdb.lookup_type("void").pointer())
            coctx_addr = int(coctx_ptr.dereference())

            gdb.execute("set $rbp = *(%d + 48)" % coctx_addr)
            gdb.execute("set $rsp = *(%d + 104)" % coctx_addr)
            gdb.execute("set $rip = *(%d + 72)" % coctx_addr)
            gdb.execute("set $rbx = *(%d + 96)" % coctx_addr)

            gdb.execute("set $rcx = *(%d + 88)" % coctx_addr)
            gdb.execute("set $rdx = *(%d + 80)" % coctx_addr)
            gdb.execute("set $rsi = *(%d + 64)" % coctx_addr)
            gdb.execute("set $rdi = *(%d + 56)" % coctx_addr)
            gdb.execute("set $r8 = *(%d + 40)" % coctx_addr)
            gdb.execute("set $r9 = *(%d + 32)" % coctx_addr)
            gdb.execute("set $r12 = *(%d + 24)" % coctx_addr)
            gdb.execute("set $r13 = *(%d + 16)" % coctx_addr)
            gdb.execute("set $r14 = *(%d + 8)" % coctx_addr)
            gdb.execute("set $r15 = *(%d)" % coctx_addr)

            print("Coroutine %d (context address: 0x%x):" % (idx, coctx_addr))
            gdb.execute("bt")
            print("\n")

        print("restore registers...\n")
        for reg, value in original_registers1.items():
            gdb.execute("set ${} = {}".format(reg, value))
        for reg, value in original_registers2.items():
            gdb.execute("set ${} = {}".format(reg, value))

PrintCoroutines()
