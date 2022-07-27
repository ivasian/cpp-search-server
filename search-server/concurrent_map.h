#pragma once
#include <iostream>
#include <mutex>
#include <map>
#include <vector>


template <typename Key, typename Value>
class ConcurrentMap {
public:

    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {

        std::lock_guard<std::mutex> guard_;
        Value &ref_to_value;

        Access(std::mutex& value_mutex, std::map<Key, Value>& current_map, Key key) :
                guard_({value_mutex}), ref_to_value(current_map[key]){
        }
    };

    explicit ConcurrentMap(std::size_t bucket_count) : maps_(bucket_count),
                                                       mutex_maps_(bucket_count),
                                                       bucket_count_(bucket_count){
    }

    Access operator[](const Key& key){
        size_t index = static_cast<uint64_t>(key) % bucket_count_;
        return {mutex_maps_[index], maps_[index], key};
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for(size_t i = 0; i < bucket_count_; ++i){
            std::lock_guard lockGuard(mutex_maps_[i]);
            result.merge(maps_[i]);
        }
        return result;
    }
    ~ConcurrentMap(){
        for_each(mutex_maps_.begin(), mutex_maps_.end(),
                 [](auto& mutex_map){
                     mutex_map.lock();
                     mutex_map.unlock();
                 });
    }

private:
    std::vector<std::map<Key, Value>> maps_;
    std::vector<std::mutex> mutex_maps_;
    std::size_t bucket_count_;
};
