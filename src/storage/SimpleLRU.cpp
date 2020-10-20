#include "SimpleLRU.h"
#include <utility>
namespace Afina {
namespace Backend {
// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    auto iter = _lru_index.find(key);
    if (iter != _lru_index.end())
        return change_value(iter, value);
    else
        return insert_new_node(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    auto iter = _lru_index.find(key);
    if (iter != _lru_index.end())
        return false;
    else
        return insert_new_node(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto iter = _lru_index.find(key);
    if (iter == _lru_index.end())
        return false;
    return change_value(iter, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto iter = _lru_index.find(key);
    if (iter == _lru_index.end())
        return false;
    lru_node &del = iter->second.get();
    _lru_index.erase(iter);
    _current_size -= del.value.size() + del.key.size();

    del.next->prev = del.prev;
    del.prev->next.swap(del.next);
    del.next.reset();
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto iter = _lru_index.find(key);

    if (iter == _lru_index.end())
        return false;

    value = (*iter).second.get().value;
    move_to_tail(iter);
    return true;
}

bool SimpleLRU::del_oldest_node() {
    lru_node *old_node = _lru_head->next.get();
    if (old_node == nullptr)
        return false;
    _current_size -= _lru_head->next->key.size() + _lru_head->next->value.size();
    _lru_index.erase(old_node->key);
    old_node->next->prev = _lru_head.get();
    swap(_lru_head->next, old_node->next);
    old_node->next = nullptr;
    return true;
}

SimpleLRU::lru_node &SimpleLRU::create_new_node(const std::string key, const std::string value) {
    lru_node *new_node = new lru_node{std::move(key), std::move(value), _lru_tail->prev, nullptr};
    new_node->next = std::unique_ptr<lru_node>(new_node);

    swap(_lru_tail->prev->next, new_node->next);

    _lru_tail->prev = new_node;

    return *new_node;
}

void SimpleLRU::move_to_tail(map_it iterator) {
    lru_node &current_node = (*iterator).second.get();

    if (_lru_tail->key == current_node.key)
        return;

    current_node.next->prev = current_node.prev;
    swap(current_node.prev->next, current_node.next);
    current_node.prev = _lru_tail->prev;
    swap(current_node.next, _lru_tail->prev->next);
    _lru_tail->prev = &current_node;
}

bool SimpleLRU::insert_new_node(const std::string &key, const std::string &value) {

    if (key.size() + value.size() > _max_size)
        return false;

    while (key.size() + value.size() + _current_size > _max_size)
        del_oldest_node();

    lru_node &new_node = create_new_node(key, value);
    _current_size += key.size() + value.size();

    return _lru_index
        .insert(std::make_pair(std::reference_wrapper<const std::string>(new_node.key),
                               std::reference_wrapper<lru_node>(new_node)))
        .second;
}

bool SimpleLRU::change_value(map_it iterator, const std::string &value) {

    lru_node &current_node = (*iterator).second.get();

    if (current_node.key.size() - current_node.value.size() + value.size() > _max_size)
        return false;
    move_to_tail(iterator);

    while (current_node.key.size() - current_node.value.size() + value.size() + _current_size > _max_size)
        if (!del_oldest_node())
            return false;
    current_node.value = value;
    return true;
}
} // namespace Backend
} // namespace Afina
