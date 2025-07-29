/*
 * Created by v1tr10l7 on 17.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Module.hpp>
#include <VFS/Filesystem.hpp>

/**
 * @brief Filesystem driver descriptor
 *
 * Represents a filesystem driver implementation that can mount, unmount,
 * and operate on specific filesystem types. Drivers should register
 * themselves via FilesystemRegistry::Register().
 */
struct FilesystemDriver : public RefCounted
{
    /// Signature of the filesystem instantiate function.
    /// @return A reference-counted Filesystem instance if successful.
    using InstantiateFn = ErrorOr<::Ref<Filesystem>> (*)();

    /// Signature of the filesystem destroy function.
    /// @param filesystem   The Filesystem instance to be destroyed.
    /// @return Empty on success, or an error code on failure.
    using DestroyFn     = ErrorOr<void> (*)(::Ref<Filesystem> filesystem);

    ///> The kernel module that provided this driver.
    Module*       Owner = nullptr;
    ///> Unique name identifying this filesystem type.
    StringView    FilesystemName = ""_sv;
    ///> Supported mount flags mask.
    isize         FlagsMask      = 0;
    ///> Pointer to instantiate implementation
    InstantiateFn Instantiate    = nullptr;
    ///> Pointer to destroy implementation.
    DestroyFn     Destroy        = nullptr;

    /** @name Internal linkage
     *  @{
     */
    friend class IntrusiveRefList<FilesystemDriver>;
    friend struct IntrusiveRefListHook<FilesystemDriver>;

    using List = IntrusiveRefList<FilesystemDriver>;
    ///> Internal hook for filesystem drivers intrusive list.
    IntrusiveRefListHook<FilesystemDriver> Hook;
    /// @}
};
