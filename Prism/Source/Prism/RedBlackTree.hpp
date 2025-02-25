/*
 * Created by v1tr10l7 on 25.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Debug/Log.hpp>
#include <Prism/NonCopyable.hpp>
#include <Prism/NonMovable.hpp>
#include <Prism/Types.hpp>

#include <magic_enum/magic_enum.hpp>
#include <utility>

namespace Prism
{
    template <typename K, typename V>
    class RedBlackTree : public NonCopyable<RedBlackTree<K, V>>,
                         public NonMovable<RedBlackTree<K, V>>
    {
      public:
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
            V     Value;

            Node() = default;
            Node(K& key, V& value)
                : Key(key)
                , Value(value)
            {
            }
            Node(K&& key, V&& value)
                : Key(std::move(key))
                , Value(std::move(value))
            {
            }
        };

        RedBlackTree() = default;
        ~RedBlackTree();

        V&              At(const K& key);
        const V&        At(const K& key) const;

        V&              operator[](const K& key);
        V&              operator[](K&& key);

        constexpr bool  IsEmpty() const { return m_Size == 0; }
        constexpr usize GetSize() const { return m_Size; }

        void            Clear();
        void            Insert(K& key, V& value);
        bool            Erase(const K& key);
        bool            Find(const K& key, V& value) const;

        bool            Contains(const K& key) const;
        template <typename T>
        bool        Contains(const K& k) const;

        inline void PrintTree();
        void        PrintNode(const std::string& prefix, const Node* node,
                              bool isLeft = false);

      private:
        Node* m_Root = nullptr;
        usize m_Size = 0;

        void  InsertFix(Node* node);
        i32   Compare(const K& lhs, const K& rhs) const;

        void  RotateLeft(Node* node);
        void  RotateRight(Node* node);
        void  RemoveNode(Node* node);

        void  RecursiveDelete(Node* node);
    };
}; // namespace Prism

#include <Prism/RedBlackTree.inl>
