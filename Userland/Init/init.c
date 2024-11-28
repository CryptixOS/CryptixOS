void __mlibc_entry()
{
    static const char* str = "Hello";
start:
    for (;;)
        __asm__ volatile(
            "movq $2, %%rdi\n"
            "movq $0, %%rax\n"
            "movq %0, %%rsi\n"
            "movq %1, %%rdx\n"
            "syscall\n"
            :
            : "r"(str), "r"(5ull)
            : "rax", "rdx", "rsi");

    goto start;
}
int main()
{
    __mlibc_entry();
    return 0;
}
