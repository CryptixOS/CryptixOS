/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Utility/Types.hpp>

namespace NVMe
{
    struct Command
    {
        u8  opcode;
        u8  flags;
        u16 commandID;
        u32 namespaceID;
        u32 cdw2[2];
        u64 metadata;
        u64 prp1;
        u64 prp2;
    };
    struct ReadWrite
    {
        u8  opcode;
        u8  flags;
        u16 cid;
        u32 nsid;
        u64 unused;
        u64 metadata;
        u64 prp1;
        u64 prp2;
        u64 slba;
        u16 len;
        u16 control;
        u32 dsmgmt;
        u32 ref;
        u16 apptag;
        u16 appmask;
    };
    struct Identify
    {
        u8  opcode;
        u8  flags;
        u16 cid;
        u32 nsid;
        u64 unused1;
        u64 unused2;
        u64 prp1;
        u64 prp2;
        u32 cns;
        u32 unused3[5];
    };
    struct Features
    {
        u8  opcode;
        u8  flags;
        u16 cid;
        u32 nsid;
        u64 unused1;
        u64 unused2;
        u64 prp1;
        u64 prp2;
        u32 fid;
        u32 dword;
        u64 unused[2];
    };
    struct CreateCompletionQueue
    {
        u8  opcode;
        u8  flags;
        u16 cid;
        u32 unused1[5];
        u64 prp1;
        u64 unused2;
        u16 cqid;
        u16 size;
        u16 cqflags;
        u16 irqvec;
        u64 unused3[2];
    };
    struct CreateSubmissionQueue
    {
        u8  opcode;
        u8  flags;
        u16 cid;
        u32 unused1[5];
        u64 prp1;
        u64 unused2;
        u16 sqid;
        u16 size;
        u16 sqflags;
        u16 cqid;
        u64 unused3[2];
    };
    struct DeleteQueue
    {
        u8  opcode;
        u8  flags;
        u16 cid;
        u32 unused1[9];
        u16 qid;
        u16 unused2;
        u32 unused3[5];
    };
    struct Abort
    {
        u8  opcode;
        u8  flags;
        u16 cid;
        u32 unused1[9];
        u16 sqid;
        u16 cqid;
        u32 unused2[5];
    };
    struct Submission
    {
        union
        {
            Command               Command;
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
        u16 SqHead;
        u16 SqID;
        u16 CommandID;
        u16 Status;
    } __attribute__((packed));

    class Queue
    {
      public:
        Queue() = default;
        Queue(volatile u32* submitDoorbell, volatile u32* completeDoorbell,
              u16 qid, u8 irq, u32 depth);

        inline volatile Submission* GetSubmit() const { return m_Submit; }
        inline volatile Completion* GetComplete() const { return m_Complete; }

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
