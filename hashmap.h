#include <forward_list>
#include <functional>
#include <vector>
#include <stdexcept>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
private:
    static const size_t sizes[];

    size_t cur_size = 0, cur_len = 0;
    std::vector<std::forward_list<std::pair<const KeyType, ValueType>>> buckets;
    Hash hasher;

    void change_size(size_t new_sz) {
        if (new_sz == cur_len) {
            return;
        }
        new_sz = sizes[cur_len = new_sz];
        std::vector<std::forward_list<std::pair<const KeyType, ValueType>>> new_buckets(new_sz);
        for (auto &b : buckets) {
            for (auto &x : b) {
                new_buckets[hasher(x.first) % new_sz].push_front(std::move(x));
            }
        }
        buckets.swap(new_buckets);
    }

    size_t load_factor() const {
        return cur_size * 100 / sizes[cur_len];
    }

    bool trySizeUp() {
        if (load_factor() > 130) {
            change_size(cur_len + 1);
            return true;
        }
        return false;
    }

    bool trySizeDown() {
        if (load_factor() < 30 && cur_len) {
            change_size(cur_len - 1);
            return true;
        }
        return false;
    }

public:
    HashMap(const Hash &Hasher = Hash()) : buckets(sizes[0]), hasher(Hasher) {}

    template<class Iter>
    HashMap(Iter beg, Iter end, const Hash &Hasher = Hash()) : HashMap(Hasher) {
        for (Iter it = beg; it != end; ++it) {
            insert(*it);
        }
    }

    HashMap(const std::initializer_list<std::pair<const KeyType, ValueType>> &l, const Hash &Hasher = Hash()) : HashMap(l.begin(), l.end(), Hasher) {}

    HashMap(const std::vector<std::pair<const KeyType, ValueType>> &vals, const Hash &Hasher = Hash()) : HashMap(vals.begin(), vals.end(), Hasher) {}

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

    void insert(const std::pair<KeyType, ValueType> &val) {
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
        size_t h = hasher(key) % sizes[cur_len];
        for (auto &[x, y] : buckets[h]) {
            if (x == key) {
                return y;
            }
        }
        buckets[h].push_front({key, ValueType()});
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
        for (auto &b : buckets) {
            b.clear();
        }
        cur_size = 0;
        change_size(0);
    }

    class const_iterator {
        friend HashMap;
    private:
        typename std::vector<std::forward_list<std::pair<const KeyType, ValueType>>>::const_iterator cur, end;
        typename std::forward_list<std::pair<const KeyType, ValueType>>::const_iterator biter;

        void push() {
            while (cur != end && biter == cur->end()) {
                ++cur;
                if (cur != end) {
                    biter = cur->begin();
                }
            }
        }

    public:
        const_iterator() {}
        const_iterator(const HashMap &mp)  : cur(mp.buckets.begin()), end(mp.buckets.end()), biter(cur->begin()) {}
        const_iterator(const const_iterator &it) : cur(it.cur), end(it.end), biter(it.biter) {}

        const_iterator& operator++() {
            ++biter;
            push();
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator cp(*this);
            ++(*this);
            return cp;
        }

        const_iterator& operator=(const const_iterator &iter) {
            cur = iter.cur;
            end = iter.end;
            biter = iter.biter;
            return *this;
        }

        const std::pair<const KeyType, ValueType>& operator*() {
            return *biter;
        }

        typename std::forward_list<std::pair<const KeyType, ValueType>>::const_iterator operator->() {
            return biter;
        }

        bool operator==(const const_iterator &it) {
            return it.cur == cur && it.biter == biter;
        }

        bool operator!=(const const_iterator &it) {
            return !(*this == it);
        }
    };

    class iterator {
        friend HashMap;
    private:
        typename std::vector<std::forward_list<std::pair<const KeyType, ValueType>>>::iterator cur, end;
        typename std::forward_list<std::pair<const KeyType, ValueType>>::iterator biter;

        void push() {
            while (cur != end && biter == cur->end()) {
                ++cur;
                if (cur != end) {
                    biter = cur->begin();
                }
            }
        }

    public:
        iterator() {}
        iterator(HashMap &mp)  : cur(mp.buckets.begin()), end(mp.buckets.end()), biter(cur->begin()) {}
        iterator(const iterator &it) : cur(it.cur), end(it.end), biter(it.biter) {}

        iterator& operator++() {
            ++biter;
            push();
            return *this;
        }

        iterator operator++(int) {
            iterator cp(*this);
            ++(*this);
            return cp;
        }

        iterator& operator=(const iterator &iter) {
            cur = iter.cur;
            end = iter.end;
            biter = iter.biter;
            return *this;
        }

        const std::pair<const KeyType, ValueType>& operator*() {
            return *biter;
        }

        typename std::forward_list<std::pair<const KeyType, ValueType>>::iterator operator->() {
            return biter;
        }

        bool operator==(const iterator &it) {
            return it.cur == cur && it.biter == biter;
        }

        bool operator!=(const iterator &it) {
            return !(*this == it);
        }
    };

    const_iterator find(const KeyType &val) const {
        size_t h = hasher(val) % buckets.size();
        for (auto it = buckets[h].begin(); it != buckets[h].end(); ++it) {
            if (it->first == val) {
                const_iterator iter(*this);
                iter.cur = buckets.begin() + h;
                iter.biter = it;
                return iter;
            }
        }
        return end();
    }

    iterator find(const KeyType &val) {
        size_t h = hasher(val) % buckets.size();
        for (auto it = buckets[h].begin(); it != buckets[h].end(); ++it) {
            if (it->first == val) {
                iterator iter(*this);
                iter.cur = buckets.begin() + h;
                iter.biter = it;
                return iter;
            }
        }
        return end();
    }

     iterator begin() {
        iterator iter(*this);
        iter.push();
        return iter;
     }

     iterator end() {
        iterator iter(*this);
        iter.cur = iter.end;
        iter.biter = buckets.back().end();
        return iter;
     }

     const_iterator begin() const {
        const_iterator iter(*this);
        iter.push();
        return iter;
     }

     const_iterator end() const {
        const_iterator iter(*this);
        iter.cur = iter.end;
        iter.biter = buckets.back().end();
        return iter;
     }
};

template<class KeyType, class ValueType, class Hash>
const size_t HashMap<KeyType, ValueType, Hash>::sizes[] = {
    5, 11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421,
    12853, 25717, 51437, 102877, 205759, 411527, 823117,
    1646237, 3292489, 6584983, 13169977, 26339969, 52679969};
