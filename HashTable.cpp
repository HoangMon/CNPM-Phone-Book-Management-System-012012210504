#include "HashTable.h"

#include <algorithm>
#include <cctype>

// ===========================================================================
// Hàm hỗ trợ (chỉ dùng trong file này)
// ===========================================================================
namespace {

bool isPrime(std::size_t n) {
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;
    for (std::size_t i = 3; i * i <= n; i += 2)
        if (n % i == 0) return false;
    return true;
}

std::size_t nextPrime(std::size_t n) {
    while (!isPrime(n)) ++n;
    return n;
}

std::string toLower(const std::string& s) {
    std::string out(s);
    for (std::size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

} // namespace

// ===========================================================================
// Constructor / Rule of three
// ===========================================================================
HashTable::HashTable(KeyField keyField, std::size_t capacity,
                     bool allowDuplicateKeys, double maxLoadFactor)
    : keyField_(keyField),
      allowDup_(allowDuplicateKeys),
      maxLoad_(maxLoadFactor > 0.1 ? maxLoadFactor : 0.75),
      count_(0),
      buckets_(nextPrime(capacity < 3 ? 3 : capacity), nullptr) {}

HashTable::HashTable(const HashTable& other)
    : keyField_(other.keyField_),
      allowDup_(other.allowDup_),
      maxLoad_(other.maxLoad_),
      count_(0) {
    copyFrom(other);
}

HashTable& HashTable::operator=(const HashTable& other) {
    if (this != &other) {
        destroy();
        keyField_ = other.keyField_;
        allowDup_ = other.allowDup_;
        maxLoad_  = other.maxLoad_;
        copyFrom(other);
    }
    return *this;
}

HashTable::~HashTable() {
    destroy();
}

// ===========================================================================
// Hàm private
// ===========================================================================
std::string HashTable::normalizeKey(const std::string& raw) const {
    std::string out;
    if (keyField_ == KeyField::Phone) {
        // "090-123 4567" và "0901234567" là cùng một số
        for (std::size_t i = 0; i < raw.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(raw[i]);
            if (std::isdigit(c) || c == '+') out.push_back(static_cast<char>(c));
        }
    } else {
        // "  Nguyen   An " -> "nguyen an"
        bool pendingSpace = false;
        for (std::size_t i = 0; i < raw.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(raw[i]);
            if (std::isspace(c)) {
                pendingSpace = !out.empty();
            } else {
                if (pendingSpace) { out.push_back(' '); pendingSpace = false; }
                out.push_back(static_cast<char>(std::tolower(c)));
            }
        }
    }
    return out;
}

std::string HashTable::keyOf(const Contact& contact) const {
    return normalizeKey(keyField_ == KeyField::Phone ? contact.getPhoneNumber() : contact.getFullName());
}

// FNV-1a 64 bit
std::size_t HashTable::hashFunction(const std::string& normalizedKey) const {
    unsigned long long h = 1469598103934665603ULL;
    for (std::size_t i = 0; i < normalizedKey.size(); ++i) {
        h ^= static_cast<unsigned char>(normalizedKey[i]);
        h *= 1099511628211ULL;
    }
    return static_cast<std::size_t>(h % buckets_.size());
}

HashTable::Entry* HashTable::findEntry(const std::string& normalizedKey) {
    if (normalizedKey.empty()) return nullptr;
    for (Entry* e = buckets_[hashFunction(normalizedKey)]; e != nullptr; e = e->next)
        if (e->key == normalizedKey) return e;
    return nullptr;
}

const HashTable::Entry* HashTable::findEntry(const std::string& normalizedKey) const {
    if (normalizedKey.empty()) return nullptr;
    for (const Entry* e = buckets_[hashFunction(normalizedKey)]; e != nullptr; e = e->next)
        if (e->key == normalizedKey) return e;
    return nullptr;
}

void HashTable::rehash(std::size_t newCapacity) {
    std::vector<Entry*> newBuckets(newCapacity, nullptr);
    std::vector<Entry*> tails(newCapacity, nullptr);   // giữ nguyên thứ tự trong chain

    std::vector<Entry*> oldBuckets;
    oldBuckets.swap(buckets_);
    buckets_.swap(newBuckets);                          // buckets_ = bảng mới (rỗng)

    for (std::size_t i = 0; i < oldBuckets.size(); ++i) {
        Entry* e = oldBuckets[i];
        while (e != nullptr) {
            Entry* nxt = e->next;
            e->next = nullptr;
            std::size_t idx = hashFunction(e->key);     // dùng buckets_.size() mới
            if (tails[idx] == nullptr) buckets_[idx] = e;
            else                       tails[idx]->next = e;
            tails[idx] = e;
            e = nxt;
        }
    }
}

void HashTable::copyFrom(const HashTable& other) {
    buckets_.assign(other.buckets_.size(), nullptr);
    count_ = 0;
    for (std::size_t i = 0; i < other.buckets_.size(); ++i) {
        Entry* tail = nullptr;
        for (Entry* e = other.buckets_[i]; e != nullptr; e = e->next) {
            Entry* copy = new Entry(e->key, e->data);
            if (tail == nullptr) buckets_[i] = copy;
            else                 tail->next = copy;
            tail = copy;
            ++count_;
        }
    }
}

void HashTable::destroy() {
    for (std::size_t i = 0; i < buckets_.size(); ++i) {
        Entry* e = buckets_[i];
        while (e != nullptr) {
            Entry* nxt = e->next;
            delete e;
            e = nxt;
        }
        buckets_[i] = nullptr;
    }
    count_ = 0;
}

// ===========================================================================
// Thêm / xoá / cập nhật
// ===========================================================================
bool HashTable::insert(const Contact& contact) {
    std::string key = keyOf(contact);
    if (key.empty()) return false;
    if (!allowDup_ && findEntry(key) != nullptr) return false;   // trùng khoá

    if (static_cast<double>(count_ + 1) / static_cast<double>(buckets_.size()) > maxLoad_)
        rehash(nextPrime(buckets_.size() * 2 + 1));

    Entry* node = new Entry(key, contact);
    std::size_t idx = hashFunction(key);

    // thêm vào cuối chain để giữ thứ tự thêm vào
    if (buckets_[idx] == nullptr) {
        buckets_[idx] = node;
    } else {
        Entry* cur = buckets_[idx];
        while (cur->next != nullptr) cur = cur->next;
        cur->next = node;
    }
    ++count_;
    return true;
}

std::size_t HashTable::remove(const std::string& key) {
    std::string k = normalizeKey(key);
    if (k.empty()) return 0;

    std::size_t idx = hashFunction(k);
    std::size_t removed = 0;
    Entry* prev = nullptr;
    Entry* cur  = buckets_[idx];

    while (cur != nullptr) {
        if (cur->key == k) {
            Entry* victim = cur;
            cur = cur->next;
            if (prev == nullptr) buckets_[idx] = cur;
            else                 prev->next = cur;
            delete victim;
            --count_;
            ++removed;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }
    return removed;
}

bool HashTable::removeById(const std::string& key, int contactId) {
    std::string k = normalizeKey(key);
    if (k.empty()) return false;

    std::size_t idx = hashFunction(k);
    Entry* prev = nullptr;
    Entry* cur  = buckets_[idx];

    while (cur != nullptr) {
        if (cur->key == k && cur->data.getContactID() == contactId) {
            if (prev == nullptr) buckets_[idx] = cur->next;
            else                 prev->next = cur->next;
            delete cur;
            --count_;
            return true;
        }
        prev = cur;
        cur = cur->next;
    }
    return false;
}

bool HashTable::update(const std::string& oldKey, const Contact& updated) {
    std::string oldK = normalizeKey(oldKey);
    std::string newK = keyOf(updated);
    if (oldK.empty() || newK.empty()) return false;

    // 1) tìm contact cũ (đúng khoá và đúng contactId)
    Entry* old = nullptr;
    for (Entry* e = buckets_[hashFunction(oldK)]; e != nullptr; e = e->next) {
        if (e->key == oldK && e->data.getContactID() == updated.getContactID()) { 
            old = e; 
            break; 
        }
    }
    if (old == nullptr) return false;

    // 2) khoá không đổi -> ghi đè dữ liệu tại chỗ
    if (newK == oldK) {
        old->data = updated;
        return true;
    }

    // 3) khoá đổi: ở chế độ unique, khoá mới không được trùng contact khác
    if (!allowDup_ && findEntry(newK) != nullptr) return false;

    // 4) xoá bản cũ rồi chèn bản mới (insert không thể thất bại tới đây)
    removeById(oldK, updated.getContactID());
    return insert(updated);
}

// ===========================================================================
// Tra cứu
// ===========================================================================
bool HashTable::contains(const std::string& key) const {
    return findEntry(normalizeKey(key)) != nullptr;
}

Contact* HashTable::find(const std::string& key) {
    Entry* e = findEntry(normalizeKey(key));
    return e ? &e->data : nullptr;
}

const Contact* HashTable::find(const std::string& key) const {
    Entry* e = const_cast<HashTable*>(this)->findEntry(normalizeKey(key));
    return e ? &e->data : nullptr;
}

std::vector<Contact> HashTable::findAll(const std::string& key) const {
    std::vector<Contact> result;
    std::string k = normalizeKey(key);
    if (k.empty()) return result;
    for (const Entry* e = buckets_[hashFunction(k)]; e != nullptr; e = e->next)
        if (e->key == k) result.push_back(e->data);
    return result;
}

std::vector<Contact> HashTable::searchByKeyword(const std::string& keyword) const {
    std::vector<Contact> result;
    std::string kw = toLower(keyword);
    if (kw.empty()) return getAll();

    // so khớp SĐT theo chữ số, để "090 12" vẫn khớp "0901234567"
    std::string kwDigits;
    for (std::size_t i = 0; i < kw.size(); ++i)
        if (std::isdigit(static_cast<unsigned char>(kw[i])) || kw[i] == '+')
            kwDigits.push_back(kw[i]);

    for (std::size_t i = 0; i < buckets_.size(); ++i) {
        for (const Entry* e = buckets_[i]; e != nullptr; e = e->next) {
            const Contact& c = e->data;
            bool hit = toLower(c.getFullName()).find(kw)  != std::string::npos ||
                       toLower(c.getEmail()).find(kw)     != std::string::npos ||
                       c.getPhoneNumber().find(keyword)   != std::string::npos;
            if (!hit && !kwDigits.empty()) {
                std::string phoneDigits;
                std::string pNum = c.getPhoneNumber();
                for (std::size_t j = 0; j < pNum.size(); ++j)
                    if (std::isdigit(static_cast<unsigned char>(pNum[j])) || pNum[j] == '+')
                        phoneDigits.push_back(pNum[j]);
                hit = phoneDigits.find(kwDigits) != std::string::npos;
            }
            if (hit) result.push_back(c);
        }
    }
    return result;
}

// ===========================================================================
// Tiện ích
// ===========================================================================
std::vector<Contact> HashTable::getAll() const {
    std::vector<Contact> result;
    result.reserve(count_);
    for (std::size_t i = 0; i < buckets_.size(); ++i)
        for (Entry* e = buckets_[i]; e != nullptr; e = e->next)
            result.push_back(e->data);
    return result;
}

double HashTable::loadFactor() const {
    return buckets_.empty() ? 0.0
                            : static_cast<double>(count_) / static_cast<double>(buckets_.size());
}

std::size_t HashTable::longestChain() const {
    std::size_t longest = 0;
    for (std::size_t i = 0; i < buckets_.size(); ++i) {
        std::size_t len = 0;
        for (Entry* e = buckets_[i]; e != nullptr; e = e->next) ++len;
        if (len > longest) longest = len;
    }
    return longest;
}

void HashTable::clear() {
    destroy();   // giữ nguyên số bucket hiện tại
}

void HashTable::display(std::ostream& os) const {
    os << "HashTable [" << (keyField_ == KeyField::Phone ? "key=phone" : "key=name")
       << ", " << (allowDup_ ? "duplicates allowed" : "unique")
       << "] size=" << count_ << ", buckets=" << buckets_.size()
       << ", load=" << loadFactor() << ", longestChain=" << longestChain() << "\n";

    for (std::size_t i = 0; i < buckets_.size(); ++i) {
        if (buckets_[i] == nullptr) continue;            // bỏ qua bucket rỗng
        os << "  [" << i << "] ";
        for (Entry* e = buckets_[i]; e != nullptr; e = e->next) {
            os << "(" << e->data.getContactID() << ": " << e->data.getFullName()
               << " | " << e->data.getPhoneNumber() << ")";
            if (e->next != nullptr) os << " -> ";
        }
        os << "\n";
    }
}