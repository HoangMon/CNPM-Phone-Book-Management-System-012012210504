#include "../include/ContactSorter.h"
#include <algorithm>
#include <cctype>

string ContactSorter::toLower(const string& str) {
    string res = str;
    transform(res.begin(), res.end(), res.begin(),
              [](unsigned char c) { return tolower(c); });
    return res;
}

void ContactSorter::sortDefault(vector<Contact>& contacts) {
    sort(contacts.begin(), contacts.end(), [](const Contact& a, const Contact& b) {
        if (a.isFavorite() != b.isFavorite()) {
            return a.isFavorite() > b.isFavorite();
        }
        return toLower(a.getFullName()) < toLower(b.getFullName());
    });
}

void ContactSorter::sortByNameAZ(vector<Contact>& contacts) {
    sort(contacts.begin(), contacts.end(), [](const Contact& a, const Contact& b) {
        return toLower(a.getFullName()) < toLower(b.getFullName());
    });
}

void ContactSorter::sortByNameZA(vector<Contact>& contacts) {
    sort(contacts.begin(), contacts.end(), [](const Contact& a, const Contact& b) {
        return toLower(a.getFullName()) > toLower(b.getFullName());
    });
}

void ContactSorter::sortByID(vector<Contact>& contacts) {
    sort(contacts.begin(), contacts.end(), [](const Contact& a, const Contact& b) {
        return a.getContactID() < b.getContactID();
    });
}