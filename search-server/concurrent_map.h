#pragma once
#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "log_duration.h"


using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct ConMap {
        std::map<Key, Value> map_;
        std::mutex value_mutex_;
    };

public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    explicit ConcurrentMap(size_t bucket_count) : concurent_maps(bucket_count)
    {}

    struct Access {
        Access(const Key& key, ConMap& concurent_maps) :
            guard(concurent_maps.value_mutex_),
            ref_to_value(concurent_maps.map_[key])
        {}

        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    Access operator[](const Key& key) {
        auto& concmap = concurent_maps[static_cast<uint64_t>(key) % concurent_maps.size()];
        return { key, concmap };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [map_, value_mutex_] : concurent_maps) {
            std::lock_guard lock(value_mutex_);
            result.insert(map_.begin(), map_.end());
        }
        return result;
    }

private:
    std::vector<ConMap> concurent_maps;
};