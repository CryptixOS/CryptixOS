/*
 * Created by v1tr10l7 on 08.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/fcntl.h>
#include <Library/ELF/Loader.hpp>
#include <Library/ExecutableProgram.hpp>
#include <Library/StackBuilder.hpp>

#include <Memory/AddressSpace.hpp>
#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Scheduler/Process.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/FileDescriptor.hpp>
#include <VFS/VFS.hpp>

ErrorOr<void> ExecutableProgram::Load(PathView path, PageMap* pageMap,
                                      AddressSpace& addressSpace,
                                      Pointer       loadBase)
{
    m_LoadBase   = loadBase;
    m_Image      = TryOrRet(LoadImage(path, pageMap, addressSpace));
    m_EntryPoint = m_Image->EntryPoint();

    auto ldPath  = m_Image->InterpreterPath();
    if (ldPath.Empty()) return {};

    m_LoadBase    = 0x41000000zu;
    m_Interpreter = TryOrRet(LoadImage(ldPath, pageMap, addressSpace, true));
    m_EntryPoint  = m_Interpreter->EntryPoint();

    return {};
}
ErrorOr<void> ExecutableProgram::Load(::Ref<DirectoryEntry> dentry,
                                      PageMap*              pageMap,
                                      AddressSpace&         addressSpace,
                                      Pointer               loadBase)
{
    m_LoadBase   = loadBase;
    m_Image      = TryOrRet(LoadImage(dentry, pageMap, addressSpace));
    m_EntryPoint = m_Image->EntryPoint();

    auto ldPath  = m_Image->InterpreterPath();
    if (ldPath.Empty()) return {};

    m_LoadBase    = 0x41000000zu;
    m_Interpreter = TryOrRet(LoadImage(ldPath, pageMap, addressSpace, true));
    m_EntryPoint  = m_Interpreter->EntryPoint();

    return {};
}

// --- Helper: small PRNG to fill AT_RANDOM if kernel RNG isn't available.
//     It's fine for non-cryptographic purposes (glibc only needs
//     unpredictability).
static void FillRandomBytes(void* dst, usize len, upointer seedHint)
{
    u8*      out   = reinterpret_cast<u8*>(dst);
    // 64-bit LCG
    uint64_t state = (uint64_t)seedHint ^ 0x9E3779B97F4A7C15ULL;
    for (usize i = 0; i < len; ++i)
    {
        state  = state * 6364136223846793005ULL + 1ULL;
        out[i] = u8((state >> (i & 7) * 8) & 0xFF);
    }
}

Pointer ExecutableProgram::PrepareStack(Pointer            stackTopWritable,
                                        Pointer            stackTopVirt,
                                        Vector<StringView> argArr,
                                        Vector<StringView> envArr)
{
    (void)FillRandomBytes;
    StackBuilder              builder(stackTopWritable);

    UserMemoryProtectionGuard guard;

#if 1
    // --- 1. Copy envArr strings onto stack ---
    Pointer          stackCursor = stackTopVirt;
    Vector<upointer> envp;
    for (auto env : envArr)
    {
        stackCursor -= env.Size() + 1;
        envp.EmplaceBack(stackCursor);
        builder.Write(env.Raw(), env.Size() + 1);
    }

    // --- 2. Copy argArr strings onto stack ---
    Vector<upointer> argv;
    for (auto arg : argArr)
    {
        stackCursor -= arg.Size() + 1;
        argv.EmplaceBack(stackCursor);
        builder.Write(arg.Raw(), arg.Size() + 1);
    }

    // --- 3. Copy executable path for AT_EXECFN ---
    auto execPath = m_ExecutablePath;
    stackCursor -= execPath.Size() + 1;
    upointer execPathAddr = stackCursor;
    builder.Write(execPath.Raw(), execPath.Size() + 1);

    // --- 4. Align stack to 16 bytes ---
    builder.Align(16);
    // padding
    if ((argArr.Size() + envArr.Size() + 1) & 1) builder.Write(0);

    // --- 5. Push null terminators for argArr/envArr ---
    builder.Write(0);
    builder.Write(0);

    using AuxVal = ELF::AuxiliaryValueType;
    // --- 6. Push auxv entries ---
    builder.Write(AuxVal::eEntry, Image().EntryPoint());
    builder.Write(AuxVal::eProgramHeaders, Image().ProgramHeaderAddress());
    builder.Write(AuxVal::eProgramHeaderEntrySize,
                  Image().ProgramHeaderEntrySize());
    builder.Write(AuxVal::eProgramHeaderCount, Image().ProgramHeaderCount());

    if (m_InterpreterBase)
        builder.Write(AuxVal::eInterpreterBase, m_InterpreterBase);
    builder.Write(AuxVal::ePageSize, PMM::PAGE_SIZE);

    Credentials creds{};
    builder.Write(AuxVal::eUserID, creds.UserID);
    builder.Write(AuxVal::eEffectiveUserID, creds.EffectiveUserID);
    builder.Write(AuxVal::eGroupID, creds.GroupID);
    builder.Write(AuxVal::eEffectiveGroupID, creds.EffectiveGroupID);
    builder.Write(AuxVal::eExecutablePath, execPathAddr);

    // --- 7. Push envArr pointers ---
    builder.Write(0);
    for (usize i = envp.Size(); i > 0; i--) builder.Write(envp[i - 1]);

    // --- 8. Push argArr pointers ---
    builder.Write(0);
    for (usize i = argv.Size(); i > 0; i--) builder.Write(argv[i - 1]);

    // --- 9. Push argc ---
    builder.Write(argArr.Size());
    // builder.Align(16);

    // --- 10. Return new stack pointer ---
    return stackTopVirt - (builder.Top() - builder.Current());
#else
    // --- 1. Copy strings (we keep strings at high addresses; stack grows down)
    // ---
    Pointer          writeCursor = stackTopVirt;
    Vector<upointer> envp; // virtual addresses of env strings (forward order)
    envp.Clear();
    for (auto& env : envArr)
    {
        writeCursor -= (env.Size() + 1);
        envp.EmplaceBack(writeCursor);
        builder.Write(env.Raw(), env.Size() + 1);
    }

    Vector<upointer> argv; // virtual addresses of argv strings (forward order)
    argv.Clear();
    for (auto& arg : argArr)
    {
        writeCursor -= (arg.Size() + 1);
        argv.EmplaceBack(writeCursor);
        builder.Write(arg.Raw(), arg.Size() + 1);
    }

    // --- Copy executable path (for AT_EXECFN) ---
    auto execPath = m_ExecutablePath;
    writeCursor -= (execPath.Size() + 1);
    upointer execPathAddr = writeCursor;
    builder.Write(execPath.Raw(), execPath.Size() + 1);

    // --- Place AT_RANDOM (16 bytes) on the stack (above strings) ---
    constexpr usize RANDOM_BYTES = 16;
    writeCursor -= RANDOM_BYTES;
    Pointer  randomDst = writeCursor;
    u8       randomBuf[RANDOM_BYTES];
    // seedHint: combine top-of-stack and entry point for variability
    upointer seedHint
        = upointer(stackTopVirt.Raw()) ^ upointer(Image().EntryPoint());
    FillRandomBytes(randomBuf, RANDOM_BYTES, seedHint);
    builder.Write(Pointer(randomBuf), RANDOM_BYTES);
    upointer randomAddr = randomDst; // virtual address of AT_RANDOM data

    // --- Align pointer-area start to 16 bytes ---
    // We aligned the top-of-strings region; builder.Align aligns m_StackCurrent
    // which is top.
    builder.Align(16);

    // --- We'll now write pointer area & auxv. Because builder writes downward,
    //     we must write objects in *reverse* of how we want them in memory.
    //     However, our helper Write(aux, val) already writes value then type
    //     so that memory ends up (type, value) in increasing addresses.
    using AuxVal = ELF::AuxiliaryValueType;

    // --- 1) auxv terminator AT_NULL (write it first because we write downward)
    // ---
    builder.Write((upointer)0);                           // a_val = 0
    builder.Write((upointer)ToUnderlying(AuxVal::eNull)); // a_type = AT_NULL

    // --- 2) auxv entries (write in reverse order so they appear forward in
    // memory) --- We'll collect them then write in reverse for clarity.
    struct AuxEntry
    {
        AuxVal   type;
        upointer val;
    };
    Vector<AuxEntry> auxs;
    auxs.Clear();

    // Add typical aux entries expected by dynamic linkers / libc.
    auxs.EmplaceBack(AuxVal::eExecutablePath, (upointer)execPathAddr);
    auxs.EmplaceBack(AuxVal::eUserID, (upointer)Credentials{}.UserID);
    auxs.EmplaceBack(AuxVal::eEffectiveUserID,
                     (upointer)Credentials{}.EffectiveUserID);
    auxs.EmplaceBack(AuxVal::eGroupID, (upointer)Credentials{}.GroupID);
    auxs.EmplaceBack(AuxVal::eEffectiveGroupID,
                     (upointer)Credentials{}.EffectiveGroupID);

    auxs.EmplaceBack(AuxVal::ePageSize, (upointer)PMM::PAGE_SIZE);

    if (m_InterpreterBase)
        auxs.EmplaceBack(AuxVal::eInterpreterBase, (upointer)m_InterpreterBase);

    // Program header / entry info
    auxs.EmplaceBack(AuxVal::eProgramHeaders,
                     (upointer)Image().ProgramHeaderAddress());
    auxs.EmplaceBack(AuxVal::eProgramHeaderEntrySize,
                     (upointer)Image().ProgramHeaderEntrySize());
    auxs.EmplaceBack(AuxVal::eProgramHeaderCount,
                     (upointer)Image().ProgramHeaderCount());
    auxs.EmplaceBack(AuxVal::eEntry, (upointer)Image().EntryPoint());

    // AT_RANDOM (address of 16 random bytes we placed above)
    auxs.EmplaceBack(AuxVal::eRandom, (upointer)randomAddr);

    // Add AT_PHDR / AT_PHENT / AT_PHNUM aliases if your ELF enum differs:
    // some systems use AT_PHDR, AT_PHENT, AT_PHNUM (these correspond to
    // ProgramHeaderAddress/EntrySize/Count)
    auxs.EmplaceBack(AuxVal::eProgramHeaders,
                     (upointer)Image().ProgramHeaderAddress());
    auxs.EmplaceBack(AuxVal::eProgramHeaderEntrySize,
                     (upointer)Image().ProgramHeaderEntrySize());
    auxs.EmplaceBack(AuxVal::eProgramHeaderCount,
                     (upointer)Image().ProgramHeaderCount());

    // Write aux entries in reverse so they appear in forward order in memory
    for (usize i = auxs.Size(); i > 0; --i)
    {
        auto& e = auxs[i - 1];
        builder.Write(e.val);
        builder.Write(ToUnderlying(e.type));
    }

    // --- 3) envp pointers (forward order in memory)
    // Because builder writes downward, write NULL first then pointers in
    // reverse.
    builder.Write((upointer)0); // envp NULL terminator
    for (usize i = envp.Size(); i > 0; --i)
        builder.Write((upointer)envp[i - 1]);

    // --- 4) argv pointers (forward order in memory)
    builder.Write((upointer)0); // argv NULL terminator
    for (usize i = argv.Size(); i > 0; --i)
        builder.Write((upointer)argv[i - 1]);

    // --- 5) argc ---
    builder.Write((upointer)argArr.Size());

    // --- Final alignment: ensure RSP % 16 == 0 at program entry ---
    // Align again in case the counts caused misalignment.
    builder.Align(16);

    // --- Return new stack pointer (virtual) ---
    Pointer newRsp = stackTopVirt - (builder.Top() - builder.Current());
    return newRsp;
#endif
}

ErrorOr<Ref<ELF::Image>>
ExecutableProgram::LoadImage(PathView path, PageMap* pageMap,
                             AddressSpace& addressSpace, bool interpreter)
{
    Ref entry
        = VFS::ResolvePath(VFS::RootDirectoryEntry().Raw(), path).Value().Entry;
    if (!entry) return Error(ENOENT);

    return LoadImage(entry, pageMap, addressSpace, interpreter);
}
ErrorOr<Ref<ELF::Image>>
ExecutableProgram::LoadImage(::Ref<DirectoryEntry> entry, PageMap* pageMap,
                             AddressSpace& addressSpace, bool interpreter)
{
    auto inode = entry->INode();
    if (!inode) return Error(ENOENT);

    auto file = TryOrRet(
        VFS::Open(VFS::RootDirectoryEntry().Raw(), entry->Path(), O_RDONLY, 0));
    Ref image = CreateRef<ELF::Image>();

    if (!image->Load(file.Raw(), m_LoadBase)) return Error(ENOEXEC);
    // if (image->IsExecutable() && !image->InterpreterPath().Empty())
    //     return image;

    ELF::Loader loader(image);
    loader.LoadSegments(*pageMap, addressSpace, PageAttributes::eRWXU,
                        m_LoadBase);
    auto minVirt       = loader.MinVirt();

    m_SignalTrampoline = loader.TrampolineVirt();
    if (interpreter) m_InterpreterBase = minVirt;
    return image;
}
