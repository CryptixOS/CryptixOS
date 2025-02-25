/*
 * Created by v1tr10l7 on 25.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Prism/RedBlackTree.hpp> // NOLINT(misc-header-include-cycle)

namespace Prism
{
    template <typename K, typename V>
    RedBlackTree<K, V>::~RedBlackTree() { Clear(); }   

    template <typename K, typename V>
    V& RedBlackTree<K, V>::At(const K& key)
    {
        Node* current = m_Root;
        while (m_Root && m_Root->Key != key)
            current = key < current->Key ? current->LeftChild : current->RightChild;

        return current;
    }
    template <typename K, typename V>
    const V&        RedBlackTree<K, V>::At(const K& key) const
    {
        return At(key);
    }

    template <typename K, typename V>
    V&              RedBlackTree<K, V>::operator[](const K& key)
    {
        return At(key);
    }
    template <typename K, typename V>
    V&              RedBlackTree<K, V>::operator[](K&& key)
    {
        return At(key);
    }

    template <typename K, typename V>
    void RedBlackTree<K, V>::Clear()
    {
        RecursiveDelete(m_Root);
    }
    template <typename K, typename V>
    void RedBlackTree<K, V>::Insert(K& key, V& value)
    {
        auto* node = new Node(key, value);
        
        if (!m_Root)
        { 
            m_Root = node;
            m_Root->Color = Color::eBlack;
            return;
        }
        
        Node* current = m_Root;
        while (current->LeftChild || current->RightChild)
        {
            if (Compare(key, current->Key) == -1) current = current->LeftChild;
            else current = current->RightChild;
        }

        node->Parent = current;
        if (Compare(key, current->Key) == -1) current->LeftChild = node;
        else current->RightChild = node;
        
        InsertFix(node);
        
        ++m_Size;
    }
    template <typename K, typename V>
    bool RedBlackTree<K, V>::Erase(const K& key)
    {
        //TODO(v1tr10l7): remove values;
    }
    


    template <typename K, typename V>
    void RedBlackTree<K, V>::PrintTree()
    {
        PrismInfo("RedBlackTree: Dump ->");    
        PrismMessage("----------------\n");

        PrintNode("", m_Root, false);

        PrismMessage("\n");
        PrismMessage("----------------\n");
    }
    template <typename K, typename V>
    void RedBlackTree<K, V>::PrintNode(const std::string& prefix, const Node* node, bool isLeft)
    {
        if (node)
        {
             PrismMessage("{}", prefix);

             PrismMessage("{}", isLeft ? "├──" : "└──" );

            // print the value of the node
             PrismMessage("{}{}\n", node->Key, node->Color == Color::eBlack ? "B" : "R", node->Color == Color::eBlack ? "B" : "R");

            // enter the next tree level - left and right branch
            PrintNode( prefix + (isLeft ? "│   " : "    "), node->LeftChild, true);
            PrintNode( prefix + (isLeft ? "│   " : "    "), node->RightChild, false);
        }
    }



    template <typename K, typename V>
    void RedBlackTree<K, V>::InsertFix(Node* node)
    {
        assert(node);

        Node* current = node;
        while (current->Parent && current->Parent->Color == Color::eRed)
        {
            Node* grandParent = current->Parent->Parent;
            if (current->Parent == grandParent->LeftChild)
            {
                Node* uncle = grandParent->RightChild;
                if (uncle && uncle->Color == Color::eRed)
                {
                    grandParent->Color = Color::eRed;
                    grandParent->LeftChild->Color = Color::eBlack;
                    grandParent->RightChild->Color = Color::eBlack;
                    current = grandParent;
                }
                else 
                {
                    if (current == current->Parent->RightChild)
                    {
                        current = current->Parent;
                        RotateLeft(current);
                    }
                    current->Parent->Color = Color::eBlack;
                    grandParent->Color = Color::eRed;
                    RotateRight(grandParent);
                }
            }
            else
            {
                if (grandParent->LeftChild && grandParent->LeftChild->Color == Color::eRed)
                {
                    grandParent->LeftChild->Color = Color::eBlack;
                    grandParent->RightChild->Color = Color::eBlack;
                    grandParent->Color = Color::eRed;
                    current = grandParent;
                }
                else 
                {
                    if (current == current->Parent->LeftChild)
                    {
                        current = current->Parent;
                        RotateRight(current);
                    }
                    current->Parent->Color = Color::eBlack;
                    grandParent->Color = Color::eRed;
                    RotateLeft(grandParent);
                }
            }
        }

        m_Root->Color = Color::eBlack;
    }
    template <typename K, typename V>
    i32 RedBlackTree<K, V>::Compare(const K& lhs, const K& rhs) const
    {
        if (lhs < rhs) return -1;
        
        return lhs == rhs;
    }

    template <typename K, typename V>
    void RedBlackTree<K, V>::RotateLeft(Node* node)
    {
        assert(node && node->RightChild);
        Node* rightChild = node->RightChild;

        node->RightChild = rightChild->LeftChild;
        if (rightChild->LeftChild) rightChild->LeftChild->Parent = node;

        rightChild->LeftChild = node;
        rightChild->Parent = node->Parent;
        node->Parent = rightChild;
        
        if (m_Root == node)
        {
            m_Root = rightChild;
            return;
        }
        
        if (rightChild->Parent->LeftChild == node)
            rightChild->Parent->LeftChild = rightChild;
        else rightChild->Parent->RightChild = rightChild;
    }
    template <typename K, typename V>
    void RedBlackTree<K, V>::RotateRight(Node* node)
    {
        assert(node->LeftChild);
	    auto temp = node->LeftChild;

  	    // update the two nodes
        node->LeftChild = temp->RightChild;
        if (temp->RightChild )
                temp->RightChild->Parent = node;
        temp->RightChild = node;
        temp->Parent = node->Parent;
        node->Parent = temp;

        // update the parent
        if (m_Root == node) 
        {
            m_Root = temp;
            return;
        }
        if (temp->Parent->LeftChild == node)
            temp->Parent->LeftChild = temp;
        else
      		temp->Parent->RightChild = temp;
    }

    template <typename K, typename V>
    void RedBlackTree<K, V>::RecursiveDelete(Node* node)
    {
        if (!node) return;
        RecursiveDelete(node->LeftChild);
        RecursiveDelete(node->RightChild);
        
        delete node;
    }
};
