#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    auto elem = _lru_index.find(key);
    if (elem == _lru_index.end())
        return PutImpl(key, value);
    else
        return SetImpl(elem, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    auto elem = _lru_index.find(key);
    if (elem == _lru_index.end())
        return PutImpl(key, value);
    else
        return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto elem = _lru_index.find(key);
    if (elem == _lru_index.end())
        return false;
    else
        return SetImpl(elem, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto elem = _lru_index.find(key);
    if (elem == _lru_index.end())
        return false;
    else
        return DeleteItImpl(elem);
}

// TOASK: если виртуальная функция - не const, то может ли её
// override быть const?
// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto elem = _lru_index.find(key);
    if (elem == _lru_index.end()) {
        return false;
    } else {
        value = elem->second.get().value;
        return RefreshImp(elem->second.get());
    }
}

// Delete node by it's iterator in _lru_index
bool SimpleLRU::DeleteItImpl(std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>,
                                      std::less<std::string>>::iterator elem_iter) {
    std::unique_ptr<lru_node> tmp;
    lru_node &node = elem_iter->second;
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

// Delete node by it's reference
bool SimpleLRU::DeleteRefImpl(lru_node &todel_ref) {
    std::unique_ptr<lru_node> tmp;
    _cur_size -= todel_ref.key.size() + todel_ref.value.size();

    _lru_index.erase(todel_ref.key);

    if (todel_ref.next) {
        todel_ref.next->prev = todel_ref.prev;
    }
    if (todel_ref.prev) {
        tmp.swap(todel_ref.prev->next);
        todel_ref.prev->next = std::move(todel_ref.next);
    } else {
        tmp.swap(_lru_head);
        _lru_head = std::move(todel_ref.next);
    }
    return true;
}

// Refresh node by it's reference
// We don't need an iterator here, since _lru_index is a map of references
// and if we carefully handle all the pointers, _lru_index needs not to be changed
bool SimpleLRU::RefreshImp(lru_node &torefresh_ref) {
    if (&torefresh_ref == _lru_tail) {
        return true;
    }
    if (&torefresh_ref == _lru_head.get()) {
        _lru_head.swap(torefresh_ref.next);
        _lru_head->prev = nullptr;
    } else {
        torefresh_ref.next->prev = torefresh_ref.prev;
        torefresh_ref.prev->next.swap(torefresh_ref.next);
    }
    _lru_tail->next.swap(torefresh_ref.next);
    torefresh_ref.prev = _lru_tail;
    _lru_tail = &torefresh_ref;
    return true;
}

// Remove LRU-nodes until we get as much as needfree free space
bool SimpleLRU::GetFreeImpl(size_t needfree) {
    if (needfree > _max_size) {
        return false;
    }
    while (_max_size - _cur_size < needfree) {
        DeleteRefImpl(*_lru_head);
    }
    return true;
}

// Put a new element w/o checking for it's existence (this check MUST be performed before calling this method)
bool SimpleLRU::PutImpl(const std::string &key, const std::string &value) {
    size_t addsize = key.size() + value.size();
    if (!GetFreeImpl(addsize)) {
        return false;
    }
    // TOASK: что будет с остальными полями структуры, которые я не указываю в списке инициализации?
    // см. вопрос в SimpleLRU.h: там в указателе был мусор, если не инициализировать его явно
    std::unique_ptr<lru_node> toput{new lru_node{key, value}};
    if (_lru_tail != nullptr) {
        toput->prev = _lru_tail;
        _lru_tail->next.swap(toput);
        _lru_tail = _lru_tail->next.get();
    } else {
        _lru_head.swap(toput);
        _lru_tail = _lru_head.get();
    }
    _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(_lru_tail->key),
                                     std::reference_wrapper<lru_node>(*_lru_tail)));
    _cur_size += addsize;
    return true;
}

// Set element value by _lru_index iterator
bool SimpleLRU::SetImpl(std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>,
                                 std::less<std::string>>::iterator toset_it,
                        const std::string &value) {
    lru_node &toset_node = toset_it->second;
    int sizedelta = value.size() - toset_node.value.size();
    // if (_cur_size + sizedelta > _max_size) {
    //     return false;
    // }
    if (sizedelta > 0)
        if (!GetFreeImpl(sizedelta))
            return false;

    toset_node.value = value;
    _cur_size += sizedelta;
    return RefreshImp(toset_node);
}

} // namespace Backend
} // namespace Afina
