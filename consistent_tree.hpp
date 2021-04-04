#include <cstdint>
#include <cstddef>
#include "smart_ptr.hpp"
#include <iostream>

/**
 * \param Key The key type. The type (class) must provide a 'less than' and 'equal to' operator
 * \param T The Data type
 * \param size_type Container size type
 * \param Size Container size
 * \param Fast If true every node stores an extra parent index. This increases memory but speed up insert/erase by factor 10
 */

using smart_pointer::SmartPointer;

template<typename Key, typename T>
class avl_array
{


    typedef struct node {
        bool deleted;

        Key key;
        T value;
        int height;
        using nodeptr = SmartPointer<node, std::allocator<node>>;
        nodeptr left;
        nodeptr right;

        node(Key k, T val){key = k; value = val; left = right = NULL; height = 1; deleted = false;}

        ~node() {
            std::cout << "node " << value << " deleted" << std::endl;
        }
    } node;
    using nodeptr = SmartPointer<node, std::allocator<node>>;
    nodeptr _tree;
    size_t _size;

    // iterator class
    typedef class tag_avl_array_iterator
    {
        nodeptr _pNode;
        nodeptr _root;

    public:
        // ctor
        explicit tag_avl_array_iterator(nodeptr instance = nullptr, nodeptr tree = nullptr)
                : _pNode(instance), _root(tree)
        { }

        tag_avl_array_iterator& operator=(const tag_avl_array_iterator& other)
        {
            _pNode = other._pNode;
            return *this;
        }

        bool operator==(const tag_avl_array_iterator& rhs) const
        {
            return _pNode == _pNode;
        }

        bool operator!=(const tag_avl_array_iterator& rhs) const
        {
            return *this != rhs;
        }

        // dereference - access value
        T& operator*() const
        {
            return val();
        }

        // access value
        T& val() const
        {
            return _pNode->value;
        }

        // access key
        Key& key() const
        {
            return _pNode->key;
        }

        // preincrement
        tag_avl_array_iterator& operator++() {


            if (_pNode->right != NULL) {
                _pNode = _pNode->right;
                while (_pNode->left != NULL)
                    _pNode = _pNode->left;
                return *this;
            } else {
                nodeptr q = _root;
                nodeptr suc = NULL;

                while (q != NULL) {
                    if (q->key > _pNode->key) {
                        suc = q;
                        q = q->left;
                    } else if (q->key < _pNode->key)
                        q = q->right;
                    else
                        break;
                }
                _pNode = suc;
                return *this;

            }
        }
        // postincrement
        const tag_avl_array_iterator operator++(int)
        {
            tag_avl_array_iterator _copy = *this;
            ++(*this);
            return _copy;
        }

        tag_avl_array_iterator operator--() {
            if (_pNode->left != NULL) {
                _pNode = _pNode->left;
                while (_pNode->right != NULL)
                    _pNode = _pNode->right;
                return *this;
            }
            else {
                nodeptr q = _root;
                nodeptr suc = NULL;

                while (q != NULL) {
                    if (q->key < _pNode->key) {
                        suc = q;
                        q = q->right;
                    }
                    else if (q->key > _pNode->key)
                        q = q->left;
                    else
                        break;
                }
                _pNode = suc;
                return *this;

            }
        }

        const tag_avl_array_iterator operator--(int) {
            tag_avl_array_iterator _copy = *this;
            --(*this);
            return _copy;
        }
    } avl_array_iterator;

    friend tag_avl_array_iterator;
public:

    typedef T                   value_type;
    typedef T*                  pointer;
    typedef const T*            const_pointer;
    typedef T&                  reference;
    typedef const T&            const_reference;
    typedef Key                 key_type;
    typedef avl_array_iterator  iterator;
    typedef size_t                size_type;

    avl_array(): _tree(NULL), _size(0) {

    }


    // iterators
    iterator begin()
    {
        return iterator(findmin(_tree), _tree);
    }

    iterator end()
    {
        return iterator(nodeptr(nullptr), _tree);
    }

    size_type size() const {
        return _size;
    }

    bool empty() const {
        return _size == static_cast<size_type>(0);
    }
    /**
     * Clear the container
     */
    void clear();
    /*
    {
        size_ = 0U;
        root_ = INVALID_IDX;
    }*/


    /**
     * Insert or update an element
     * \param key The key to insert. If the key already exists, it is updated
     * \param val Value to insert or update
     * \return True if the key was successfully inserted or updated, false if container is full
     */
    bool insert(const key_type& key, const value_type& val) {
        _tree = _insert(_tree, key, val);
        _size++;
        return true;
    }


    /**
     * Find an element
     * \param key The key to find
     * \param val If key is found, the value of the element is set
     * \return True if key was found
     */
    bool find(const key_type& key, value_type& val) const;


    /**
     * Find an element and return an iterator as result
     * \param key The key to find
     * \return Iterator if key was found, else end() is returned
     */
    iterator find(const key_type& key) {
        if(!_tree) return end();
        return iterator(_find(_tree,key), _tree);
    }

    /**
     * Remove element by key
     * \param key The key of the element to remove
     * \return True if the element ws removed, false if key was not found
     */
    bool erase(const key_type& key);

    /**
     * Remove element by iterator position
     * THIS ERASE OPERATION INVALIDATES ALL ITERATORS!
     * \param position The iterator position of the element to remove
     * \return True if the element was successfully removed, false if error
     */
    bool erase(iterator position) {
        _tree = _remove(_tree, position.key());
        _size--;
        return true;
    }

    /////////////////////////////////////////////////////////////////////////////
    // Helper functions
private:
    int height(nodeptr n)
    {
        return n ? n->height : 0;
    }

    int balancefactor(nodeptr n)
    {
        return height(n->right) - height(n->left);
    }

    void fixheight(nodeptr n)
    {
        n->height = (height(n->left) > height(n->right) ?
                     height(n->left) : height(n->right))+1;
    }

    nodeptr RRotation(nodeptr n)
    {
        nodeptr tmp = n->left;
        n->left = tmp->right;
        tmp->right = n;
        fixheight(n);fixheight(tmp);
        return tmp;
    }

    nodeptr LRotation(nodeptr n)
    {
        nodeptr tmp = n->right;
        n->right = tmp->left;
        tmp->left = n;
        fixheight(n);fixheight(tmp);
        return tmp;
    }

    nodeptr _insert(nodeptr n, Key k, T val)
    {
        if(!n) n = nodeptr(new node(k, val));
        if(k < n->key)
            n->left = _insert(n->left, k, val);
        else if(k > n->key)
            n->right = _insert(n->right, k, val);
        else {
            n->value = val;
            return n;
        }

        return balance(n);
    }

    nodeptr balance(nodeptr n)
    {
        fixheight(n);
        if(balancefactor(n) == 2)
        {
            if(balancefactor(n->right) < 0)
                n->right = RRotation(n->right);
            return LRotation(n);
        }
        if (balancefactor(n) == -2)
        {
            if(balancefactor(n->left) > 0)
                n->left = LRotation(n->left);
            return RRotation(n);
        }
        return n;
    }

    nodeptr _find(nodeptr n, const key_type& key) {
        if(n->left && n->key > key)
            return _find(n->left, key);
        else if(n->right && n->key < key)
            return _find(n->right, key);
        else {
            if(n->key != key) return nodeptr(nullptr);
            return n;
        }
    }

    nodeptr findmin(nodeptr n)
    {
        return n->left ? findmin(n->left) : n;
    }

    nodeptr findmax(nodeptr n)
    {
        return n->right ? findmax(n->right) : n;
    }

    nodeptr removemin(nodeptr n)
    {
        if(n->left == 0)
            return n->right;
        n->left = removemin(n->left);
        return balance(n);
    }

    nodeptr _remove(nodeptr n, Key k)
    {
        if(!n) return nodeptr(nullptr);
        if(k < n->key)
            n->left = _remove(n->left, k);
        else if(k > n->key)
            n->right = _remove(n->right,k);
        else
        {
            nodeptr tmpl = n->left;
            nodeptr tmpr = n->right;
            n->deleted = true;
            if(!tmpr) return tmpl;
            nodeptr min = findmin(tmpr);
            min->right = removemin(tmpr);
            min->left = tmpl;
            return balance(min);
        }
        return balance(n);
    }
};
