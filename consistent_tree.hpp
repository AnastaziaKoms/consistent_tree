#include <cstdint>
#include <cstddef>


/**
 * \param Key The key type. The type (class) must provide a 'less than' and 'equal to' operator
 * \param T The Data type
 * \param size_type Container size type
 * \param Size Container size
 * \param Fast If true every node stores an extra parent index. This increases memory but speed up insert/erase by factor 10
 */
template<typename Key, typename T>
class avl_array
{
    typedef struct node {
        Key key;
        T value;
        int height;
        node* left;
        node* right;
        node(Key k, T val){key = k; value = val; left = right = NULL; height = 1;}
    } node;

    node* _tree;
    size_t _size;

    // iterator class
    typedef class tag_avl_array_iterator
    {
        //avl_array*  instance_;    // array instance
        //size_type   idx_;         // actual node

        friend avl_array;         // avl_array may access index pointer

    public:
        // ctor
        explicit tag_avl_array_iterator();/*avl_array* instance = nullptr, size_type idx = 0U);
                : instance_(instance)
                , idx_(idx)
        { }*/

        tag_avl_array_iterator& operator=(const tag_avl_array_iterator& other);
        /*
        {
            instance_ = other.instance_;
            idx_      = other.idx_;
            return *this;
        }*/

        bool operator==(const tag_avl_array_iterator& rhs) const;
        //{ return idx_ == rhs.idx_; }

        bool operator!=(const tag_avl_array_iterator& rhs) const;
        //{ return *this != rhs; }

        // dereference - access value
        T& operator*() const;
        //{ return val(); }

        // access value
        T& val() const;
        //{ return instance_->val_[idx_]; }

        // access key
        Key& key() const;
        //{ return instance_->key_[idx_]; }

        // preincrement
        tag_avl_array_iterator& operator++();
        /*
        {
            // end reached?
            if (idx_ >= Size) {
                return *this;
            }
            // take left most child of right child, if not existent, take parent
            size_type i = instance_->child_[idx_].right;
            if (i != instance_->INVALID_IDX) {
                // successor is the furthest left node of right subtree
                for (; i != instance_->INVALID_IDX; i = instance_->child_[i].left) {
                    idx_ = i;
                }
            }
            else {
                // have already processed the left subtree, and
                // there is no right subtree. move up the tree,
                // looking for a parent for which nodePtr is a left child,
                // stopping if the parent becomes NULL. a non-NULL parent
                // is the successor. if parent is NULL, the original node
                // was the last node inorder, and its successor
                // is the end of the list
                i = instance_->get_parent(idx_);
                while ((i != instance_->INVALID_IDX) && (idx_ == instance_->child_[i].right)) {
                    idx_ = i;
                    i = instance_->get_parent(idx_);
                }
                idx_ = i;
            }
            return *this;
        }*/

        // postincrement
        tag_avl_array_iterator operator++(int);
        /*
        {
            tag_avl_array_iterator _copy = *this;
            ++(*this);
            return _copy;
        }*/
    } avl_array_iterator;


public:

    typedef T                   value_type;
    typedef T*                  pointer;
    typedef const T*            const_pointer;
    typedef T&                  reference;
    typedef const T&            const_reference;
    typedef Key                 key_type;
    typedef avl_array_iterator  iterator;
    typedef size_t                size_type;

    avl_array(): _tree(NULL), _size(0) {}


    // iterators
    iterator begin();
    /*
    {
        size_type i = INVALID_IDX;
        if (root_ != INVALID_IDX) {
            // find smallest element, it's the farthest node left from root
            for (i = root_; child_[i].left != INVALID_IDX; i = child_[i].left);
        }
        return iterator(this, i);
    }*/

    iterator end();
    //{ return iterator(this, INVALID_IDX); }


    // capacity
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
    iterator find(const key_type& key);


    /**
     * Count elements with a specific key
     * Searches the container for elements with a key equivalent to key and returns the number of matches.
     * Because all elements are unique, the function can only return 1 (if the element is found) or zero (otherwise).
     * \param key The key to find/count
     * \return 0 if key was not found, 1 if key was found
     */
    size_type count(const key_type& key);


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
    bool erase(iterator position);

    /////////////////////////////////////////////////////////////////////////////
    // Helper functions
private:
    int height(node* n)
    {
        return n ? n->height : 0;
    }

    int balancefactor(node* n)
    {
        return height(n->right) - height(n->left);
    }

    void fixheight(node* n)
    {
        n->height = (height(n->left) > height(n->right) ?
                     height(n->left) : height(n->right))+1;
    }

    node* RRotation(node* n)
    {
        node* tmp = n->left;
        n->left = tmp->right;
        tmp->right = n;
        fixheight(n);fixheight(tmp);
        return tmp;
    }

    node* LRotation(node* n)
    {
        node* tmp = n->right;
        n->right = tmp->left;
        tmp->left = n;
        fixheight(n);fixheight(tmp);
        return tmp;
    }

    node* _insert(node* n, Key k, T val)
    {
        if(!n) n = new node(k, val);
        if(k < n->key)
            n->left = _insert(n->left, k, val);
        else if(k > n->key)
            n->right = _insert(n->right, k, val);
        else
            return n;

        return balance(n);
    }

    node* balance(node* n)
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

    node* findmin(node* n)
    {
        return n->left ? findmin(n->left) : n;
    }

    node* removemin(node* n)
    {
        if(n->left == 0)
            return n->right;
        n->left = removemin(n->left);
        return balance(n);
    }

    node* remove(node* n, long long int k)
    {
        if(!n) return 0;
        if(k < n->key)
            n->left = remove(n->left, k);
        else if(k > n->key)
            n->right = remove(n->right,k);
        else
        {
            node* tmpl = n->left;
            node* tmpr = n->right;
            delete n;
            if(!tmpr) return tmpl;
            node* min = findmin(tmpr);
            min->right = removemin(tmpr);
            min->left = tmpl;
            return balance(min);
        }
        return balance(n);
    }
};
