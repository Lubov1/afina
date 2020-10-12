#include <utility>
#ifndef AFINA_STORAGE_STRIPED_LOCK_LRU_H
#define AFINA_STORAGE_STRIPED_LOCK_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "ThreadSafeSimpleLRU.h"
#include <functional>
#include <unistd.h>

#include <afina/Storage.h>
// #include "SimpleLRU.h"




namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class StripedLockLRU : public Afina::Storage {
public:
    StripedLockLRU(size_t max_size = 1024, size_t N = 4) : _max_size(max_size), _N(N) {
        size_t stripe_limit = max_size/N;
        if (stripe_limit < 2*1024*1024){
            throw std::runtime_error("Small stripe_limit");
        }

        for (size_t i=0;i < _N;i++)
            shard.emplace_back(new ThreadSafeSimplLRU(max_size/N));
    }

    ~StripedLockLRU() {
    }

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;
    
    
private:
    // LRU cache node
    std::hash<std::string> _hash;
    std::size_t _N;
    std::size_t _max_size;
    std::vector<std::unique_ptr<ThreadSafeSimplLRU>> shard;

};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
