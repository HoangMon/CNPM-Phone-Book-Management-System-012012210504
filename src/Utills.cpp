#include "Utils.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <regex>

bool validateEmail(const std::string& email) {
    // Email tuy chon theo ma nguon hien tai, nhung neu nhap phai dung mau.
    if (email.empty()) return true;
    if (email.size() > 254) return false;
    static const std::regex pattern(
        R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9-]+(\.[A-Za-z0-9-]+)*\.[A-Za-z]{2,}$)");
    return std::regex_match(email, pattern);
}

bool validatePhone(const std::string& phone) {
    if (phone.size() < 9 || phone.size() > 11) return false;
    return std::all_of(phone.begin(), phone.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0;
    });
}

void clearInputBuffer() {
    if (!std::cin.good()) std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
