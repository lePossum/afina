#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

    // See MapBasedGlobalLockImpl.h
    bool SimpleLRU::Put(const std::string& key, const std::string& value)
    {
        std::size_t add_val_size = value.size();
        std::size_t add_size = add_val_size + key.size();

        auto elem = _lru_index.find(key);

        if (elem != _lru_index.end()) { // key exists, override
            return Set(key, value);
        } else { // writing new key
            return PutIfAbsent(key, value);
        }
    }

    // See MapBasedGlobalLockImpl.h
    bool SimpleLRU::PutIfAbsent(const std::string& key, const std::string& value)
    {
        std::size_t add_val_size = value.size();
        std::size_t add_size = add_val_size + key.size();

        auto elem = _lru_index.find(key);

        if (elem != _lru_index.end()) { // key exists
            return false;
        } else { // adding
            if (_cur_size + add_size <= _max_size) {
                if (!_lru_index.empty()) {
                    return _new_head(key, value, add_size);
                } else {
                    return _new_root(key, value, add_size);
                }
            } else {
                if (add_size > _max_size) {
                    return false;
                } else {
                    while (_cur_size + add_size > _max_size) {
                        Delete(_lru_head->next->key);
                    }

                    if (_lru_index.empty()) {
                        return _new_root(key, value, add_size);
                    } else {
                        return _new_head(key, value, add_size);
                    }
                }
            }
        }
    }

    // See MapBasedGlobalLockImpl.h
    bool SimpleLRU::Set(const std::string& key, const std::string& value)
    {
        std::size_t new_val_size = value.size();
        std::size_t new_size = new_val_size + key.size();

        auto elem = _lru_index.find(std::reference_wrapper<const std::string>(key));
        if (elem != _lru_index.end()) {
            lru_node& node = elem->second;
            std::size_t old_val_size = node.value.size();

            if (_cur_size - old_val_size + new_val_size <= _max_size) {
                node.value = value;
                _node_to_top(node);
                _cur_size += (new_val_size - old_val_size);
                return true;
            } else {
                if (new_size > _max_size) {
                    return false;
                } else {
                    while (_cur_size - old_val_size + new_val_size > _max_size) { // deleting tail elems
                        Delete(_lru_head->next->key);
                    }

                    if (_lru_index.empty()) { // we deleted everything
                        return _new_root(key, value, new_size);
                    } else { // we still have our node
                        node.value = value;
                        _node_to_top(node);
                        _cur_size += (new_val_size -  old_val_size);
                        return true;
                    }
                }
            }
        } else {
            return false;
        }
    }

    // See MapBasedGlobalLockImpl.h
    bool SimpleLRU::Delete(const std::string& key)
    {
        auto elem = _lru_index.find(std::reference_wrapper<const std::string>(key));
        if (elem != _lru_index.end()) {
            lru_node& temp_node = elem->second;
            std::unique_ptr<lru_node> tmp_ptr;
            std::size_t elem_size = temp_node.key.size() + temp_node.value.size();
            if (temp_node.next) {
                temp_node.next->prev = temp_node.prev;
            }
            if (temp_node.prev) {
                tmp_ptr.swap(temp_node.prev->next);
                temp_node.prev->next = std::move(temp_node.next);
            } else {
                tmp_ptr.swap(_lru_head);
                _lru_head = std::move(temp_node.next);
            }
            _lru_index.erase(key);
            _cur_size -= elem_size;
            return true;
        } else {
            return false;
        }
    }

    // See MapBasedGlobalLockImpl.h
    bool SimpleLRU::Get(const std::string& key, std::string& value)
    {
        auto elem = _lru_index.find(std::reference_wrapper<const std::string>(key));
        if (elem != _lru_index.end()) {
            lru_node& temp_node = elem->second;
            value = temp_node.value;
            return true;
        } else {
            return false;
        }
    }

    bool SimpleLRU::_new_root(const std::string& key, const std::string& value,
                              const std::size_t size)
    {
        std::unique_ptr<lru_node> new_root(new lru_node(key, value));
        new_root->prev = nullptr;
        new_root->next = std::move(new_root);
        _lru_head = std::move(new_root);
        _cur_size = size;
        _lru_index.emplace(key, *_lru_head);
        return true;
    }

    void SimpleLRU::_node_to_top(lru_node& node)
    {
        std::unique_ptr<lru_node> tmp_ptr;
        _lru_head->next.reset(&node);
        if (node.prev != nullptr) {
            node.prev->next.swap(node.next);
            node.next->prev = node.prev;
        } else {
            node.prev = node.next->prev;
            node.next->prev = nullptr;
        }
        _lru_head.reset(node.prev);
    }

    bool SimpleLRU::_new_head(const std::string& key, const std::string& value, const std::size_t size)
    {
        std::unique_ptr<lru_node> new_head(new lru_node(key, value));
        new_head->next = std::move(_lru_head->next);
        _lru_head->next = std::move(new_head);
        new_head->prev = &*_lru_head;
        _lru_head = std::move(new_head);
        _cur_size += size;
        _lru_index.emplace(key, *_lru_head);
        return true;
    }

} // namespace Backend
} // namespace Afina
