#pragma once

#include <Prism/Types.hpp>

#include <cassert>
#include <concepts>
#include <utility>

namespace Prism
{
    // FIXME: Make non movable
    template <std::signed_integral K>
    class BaseRedBlackTree //: public NonCopyable<BaseRedBlackTree<K>>
    {

      public:
        [[nodiscard]]
        usize GetSize() const
        {
            return m_Size;
        }
        [[nodiscard]]
        bool IsEmpty() const
        {
            return m_Size == 0;
        }

        enum class Color : bool
        {
            eRed,
            eBlack
        };
        struct Node
        {
            Node* Parent     = nullptr;
            Node* LeftChild  = nullptr;
            Node* RightChild = nullptr;

            Color Color      = Color::eRed;
            K     Key;

            Node(K key)
                : Key(key)
            {
            }
            Node() {}
            virtual ~Node(){};
        };

      protected:
        BaseRedBlackTree()
            = default; // These are protected to ensure no one instantiates the
                       // leaky base red black tree directly
        virtual ~BaseRedBlackTree() = default;

        void RotateLeft(Node* subtreeRoot)
        {
            assert(subtreeRoot);
            auto* pivot = subtreeRoot->RightChild;
            assert(pivot);
            auto* parent            = subtreeRoot->Parent;

            // stage 1 - SubtreeRoot's right child is now pivot's left child
            subtreeRoot->RightChild = pivot->LeftChild;
            if (subtreeRoot->RightChild)
                subtreeRoot->RightChild->Parent = subtreeRoot;

            // stage 2 - pivot's left child is now SubtreeRoot
            pivot->LeftChild    = subtreeRoot;
            subtreeRoot->Parent = pivot;

            // stage 3 - update pivot's parent
            pivot->Parent       = parent;
            if (!parent) m_Root = pivot;
            else if (parent->LeftChild == subtreeRoot)
                parent->LeftChild = pivot;
            else parent->RightChild = pivot;
        }

        void RotateRight(Node* subtreeRoot)
        {
            assert(subtreeRoot);
            auto* pivot = subtreeRoot->LeftChild;
            assert(pivot);
            auto* parent           = subtreeRoot->Parent;

            // stage 1 - SubtreeRoot's left child is now pivot's right child
            subtreeRoot->LeftChild = pivot->RightChild;
            if (subtreeRoot->LeftChild)
                subtreeRoot->LeftChild->Parent = subtreeRoot;

            // stage 2 - pivot's right child is now SubtreeRoot
            pivot->RightChild   = subtreeRoot;
            subtreeRoot->Parent = pivot;

            // stage 3 - update pivot's parent
            pivot->Parent       = parent;
            if (!parent) m_Root = pivot;
            else if (parent->LeftChild == subtreeRoot)
                parent->LeftChild = pivot;
            else parent->RightChild = pivot;
        }

        static Node* Find(Node* node, K key)
        {
            while (node && node->Key != key)
            {
                if (key < node->Key) node = node->LeftChild;
                else node = node->RightChild;
            }
            return node;
        }

        static Node* FindLargestNotAbove(Node* node, K key)
        {
            Node* candidate = nullptr;
            while (node)
            {
                if (key == node->Key) return node;
                if (key < node->Key) node = node->LeftChild;
                else
                {
                    candidate = node;
                    node      = node->RightChild;
                }
            }
            return candidate;
        }

        static Node* FindSmallestNotBelow(Node* node, K key)
        {
            Node* candidate = nullptr;
            while (node)
            {
                if (node->Key == key) return node;

                if (node->Key <= key) node = node->RightChild;
                else
                {
                    candidate = node;
                    node      = node->LeftChild;
                }
            }
            return candidate;
        }

        void Insert(Node* node)
        {
            assert(node);
            Node* parent = nullptr;
            Node* temp   = m_Root;
            while (temp)
            {
                parent = temp;
                if (node->Key < temp->Key) temp = temp->LeftChild;
                else temp = temp->RightChild;
            }
            if (!parent)
            { // new root
                node->Color = Color::eBlack;
                m_Root      = node;
                m_Size      = 1;
                m_Minimum   = node;
                return;
            }
            if (node->Key < parent->Key) // we are the left child
                parent->LeftChild = node;
            else // we are the right child
                parent->RightChild = node;
            node->Parent = parent;

            if (node->Parent
                    ->Parent) // no fixups to be done for a height <= 2 tree
                FixInsert(node);

            m_Size++;
            if (m_Minimum->LeftChild == node) m_Minimum = node;
        }

        void FixInsert(Node* node)
        {
            assert(node && node->Color == Color::eRed);
            while (node->Parent && node->Parent->Color == Color::eRed)
            {
                auto* grandParent = node->Parent->Parent;
                if (grandParent->RightChild == node->Parent)
                {
                    auto* uncle = grandParent->LeftChild;
                    if (uncle && uncle->Color == Color::eRed)
                    {
                        node->Parent->Color = Color::eBlack;
                        uncle->Color        = Color::eBlack;
                        grandParent->Color  = Color::eRed;
                        node                = grandParent;
                    }
                    else
                    {
                        if (node->Parent->LeftChild == node)
                        {
                            node = node->Parent;
                            RotateRight(node);
                        }
                        node->Parent->Color = Color::eBlack;
                        grandParent->Color  = Color::eRed;
                        RotateLeft(grandParent);
                    }
                }
                else
                {
                    auto* uncle = grandParent->RightChild;
                    if (uncle && uncle->Color == Color::eRed)
                    {
                        node->parent->Color = Color::eBlack;
                        uncle->Color        = Color::eBlack;
                        grandParent->Color  = Color::eRed;
                        node                = grandParent;
                    }
                    else
                    {
                        if (node->Parent->RightChild == node)
                        {
                            node = node->Parent;
                            RotateLeft(node);
                        }
                        node->Parent->Color = Color::eBlack;
                        grandParent->Color  = Color::eRed;
                        RotateRight(grandParent);
                    }
                }
            }
            m_Root->Color = Color::eBlack; // the root should always be black
        }

        void Remove(Node* node)
        {
            assert(node);

            // special case: deleting the only node
            if (m_Size == 1)
            {
                m_Root    = nullptr;
                m_Minimum = nullptr;
                m_Size    = 0;
                return;
            }

            if (m_Minimum == node) m_Minimum = Successor(node);

            // removal assumes the node has 0 or 1 child, so if we have 2,
            // relink with the successor first (by definition the successor has
            // no left child)
            // FIXME: since we dont know how a value is represented in the node,
            // we can't simply swap the values and keys, and instead we relink
            // the nodes
            //  in place, this is quite a bit more expensive, as well as much
            //  less readable, is there a better way?
            if (node->LeftChild && node->RightChild)
            {
                auto* successorNode = Successor(
                    node); // this is always non-null as all nodes besides the
                           // maximum node have a successor, and the maximum
                           // node has no right child
                auto neighborSwap       = successorNode->Parent == node;
                node->LeftChild->Parent = successorNode;
                if (!neighborSwap) node->RightChild->Parent = successorNode;
                if (node->Parent)
                {
                    if (node->Parent->LeftChild == node)
                        node->Parent->LeftChild = successorNode;
                    else node->Parent->RightChild = successorNode;
                }
                else m_Root = successorNode;
                if (successorNode->RightChild)
                    successorNode->RightChild->Parent = node;
                if (neighborSwap)
                {
                    successorNode->Parent = node->Parent;
                    node->Parent          = successorNode;
                }
                else
                {
                    if (successorNode->Parent)
                    {
                        if (successorNode->Parent->LeftChild == successorNode)
                            successorNode->Parent->LeftChild = node;
                        else successorNode->Parent->RightChild = node;
                    }
                    else m_Root = node;
                    std::swap(node->Parent, successorNode->Parent);
                }
                std::swap(node->LeftChild, successorNode->LeftChild);
                if (neighborSwap)
                {
                    node->RightChild          = successorNode->RightChild;
                    successorNode->RightChild = node;
                }
                else std::swap(node->RightChild, successorNode->RightChild);
                std::swap(node->Color, successorNode->Color);
            }

            auto* child = node->LeftChild ?: node->RightChild;

            if (child) child->Parent = node->Parent;
            if (node->Parent)
            {
                if (node->Parent->LeftChild == node)
                    node->Parent->LeftChild = child;
                else node->Parent->RightChild = child;
            }
            else m_Root = child;

            // if the node is red then child must be black, and just replacing
            // the node with its child should result in a valid tree (no change
            // to black height)
            if (node->Color != Color::eRed) FixRemove(child, node->Parent);

            m_Size--;
        }

        // We maintain parent as a separate argument since node might be null
        void FixRemove(Node* node, Node* parent)
        {
            while (node != m_Root && (!node || node->Color == Color::eBlack))
            {
                if (parent->LeftChild == node)
                {
                    auto* sibling = parent->RightChild;
                    if (sibling->Color == Color::eRed)
                    {
                        sibling->Color = Color::eBlack;
                        parent->Color  = Color::eRed;
                        RotateLeft(parent);
                        sibling = parent->RightChild;
                    }
                    if ((!sibling->LeftChild
                         || sibling->LeftChild->Color == Color::eBlack)
                        && (!sibling->RightChild
                            || sibling->RightChild->Color == Color::eBlack))
                    {
                        sibling->Color = Color::eRed;
                        node           = parent;
                    }
                    else
                    {
                        if (!sibling->RightChild
                            || sibling->RightChild->Color == Color::eBlack)
                        {
                            sibling->LeftChild->Color
                                = Color::eBlack; // null check?
                            sibling->Color = Color::eRed;
                            RotateRight(sibling);
                            sibling = parent->RightChild;
                        }
                        sibling->Color = parent->Color;
                        parent->Color  = Color::eBlack;
                        sibling->RightChild->Color
                            = Color::eBlack; // null check?
                        RotateLeft(parent);
                        node = m_Root; // fixed
                    }
                }
                else
                {
                    auto* sibling = parent->LeftChild;
                    if (sibling->Color == Color::eRed)
                    {
                        sibling->Color = Color::eBlack;
                        parent->Color  = Color::eRed;
                        RotateRight(parent);
                        sibling = parent->LeftChild;
                    }
                    if ((!sibling->LeftChild
                         || sibling->LeftChild->Color == Color::eBlack)
                        && (!sibling->RightChild
                            || sibling->RightChild->Color == Color::eBlack))
                    {
                        sibling->Color = Color::eRed;
                        node           = parent;
                    }
                    else
                    {
                        if (!sibling->LeftChild
                            || sibling->LeftChild->Color == Color::eBlack)
                        {
                            sibling->RightChild->Color
                                = Color::eBlack; // null check?
                            sibling->Color = Color::eRed;
                            RotateLeft(sibling);
                            sibling = parent->LeftChild;
                        }
                        sibling->Color = parent->Color;
                        parent->Color  = Color::eBlack;
                        sibling->LeftChild->Color
                            = Color::eBlack; // null check?
                        RotateRight(parent);
                        node = m_Root; // fixed
                    }
                }
                parent = node->Parent;
            }
            node->Color = Color::eBlack; // by this point node can't be null
        }

        static Node* Successor(Node* node)
        {
            assert(node);
            if (node->RightChild)
            {
                node = node->RightChild;
                while (node->LeftChild) node = node->LeftChild;
                return node;
            }
            auto temp = node->Parent;
            while (temp && node == temp->RightChild)
            {
                node = temp;
                temp = temp->Parent;
            }
            return temp;
        }

        static Node* Predecessor(Node* node)
        {
            assert(node);
            if (node->LeftChild)
            {
                node = node->LeftChild;
                while (node->RightChild) node = node->RightChild;
                return node;
            }
            auto temp = node->Parent;
            while (temp && node == temp->LeftChild)
            {
                node = temp;
                temp = temp->Parent;
            }
            return temp;
        }

        Node* m_Root    = nullptr;
        usize m_Size    = 0;
        Node* m_Minimum = nullptr; // maintained for O(1) begin()
    };

    template <typename TreeType, typename ElementType>
    class RedBlackTreeIterator
    {
      public:
        RedBlackTreeIterator() = default;
        bool operator!=(RedBlackTreeIterator const& other) const
        {
            return m_Node != other.m_Node;
        }
        RedBlackTreeIterator& operator++()
        {
            if (!m_Node) return *this;
            m_Prev = m_Node;
            // the complexity is O(logn) for each successor call, but the total
            // complexity for all elements comes out to O(n), meaning the
            // amortized cost for a single call is O(1)
            m_Node = static_cast<typename TreeType::Node*>(
                TreeType::Successor(m_Node));
            return *this;
        }
        RedBlackTreeIterator& operator--()
        {
            if (!m_Prev) return *this;
            m_Node = m_Prev;
            m_Prev = static_cast<typename TreeType::Node*>(
                TreeType::Predecessor(m_Prev));
            return *this;
        }
        ElementType& operator*() { return m_Node->Value; }
        ElementType* operator->() { return &m_Node->Value; }
        [[nodiscard]]
        bool IsEnd() const
        {
            return !m_Node;
        }
        [[nodiscard]]
        bool IsBegin() const
        {
            return !m_Prev;
        }

        [[nodiscard]]
        auto GetKey() const
        {
            return m_Node->Key;
        }

      private:
        friend TreeType;
        explicit RedBlackTreeIterator(typename TreeType::Node* node,
                                      typename TreeType::Node* prev = nullptr)
            : m_Node(node)
            , m_Prev(prev)
        {
        }
        typename TreeType::Node* m_Node = nullptr;
        typename TreeType::Node* m_Prev = nullptr;
    };

    template <std::signed_integral K, typename V>
    class RedBlackTree final : public BaseRedBlackTree<K>
    {
      public:
        RedBlackTree() = default;
        virtual ~RedBlackTree() override { Clear(); }

        using BaseTree = BaseRedBlackTree<K>;

        [[nodiscard]]
        V* Find(K key)
        {
            auto* node = static_cast<Node*>(BaseTree::Find(m_Root, key));
            if (!node) return nullptr;
            return &node->Value;
        }

        [[nodiscard]]
        V* FindLargestNotAbove(K key)
        {
            auto* node = static_cast<Node*>(
                BaseTree::FindSmallestNotBelow(m_Root, key));
            if (!node) return nullptr;
            return &node->Value;
        }

        [[nodiscard]]
        V* FindSmallestNotBelow(K key)
        {
            auto* node = static_cast<Node*>(
                BaseTree::FindSmallestNotBelow(m_Root, key));
            if (!node) return nullptr;
            return &node->Value;
        }

        ErrorOr<void> TryInsert(K key, V const& value)
        {
            return TryInsert(key, V(value));
        }

        void          Insert(K key, V const& value) { TryInsert(key, value); }

        ErrorOr<void> TryInsert(K key, V&& value)
        {
            auto* node = new Node(key, std::move(value));
            if (!node) return Error(ENOMEM);
            BaseTree::Insert(node);
            return {};
        }

        void Insert(K key, V&& value) { tryInsert(key, std::move(value)); }

        using Iterator = RedBlackTreeIterator<RedBlackTree, V>;
        friend Iterator;
        Iterator begin() { return Iterator(static_cast<Node*>(m_Minimum)); }
        Iterator end() { return {}; }
        Iterator BeginFrom(K key)
        {
            return Iterator(static_cast<Node*>(BaseTree::Find(m_Root, key)));
        }

        using ConstIterator = RedBlackTreeIterator<RedBlackTree const, V const>;
        friend ConstIterator;
        ConstIterator begin() const
        {
            return ConstIterator(static_cast<Node*>(m_Minimum));
        }
        ConstIterator end() const { return {}; }
        ConstIterator BeginFrom(K key) const
        {
            return ConstIterator(
                static_cast<Node*>(BaseTree::Find(m_Root, key)));
        }

        ConstIterator FindLargestNotAboveIterator(K key) const
        {
            auto node = static_cast<Node*>(
                BaseTree::FindLargestNotAbove(m_Root, key));
            if (!node) return end();
            return ConstIterator(
                node, static_cast<Node*>(BaseTree::Predecessor(node)));
        }

        ConstIterator FindSmallestNotBelow(K key) const
        {
            auto node = static_cast<Node*>(
                BaseTree::find_smallest_not_below(m_Root, key));
            if (!node) return end();
            return ConstIterator(
                node, static_cast<Node*>(BaseTree::Predecessor(node)));
        }

        V RemoveUnsafe(K key)
        {
            auto* node = BaseTree::Find(m_Root, key);
            assert(node);

            BaseTree::remove(node);

            V temp           = std::move(static_cast<Node*>(node)->Value);

            node->RightChild = nullptr;
            node->LeftChild  = nullptr;
            delete node;

            return temp;
        }

        bool Remove(K key)
        {
            auto* node = BaseTree::Find(m_Root, key);
            if (!node) return false;

            BaseTree::Remove(node);

            node->RightChild = nullptr;
            node->LeftChild  = nullptr;
            delete node;

            return true;
        }

        void Clear()
        {
            delete m_Root;
            m_Root    = nullptr;
            m_Minimum = nullptr;
            m_Size    = 0;
        }

      private:
        struct Node : BaseRedBlackTree<K>::Node
        {

            V Value;

            Node(K key, V value)
                : BaseRedBlackTree<K>::Node(key)
                , Value(std::move(value))
            {
            }

            ~Node()
            {
                delete LeftChild;
                delete RightChild;
            }
        };
    };
} // namespace Prism
