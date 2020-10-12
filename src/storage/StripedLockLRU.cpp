// #ifndef AFINA_STORAGE_STRIPED_LOCK_LRU_H
// #define AFINA_STORAGE_STRIPED_LOCK_LRU_H

#include <map>
#include <mutex>
#include <string>
#include "ThreadSafeSimpleLRU.h"

#include "StripedLockLRU.h"

namespace Afina {
namespace Backend {

/**
 * # SimpleLRU thread safe version
 *
 *
 */
    // see SimpleLRU.h
    bool StripedLockLRU::Put(const std::string &key, const std::string &value)  {
        // TODO: sinchronization
        return shard[_hash(key)%_N]->Put(key, value);
    }

    // see SimpleLRU.h
    bool StripedLockLRU::PutIfAbsent(const std::string &key, const std::string &value)  {
        return shard[_hash(key)%_N]->PutIfAbsent(key, value);
    }

    // see SimpleLRU.h
    bool StripedLockLRU::Set(const std::string &key, const std::string &value)  {
        return shard[_hash(key)%_N]->Set(key, value);
    }

    // see SimpleLRU.h
    bool StripedLockLRU::Delete(const std::string &key)  {
        return shard[_hash(key)%_N]->Delete(key);
    }

    // see SimpleLRU.h
    bool StripedLockLRU::Get(const std::string &key, std::string &value)  {
        return shard[_hash(key)%_N]->Get(key, value);
    }



} // namespace Backend
} // namespace Afina

// #endif // AFINA_STORAGE_STRIPED_LOCK_LRU_H
