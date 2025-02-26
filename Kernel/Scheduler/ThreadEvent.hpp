/*
 * Created by v1tr10l7 on 25.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>
#include <Arch/CPU.hpp>
#include <Prism/Types.hpp>

#include <deque>

enum class EventType
{
    eUndefined = 0,
    eWait,
};

class BaseEvent
{
  public:
    virtual ~BaseEvent()              = default;
    virtual EventType GetType() const = 0;

    Thread*           GetThread() { return m_Thread; }

  protected:
    BaseEvent()
        : m_Thread(CPU::GetCurrentThread())
    {
    }

  private:
    Thread* m_Thread = nullptr;
};

class WaitEvent : public BaseEvent
{
  public:
    enum class WaitReason
    {
        eTerminated = Bit(0),
        eStopped    = Bit(1),
        eContinued  = Bit(2),
        eDisowned   = Bit(3),
    };

    WaitEvent() = default;
    virtual EventType GetType() const override { return EventType::eWait; }

    struct Listener
    {
        enum WaitReason    WaitReason;
        class Process*     Waitee;
        Thread*            WaitingThread;
        std::optional<i32> Status = std::nullopt;
        pid_t              Pid    = 0;

        bool Dispatch(enum WaitReason waitReason, i32 status, pid_t pid)
        {
            Pid    = pid;
            Status = status;
            return true;
        }
    };

    void AddListener(Listener& listener) { m_Listeners.push_back(listener); }
    void Dispatch(WaitReason waitReason, i32 status, pid_t pid)
    {
        LogInfo("Dispatching!");
        for (auto it = m_Listeners.begin(); it < m_Listeners.end(); it++)
        {
            if (true || it->Dispatch(waitReason, status, pid))
            {
                it = m_Listeners.erase(it);
                // Scheduler::Unblock(it->WaitingThread);
                break;
            }
        }
    }

  private:
    std::deque<Listener> m_Listeners;
};
