#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include "Contact.h"
#include "Group.h"

#ifdef PHONEBOOK_USE_MYSQL
#  include <mysql.h>
#else
// Compatibility adapter: AccountService dang dung MYSQL_RES/MYSQL_ROW.
// Day KHONG phai ket noi MySQL. Ket qua duoc tra ve tu file cuc bo.
struct MYSQL { int dummy = 0; };
struct MYSQL_RES {
    std::vector<std::vector<std::string>> rows;
    std::size_t currentRow = 0;
    std::vector<char*> activeRow;
};
using MYSQL_ROW = char**;
inline MYSQL* mysql_init(MYSQL*) { return new MYSQL(); }
inline void mysql_close(MYSQL* p) { delete p; }
inline MYSQL_ROW mysql_fetch_row(MYSQL_RES* r) {
    if (!r || r->currentRow >= r->rows.size()) return nullptr;
    r->activeRow.clear();
    auto& row = r->rows[r->currentRow++];
    for (std::string& value : row) r->activeRow.push_back(value.data());
    r->activeRow.push_back(nullptr);
    return r->activeRow.data();
}
inline void mysql_free_result(MYSQL_RES* r) { delete r; }
#endif

struct AccountData {
    int id = 0;
    std::string username;
    std::string email;
    std::string password;   // SHA-256 hex digest (khong phai mat khau goc)
    std::string fullname;
    std::string phone;
    std::string role;
};

class DatabaseConnector {
public:
    virtual ~DatabaseConnector() = default;
    virtual MYSQL_RES* executeQuery(const std::string& query) = 0;
    virtual bool executeNonQuery(const std::string& query) = 0;
    virtual MYSQL_RES* fetchQuery(const std::string& query) = 0;
    virtual void freeResult(MYSQL_RES* result) = 0;
    virtual int getInsertId() const = 0;
    virtual std::string escapeString(const std::string& str) const = 0;
    virtual MYSQL* getConn() = 0;
    virtual bool isConnected() const = 0;
};

class DBManager : public DatabaseConnector {
private:
    MYSQL* conn = nullptr;
    bool connected = false;
    int lastInsertId = 0;
    std::string storageFile;

    struct AccountRecord {
        int id = 0;
        std::string username, email, password, fullname, phone, role;
    };
    std::vector<AccountRecord> accountRecords;

    bool loadAccounts();
    bool saveAccounts() const;
    std::string getRelatedStorageFile(const std::string& suffix) const;
    bool connectToServer(const std::string& host, const std::string& user,
                         const std::string& password, const std::string& database,
                         unsigned int port);
    bool initializeSchema();

public:
    DBManager();
    ~DBManager() override;
    DBManager(const DBManager&) = delete;
    DBManager& operator=(const DBManager&) = delete;

    bool connect(const std::string& host, const std::string& user,
                 const std::string& password, const std::string& database,
                 unsigned int port = 3306);
    MYSQL* getConn() override;
    MYSQL_RES* executeQuery(const std::string& query) override;
    bool executeNonQuery(const std::string& query) override;
    MYSQL_RES* fetchQuery(const std::string& query) override;
    void freeResult(MYSQL_RES* result) override;
    std::string escapeString(const std::string& str) const override;
    int getInsertId() const override;
    bool isConnected() const override;
    std::string getStorageFile() const;

    std::vector<AccountData> getAccounts() const;
    bool getAccount(int accountId, AccountData& account) const;
    bool updateAccount(const AccountData& account);
    bool deleteAccount(int accountId);

    std::vector<Contact> loadContacts(int accountId) const;
    bool saveContact(const Contact& contact) const;
    bool updateContact(const Contact& contact) const;
    bool deleteContact(int accountId, int contactId) const;

    std::vector<Group> loadGroups(int accountId) const;
    bool saveGroup(const Group& group) const;
    bool updateGroup(const Group& group) const;
    bool deleteGroup(int accountId, int groupId) const;

    std::vector<ContactGroup> loadContactGroups(int accountId) const;
    bool saveContactGroup(int accountId, const ContactGroup& relation) const;
    bool deleteContactGroups(int accountId, int groupId) const;
    bool deleteContactGroup(int accountId, int contactId) const;
};

extern DBManager db;

