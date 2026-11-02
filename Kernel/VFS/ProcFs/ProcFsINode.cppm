/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Containers/Vector.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/String/StringView.hpp>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Core/Types.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>

export module Cryptix:VFS.ProcFs.ProcFsINode;

export import :VFS.INode;
import :Scheduler.Process;

export class ProcFsINode;
export struct ProcFsProperty
{
    using ProcFsGenPropertyFunc = Delegate<void(String&)>;
    Delegate<void(ProcFsProperty&)> GenProp;
    ProcFsINode*                    m_Parent = nullptr;
    String                          Buffer;
    usize                           Offset = 0;

    Spinlock                        Lock;

    ProcFsProperty()
    {
        GenProp.BindLambda([](ProcFsProperty&) {});
    }
    virtual ~ProcFsProperty() = default;

    template <typename... Args>
    void Write(fmt::format_string<Args...> format, Args&&... args)
    {
        if (Offset < Buffer.Size())
        {
            auto result = fmt::format_to_n(Buffer.Raw() + Offset,
                                           Buffer.Size() - Offset, format,
                                           Forward<Args>(args)...);
            if (Offset + result.size < Buffer.Size())
            {
                Offset += result.size;
                return;
            }
        }

        Offset = Buffer.Size();
    }
    isize Read(u8* outBuffer, off_t offset, usize count);

    template <typename F>
    explicit ProcFsProperty(F f)
    {
        GenProp.BindLambda(f);
    }

    virtual void GenerateRecord()
    {
        Buffer.Clear();
        GenProp(*this);
    }
    operator String&()
    {
        if (Buffer.Empty()) GenerateRecord();
        Buffer.ShrinkToFit();
        return Buffer;
    }
};

export class ProcFsINode : public INode
{
  public:
    ProcFsINode(StringView name, class Filesystem* fs, INodeID id,
                INodeMode mode, ProcFsProperty* property = nullptr);
    ProcFsINode(StringView name, class Filesystem* fs, INodeMode mode,
                ProcFsProperty* property = nullptr);
    virtual ~ProcFsINode() override
    {
        if (m_Property) delete m_Property;
    }

    virtual ErrorOr<void>
    TraverseDirectories(::Ref<class DirectoryEntry> parent,
                        DirectoryIterator           iterator) override;
    virtual ErrorOr<::Ref<DirectoryEntry>>
                  Lookup(::Ref<DirectoryEntry> dentry) override;

    virtual void  InsertChild(::Ref<INode> node, StringView name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Truncate(usize size) override;

    friend class ProcFs;

    virtual bool Populate() { return false; }

  protected:
    ProcFsProperty*                    m_Property = nullptr;
    UnorderedMap<String, ::Ref<INode>> m_Children;
    bool                               m_Populated = false;
};
export class Process;
export class ProcFsRootINode : public ProcFsINode
{
  public:
    ProcFsRootINode(StringView name, class Filesystem* fs);

    virtual bool Populate() override;
};
export class ProcFsSelfLinkINode : public ProcFsINode
{
  public:
    ProcFsSelfLinkINode(StringView name, class Filesystem* fs);
    virtual ErrorOr<Path> ReadLink() override;
};
export class ProcFsProcessINode : public ProcFsINode
{
  public:
    ProcFsProcessINode(StringView name, class Filesystem* fs, INodeID id,
                       INodeMode mode, ProcessID pid);

    virtual bool Populate() override;

  private:
    ProcessID m_ProcessID = -1;
};
export class ProcFsFdINode : public ProcFsINode
{
  public:
    ProcFsFdINode(StringView name, class Filesystem* fs, ProcessID pid);

    virtual bool Populate() override;

  private:
    ProcessID m_ProcessID = -1;
};

export class ProcFsSymlinkINode : public ProcFsINode
{
  public:
    ProcFsSymlinkINode(StringView name, class Filesystem* fs, INodeMode mode,
                       PathView target);

    virtual ErrorOr<Path> ReadLink() override;

  private:
    Path m_Target = ""_sv;
};
