#pragma once
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <iterator>

using namespace std::string_literals;

//this class implements secure multithread adding, changing and erasing elements of the map 
//and creating ordinary map
template <typename Key, typename Value>
class ConcurrentMap {
public:
	//key must be an integrer
	static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

	//this structure implements secure multithread access to element
	struct Access {
		std::lock_guard<std::mutex> guard;

		Value& ref_to_value;

		Access(const Key& key, std::mutex& bucket_mutex, std::map<Key, Value>& map_part) :
			guard(bucket_mutex), ref_to_value(map_part[key]) {
		}

		Access& operator+=(const Value& other) {
			ref_to_value += other;
			return *this;
		}
	};

	explicit ConcurrentMap(size_t bucket_count) :
		buckets_mutexes_(bucket_count), buckets_amount(bucket_count) {
		while (bucket_count > 0) {
			conc_map_[--bucket_count];
		};
	}

	//overloading [] operator to secure multithread access to map element
	Access operator[](const Key& key) {
		size_t bucket_number = static_cast<size_t>(key) % buckets_amount;
		return Access(key, buckets_mutexes_[bucket_number], conc_map_[bucket_number]);
	}

	std::map<Key, Value> BuildOrdinaryMap() {
		std::map<Key, Value> ordinary_map;

		for (auto& [index, map_part] : conc_map_) {
			std::lock_guard lock(buckets_mutexes_[index]);
			ordinary_map.insert(map_part.begin(), map_part.end());
		};

		return ordinary_map;
	}

	//secure erasing of element using locking
	void erase(const Key& key) {
		size_t bucket_number = static_cast<size_t>(key) % buckets_amount;
		std::lock_guard lock(buckets_mutexes_[bucket_number]);
		conc_map_[bucket_number].erase(key);
	}
private:
	std::map<size_t, std::map<Key, Value>> conc_map_;
	std::vector<std::mutex> buckets_mutexes_;
	size_t buckets_amount;
};