/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Spinlock.hpp>

#include <Memory/PMM.hpp>
#include <Memory/PageFault.hpp>

#include <Prism/Utility/Math.hpp>

enum class PageAttributes : isize
{
    eRead              = Bit(0),
    eWrite             = Bit(1),
    eExecutable        = Bit(2),
    eUser              = Bit(3),
    eGlobal            = Bit(4),
    eLPage             = Bit(5),
    eLLPage            = Bit(6),

    eRW                = eRead | eWrite,
    eRWX               = eRW | eExecutable,
    eRWXU              = eRWX | eUser,

    eUncacheableStrong = Bit(8),
    eWriteCombining    = Bit(7) | Bit(7),
    eWriteThrough      = Bit(9),
    eWriteProtected    = Bit(9) | Bit(7),
    eWriteBack         = Bit(9) | Bit(8),
    eUncacheable       = Bit(7) | Bit(8) | Bit(9),
};
inline bool operator!(const PageAttributes& value)
{
    return !static_cast<usize>(value);
}
inline PageAttributes operator~(const PageAttributes& value)
{
    return static_cast<PageAttributes>(~static_cast<int>(value));
}

inline PageAttributes operator|(const PageAttributes& left,
                                const PageAttributes& right)
{
    return static_cast<PageAttributes>(static_cast<int>(left)
                                       | static_cast<int>(right));
}

inline PageAttributes& operator|=(PageAttributes&       left,
                                  const PageAttributes& right)
{
    return left = left | right;
}

inline isize operator&(const PageAttributes& left, const PageAttributes& right)
{
    return static_cast<isize>(static_cast<isize>(left)
                              & static_cast<isize>(right));
}

inline PageAttributes& operator&=(PageAttributes&       left,
                                  const PageAttributes& right)
{
    return left = static_cast<PageAttributes>(left & right);
}

inline PageAttributes operator^(const PageAttributes& left,
                                const PageAttributes& right)
{
    return static_cast<PageAttributes>(static_cast<int>(left)
                                       ^ static_cast<int>(right));
}

inline PageAttributes& operator^=(PageAttributes&       left,
                                  const PageAttributes& right)
{
    return left = left ^ right;
}

template <typename Address>
inline static constexpr bool IsHigherHalfAddress(Address addr)
{
    return reinterpret_cast<uintptr_t>(addr) >= BootInfo::GetHHDMOffset();
}

template <typename T, typename U>
inline static constexpr T ToHigherHalfAddress(U addr)
{
    T ret = IsHigherHalfAddress(addr)
              ? reinterpret_cast<T>(addr)
              : reinterpret_cast<T>(reinterpret_cast<uintptr_t>(addr)
                                    + BootInfo::GetHHDMOffset());
    return ret;
}

template <typename T, typename U>
inline static constexpr T FromHigherHalfAddress(U addr)
{
    T ret = !IsHigherHalfAddress(addr)
              ? reinterpret_cast<T>(addr)
              : reinterpret_cast<T>(reinterpret_cast<uintptr_t>(addr)
                                    - BootInfo::GetHHDMOffset());
    return ret;
}

class PageMap;
namespace Arch::VMM
{
    extern uintptr_t      g_DefaultPteFlags;

    extern void           Initialize();

    extern void*          AllocatePageTable();
    void                  DestroyPageMap(PageMap* pageMap);

    extern PageAttributes FromNativeFlags(usize flags);
    extern usize          ToNativeFlags(PageAttributes flags);

    extern uintptr_t      GetAddressMask();
    extern usize          GetPageSize(PageAttributes flags
                                      = static_cast<PageAttributes>(0));
} // namespace Arch::VMM

struct PageTable;
class PageTableEntry final
{
  public:
    PageTableEntry() = default;

    inline uintptr_t GetAddress() const
    {
        return value & Arch::VMM::GetAddressMask();
    }
    inline bool      GetFlag(uintptr_t flag) const { return value & flag; }
    inline uintptr_t GetFlags() const
    {
        return value & ~Arch::VMM::GetAddressMask();
    }

    inline void SetAddress(uintptr_t address)
    {
        value &= ~Arch::VMM::GetAddressMask();
        value |= address;
    }
    inline void SetFlags(uintptr_t flags, bool enabled)
    {
        if (enabled) value |= flags;
        else value &= ~flags;
    }

    inline void Clear() { value = 0; }

    bool        IsValid();
    bool        IsLarge();

  private:
    uintptr_t value;
};

namespace VirtualMemoryManager
{
    void     Initialize();

    usize    GetHigherHalfOffset();

    Pointer  AllocateSpace(usize increment = 0, usize alignment = 0,
                           bool lowerHalf = false);

    PageMap* GetKernelPageMap();
    void     HandlePageFault(const PageFaultInfo& info);

    void     SaveCurrentPageMap(PageMap& out);
    void     LoadPageMap(PageMap& pageMap, bool);

    Pointer  MapIoRegion(PhysAddr phys, usize size, bool write = true);
    Pointer  MapIoRegion(PhysAddr phys, usize size, bool write,
                         usize alignment = 0);
    template <typename T>
    inline T* MapIoRegion(PhysAddr phys, bool write = true,
                          usize alignment = alignof(T))
    {
        return MapIoRegion(phys, sizeof(T) + alignment, write, alignment)
            .As<T>();
    }
} // namespace VirtualMemoryManager

namespace VMM = VirtualMemoryManager;

class PageMap
{
  public:
    PageMap();
    PageMap(uintptr_t topLevel)
        : topLevel(reinterpret_cast<PageTable*>(topLevel))
    {
    }

    void operator=(uintptr_t topLevel)
    {
        this->topLevel = reinterpret_cast<PageTable*>(topLevel);
    }

    [[nodiscard]] inline bool       Exists() const { return topLevel; }
    [[nodiscard]] inline PageTable* GetTopLevel() const { return topLevel; }

    inline PageAttributes           GetPageSizeFlags(usize pageSize) const
    {
        if (pageSize == Arch::VMM::GetPageSize(PageAttributes::eLPage))
            return PageAttributes::eLPage;
        if (pageSize == Arch::VMM::GetPageSize(PageAttributes::eLLPage))
            return PageAttributes::eLLPage;

        return static_cast<PageAttributes>(0);
    }

    std::pair<usize, PageAttributes> RequiredSize(usize size) const
    {
        usize lPageSize  = Arch::VMM::GetPageSize(PageAttributes::eLPage);
        usize llPageSize = Arch::VMM::GetPageSize(PageAttributes::eLLPage);

        if (size >= lPageSize) return {llPageSize, PageAttributes::eLLPage};
        else if (size >= lPageSize) return {lPageSize, PageAttributes::eLPage};

        return {pageSize, static_cast<PageAttributes>(0)};
    }

    void*           GetNextLevel(PageTableEntry& entry, bool allocate,
                                 uintptr_t virt = -1);

    PageTableEntry* Virt2Pte(PageTable* topLevel, uintptr_t virt, bool allocate,
                             u64 pageSize);
    uintptr_t       Virt2Phys(uintptr_t      virt,
                              PageAttributes flags = PageAttributes::eRead);

    bool            InternalMap(uintptr_t virt, uintptr_t phys,
                                PageAttributes flags
                                = PageAttributes::eRW | PageAttributes::eWriteBack);

    bool            InternalUnmap(uintptr_t      virt,
                                  PageAttributes flags = static_cast<PageAttributes>(0));

    template <typename T>
    inline ErrorOr<T*> MapIoRegion(PM::Pointer phys)
    {
        usize       length = Math::AlignUp(sizeof(T), PMM::PAGE_SIZE);

        PM::Pointer virt   = VMM::AllocateSpace(length, alignof(u64), true);
        phys               = Math::AlignDown(phys, PMM::PAGE_SIZE);

        if (MapRange(virt, phys, length,
                     PageAttributes::eRW | PageAttributes::eUncacheableStrong))
            return virt.As<T>();
        return Error(ENODEV);
    }

    bool Map(uintptr_t virt, uintptr_t phys,
             PageAttributes flags
             = PageAttributes::eRW | PageAttributes::eWriteBack)
    {
        ScopedLock guard(m_Lock);
        return InternalMap(virt, phys, flags);
    }

    bool Unmap(uintptr_t      virt,
               PageAttributes flags = static_cast<PageAttributes>(0))
    {
        ScopedLock guard(m_Lock);
        return InternalUnmap(virt, flags);
    }

    bool Remap(uintptr_t virtOld, uintptr_t virtNew,
               PageAttributes flags
               = PageAttributes::eRW | PageAttributes::eWriteBack)
    {
        uintptr_t phys = Virt2Phys(virtOld, flags);
        Unmap(virtOld, flags);

        return Map(virtNew, phys, flags);
    }

    bool MapRange(uintptr_t virt, uintptr_t phys, usize size,
                  PageAttributes flags
                  = PageAttributes::eRW | PageAttributes::eWriteBack)
    {
        usize pageSize = Arch::VMM::GetPageSize(flags);
        for (usize i = 0; i < size; i += pageSize)
        {
            if (!Map(virt + i, phys + i, flags))
            {
                UnmapRange(virt, i - pageSize);
                return false;
            }
        }
        return true;
    }

    bool UnmapRange(uintptr_t virt, usize size,
                    PageAttributes flags = static_cast<PageAttributes>(0))
    {
        usize pageSize = Arch::VMM::GetPageSize(flags);
        for (usize i = 0; i < size; i += pageSize)
            if (!Unmap(virt + i, flags)) return false;

        return true;
    }

    bool RemapRange(uintptr_t virtOld, uintptr_t virtNew, usize size,
                    PageAttributes flags
                    = PageAttributes::eRW | PageAttributes::eWriteBack)
    {
        usize pageSize = Arch::VMM::GetPageSize(flags);
        for (usize i = 0; i < size; i += pageSize)
            if (!Remap(virtOld + i, virtNew + i, flags)) return false;

        return true;
    }

    bool SetFlagsRange(uintptr_t virt, usize size,

                       PageAttributes flags
                       = PageAttributes::eRW | PageAttributes::eWriteBack)
    {
        usize pageSize = Arch::VMM::GetPageSize(flags);
        for (usize i = 0; i < size; i += pageSize)
            if (!SetFlags(virt + i, flags)) return false;

        return true;
    }
    bool        SetFlags(uintptr_t      virt,
                         PageAttributes flags
                         = PageAttributes::eRW | PageAttributes::eWriteBack);

    inline void Load() { VirtualMemoryManager::LoadPageMap(*this, true); }

  private:
    PageTable* topLevel = 0;
    Spinlock   m_Lock;

    usize      pageSize = 0;
};
