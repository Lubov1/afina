#include <utility>
#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
public:
    SimpleLRU(size_t max_size = 1024) : _max_size(max_size) {
        _current_size = 0;
        _lru_head = std::unique_ptr<lru_node>(new lru_node);
        _lru_head->prev = nullptr;

        _lru_tail = new lru_node;
        _lru_tail->next = nullptr;
        _lru_head->next = std::unique_ptr<lru_node>(_lru_tail);
        _lru_head->next->prev = _lru_head.get();
    }

    ~SimpleLRU() {
        _lru_index.clear();
        while (_lru_head.get() != _lru_tail) {
            _lru_tail = _lru_tail->prev;
            _lru_tail->next.reset();
        }
        _lru_head.reset(); // TODO: Here is stack overflow
    }

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value);

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value);

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value);

    // Implements Afina::Storage interface
    bool Delete(const std::string &key);

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value);

private:
    // LRU cache node
    using lru_node = struct lru_node {
        std::string key;
        std::string value;
        lru_node *prev;
        std::unique_ptr<lru_node> next;
    };

    bool del_oldest_node();
    lru_node &create_new_node(std::string key, std::string value);
    void move_to_tail(
        std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>>::iterator iterator);
    bool insert_new_node(const std::string &key, const std::string &value);
    bool change_value(
        std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>>::iterator iterator,
        const std::string &value);

    std::size_t _max_size;
    std::size_t _current_size;

    std::unique_ptr<lru_node> _lru_head;

    lru_node *_lru_tail;

    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>, std::less<std::string>>
        _lru_index;
    using map_it = std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>>::iterator;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
