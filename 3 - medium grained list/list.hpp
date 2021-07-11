#pragma once

#include <atomic>
#include <shared_mutex>
#include <mutex>
#include <list>

using std::shared_lock;
using std::unique_lock;

template <typename T>
class list {
  class list_node {
    friend list;

    T _data;
    list_node *_next;
    list_node *_prev;
    std::atomic<size_t> _ref_count;
    std::atomic<bool> _is_deleted;
    std::shared_mutex _node_lock;

    list_node() : _prev(nullptr), _next(nullptr), _ref_count(0) {}
    list_node(T data) : list_node() { 
        _data = data; 
    }

    void incRefCount() { 
        _ref_count++; 
    }

    void decRefCount() { 
        _ref_count--; 
    }

    static void capture(list_node **parent, list_node *n) {
      *parent = n;
      n->_ref_count++;
    }

    static void release(list_node *n) {
      n->_ref_count--;
      if (n->_ref_count == 0) {
        if (n->_prev)
            release(n->_prev);
        if (n->_next)
            release(n->_next);
        delete n;
      }
    }
  };

  class list_iterator {
    friend list;
    using iterator_category = std::bidirectional_iterator_tag;

    list_node *ptr;

    list_iterator(list_node *ptr) : ptr(ptr) { 
        ptr->incRefCount(); 
    }

   public:
    list_iterator(const list_iterator &iter) : ptr(iter.ptr) {
      ptr->incRefCount();
    }

    ~list_iterator() {
      unique_lock lock(ptr->_node_lock);
      list_node::release(ptr);
    }

    list_iterator &operator=(const list_iterator &other) {
      if (ptr == other.ptr) 
          return *this;

      std::scoped_lock(ptr->_node_lock, other.ptr->_node_lock);

      if (other.ptr) 
          other.ptr->incRefCount();

      list_node::release(ptr);
      ptr = other.ptr;
      return *this;
    }

    list_iterator &operator=(list_iterator &&other) {
      return operator=(std::move(other));
    }

    T &operator*() const { 
        return ptr->_data; 
    }

    T *operator->() const { 
        return &(ptr->_data); 
    }

    list_iterator &operator++() {
      shared_lock r_lock(ptr->_node_lock);
      list_node *tmp = ptr;
      list_node *next = ptr->_next;

      list_node::capture(&ptr, next);

      if (tmp->_ref_count == 1) {
        r_lock.unlock();
        unique_lock w_lock(tmp->_node_lock);
        list_node::release(tmp);
      } else {
        list_node::release(tmp); 
      }

      return *this;
    }

    list_iterator operator++(int) {
      list_iterator copy(*this);
      ++(*this);
      return copy;
    }

    list_iterator &operator--() {
      shared_lock r_lock(ptr->_node_lock);
      list_node *tmp = ptr;
      list_node *prev = ptr->_prev;

      list_node::capture(&ptr, prev);

      if (tmp->_ref_count == 1) {
        r_lock.unlock();
        unique_lock w_lock(tmp->_node_lock);
        list_node::release(tmp);
      } else
        list_node::release(tmp);

      return *this;
    }

    list_iterator operator--(int) {
      list_iterator copy(*this);
      --(*this);
      return copy;
    }

    bool operator==(const list_iterator &rhs) { 
        return ptr == rhs.ptr; 
    }

    bool operator!=(const list_iterator &rhs) { 
        return ptr != rhs.ptr; 
    }
  };

  list_node *_start;
  list_node *_finish;
  std::atomic<size_t> _size = 0;

  list_iterator &common_erase(list_iterator &it, bool is_popped) {
    list_node *node = it.ptr;
    bool retry = true;
    while (retry) {
      shared_lock lock_current(node->_node_lock);

      if (node == _finish || node == _start)
        throw std::out_of_range("out of range");

      list_node *prev = node->_prev;
      prev->_ref_count++;

      list_node *next = node->_next;
      next->_ref_count++;

      lock_current.unlock();

      unique_lock lock_prev(prev->_node_lock);
      lock_current.lock();
      unique_lock lock_next(next->_node_lock);

      if (node->_is_deleted) {
        if (is_popped) {
          node = _start->_next;
          continue;
        }
        list_node::capture(&it.ptr, next);
        list_node::release(node);

        list_node::release(prev);
        list_node::release(next);

        return it;
      }

      if (prev->_next == node && next->_prev == node) {
        list_node::capture(&prev->_next, next);
        list_node::capture(&next->_prev, prev);

        node->_is_deleted = true;

        list_node::release(node);
        list_node::release(node);

        _size--;
        retry = false;

        list_node::capture(&it.ptr, next);
      }
      list_node::release(prev);
      list_node::release(next);
    }

    return it;
  }

 public:
  using iterator = list_iterator;
  using value_type = T;

  list() : _start(new list_node()), _finish(new list_node()) {
    list_node::capture(&_start->_next, _finish);
    list_node::capture(&_finish->_prev, _start);
  }

  list(std::initializer_list<value_type> list)
      : _start(new list_node()), _finish(new list_node()) {
    list_node::capture(&_start->_next, _finish);
    list_node::capture(&_finish->_prev, _start);

    for (auto &e : list) 
        push_back(e);
  }

  ~list() {
    list_node *node = _start->_next;
    while (node != _finish) {
      list_node *buf = node;
      node = node->_next;
      list_node::release(buf);
    }

    delete _start;
    delete _finish;
  }

  iterator begin() const {
    shared_lock lock(_start->_next->_node_lock);
    if (!_size) 
        return end();

    return iterator(_start->_next);
  }

  iterator end() const { 
      return iterator(_finish); 
  }

  bool empty() const { 
      return !_size; 
  }

  size_t size() const { 
      return _size; 
  }

  void push_front(const value_type &data) {
    auto it = iterator(_start);
    insert(it, data);
  }

  void push_back(const value_type &data) {
    auto it = iterator(_finish->_prev);
    insert(it, data);
  }

  iterator insert(iterator &iter, value_type data) {
    list_node *prev = iter.ptr;
    if (iter.ptr == _finish)
      throw std::out_of_range("out of range");
    bool retry = true;
    while (retry) {
      if (iter.ptr->_is_deleted) return end();

      unique_lock lock_prev(prev->_node_lock);

      list_node *next = prev->_next;
      unique_lock lock_next(next->_node_lock);

      if (prev == next->_prev) {
        list_node *new_node = new list_node(data);
        unique_lock lock_new(new_node->_node_lock);

        list_node::capture(&prev->_next, new_node);
        list_node::capture(&new_node->_prev, prev);

        list_node::capture(&next->_prev, new_node);
        list_node::capture(&new_node->_next, next);

        list_node::release(prev);
        list_node::release(next);

        _size++;
        retry = false;

        list_node::capture(&(iter.ptr), new_node);
        list_node::release(prev);
      }
    }
    return iter;
  }

  void pop_front() {
    auto it = begin();
    common_erase(it, true);
  }

  void pop_back() {
    auto it = iterator(end().ptr->_prev);
    common_erase(it, true);
  }

  iterator &erase(iterator &it) { 
      return common_erase(it, false); 
  }
};