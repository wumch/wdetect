
#pragma once

#include "meta.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace staging {

namespace {

template<typename ElemType>
class Node
{
    typedef ElemType elem_type;
    typedef Node<ElemType> NodeType;
    typedef NodeType* node_ptr_type;
    typedef std::vector<NodeType*> Children;
private:
    Children children;
    node_ptr_type parent;
    elem_type value;

    static const class NodeValEqual
    {
    public:
        bool operator()(const node_ptr_type node_ptr, const elem_type value) const
        {
            return node_ptr->value == value;
        }
    } node_val_equal;

public:
    explicit Node(const elem_type& value_)
        : parent(NULL), value(value_)
    {}

    explicit Node(const elem_type& value_, node_ptr_type parent_)
        : parent(parent_), value(value_)
    {}

    void attach_child(elem_type child_value)
    {
        if (child_value != value)
        {
            if (child_value < value)
            {
                std::swap(value, child_value);
                add_child(child_value);
                rotate_recursive();
            }
            else
            {
                if (!exists(child_value))
                {
                    add_child(child_value);
                }
            }
        }
    }

    node_ptr_type find_child(const elem_type& value_) const
    {
        CS_DUMP(children.size());
        for (typename Children::const_iterator it = children.begin(); it != children.end(); ++it)
        {
            if ((*it)->value == value_)
            {
                return *it;
            }
        }
        return NULL;
    }

    node_ptr_type find(const elem_type& value_) const
    {
        node_ptr_type node = find_child(value_);
        if (node != NULL)
        {
            return node;
        }
        else
        {
            for (typename Children::const_iterator it = children.begin(); it != children.end(); ++it)
            {
                node = (*it)->find(value_);
                if (node != NULL)
                {
                    return node;
                }
            }
            return NULL;
        }
    }

    static void swap(node_ptr_type one, node_ptr_type other)
    {
        std::swap(one->value, other->value);
    }

    static void rotate(node_ptr_type node)
    {
        swap(node, node->parent);
    }

    void rotate_recursive()
    {
        node_ptr_type node = this;
        while (node->parent != NULL && node->parent->value < node->value)
        {
            rotate(node);
        }
    }

    bool exists(const elem_type& value_) const
    {
        return find_child(value_) != NULL;
    }

    void add_child(const elem_type& child_value)
    {
        CS_DUMP(child_value);
        children.push_back(new NodeType(child_value, this));
        CS_DUMP(children.size());
    }

    const elem_type& get_value() const
    {
        return value;
    }

    const elem_type& get_min_equivalence() const
    {
        if (!parent)
        {
            return value;
        }
        else
        {
            node_ptr_type node = parent;
            while (node->parent != NULL)
            {
                node = node->parent;
            }
            return node->value;
        }
    }

    virtual ~Node()
    {
        for (typename Children::iterator it = children.begin(); it != children.end(); ++it)
        {
            delete *it;
        }
    }

    static void dump(const node_ptr_type node)
    {
        for (typename Children::iterator it = node->children.begin(); it != node->children.end(); ++it)
        {
            printf("%4d", (*it)->value);
        }
        printf("\n");
        for (typename Children::iterator it = node->children.begin(); it != node->children.end(); ++it)
        {
            dump(*it);
        }
    }
};

}

// 二元关系传递 树（最小堆）
template<typename ElemType, ElemType invalid_elem_value_>
class Forest
{
protected:
    typedef ElemType elem_type;
    typedef std::pair<ElemType, ElemType> Pair;
    typedef Node<ElemType> NodeType;
    typedef NodeType* node_ptr_type;

    static const elem_type invalid_elem_value = invalid_elem_value_;

    typedef std::vector<NodeType*> TreeList;
    TreeList trees;

public:
    Forest()
    {}

    void attach(const ElemType& a, const ElemType& b)
    {
        if (a != b)
        {
            attach(make_pair(a, b));
        }
    }

    const elem_type& get_value(const elem_type& value_) const
    {
        for (typename TreeList::const_iterator it = trees.begin(); it != trees.end(); ++it)
        {
            node_ptr_type node = (*it)->find(value_);
            if (node != NULL)
            {
                return node->get_min_equivalence();
            }
        }
        return invalid_elem_value;
    }

protected:
    void attach(const Pair& pair)
    {
        node_ptr_type node;
        CS_DUMP(trees.size());
        for (typename TreeList::iterator it = trees.begin(); it != trees.end(); ++it)
        {
            node = (*it)->find(pair.first);
            if (node != NULL)
            {
                node->attach_child(pair.second);
                return;
            }
            else
            {
                node = (*it)->find(pair.second);
                if (node != NULL)
                {
                    node->attach_child(pair.first);
                    return;
                }
            }
        }
        add(pair);
    }

    void add(const Pair& pair)
    {
        node_ptr_type tree = new NodeType(pair.first);
        trees.push_back(tree);
        tree->attach_child(pair.second);
    }

    Pair make_pair(const elem_type& a, const elem_type& b) const
    {
        if (b < a)
        {
            return std::make_pair(b, a);
        }
        else
        {
            return std::make_pair(a, b);
        }
    }

public:
    void dump()
    {
        for (typename TreeList::iterator it = trees.begin(); it != trees.end(); ++it)
        {
            NodeType::dump(*it);
        }
    }
};

template<typename ElemType, ElemType invalid_elem_value_>
    const typename Forest<ElemType, invalid_elem_value_>::elem_type
    Forest<ElemType, invalid_elem_value_>::invalid_elem_value;

}
