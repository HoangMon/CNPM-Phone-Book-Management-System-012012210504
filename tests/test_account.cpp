#include <iostream>
#include <string>
#include <vector>

// Giả lập thư viện MySQL và lớp DatabaseConnector để chạy độc lập không cần MySQL Server
typedef char** MYSQL_ROW;
struct MYSQL_RES {
    std::vector<std::vector<std::string>> rows;
    size_t index = 0;
};

MYSQL_ROW mysql_fetch_row(MYSQL_RES* res) {
    if (!res || res->index >= res->rows.size()) return nullptr;
    static std::vector<char*> ptrs;
    ptrs.clear();
    for (auto& s : res->rows[res->index]) {
        ptrs.push_back(const_cast<char*>(s.c_str()));
    }
    res->index++;
    return ptrs.data();
}

class DatabaseConnector {
public:
    bool connected = true;
    bool mockRowExists = false;
    std::string mockReturnId = "1";

    bool isConnected() { return connected; }
    std::string escapeString(const std::string& str) { return str; }
    bool executeNonQuery(const std::string&) { return connected; }

    MYSQL_RES* executeQuery(const std::string&) {
        if (!connected) return nullptr;
        MYSQL_RES* res = new MYSQL_RES();
        if (mockRowExists) {
            res->rows.push_back({mockReturnId});
        }
        return res;
    }

    void freeResult(MYSQL_RES* res) { delete res; }
};

// Nạp trực tiếp code của bạn để kiểm thử
#include "../include/Account.h"
#include "../include/AccountServices.h"
#include "../src/Account.cpp"
#include "../src/AccountServices.cpp"

int main() {
    DatabaseConnector db;
    AccountService service(db);

    std::cout << "=== BAT DAU CHAY KIEM THU MODULE ACCOUNT ===" << std::endl;

    // TC03: Username < 3 ky tu
    std::cout << "\n[TC03] Expect: Username khong hop le! -> Actual: ";
    service.registerAccount("ab", "user@gmail.com", "123456", "Nguyen Van A", "0912345678");

    // TC06: Email sai dinh dang
    std::cout << "[TC06] Expect: Email khong hop le! -> Actual: ";
    service.registerAccount("huybao02", "huybaogmail.com", "123456", "Tran Huy Bao", "0901234567");

    // TC07: Password < 6 ky tu
    std::cout << "[TC07] Expect: Password phai co it nhat 6 ky tu! -> Actual: ";
    service.registerAccount("huybao02", "huybao02@gmail.com", "12345", "Tran Huy Bao", "0901234567");

    // TC08: Role khong hop le
    std::cout << "[TC08] Expect: Role khong hop le! -> Actual: ";
    service.registerAccount("huybao02", "huybao02@gmail.com", "123456", "Tran Huy Bao", "0901234567", "Guest");

    // TC01: Dang ky thanh cong
    db.mockRowExists = false;
    std::cout << "[TC01] Expect: Dang ky thanh cong! -> Actual: ";
    service.registerAccount("huybao_01", "huybao@gmail.com", "123456", "Tran Huy Bao", "0901234567");

    // TC09: Trung username
    db.mockRowExists = true;
    std::cout << "[TC09] Expect: Username da ton tai! -> Actual: ";
    service.registerAccount("huybao_01", "newmail@gmail.com", "123456", "Tran Huy Bao", "0901234567");

    // TC15: Login bo trong
    std::cout << "[TC15] Expect: Username va password khong duoc de trong! -> Actual: ";
    service.login("", "");

    // TC13: Login sai mat khau
    db.mockRowExists = false;
    std::cout << "[TC13] Expect: Username hoac password khong dung! -> Actual: ";
    service.login("huybao_01", "wrongpass");

    // TC12: Login thanh cong
    db.mockRowExists = true;
    db.mockReturnId = "10";
    std::cout << "[TC12] Expect: Dang nhap thanh cong! -> Actual: ";
    service.login("huybao_01", "123456");
    std::cout << "       -> isLoggedIn(): " << (service.isLoggedIn() ? "true" : "false")
              << ", currentUserId: " << service.getCurrentUserId() << std::endl;

    // TC19: Doi mat khau moi < 6 ky tu
    std::cout << "[TC19] Expect: Password moi phai co it nhat 6 ky tu! -> Actual: ";
    service.changePassword("123456", "123");

    // TC20: Doi mat khau sai mat khau cu
    db.mockRowExists = false;
    std::cout << "[TC20] Expect: Password cu khong dung! -> Actual: ";
    service.changePassword("wrongold", "newpass123");

    // TC17: Doi mat khau thanh cong
    db.mockRowExists = true;
    std::cout << "[TC17] Expect: Doi password thanh cong! -> Actual: ";
    service.changePassword("123456", "newpass123");

    // TC21: Dang xuat
    std::cout << "[TC21] Expect: Dang xuat thanh cong! -> Actual: ";
    service.logout();

    // TC18: Doi mat khau khi chua dang nhap
    std::cout << "[TC18] Expect: Ban chua dang nhap! -> Actual: ";
    service.changePassword("newpass123", "123456");

    // TC23 & TC24: Kiem tra Account Model
    Account acc(10, "user10", "pass10", "Le Van B", "b@gmail.com", "0987654321", "Admin");
    std::cout << "[TC24] Account Model Check -> ID: " << acc.getAccountId()
              << ", User: " << acc.getUsername() << ", Role: " << acc.getRole() << std::endl;

    return 0;
}