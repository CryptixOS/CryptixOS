/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Memory/Pointer.hpp>
#include <Prism/Core/Types.hpp>

namespace NVMe
{
    namespace OpCode
    {

        constexpr usize ADMIN_CREATE_SQ = 0x01;
        constexpr usize ADMIN_DELETE_CQ = 0x04;
        constexpr usize ADMIN_CREATE_CQ = 0x05;
        constexpr usize ADMIN_IDENTIFY  = 0x06;
        constexpr usize ADMIN_ABORT     = 0x08;
        constexpr usize ADMIN_SETFT     = 0x09;
        constexpr usize ADMIN_GETFT     = 0x0a;

        constexpr usize IO_FLUSH        = 0x00;
        constexpr usize IO_WRITE        = 0x01;
        constexpr usize IO_READ         = 0x02;
    }; // namespace OpCode

    struct ReadWrite
    {
        u64 SLba;
        u16 Len;
        u16 Control;
        u32 Dsmgmt;
        u32 Ref;
        u16 AppTag;
        u16 AppMask;
    };
    struct Identify
    {
        u32 Cns;
        u32 Unused2[5];
    };
    struct Features
    {
        u32 Fid;
        u32 Dword;
        u64 Unused2[2];
    };
    struct CreateCompletionQueue
    {
        u16 CompleteQueueID;
        u16 Size;
        u16 CompleteQueueFlags;
        u16 IrqVec;
        u64 Unused2[2];
    };
    struct CreateSubmissionQueue
    {
        u16 SubmitQueueID;
        u16 Size;
        u16 SubmitQueueFlags;
        u16 CompleteQueueID;
        u64 Unused3[2];
    };
    struct DeleteQueue
    {
        u16 QueueID;
        u16 Unused2;
        u32 Unused3[5];
    };
    struct Abort
    {
        u16 SubmitQueueID;
        u16 CompleteQueueID;
        u32 Unused2[5];
    };
    struct Submission
    {
        u8  OpCode;
        u8  Flags;
        u16 CompleteID;
        u32 NameSpaceID;
        u32 Cdw2[2];
        u64 Metadata;
        u64 Prp1;
        u64 Prp2;
        union
        {
            ReadWrite             ReadWrite;
            Identify              Identify;
            Features              Features;
            CreateCompletionQueue CreateCompletionQueue;
            CreateSubmissionQueue CreateSubmissionQueue;
            DeleteQueue           DeleteQueue;
            Abort                 Abort;
        };
    } __attribute__((packed));
    struct Completion
    {
        u32 Result;
        u32 Reserved;
        u16 SubmitQueueHead;
        u16 SubmitQueueID;
        u16 CommandID;
        u16 Status;
    } __attribute__((packed));

    class NameSpace;
    class Queue
    {
      public:
        Queue() = default;
        Queue(PM::Pointer crAddress, u16 qid, u32 doorbellShift, u64 depth);
        Queue(PM::Pointer crAddress, NameSpace& ns, u16 qid, u32 doorbellShift,
              u64 depth);

        inline volatile Submission* GetSubmit() const { return m_Submit; }
        inline volatile Completion* GetComplete() const { return m_Complete; }

        inline u32                  GetCommandID() const { return m_CmdId; }
        inline u64*                 GetPhysRegPages() { return m_PhysRegPgs; }

        u16                         AwaitSubmit(Submission* submission);
        void                        Submit(Submission* cmd);

      private:
        volatile Submission* m_Submit;
        volatile Completion* m_Complete;
        volatile u32*        m_SubmitDoorbell;
        volatile u32*        m_CompleteDoorbell;
        u16                  m_Depth;
        u16                  m_CompleteVec;
        u16                  m_SubmitHead;
        u16                  m_SubmitTail;
        u16                  m_CompleteHead;
        u8                   m_CompletePhase;
        u16                  m_ID;
        u32                  m_CmdId;
        u64*                 m_PhysRegPgs;
    };
}; // namespace NVMe
