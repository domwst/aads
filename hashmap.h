#include <forward_list>
#include <functional>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include <utility>

template<class vector_iter, class list_iter>
class base_hashmap_iterator {
private:
    vector_iter cur, end;
    list_iter biter;

    void push() {
        while (cur != end && biter == cur->end()) {
            ++cur;
            if (cur != end) {
                biter = cur->begin();
            }
        }
    }

public:
    base_hashmap_iterator() = default;
    base_hashmap_iterator(const base_hashmap_iterator &it) = default;

    base_hashmap_iterator(const vector_iter &_cur, const list_iter &_biter, const vector_iter &_end) :
        cur(_cur), end(_end), biter(_biter) {}

    template<class list_type>
    base_hashmap_iterator(std::vector<list_type> &buckets, bool back = false) :
        cur(buckets.begin()), end(buckets.end()), biter(cur->begin()) {
        
        if (!back) {
            push();
        } else {
            cur = end = buckets.end();
            biter = buckets.back().end();
        }
    }

    template<class list_type>
    base_hashmap_iterator(const std::vector<list_type> &buckets, bool back = false) :
        cur(buckets.begin()), end(buckets.end()), biter(cur->begin()) {
        
        if (!back) {
            push();
        } else {
            cur = end = buckets.end();
            biter = buckets.back().end();
        }
    }

    void bucket_advance(size_t l) {
        cur += l;
    }

    base_hashmap_iterator& operator++() {
        ++biter;
        push();
        return *this;
    }

    base_hashmap_iterator operator++(int) {
        base_hashmap_iterator cp(*this);
        ++(*this);
        return cp;
    }

    base_hashmap_iterator& operator=(const base_hashmap_iterator &iter) {
        cur = iter.cur;
        end = iter.end;
        biter = iter.biter;
        return *this;
    }

    typename list_iter::value_type operator*() {
        return *biter;
    }

    list_iter operator->() {
        return biter;
    }

    bool operator==(const base_hashmap_iterator &it) {
        return it.cur == cur && it.biter == biter;
    }

    bool operator!=(const base_hashmap_iterator &it) {
        return !(*this == it);
    }
};

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
private:
    static constexpr size_t sizes[] = {
    5, 11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421,
    12853, 25717, 51437, 102877, 205759, 411527, 823117,
    1646237, 3292489, 6584983, 13169977, 26339969, 52679969};
    static constexpr size_t growLoadFactorThreshold = 130, shrintLoadFactorThreshold = 30;

    using stored_type = std::pair<const KeyType, ValueType>;
    using list_type = std::forward_list<stored_type>;
    using vector_type = std::vector<list_type>;

    size_t cur_size = 0, cur_capacity = 0;
    vector_type buckets;
    Hash hasher;

    void change_size(size_t new_capacity) {
        if (new_capacity == cur_capacity) {
            return;
        }
        cur_capacity = new_capacity;
        new_capacity = sizes[cur_capacity];
        vector_type new_buckets(new_capacity);
        for (auto &b : buckets) {
            for (auto &x : b) {
                new_buckets[hasher(x.first) % new_capacity].push_front(std::move(x));
            }
        }
        buckets.swap(new_buckets);
    }

    size_t load_factor() const {  // returns cur_size / cur_capacity in percents. i.e. returns load factor in percents
        return cur_size * 100 / sizes[cur_capacity];
    }

    bool trySizeUp() {
        if (load_factor() > growLoadFactorThreshold) {
            change_size(cur_capacity + 1);
            return true;
        }
        return false;
    }

    bool trySizeDown() {
        if (load_factor() < shrintLoadFactorThreshold && cur_capacity > 0) {
            change_size(cur_capacity - 1);
            return true;
        }
        return false;
    }

    std::pair<size_t, size_t> base_find(const KeyType &val) const {
        size_t h = hasher(val) % buckets.size();
        for (auto it = buckets[h].begin(), cur = 0ull; it != buckets[h].end(); ++it, ++cur) {
            if (it->first == val) {
                return std::make_pair(h, (size_t)cur);
            }
        }
        return std::make_pair(buckets.size(), std::distance(buckets.back().begin(), buckets.back().end()));
    }

public:
    explicit HashMap(const Hash &Hasher = Hash()) : buckets(sizes[0]), hasher(Hasher) {}

    template<class Iter>
    HashMap(Iter beg, Iter end, const Hash &Hasher = Hash()) : HashMap(Hasher) {
        for (Iter it = beg; it != end; ++it) {
            insert(*it);
        }
    }

    HashMap(const std::initializer_list<stored_type> &l, const Hash &Hasher = Hash()) :
        HashMap(l.begin(), l.end(), Hasher) {}

    HashMap(const std::vector<stored_type> &vals, const Hash &Hasher = Hash()) :
        HashMap(vals.begin(), vals.end(), Hasher) {}

    using iterator = base_hashmap_iterator<typename vector_type::iterator, typename list_type::iterator>;
    using const_iterator = base_hashmap_iterator<typename vector_type::const_iterator,
                                                 typename list_type::const_iterator>;

    size_t size() const {
        return cur_size;
    }

    bool empty() const {
        return cur_size == 0;
    }

    const Hash& hash_function() const {
        return hasher;
    }

    size_t count(const KeyType &key) const {
        size_t h = hasher(key) % buckets.size();
        for (auto &[x, y] : buckets[h]) {
            if (x == key) {
                return 1;
            }
        }
        return 0;
    }

    void insert(const stored_type &val) {
        if (count(val.first))
            return;
        buckets[hasher(val.first) % buckets.size()].push_front(val);
        ++cur_size;
        trySizeUp();
    }

    void erase(const KeyType &key) {
        size_t h = hasher(key) % buckets.size();
        for (auto it = buckets[h].before_begin(); next(it) != buckets[h].end(); ++it) {
            if (next(it)->first == key) {
                buckets[h].erase_after(it);
                --cur_size;
                break;
            }
        }
        trySizeDown();
    }

    ValueType& operator[](const KeyType &key) {
        size_t h = hasher(key) % sizes[cur_capacity];
        for (auto &[x, y] : buckets[h]) {
            if (x == key) {
                return y;
            }
        }
        buckets[h].emplace_front(key, ValueType());
        ++cur_size;
        if (trySizeUp()) {
            return operator[](key);
        }
        return buckets[h].front().second;
    }

    const ValueType& at(const KeyType &key) const {
        size_t h = hasher(key) % buckets.size();
        for (auto &[x, y] : buckets[h]) {
            if (x == key) {
                return y;
            }
        }
        throw std::out_of_range("No such key");
    }

    void clear() {
        for (auto &bucket : buckets) {
            bucket.clear();
        }
        cur_size = 0;
        change_size(0);
    }

    const_iterator find(const KeyType &val) const {
        auto [block_idx, item_idx] = base_find(val);
        auto item_iterator = buckets[std::min(block_idx, buckets.size() - 1)].begin();  // For case of end iterator
        std::advance(item_iterator, item_idx);
        return const_iterator(buckets.begin() + block_idx,
                              item_iterator, buckets.end());
    }

    iterator find(const KeyType &val) {
        auto [block_idx, item_idx] = base_find(val);
        auto item_iterator = buckets[std::min(block_idx, buckets.size() - 1)].begin();
        std::advance(item_iterator, item_idx);
        return iterator(buckets.begin() + block_idx,
                        item_iterator, buckets.end());
    }

     iterator begin() {
        iterator iter(buckets);
        return iter;
     }

     iterator end() {
        iterator iter(buckets, true);
        return iter;
     }

     const_iterator begin() const {
        const_iterator iter(buckets);
        return iter;
     }

     const_iterator end() const {
        const_iterator iter(buckets, true);
        return iter;
     }
};
