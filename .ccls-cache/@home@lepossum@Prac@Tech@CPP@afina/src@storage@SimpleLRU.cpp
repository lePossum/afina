#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

    // See MapBasedGlobalLockImpl.h
    bool SimpleLRU::Put(const std::string& key, const std::string& value)
    {
        auto elem = _lru_index.find(key);
        if (elem == _lru_index.end())
            return _put(key, value);
        else
            return _set(elem, value);
    }

    // See MapBasedGlobalLockImpl.h
    bool SimpleLRU::PutIfAbsent(const std::string& key, const std::string& value)
    {
        auto elem = _lru_index.find(key);
        if (elem == _lru_index.end())
            return _put(key, value);
        else
            return false;
    }

    // See MapBasedGlobalLockImpl.h
    bool SimpleLRU::Set(const std::string& key, const std::string& value)
    {
        auto elem = _lru_index.find(key);
        if (elem == _lru_index.end())
            return false;
        else
            return _set(elem, value);
    }

    // See MapBasedGlobalLockImpl.h
    bool SimpleLRU::Delete(const std::string& key)
    {
        auto elem = _lru_index.find(key);
        if (elem == _lru_index.end())
            return false;
        else
            return _delete_at_iter(elem);
    }

    // See MapBasedGlobalLockImpl.h
    bool SimpleLRU::Get(const std::string& key, std::string& value)
    {
        auto elem = _lru_index.find(key);
        if (elem == _lru_index.end()) {
            return false;
        } else {
            value = elem->second.get().value;
            return _node_to_tail(elem->second.get());
        }
    }

    bool SimpleLRU::_delete_at_iter(back_node_iter elem_iter)
    {
        std::unique_ptr<lru_node> tmp;
        lru_node& node = elem_iter->second;
        _cur_size -= node.key.size() + node.value.size();

        // deleting from interface
        _lru_index.erase(elem_iter);

        // deleting from main list
        if (node.next) {
            node.next->prev = node.prev;
        }
        if (node.prev) {
            tmp.swap(node.prev->next);
            node.prev->next = std::move(node.next);
        } else {
            tmp.swap(_lru_head);
            _lru_head = std::move(node.next);
        }
        return true;
    }

    bool SimpleLRU::_delete_node(lru_node& node_ref)
    {
        std::unique_ptr<lru_node> tmp_ptr;
        _cur_size -= node_ref.key.size() + node_ref.value.size();

        _lru_index.erase(node_ref.key);

        if (node_ref.next) {
            node_ref.next->prev = node_ref.prev;
        }
        if (node_ref.prev) {
            tmp_ptr.swap(node_ref.prev->next);
            node_ref.prev->next = std::move(node_ref.next);
        } else {
            tmp_ptr.swap(_lru_head);
            _lru_head = std::move(node_ref.next);
        }
        return true;
    }

    bool SimpleLRU::_node_to_tail(lru_node& node_ref)
    {
        if (&node_ref == _lru_tail) {
            return true;
        }
        if (&node_ref == _lru_head.get()) {
            _lru_head.swap(node_ref.next);
            _lru_head->prev = nullptr;
        } else {
            node_ref.next->prev = node_ref.prev;
            node_ref.prev->next.swap(node_ref.next);
        }
        _lru_tail->next.swap(node_ref.next);
        node_ref.prev = _lru_tail;
        _lru_tail = &node_ref;
        return true;
    }

    bool SimpleLRU::_is_free(size_t need_to_free)
    {
        if (need_to_free > _max_size) {
            return false;
        }
        while (_max_size - _cur_size < need_to_free) {
            _delete_node(*_lru_head);
        }
        return true;
    }

    bool SimpleLRU::_put(const std::string& key, const std::string& value)
    {
        size_t addsize = key.size() + value.size();
        if (!_is_free(addsize)) {
            return false;
        }
        std::unique_ptr<lru_node> temp_ptr(new lru_node(key, value));
        if (_lru_tail != nullptr) {
            temp_ptr->prev = _lru_tail;
            _lru_tail->next.swap(temp_ptr);
            _lru_tail = _lru_tail->next.get();
        } else {
            _lru_head.swap(temp_ptr);
            _lru_tail = _lru_head.get();
        }
        _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(_lru_tail->key),
            std::reference_wrapper<lru_node>(*_lru_tail)));
        _cur_size += addsize;
        return true;
    }

    // Set element value by _lru_index iterator
    bool SimpleLRU::_set(back_node_iter elem_iter, const std::string& value)
    {
        lru_node& elem_node = elem_iter->second;
        int size_dif = value.size() - elem_node.value.size();
        if (size_dif > 0)
            if (!_is_free(size_dif))
                return false;

        elem_node.value = value;
        _cur_size += size_dif;
        return _node_to_tail(elem_node);
    }

} // namespace Backend
} // namespace Afina
