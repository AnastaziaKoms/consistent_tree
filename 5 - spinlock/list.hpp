#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <thread>

#include "rw_spin_lock.hpp"

using std::shared_lock;
using std::unique_lock;

template <typename T>
class list {
  class list_node {
    friend list;

    T _data;
    list_node *_next;
    list_node *_prev;
    list *_list;
    std::atomic<size_t> _ref_count = 0;
    std::atomic<bool> _is_deleted = false;
    std::shared_mutex _node_lock;
    rw_spin_lock _rw_lock;

    list_node() : _prev(nullptr), _next(nullptr) {}
    list_node(T data, list *list) : list_node() {
      _list = list;
      _data = data;
    }

    void incrementRef() { 
        _ref_count++; 
    }

    void decrementRef() { 
        _ref_count--;
    }

    static void capture(list_node **parent, list_node *node) {
      *parent = node;
      node->_ref_count++;
    }

    static void release(list_node *node) {
      if (node->_list) {
        node->_list->_rw_spin_lock.r_lock();
        size_t prev_ref_count = --node->_ref_count;
        if (prev_ref_count == 0)
          node->_list->_purgatory->push(node);
        node->_list->_rw_spin_lock.unlock();
      }
    }
  };

  class purgatory {
   public:
    struct purge_node {
      list_node *body = nullptr;
      purge_node *next = nullptr;

      purge_node(list_node *body) : body(body) {}
    };

    purgatory(list *list)
        : _valid_list(list),
          _head(nullptr),
          _cleaner(&purgatory::clear_thread, this) {}

    ~purgatory() {
      _is_destroyed = true;
      _cleaner.join();
    }

    void push(list_node *body) {
      purge_node *pnode = new purge_node(body);
      do {
        pnode->next = _head.load();
      } while (!_head.compare_exchange_strong(pnode->next, pnode));
    }

    void remove(purge_node *prev, purge_node *pnode) {
      prev->next = pnode->next;
      delete pnode;
    }

    void free_node(purge_node *pnode) {
      list_node *prev = pnode->body->_prev;
      list_node *next = pnode->body->_next;

      if (prev != nullptr)
        prev->release(prev);

      if (next != nullptr)
        next->release(next);

      delete pnode->body;
      delete pnode;
    }

    void clear_thread() {
      do {
        _valid_list->_rw_spin_lock.w_lock();

        purge_node *purge_start = _head;

        _valid_list->_rw_spin_lock.unlock();

        if (purge_start != nullptr) {
          int k = 0;
          purge_node *prev = purge_start;
          for (purge_node *pnode = purge_start; pnode != nullptr;) {
            ++k;
            purge_node *old_pnode = pnode;
            pnode = pnode->next;

            if (old_pnode->body->_ref_count > 0 || old_pnode->body->_is_deleted) {
              remove(prev, old_pnode);
            } else {
              old_pnode->body->_is_deleted = true;
              prev = old_pnode;
            }
          }

          _valid_list->_rw_spin_lock.w_lock();
          purge_node *new_purge_start = _head;

          if (new_purge_start == purge_start)
            _head = nullptr;

          _valid_list->_rw_spin_lock.unlock();

          prev = new_purge_start;
          purge_node *pnode = new_purge_start;
          while (pnode != purge_start) {
            purge_node *old_pnode = pnode;
            pnode = pnode->next;

            if (old_pnode->body->_is_deleted)
              remove(prev, old_pnode);
            else
              prev = old_pnode;
          }
          prev->next = nullptr;

          for (pnode = purge_start; pnode != nullptr;) {
            purge_node *old_pnode = pnode;
            pnode = pnode->next;
            free_node(old_pnode);
          }
        }

        if (!_is_destroyed)
          std::this_thread::sleep_for(std::chrono::milliseconds(100));

      } while (!_is_destroyed || _head.load() != nullptr);
    }

   private:
    friend list;

    list *_valid_list;
    std::atomic<purge_node *> _head;
    std::thread _cleaner;
    bool _is_destroyed = false;
  };

  class list_iterator {
    friend list;
    using iterator_category = std::bidirectional_iterator_tag;

    list_node *ptr;
    const list *_list = nullptr;
    list_iterator(list_node *ptr, const list *list) : ptr(ptr), _list(list) {
      ptr->incrementRef();
    }

   public:
    list_iterator(const list_iterator &iter) : ptr(iter.ptr), _list(iter._list) {
      ptr->incrementRef();
    }

    list_iterator &operator=(const list_iterator &other) {
      if (ptr == other.ptr) 
          return *this;

      ptr->_rw_lock.w_lock();
      other.ptr->_rw_lock.w_lock();

      if (other.ptr) 
          other.ptr->incrementRef();

      list_node::release(ptr);
      ptr = other.ptr;
      _list = other._list;

      other.ptr->_rw_lock.unlock();
      ptr->_rw_lock.unlock();

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
      _list->_rw_spin_lock.r_lock();
      list_node *tmp = ptr;
      list_node *next = ptr->_next;

      list_node::capture(&ptr, next);
      _list->_rw_spin_lock.unlock();
      list_node::release(tmp);

      return *this;
    }

    list_iterator operator++(int) {
      list_iterator copy(*this);
      ++(*this);
      return copy;
    }

    list_iterator &operator--() {
      _list->_rw_spin_lock.r_lock();
      list_node *tmp = ptr;
      list_node *prev = ptr->_prev;

      list_node::capture(&ptr, prev);
      _list->_rw_spin_lock.unlock();
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

    ~list_iterator() {
      ptr->_rw_lock.w_lock();
      list_node::release(ptr);
      ptr->_rw_lock.unlock();
    }
  };
  list_node *_start;
  list_node *_finish;
  purgatory *_purgatory;
  std::atomic<size_t> _size = 0;
  mutable rw_spin_lock _rw_spin_lock;

  list_iterator &common_erase(list_iterator &it, bool is_popped) {
    list_node *node = it.ptr;
    bool retry = true;
    while (retry) {
      node->_rw_lock.r_lock();

      if (node == _finish || node == _start)
        throw std::out_of_range("out of range");

      list_node *prev = node->_prev;
      prev->_ref_count++;

      list_node *next = node->_next;
      next->_ref_count++;

      node->_rw_lock.unlock();

      prev->_rw_lock.w_lock();
      node->_rw_lock.r_lock();
      next->_rw_lock.w_lock();

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
      next->_rw_lock.unlock();
      node->_rw_lock.unlock();
      prev->_rw_lock.unlock();
    }

    return it;
  }

 public:
  using iterator = list_iterator;
  using value_type = T;

  list() : _start(new list_node()), _finish(new list_node()) {
    _purgatory = new purgatory(this);
    list_node::capture(&_start->_next, _finish);
    list_node::capture(&_finish->_prev, _start);
  }

  list(std::initializer_list<value_type> list)
      : _start(new list_node()), _finish(new list_node()) {
    _purgatory = new purgatory(this);
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
    delete _purgatory;
  }

  iterator begin() const {
    _start->_rw_lock.r_lock();

    if (!_size) {
      _start->_rw_lock.unlock();
      return end();
    }

    auto it = iterator(_start->_next, this);
    _start->_rw_lock.unlock();
    return it;
  }

  iterator end() const { 
      return iterator(_finish, this); 
  }

  bool empty() const { 
      return !_size; 
  }

  size_t size() const { 
      return _size; 
  }

  void push_front(const value_type &data) {
    auto it = iterator(_start, this);
    insert(it, data);
  }

  void push_back(const value_type &data) {
    auto it = iterator(_finish->_prev, this);
    insert(it, data);
  }

  iterator insert(iterator &iter, value_type data) {
    list_node *prev = iter.ptr;

    if (iter.ptr == _finish)
      throw std::out_of_range("out of range");
    
    bool retry = true;
    while (retry) {
      if (iter.ptr->_is_deleted) 
          return end();

      prev->_rw_lock.w_lock();
      list_node *next = prev->_next;
      next->_rw_lock.w_lock();
      if (prev == next->_prev) {
        list_node *new_node = new list_node(data, this);
        new_node->_rw_lock.w_lock();

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
        new_node->_rw_lock.unlock();
      }
      next->_rw_lock.unlock();
      prev->_rw_lock.unlock();
    }
    return iter;
  }

  void pop_front() {
    auto it = begin();
    common_erase(it, true);
  }

  void pop_back() {
    auto it = iterator(end().ptr->_prev, this);
    common_erase(it, true);
  }

  iterator &erase(iterator &it) { 
      return common_erase(it, false); 
  }
};