#ifndef PHONEBOOK_USE_MYSQL
#include "DBManager.h"
#include "Utils.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <tuple>
#include <vector>
#ifdef _WIN32
#  define NOMINMAX
#  include <windows.h>
#endif

DBManager db;
namespace {
namespace fs = std::filesystem;

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}
std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \r\n\t");
    if (b == std::string::npos) return {};
    const auto e = s.find_last_not_of(" \r\n\t");
    return s.substr(b, e - b + 1);
}
std::string unescape(const std::string& s) {
    std::string result;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) ++i;
        result.push_back(s[i]);
    }
    return result;
}
std::string quotedParam(const std::string& sql, const std::string& name) {
    // Hanh vi chi phuc vu cau lenh SQL do AccountService tao, KHONG phai SQL parser tong quat.
    const std::regex pattern("(^|[^A-Za-z0-9_])" + name +
        R"(\s*=\s*'((?:\\.|[^'])*)')", std::regex::icase);
    std::smatch match;
    return std::regex_search(sql, match, pattern) ? unescape(match[2].str()) : "";
}
std::string shaParam(const std::string& sql, const std::string& name) {
    const std::regex pattern("(^|[^A-Za-z0-9_])" + name +
        R"(\s*=\s*SHA2\s*\(\s*'((?:\\.|[^'])*)'\s*,\s*256\s*\))", std::regex::icase);
    std::smatch match;
    return std::regex_search(sql, match, pattern) ? unescape(match[2].str()) : "";
}
int numberParam(const std::string& sql, const std::string& name) {
    const std::regex pattern("(^|[^A-Za-z0-9_])" + name +
        R"(\s*=\s*(\d+)\b)", std::regex::icase);
    std::smatch match;
    if (!std::regex_search(sql, match, pattern)) return -1;
    try { return std::stoi(match[2].str()); } catch (...) { return -1; }
}

// SHA-256: can thiet vi AccountService dung ham SQL SHA2(password,256).
// File fallback luu HASH thay vi mat khau ro. (Khong duoc coi la KDF manh.)
constexpr std::array<std::uint32_t, 64> K = {{
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
}};
std::uint32_t rotr(std::uint32_t x, int n) { return (x >> n) | (x << (32-n)); }
std::string sha256(const std::string& text) {
    std::vector<std::uint8_t> msg(text.begin(), text.end());
    const std::uint64_t bits = static_cast<std::uint64_t>(msg.size()) * 8;
    msg.push_back(0x80);
    while (msg.size() % 64 != 56) msg.push_back(0);
    for (int i = 7; i >= 0; --i) msg.push_back(static_cast<std::uint8_t>(bits >> (i * 8)));
    std::array<std::uint32_t,8> h = {{
        0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
        0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
    }};
    for (std::size_t p = 0; p < msg.size(); p += 64) {
        std::uint32_t w[64] = {};
        for (int i = 0; i < 16; ++i) {
            const std::size_t t = p + static_cast<std::size_t>(i) * 4;
            w[i] = (static_cast<std::uint32_t>(msg[t]) << 24) |
                   (static_cast<std::uint32_t>(msg[t+1]) << 16) |
                   (static_cast<std::uint32_t>(msg[t+2]) << 8) | msg[t+3];
        }
        for (int i = 16; i < 64; ++i) {
            const std::uint32_t a = rotr(w[i-15],7) ^ rotr(w[i-15],18) ^ (w[i-15] >> 3);
            const std::uint32_t b = rotr(w[i-2],17) ^ rotr(w[i-2],19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + a + w[i-7] + b;
        }
        auto a=h[0], b=h[1], c=h[2], d=h[3], e=h[4], f=h[5], g=h[6], hh=h[7];
        for (int i = 0; i < 64; ++i) {
            const auto s1 = rotr(e,6) ^ rotr(e,11) ^ rotr(e,25);
            const auto ch = (e & f) ^ (~e & g);
            const auto t1 = hh + s1 + ch + K[i] + w[i];
            const auto s0 = rotr(a,2) ^ rotr(a,13) ^ rotr(a,22);
            const auto maj = (a & b) ^ (a & c) ^ (b & c);
            const auto t2 = s0 + maj;
            hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }
    std::ostringstream os;
    os << std::hex << std::setfill('0');
    for (auto value : h) os << std::setw(8) << value;
    return os.str();
}

// Ghi file tam, sau do thay the file dich. Han che nguy co ghi file nua chung.
template<class T, class Writer>
bool writeAll(const std::string& filename, const std::vector<T>& rows, Writer writer) {
    if (filename.empty()) return false;
    const fs::path temp = filename + ".writing";
    {
        std::ofstream out(temp, std::ios::trunc | std::ios::binary);
        if (!out) return false;
        for (const auto& item : rows) writer(out, item);
        out.flush();
        if (!out) { out.close(); std::error_code ec; fs::remove(temp, ec); return false; }
    }
    std::error_code ec;
    fs::rename(temp, filename, ec);
    if (ec) { // Windows khong cho rename de len file cu.
        ec.clear();
        fs::copy_file(temp, filename, fs::copy_options::overwrite_existing, ec);
        std::error_code ignored;
        fs::remove(temp, ignored);
    }
    return !ec;
}

using Membership = std::tuple<int,int,int>;
std::vector<Contact> readAllContacts(const std::string& filename) {
    std::vector<Contact> result;
    std::ifstream in(filename);
    Contact r;
    while (in >> r.id >> r.account_id >> r.group_id >> std::quoted(r.name)
              >> std::quoted(r.phone) >> std::quoted(r.email) >> std::quoted(r.address)
              >> r.isFavorite) result.push_back(r);
    return result;
}
void writeContact(std::ostream& out, const Contact& r) {
    out << r.id << ' ' << r.account_id << ' ' << r.group_id << ' '
        << std::quoted(r.name) << ' ' << std::quoted(r.phone) << ' '
        << std::quoted(r.email) << ' ' << std::quoted(r.address) << ' '
        << r.isFavorite << '\n';
}
std::vector<Group> readAllGroups(const std::string& filename) {
    std::vector<Group> result;
    std::ifstream in(filename);
    Group r;
    while (in >> r.id >> r.account_id >> std::quoted(r.name)
              >> std::quoted(r.description)) result.push_back(r);
    return result;
}
void writeGroup(std::ostream& out, const Group& r) {
    out << r.id << ' ' << r.account_id << ' '
        << std::quoted(r.name) << ' ' << std::quoted(r.description) << '\n';
}
std::vector<Membership> readAllMemberships(const std::string& filename) {
    std::vector<Membership> result;
    std::ifstream in(filename);
    int a,c,g;
    while (in >> a >> c >> g) result.emplace_back(a,c,g);
    return result;
}
void writeMembership(std::ostream& out, const Membership& r) {
    out << std::get<0>(r) << ' ' << std::get<1>(r) << ' '
        << std::get<2>(r) << '\n';
}
}

DBManager::DBManager() : conn(mysql_init(nullptr)) {}
DBManager::~DBManager() { mysql_close(conn); }

bool DBManager::connect(const std::string&, const std::string&, const std::string&,
                        const std::string& database, unsigned int) {
    connected = false;
    fs::path folder;
    if (const char* overrideDir = std::getenv("PHONEBOOK_DATA_DIR")) {
        if (*overrideDir) folder = overrideDir;
    }
    if (folder.empty()) {
#ifdef _WIN32
        char path[MAX_PATH] = {};
        if (GetModuleFileNameA(nullptr, path, MAX_PATH))
            folder = fs::path(path).parent_path();
#else
        folder = fs::current_path();
#endif
    }
    if (folder.empty()) return false;
    std::error_code ec;
    fs::create_directories(folder, ec);
    if (ec) return false;
    // Khong cho phep database lam path traversal.
    const std::string name = std::regex_match(database, std::regex("[A-Za-z0-9_]+"))
        ? database : "phonebook";
    storageFile = (folder / (name + "_accounts.db")).string();
    if (!loadAccounts()) return false;
    connected = initializeSchema();
    return connected;
}
bool DBManager::connectToServer(const std::string&, const std::string&,
                                const std::string&, const std::string&, unsigned int) {
    return false; // Ban file KHONG ket noi MySQL. Bat PHONEBOOK_USE_MYSQL neu can.
}
bool DBManager::initializeSchema() { return !storageFile.empty(); }
bool DBManager::loadAccounts() {
    accountRecords.clear(); lastInsertId = 0;
    std::ifstream in(storageFile);
    if (!in) return !fs::exists(storageFile); // File moi chua co: hop le.
    AccountRecord r;
    while (in >> r.id >> std::quoted(r.username) >> std::quoted(r.email)
              >> std::quoted(r.password) >> std::quoted(r.fullname)
              >> std::quoted(r.phone) >> std::quoted(r.role)) {
        if (r.id <= 0) return false;
        accountRecords.push_back(r);
        lastInsertId = std::max(lastInsertId, r.id);
    }
    return !in.bad();
}
bool DBManager::saveAccounts() const {
    return writeAll(storageFile, accountRecords, [](std::ostream& out, const AccountRecord& r) {
        out << r.id << ' ' << std::quoted(r.username) << ' ' << std::quoted(r.email)
            << ' ' << std::quoted(r.password) << ' ' << std::quoted(r.fullname)
            << ' ' << std::quoted(r.phone) << ' ' << std::quoted(r.role) << '\n';
    });
}
std::string DBManager::getRelatedStorageFile(const std::string& suffix) const {
    return storageFile + suffix;
}
MYSQL* DBManager::getConn() { return conn; }
MYSQL_RES* DBManager::executeQuery(const std::string& query) {
    if (!connected) return nullptr;
    const auto normalized = lower(trim(query));
    if (normalized.rfind("select", 0) != 0 ||
        normalized.find("from accounts") == std::string::npos) return nullptr;
    const auto where = normalized.find("where");
    if (where == std::string::npos) return nullptr; // Khong ho tro SQL SELECT * tong quat.
    const std::string condition = query.substr(where + 5);
    const auto username = quotedParam(condition, "username");
    const auto email = quotedParam(condition, "email");
    const auto password = shaParam(condition, "password");
    const int accountId = numberParam(condition, "account_id");
    const int legacyId = numberParam(condition, "id");
    if (username.empty() && email.empty() && password.empty() && accountId < 0 && legacyId < 0)
        return nullptr;
    auto* result = new MYSQL_RES;
    bool migrated = false;
    for (auto& r : accountRecords) {
        if (!username.empty() && r.username != username) continue;
        if (!email.empty() && r.email != email) continue;
        if (accountId >= 0 && r.id != accountId) continue;
        if (legacyId >= 0 && r.id != legacyId) continue;
        if (!password.empty()) {
            const std::string digest = sha256(password);
            if (r.password != digest && r.password != password) continue;
            // Tu dong nang cap mat khau ro cua file cu sau khi xac thuc.
            if (r.password == password) { r.password = digest; migrated = true; }
        }
        result->rows.push_back({std::to_string(r.id)});
    }
    if (migrated) saveAccounts();
    return result; // Result rong: mysql_fetch_row se tra nullptr.
}
bool DBManager::executeNonQuery(const std::string& query) {
    if (!connected) return false;
    const auto normalized = lower(trim(query));
    if (normalized.rfind("create table", 0) == 0 ||
        normalized.rfind("create database", 0) == 0) return true;
    if (normalized.rfind("insert into accounts", 0) == 0) {
        const std::regex pattern(
            R"(VALUES\s*\(\s*'((?:\\.|[^'])*)'\s*,\s*'((?:\\.|[^'])*)'\s*,\s*SHA2\(\s*'((?:\\.|[^'])*)'\s*,\s*256\s*\)\s*,\s*'((?:\\.|[^'])*)'\s*,\s*'((?:\\.|[^'])*)'\s*,\s*'((?:\\.|[^'])*)'\s*\))",
            std::regex::icase);
        std::smatch m;
        if (!std::regex_search(query, m, pattern)) return false;
        AccountRecord r;
        r.username = unescape(m[1].str());
        r.email = unescape(m[2].str());
        const std::string rawPassword = unescape(m[3].str());
        r.fullname = unescape(m[4].str());
        r.phone = unescape(m[5].str());
        r.role = unescape(m[6].str());
        if (r.username.empty() || r.email.empty() || rawPassword.empty()) return false;
        for (const auto& existing : accountRecords)
            if (existing.username == r.username || existing.email == r.email) return false;
        r.id = lastInsertId + 1;
        r.password = sha256(rawPassword);
        if (r.role.empty()) r.role = "User";
        accountRecords.push_back(r);
        if (!saveAccounts()) { accountRecords.pop_back(); return false; }
        lastInsertId = r.id;
        return true;
    }
    if (normalized.rfind("update accounts", 0) == 0) {
        const int a = numberParam(query, "account_id");
        const int b = numberParam(query, "id");
        const int id = a >= 0 ? a : b;
        if (id <= 0) return false;
        const std::string rawPassword = shaParam(query, "password");
        if (rawPassword.empty()) return false;
        for (auto& r : accountRecords) if (r.id == id) {
            const std::string old = r.password;
            r.password = sha256(rawPassword);
            if (saveAccounts()) return true;
            r.password = old; return false;
        }
    }
    return false;
}
MYSQL_RES* DBManager::fetchQuery(const std::string& q) { return executeQuery(q); }
void DBManager::freeResult(MYSQL_RES* p) { mysql_free_result(p); }
std::string DBManager::escapeString(const std::string& value) const {
    std::string escaped;
    escaped.reserve(value.size() * 2);
    for (char c : value) {
        if (c == '\\' || c == '\'') escaped.push_back('\\');
        escaped.push_back(c);
    }
    return escaped;
}
int DBManager::getInsertId() const { return lastInsertId; }
bool DBManager::isConnected() const { return connected; }
std::string DBManager::getStorageFile() const { return storageFile; }

std::vector<AccountData> DBManager::getAccounts() const {
    std::vector<AccountData> result;
    for (const auto& r : accountRecords)
        result.push_back({r.id, r.username, r.email, r.password, r.fullname, r.phone, r.role});
    return result;
}
bool DBManager::getAccount(int id, AccountData& result) const {
    for (const auto& r : accountRecords) if (r.id == id) {
        result = {r.id, r.username, r.email, r.password, r.fullname, r.phone, r.role};
        return true;
    }
    return false;
}
bool DBManager::updateAccount(const AccountData& account) {
    if (!connected || account.id <= 0) return false;
    for (auto& r : accountRecords) if (r.id == account.id) {
        for (const auto& other : accountRecords) if (other.id != account.id &&
            (other.username == account.username || other.email == account.email)) return false;
        const auto before = r;
        r.username = account.username;
        r.email = account.email;
        r.fullname = account.fullname;
        r.phone = account.phone;
        r.role = account.role;
        if (!account.password.empty()) r.password = account.password;
        if (saveAccounts()) return true;
        r = before; return false;
    }
    return false;
}
bool DBManager::deleteAccount(int id) {
    if (!connected || id <= 0) return false;
    const auto it = std::find_if(accountRecords.begin(), accountRecords.end(),
        [id](const AccountRecord& a) { return a.id == id; });
    if (it == accountRecords.end()) return false;
    // Xoa cac ban ghi phu thuoc tren file truoc khi xoa tai khoan.
    auto contacts = readAllContacts(getRelatedStorageFile(".contacts"));
    contacts.erase(std::remove_if(contacts.begin(), contacts.end(), [id](const Contact& c) {
        return c.account_id == id;
    }), contacts.end());
    auto groups = readAllGroups(getRelatedStorageFile(".groups"));
    groups.erase(std::remove_if(groups.begin(), groups.end(), [id](const Group& g) {
        return g.account_id == id;
    }), groups.end());
    auto relations = readAllMemberships(getRelatedStorageFile(".memberships"));
    relations.erase(std::remove_if(relations.begin(), relations.end(), [id](const Membership& r) {
        return std::get<0>(r) == id;
    }), relations.end());
    if (!writeAll(getRelatedStorageFile(".contacts"), contacts, writeContact) ||
        !writeAll(getRelatedStorageFile(".groups"), groups, writeGroup) ||
        !writeAll(getRelatedStorageFile(".memberships"), relations, writeMembership)) return false;
    const AccountRecord old = *it;
    accountRecords.erase(it);
    if (saveAccounts()) return true;
    accountRecords.push_back(old);
    return false;
}

std::vector<Contact> DBManager::loadContacts(int accountId) const {
    if (!connected) return {};
    auto all = readAllContacts(getRelatedStorageFile(".contacts"));
    all.erase(std::remove_if(all.begin(), all.end(), [accountId](const Contact& c) {
        return c.account_id != accountId;
    }), all.end());
    return all;
}
bool DBManager::saveContact(const Contact& c) const {
    if (!connected || c.id <= 0 || c.account_id <= 0 || c.name.empty()) return false;
    auto all = readAllContacts(getRelatedStorageFile(".contacts"));
    for (const auto& x : all) if (x.id == c.id && x.account_id == c.account_id) return false;
    all.push_back(c);
    return writeAll(getRelatedStorageFile(".contacts"), all, writeContact);
}
bool DBManager::updateContact(const Contact& c) const {
    if (!connected) return false;
    auto all = readAllContacts(getRelatedStorageFile(".contacts"));
    bool found = false;
    for (auto& x : all) if (x.id == c.id && x.account_id == c.account_id) {
        x = c; found = true; break;
    }
    return found && writeAll(getRelatedStorageFile(".contacts"), all, writeContact);
}
bool DBManager::deleteContact(int accountId, int contactId) const {
    if (!connected) return false;
    auto all = readAllContacts(getRelatedStorageFile(".contacts"));
    const auto oldSize = all.size();
    all.erase(std::remove_if(all.begin(), all.end(), [=](const Contact& c) {
        return c.account_id == accountId && c.id == contactId;
    }), all.end());
    if (all.size() == oldSize) return false;
    // Cac service dang goi deleteContactGroup() rieng, tuy nhien don dep ngay o day an toan hon.
    return writeAll(getRelatedStorageFile(".contacts"), all, writeContact) &&
           deleteContactGroup(accountId, contactId);
}
std::vector<Group> DBManager::loadGroups(int accountId) const {
    if (!connected) return {};
    auto all = readAllGroups(getRelatedStorageFile(".groups"));
    all.erase(std::remove_if(all.begin(), all.end(), [accountId](const Group& g) {
        return g.account_id != accountId;
    }), all.end());
    return all;
}
bool DBManager::saveGroup(const Group& g) const {
    if (!connected || g.id <= 0 || g.account_id <= 0 || g.name.empty()) return false;
    auto all = readAllGroups(getRelatedStorageFile(".groups"));
    for (const auto& x : all) if (x.id == g.id && x.account_id == g.account_id) return false;
    all.push_back(g);
    return writeAll(getRelatedStorageFile(".groups"), all, writeGroup);
}
bool DBManager::updateGroup(const Group& g) const {
    if (!connected) return false;
    auto all = readAllGroups(getRelatedStorageFile(".groups"));
    bool found = false;
    for (auto& x : all) if (x.id == g.id && x.account_id == g.account_id) {
        x = g; found = true; break;
    }
    return found && writeAll(getRelatedStorageFile(".groups"), all, writeGroup);
}
bool DBManager::deleteGroup(int accountId, int groupId) const {
    if (!connected) return false;
    auto all = readAllGroups(getRelatedStorageFile(".groups"));
    const auto oldSize = all.size();
    all.erase(std::remove_if(all.begin(), all.end(), [=](const Group& g) {
        return g.account_id == accountId && g.id == groupId;
    }), all.end());
    if (all.size() == oldSize) return false;
    if (!writeAll(getRelatedStorageFile(".groups"), all, writeGroup)) return false;
    if (!deleteContactGroups(accountId, groupId)) return false;
    // Lien he thuoc group nay duoc dua ve 'Chua phan nhom'.
    auto contacts = readAllContacts(getRelatedStorageFile(".contacts"));
    for (auto& c : contacts) if (c.account_id == accountId && c.group_id == groupId)
        c.group_id = 0;
    return writeAll(getRelatedStorageFile(".contacts"), contacts, writeContact);
}
std::vector<ContactGroup> DBManager::loadContactGroups(int accountId) const {
    std::vector<ContactGroup> result;
    if (!connected) return result;
    for (const auto& r : readAllMemberships(getRelatedStorageFile(".memberships")))
        if (std::get<0>(r) == accountId)
            result.push_back({std::get<1>(r), std::get<2>(r)});
    return result;
}
bool DBManager::saveContactGroup(int accountId, const ContactGroup& r) const {
    if (!connected || accountId <= 0 || r.contactId <= 0 || r.groupId <= 0) return false;
    const auto contacts = loadContacts(accountId);
    const auto groups = loadGroups(accountId);
    if (std::none_of(contacts.begin(), contacts.end(), [&](const Contact& c) { return c.id == r.contactId; }) ||
        std::none_of(groups.begin(), groups.end(), [&](const Group& g) { return g.id == r.groupId; })) return false;
    auto all = readAllMemberships(getRelatedStorageFile(".memberships"));
    const Membership next = {accountId, r.contactId, r.groupId};
    if (std::find(all.begin(), all.end(), next) != all.end()) return true;
    all.push_back(next);
    return writeAll(getRelatedStorageFile(".memberships"), all, writeMembership);
}
bool DBManager::deleteContactGroups(int accountId, int groupId) const {
    if (!connected) return false;
    auto all = readAllMemberships(getRelatedStorageFile(".memberships"));
    all.erase(std::remove_if(all.begin(), all.end(), [=](const Membership& r) {
        return std::get<0>(r) == accountId && std::get<2>(r) == groupId;
    }), all.end());
    return writeAll(getRelatedStorageFile(".memberships"), all, writeMembership);
}
bool DBManager::deleteContactGroup(int accountId, int contactId) const {
    if (!connected) return false;
    auto all = readAllMemberships(getRelatedStorageFile(".memberships"));
    all.erase(std::remove_if(all.begin(), all.end(), [=](const Membership& r) {
        return std::get<0>(r) == accountId && std::get<1>(r) == contactId;
    }), all.end());
    return writeAll(getRelatedStorageFile(".memberships"), all, writeMembership);
}
#endif // !PHONEBOOK_USE_MYSQL
#ifdef PHONEBOOK_USE_MYSQL
#include "DBManager.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

// MySQL C API backend: duoc bat thong qua -DPHONEBOOK_USE_MYSQL.
// Can co MySQL Connector/C hoac MariaDB Connector/C va MySQL Server.
DBManager db;
namespace {
std::string sqlText(MYSQL_ROW row, unsigned int i) {
    return row && row[i] ? row[i] : "";
}
int sqlInt(MYSQL_ROW row, unsigned int i) {
    const auto v = sqlText(row, i);
    try { return std::stoi(v); } catch (...) { return 0; }
}
bool runSql(MYSQL* conn, const std::string& sql) {
    if (!conn || mysql_query(conn, sql.c_str()) != 0) {
        if (conn) std::cerr << "MySQL error: " << mysql_error(conn) << '\n';
        return false;
    }
    return true;
}
MYSQL_RES* querySql(MYSQL* conn, const std::string& sql) {
    if (!runSql(conn, sql)) return nullptr;
    return mysql_store_result(conn);
}
std::string escape(MYSQL* conn, const std::string& s) {
    if (!conn) return {};
    std::string r(s.size() * 2 + 1, '\0');
    const auto len = mysql_real_escape_string(conn, &r[0], s.data(),
                                               static_cast<unsigned long>(s.size()));
    r.resize(len);
    return r;
}
std::string sq(MYSQL* conn, const std::string& s) {
    return "'" + escape(conn, s) + "'";
}
}

DBManager::DBManager() = default;
DBManager::~DBManager() {
    if (conn) mysql_close(conn);
}
bool DBManager::connectToServer(const std::string& host, const std::string& user,
                                const std::string& password, const std::string& database,
                                unsigned int port) {
    if (!std::regex_match(database, std::regex("[A-Za-z0-9_]+"))) return false;
    if (conn) { mysql_close(conn); conn = nullptr; }
    conn = mysql_init(nullptr);
    if (!conn) return false;
    if (!mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(),
                            nullptr, port, nullptr, 0)) {
        std::cerr << "Cannot connect to MySQL: " << mysql_error(conn) << '\n';
        mysql_close(conn); conn = nullptr;
        return false;
    }
    if (!runSql(conn, "CREATE DATABASE IF NOT EXISTS `" + database +
                      "` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci") ||
        !runSql(conn, "USE `" + database + "`")) return false;
    if (mysql_set_character_set(conn, "utf8mb4") != 0) return false;
    return true;
}
bool DBManager::connect(const std::string& host, const std::string& user,
                        const std::string& password, const std::string& database,
                        unsigned int port) {
    connected = false;
    if (!connectToServer(host, user, password, database, port)) return false;
    connected = initializeSchema();
    return connected;
}
bool DBManager::initializeSchema() {
    return runSql(conn,
        "CREATE TABLE IF NOT EXISTS Accounts ("
        "account_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,"
        "username VARCHAR(30) NOT NULL UNIQUE, email VARCHAR(254) NOT NULL UNIQUE,"
        "password VARCHAR(255) NOT NULL, fullname VARCHAR(255) NOT NULL DEFAULT '',"
        "phone VARCHAR(20) NOT NULL DEFAULT '',"
        "role VARCHAR(20) NOT NULL DEFAULT 'User') ENGINE=InnoDB DEFAULT CHARSET=utf8mb4") &&
      runSql(conn,
        "CREATE TABLE IF NOT EXISTS Contacts ("
        "contact_id INT NOT NULL, account_id INT NOT NULL, group_id INT NOT NULL DEFAULT 0,"
        "name VARCHAR(255) NOT NULL, phone VARCHAR(20) NOT NULL,"
        "email VARCHAR(254) NOT NULL DEFAULT '', address TEXT,"
        "isFavorite TINYINT(1) NOT NULL DEFAULT 0,"
        "PRIMARY KEY(account_id,contact_id),"
        "FOREIGN KEY(account_id) REFERENCES Accounts(account_id) ON DELETE CASCADE"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4") &&
      runSql(conn,
        "CREATE TABLE IF NOT EXISTS `Groups` ("
        "group_id INT NOT NULL, account_id INT NOT NULL,"
        "name VARCHAR(255) NOT NULL, description TEXT,"
        "PRIMARY KEY(account_id,group_id),"
        "FOREIGN KEY(account_id) REFERENCES Accounts(account_id) ON DELETE CASCADE"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4") &&
      runSql(conn,
        "CREATE TABLE IF NOT EXISTS ContactGroups ("
        "account_id INT NOT NULL, contact_id INT NOT NULL, group_id INT NOT NULL,"
        "PRIMARY KEY(account_id,contact_id,group_id),"
        "FOREIGN KEY(account_id,contact_id) REFERENCES Contacts(account_id,contact_id) ON DELETE CASCADE,"
        "FOREIGN KEY(account_id,group_id) REFERENCES `Groups`(account_id,group_id) ON DELETE CASCADE"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
}

bool DBManager::loadAccounts() { return true; }
bool DBManager::saveAccounts() const { return true; }
std::string DBManager::getRelatedStorageFile(const std::string& suffix) const {
    return storageFile + suffix;
}
MYSQL* DBManager::getConn() { return conn; }
MYSQL_RES* DBManager::executeQuery(const std::string& input) {
    if (!connected) return nullptr;
    // AccountService.cpp dung WHERE id=... thay vi account_id=... o changePassword.
    const std::regex legacy(R"(\bWHERE\s+id\s*=)", std::regex::icase);
    const std::string sql = std::regex_replace(input, legacy, "WHERE account_id =");
    return querySql(conn, sql);
}
bool DBManager::executeNonQuery(const std::string& query) {
    if (!connected || !runSql(conn, query)) return false;
    const auto id = mysql_insert_id(conn);
    if (id != 0) lastInsertId = static_cast<int>(id);
    return true;
}
MYSQL_RES* DBManager::fetchQuery(const std::string& sql) { return executeQuery(sql); }
void DBManager::freeResult(MYSQL_RES* res) { if (res) mysql_free_result(res); }
std::string DBManager::escapeString(const std::string& s) const { return escape(conn, s); }
int DBManager::getInsertId() const { return lastInsertId; }
bool DBManager::isConnected() const { return connected; }
std::string DBManager::getStorageFile() const { return {}; } // MySQL khong phai file .db.

std::vector<AccountData> DBManager::getAccounts() const {
    std::vector<AccountData> out;
    if (!connected) return out;
    MYSQL_RES* res = querySql(conn,
        "SELECT account_id,username,email,password,fullname,phone,role FROM Accounts ORDER BY account_id");
    if (!res) return out;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        out.push_back({sqlInt(row,0),sqlText(row,1),sqlText(row,2),sqlText(row,3),
                       sqlText(row,4),sqlText(row,5),sqlText(row,6)});
    }
    mysql_free_result(res);
    return out;
}
bool DBManager::getAccount(int id, AccountData& a) const {
    if (!connected || id <= 0) return false;
    MYSQL_RES* res = querySql(conn,
        "SELECT account_id,username,email,password,fullname,phone,role FROM Accounts WHERE account_id=" + std::to_string(id));
    if (!res) return false;
    MYSQL_ROW r = mysql_fetch_row(res);
    const bool ok = (r != nullptr);
    if (ok) a = {sqlInt(r,0),sqlText(r,1),sqlText(r,2),sqlText(r,3),
                 sqlText(r,4),sqlText(r,5),sqlText(r,6)};
    mysql_free_result(res);
    return ok;
}
bool DBManager::updateAccount(const AccountData& a) {
    if (!connected || a.id <= 0) return false;
    std::string sql = "UPDATE Accounts SET username=" + sq(conn,a.username) +
        ",email=" + sq(conn,a.email) + ",fullname=" + sq(conn,a.fullname) +
        ",phone=" + sq(conn,a.phone) + ",role=" + sq(conn,a.role);
    if (!a.password.empty()) sql += ",password=" + sq(conn,a.password);
    sql += " WHERE account_id=" + std::to_string(a.id);
    return runSql(conn, sql);
}
bool DBManager::deleteAccount(int id) {
    return connected && id > 0 && runSql(conn,
        "DELETE FROM Accounts WHERE account_id=" + std::to_string(id)) &&
        mysql_affected_rows(conn) > 0;
}
std::vector<Contact> DBManager::loadContacts(int id) const {
    std::vector<Contact> out;
    if (!connected || id <= 0) return out;
    MYSQL_RES* res = querySql(conn,
        "SELECT contact_id,account_id,group_id,name,phone,email,address,isFavorite "
        "FROM Contacts WHERE account_id=" + std::to_string(id) + " ORDER BY contact_id");
    if (!res) return out;
    MYSQL_ROW r;
    while ((r = mysql_fetch_row(res)))
        out.emplace_back(sqlInt(r,0),sqlInt(r,1),sqlInt(r,2),sqlText(r,3),
                         sqlText(r,4),sqlText(r,5),sqlText(r,6),sqlInt(r,7)!=0);
    mysql_free_result(res);
    return out;
}
bool DBManager::saveContact(const Contact& c) const {
    if (!connected || c.id<=0 || c.account_id<=0 || c.name.empty()) return false;
    return runSql(conn, "INSERT INTO Contacts (contact_id,account_id,group_id,name,phone,email,address,isFavorite) VALUES ("+
        std::to_string(c.id)+","+std::to_string(c.account_id)+","+std::to_string(c.group_id)+","+
        sq(conn,c.name)+","+sq(conn,c.phone)+","+sq(conn,c.email)+","+sq(conn,c.address)+","+
        (c.isFavorite ? "1" : "0")+")");
}
bool DBManager::updateContact(const Contact& c) const {
    if (!connected || c.id<=0 || c.account_id<=0) return false;
    return runSql(conn, "UPDATE Contacts SET group_id="+std::to_string(c.group_id)+
        ",name="+sq(conn,c.name)+",phone="+sq(conn,c.phone)+",email="+sq(conn,c.email)+
        ",address="+sq(conn,c.address)+",isFavorite="+(c.isFavorite?"1":"0")+
        " WHERE account_id="+std::to_string(c.account_id)+" AND contact_id="+std::to_string(c.id));
}
bool DBManager::deleteContact(int accountId, int contactId) const {
    return connected && accountId>0 && contactId>0 && runSql(conn,
        "DELETE FROM Contacts WHERE account_id="+std::to_string(accountId)+
        " AND contact_id="+std::to_string(contactId)) && mysql_affected_rows(conn)>0;
}
std::vector<Group> DBManager::loadGroups(int id) const {
    std::vector<Group> out;
    if (!connected || id<=0) return out;
    MYSQL_RES* res = querySql(conn,
        "SELECT group_id,account_id,name,description FROM `Groups` WHERE account_id="+
        std::to_string(id)+" ORDER BY group_id");
    if (!res) return out;
    MYSQL_ROW r;
    while ((r = mysql_fetch_row(res)))
        out.emplace_back(sqlInt(r,0),sqlInt(r,1),sqlText(r,2),sqlText(r,3));
    mysql_free_result(res);
    return out;
}
bool DBManager::saveGroup(const Group& g) const {
    return connected && g.id>0 && g.account_id>0 && !g.name.empty() && runSql(conn,
        "INSERT INTO `Groups` (group_id,account_id,name,description) VALUES ("+
        std::to_string(g.id)+","+std::to_string(g.account_id)+","+
        sq(conn,g.name)+","+sq(conn,g.description)+")");
}
bool DBManager::updateGroup(const Group& g) const {
    return connected && g.id>0 && g.account_id>0 && runSql(conn,
        "UPDATE `Groups` SET name="+sq(conn,g.name)+",description="+
        sq(conn,g.description)+" WHERE account_id="+std::to_string(g.account_id)+
        " AND group_id="+std::to_string(g.id));
}
bool DBManager::deleteGroup(int accountId, int groupId) const {
    if (!connected || accountId<=0 || groupId<=0) return false;
    if (!runSql(conn, "DELETE FROM `Groups` WHERE account_id="+
         std::to_string(accountId)+" AND group_id="+std::to_string(groupId))) return false;
    if (mysql_affected_rows(conn)==0) return false;
    return runSql(conn, "UPDATE Contacts SET group_id=0 WHERE account_id="+
        std::to_string(accountId)+" AND group_id="+std::to_string(groupId));
}
std::vector<ContactGroup> DBManager::loadContactGroups(int id) const {
    std::vector<ContactGroup> out;
    if (!connected || id<=0) return out;
    MYSQL_RES* res = querySql(conn, "SELECT contact_id,group_id FROM ContactGroups WHERE account_id="+
                               std::to_string(id)+" ORDER BY contact_id,group_id");
    if (!res) return out;
    MYSQL_ROW r;
    while ((r = mysql_fetch_row(res))) out.push_back({sqlInt(r,0),sqlInt(r,1)});
    mysql_free_result(res);
    return out;
}
bool DBManager::saveContactGroup(int accountId, const ContactGroup& r) const {
    return connected && accountId>0 && r.contactId>0 && r.groupId>0 && runSql(conn,
        "INSERT IGNORE INTO ContactGroups(account_id,contact_id,group_id) VALUES ("+
        std::to_string(accountId)+","+std::to_string(r.contactId)+","+
        std::to_string(r.groupId)+")");
}
bool DBManager::deleteContactGroups(int accountId, int groupId) const {
    return connected && runSql(conn,
        "DELETE FROM ContactGroups WHERE account_id="+std::to_string(accountId)+
        " AND group_id="+std::to_string(groupId));
}
bool DBManager::deleteContactGroup(int accountId, int contactId) const {
    return connected && runSql(conn,
        "DELETE FROM ContactGroups WHERE account_id="+std::to_string(accountId)+
        " AND contact_id="+std::to_string(contactId));
}
#endif // PHONEBOOK_USE_MYSQL
