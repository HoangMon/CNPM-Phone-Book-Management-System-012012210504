#ifndef HASHTABLE_H
#define HASHTABLE_H
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include "Contact.h"
class HashTable {
public:
    enum class KeyField { Phone, Name };

    // capacity: số bucket ban đầu (sẽ được làm tròn lên số nguyên tố)
    // allowDuplicateKeys: true -> cho nhiều contact cùng khoá
    // maxLoadFactor: vượt ngưỡng này thì tự rehash (nhân đôi số bucket)
    explicit HashTable(KeyField keyField = KeyField::Phone,
                       std::size_t capacity = 101,
                       bool allowDuplicateKeys = false,
                       double maxLoadFactor = 0.75);

    // Rule of three: bảng tự quản lý bộ nhớ động
    HashTable(const HashTable& other);
    HashTable& operator=(const HashTable& other);
    ~HashTable();

    // ----- Thêm / xoá / cập nhật -----
    // Trả về false nếu khoá rỗng, hoặc (chế độ unique) khoá đã tồn tại.
    bool insert(const Contact& contact);

    // Xoá TẤT CẢ contact có khoá này. Trả về số phần tử đã xoá.
    std::size_t remove(const std::string& key);

    // Xoá đúng 1 contact (theo contactId) trong số các contact cùng khoá.
    bool removeById(const std::string& key, int contactId);

    // Cập nhật contact (xác định theo updated.contactId) đang nằm ở oldKey.
    // Khoá mới (SĐT/tên mới) được lấy từ `updated`.
    // Trả về false nếu không thấy contact cũ, khoá mới rỗng,
    // hoặc (chế độ unique) khoá mới đã bị contact khác dùng -> bảng không đổi.
    bool update(const std::string& oldKey, const Contact& updated);

    // ----- Tra cứu -----
    bool contains(const std::string& key) const;
    Contact*       find(const std::string& key);               // nullptr nếu không có
    const Contact* find(const std::string& key) const;
    std::vector<Contact> findAll(const std::string& key) const;  // mọi contact cùng khoá

    // Tìm gần đúng (không phân biệt hoa/thường) theo tên, SĐT hoặc email.
    // Phải duyệt toàn bảng O(n) vì bảng băm không hỗ trợ tìm theo tiền tố.
    std::vector<Contact> searchByKeyword(const std::string& keyword) const;

    // ----- Tiện ích -----
    std::vector<Contact> getAll() const;
    std::size_t size() const        { return count_; }
    bool        empty() const       { return count_ == 0; }
    std::size_t bucketCount() const { return buckets_.size(); }
    double      loadFactor() const;
    std::size_t longestChain() const;
    void clear();
    void display(std::ostream& os = std::cout) const; 

private:
    struct Entry {
        std::string key;        
        Contact     data;
        Entry*      next;
        Entry(const std::string& k, const Contact& c) : key(k), data(c), next(nullptr) {}
    };

    KeyField            keyField_;
    bool                allowDup_;
    double              maxLoad_;
    std::size_t         count_;
    std::vector<Entry*> buckets_;

    // Chuẩn hoá khoá: SĐT chỉ giữ chữ số và '+', tên bỏ khoảng trắng thừa + chữ thường
    std::string normalizeKey(const std::string& raw) const;
    std::string keyOf(const Contact& contact) const;

    std::size_t hashFunction(const std::string& normalizedKey) const;  // FNV-1a
    Entry*      findEntry(const std::string& normalizedKey);
    const Entry* findEntry(const std::string& normalizedKey) const;
    void        rehash(std::size_t newCapacity);
    void        copyFrom(const HashTable& other);
    void        destroy();
};

#endif 