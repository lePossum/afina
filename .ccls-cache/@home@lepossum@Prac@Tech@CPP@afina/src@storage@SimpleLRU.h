#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

    /*
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
    class SimpleLRU : public Afina::Storage {
    public:
        SimpleLRU(size_t max_size = 1024)
            : _max_size(max_size)
            , _cur_size(0)
            , _lru_head(nullptr)
            , _lru_tail(nullptr)
        {
        }

        ~SimpleLRU()
        {
            _lru_index.clear();
            if (_lru_head != nullptr) {
                while (_lru_head->next != nullptr) {
                    std::unique_ptr<lru_node> tmp(nullptr);
                    tmp.swap(_lru_head->next);
                    _lru_head.swap(tmp);
                    tmp.reset();
                }
                _lru_head.reset();
            }
        }

        // Implements Afina::Storage interface
        bool Put(const std::string& key, const std::string& value) override;

        // Implements Afina::Storage interface
        bool PutIfAbsent(const std::string& key, const std::string& value) override;

        // Implements Afina::Storage interface
        bool Set(const std::string& key, const std::string& value) override;

        // Implements Afina::Storage interface
        bool Delete(const std::string& key) override;

        // Implements Afina::Storage interface
        bool Get(const std::string& key, std::string& value) override;

    private:
        // LRU cache node
        using lru_node = struct lru_node {
            const std::string key;
            std::string value;
            // std::unique_ptr<lru_node> prev;
            lru_node* prev;
            std::unique_ptr<lru_node> next;

            lru_node(const std::string& _k, const std::string& _v)
                : key(_k),
                  value(_v) { }
        };

        // Maximum number of bytes could be stored in this cache.
        // i.e all (keys+values) must be less the _max_size
        std::size_t _max_size;
        std::size_t _cur_size;

        // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
        // element that wasn't used for longest time.
        //
        // List owns all nodes
        std::unique_ptr<lru_node> _lru_head;
        lru_node* _lru_tail;

        using back_node = std::map<std::reference_wrapper<const std::string>,
            std::reference_wrapper<lru_node>,
            std::less<std::string>>;

        // Index of nodes from list above, allows fast random access to elements by lru_node#key
        back_node _lru_index;

        using back_node_iter = back_node::iterator;

        // Delete node by it's iterator in _lru_index
        bool _delete_at_iter(back_node_iter elem_iter);

        bool _delete_node(lru_node& node_ref);

        bool _node_to_tail(lru_node& node_ref);

        bool _is_free(size_t need_to_free);

        bool _put(const std::string& key, const std::string& value);

        bool _set(back_node_iter elem_iter, const std::string& value);
    };

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
