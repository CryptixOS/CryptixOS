void __mlibc_entry()
{
    static const char* str = "Hello";
    for (;;)
        __asm__ volatile(
            "push $0\n"
            "push %%rsi\n"
            "movq $0, %%rax\n"
            "movq %0, %%rsi\n"
            "movq %1, %%rdx\n"
            "syscall"
            :
            : "r"(str), "r"(5ull)
            : "rax", "rdx", "rsi");

    __asm__ volatile("hlt");
}
int main()
{
    __mlibc_entry();
    return 0;
}
