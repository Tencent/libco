import gdb

class CoroutinesCallStack(gdb.Command):
    def __init__(self):
        super(CoroutinesCallStack, self).__init__("co_bt", gdb.COMMAND_STACK)

    def invoke(self, arg, from_tty):
        coctx_array_type = gdb.lookup_type("void").pointer().pointer()
        coctx_array = gdb.parse_and_eval("coctx_array").cast(coctx_array_type)

        coctx_array_length = int(gdb.parse_and_eval("coctx_array_length"))
        print("Coroutine count: %d" % coctx_array_length)

        print("print callstacks...\n")
        max_depth = 24
        for idx in range(coctx_array_length):
            coctx_ptr = coctx_array[idx].cast(gdb.lookup_type("void").pointer())
            coctx_addr = int(coctx_ptr.dereference())

            rbp = int(gdb.Value(coctx_addr + 48).cast(gdb.lookup_type("long").pointer()))
            rsp = int(gdb.Value(coctx_addr + 104).cast(gdb.lookup_type("long").pointer()))
            rip = int(gdb.Value(coctx_addr + 72).cast(gdb.lookup_type("long").pointer()))
            rbx = int(gdb.Value(coctx_addr + 96).cast(gdb.lookup_type("long").pointer()))

            print("Coroutine %d (context address: 0x%x):" % (idx, coctx_addr))
            for i in range(max_depth):
                # 读取当前栈帧的调用地址和栈帧指针
                call_addr = int(gdb.Value(rbp + 8).cast(gdb.lookup_type("long").pointer()))
                frame_ptr = int(gdb.Value(rbp).cast(gdb.lookup_type("long").pointer()))

                # 如果调用地址为0，说明已经到达调用栈底部
                if call_addr == 0:
                    break

                # 打印当前栈帧的信息
                print("  frame %d: 0x%x" % (i, call_addr))

                # 更新栈帧指针和调用地址，继续遍历调用栈
                rbp = frame_ptr
                rip = call_addr

                # 如果遍历到了最大深度，结束遍历
                if i == max_depth - 1:
                    print("  ...")
                    break
            print("\n")

CoroutinesCallStack()
