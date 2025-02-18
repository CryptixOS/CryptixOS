/*
 * Created by v1tr10l7 on 18.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Utility/Logger.hpp>
#include <Utility/Types.hpp>

#include <magic_enum/magic_enum.hpp>
#include <utility>

template <typename K, typename T>
class RedBlackTree
{
  public:
    RedBlackTree() = default;
    ~RedBlackTree() { Clear(); }

    enum class Color : bool
    {
        eRed,
        eBlack,
    };
    struct Node
    {
        Node* Parent     = nullptr;
        Node* LeftChild  = nullptr;
        Node* RightChild = nullptr;

        Color Color      = Color::eRed;
        K     Key;

        Node() = default;
        Node(K key)
            : Key(key)
        {
        }
    };

    constexpr bool  IsEmpty() const { return m_Size == 0; }
    constexpr usize GetSize() const { return m_Size; }

    void            Clear();
    inline void     Insert(K key, const T& value)
    {
        auto* node = new Node(key, std::move(value));
        assert(node);

        Node* parent  = nullptr;
        Node* current = m_Root;
        while (current)
        {
            parent  = current;
            current = node->Key < current->Key ? current->LeftChild
                                               : current->RightChild;
        }

        if (!parent)
        {
            node->Color = Color::eBlack;
            m_Root      = node;
            m_Size      = 1;
            m_Least     = node;
            return;
        }

        if (node->Key < parent->Key) parent->LeftChild = node;
        else parent->RightChild = node;
        node->Parent = parent;

        // TODO(v1tr10l7): What about some insert-fixup?

        ++m_Size;
        if (m_Least->LeftChild == node) m_Least = node;
    }

    Node* Find(Node* node, K key)
    {
        while (node && node->Key != key)
        {
            if (key < node->Key) node = node->LeftChild;
            else node = node->RightChild;
        }

        return node;
    }

    void PrintNode(Node* root, std::string indent, bool last)
    {
        if (!root) return;

        LogMessage("{}", indent);
        if (last)
        {
            LogMessage("R----");
            indent += "   ";
        }
        else
        {
            LogMessage("L----");
            indent += "|  ";
        }

        LogMessage("{}({})\n", root->Value, magic_enum::enum_name(root->Color));
        PrintNode(root->LeftChild, indent, false);
        PrintNode(root->RightChild, indent, true);
    }
    void Print()
    {
        if (!m_Root) return;
        LogDebug("Dumping RedBlackTree...");
        PrintNode(m_Root, "", true);
    }

  private:
    Node* m_Root  = nullptr;
    Node* m_Least = nullptr;
    usize m_Size  = 0;
};
