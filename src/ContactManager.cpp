#include "../include/ContactManager.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>

ContactManager::ContactManager() {
    nextID = 1;
}

string ContactManager::toLower(const string& str) const {
    string res = str;
    transform(res.begin(), res.end(), res.begin(),
              [](unsigned char c) { return tolower(c); });
    return res;
}

bool ContactManager::isValidPhone(const string& phone) const {
    if (phone.length() < 9 || phone.length() > 11) return false;
    for (size_t i = 0; i < phone.length(); ++i) {
        if (i == 0 && phone[i] == '+') continue;
        if (!isdigit(static_cast<unsigned char>(phone[i]))) return false;
    }
    return true;
}

bool ContactManager::isValidEmail(const string& email) const {
    if (email.empty()) return true;
    size_t atPos = email.find('@');
    size_t dotPos = email.find('.', atPos);
    return (atPos != string::npos && dotPos != string::npos && atPos > 0 && dotPos < email.length() - 1);
}

bool ContactManager::isDuplicatePhone(const string& phone, int ignoreID) const {
    for (const auto& c : contacts) {
        if (c.getContactID() != ignoreID && c.getPhoneNumber() == phone) {
            return true;
        }
    }
    return false;
}

bool ContactManager::addContact(const string& name, const string& phone,
                                    const string& email, const string& address, bool favorite) {
    if (name.empty()) {
        cout << "Error: Full name cannot be empty!\n";
        return false;
    }
    if (!isValidPhone(phone)) {
        cout << "Error: Invalid phone number format (must be 9-11 digits)!\n";
        return false;
    }
    if (!isValidEmail(email)) {
        cout << "Error: Invalid email format!\n";
        return false;
    }
    if (isDuplicatePhone(phone)) {
        cout << "Warning: Phone number '" << phone << "' already exists in your phone book!\n";
        return false;
    }

    Contact newCand(nextID++, name, phone, email, address, favorite);
    contacts.push_back(newCand);
    cout << "Contact added successfully (ID: " << newCand.getContactID() << ").\n";
    return true;
}

void ContactManager::viewAllContacts() const {
    if (contacts.empty()) {
        cout << "You have no contacts in your address book.\n";
        return;
    }

    vector<Contact> displayList = contacts;
    ContactSorter::sortDefault(displayList);

    cout << "\n-----------------------------------------------------------------------------------------\n";
    cout << left << setw(5) << "ID"
         << setw(6) << "Fav"
         << setw(22) << "Full Name"
         << setw(15) << "Phone"
         << setw(25) << "Email"
         << "Address\n";
    cout << "-----------------------------------------------------------------------------------------\n";
    for (const auto& c : displayList) {
        c.display();
    }
    cout << "-----------------------------------------------------------------------------------------\n";
}

bool ContactManager::updateContact(int id, const string& newName, const string& newPhone,
                                       const string& newEmail, const string& newAddress) {
    Contact* c = findContactByID(id);
    if (!c) {
        cout << "Error: Contact ID " << id << " not found!\n";
        return false;
    }
    if (newName.empty() || !isValidPhone(newPhone) || !isValidEmail(newEmail)) {
        cout << "Error: Invalid updated data!\n";
        return false;
    }
    if (isDuplicatePhone(newPhone, id)) {
        cout << "Warning: Phone number '" << newPhone << "' conflicts with another contact!\n";
        return false;
    }

    c->setFullName(newName);
    c->setPhoneNumber(newPhone);
    c->setEmail(newEmail);
    c->setAddress(newAddress);
    cout << "Contact updated successfully.\n";
    return true;
}

bool ContactManager::deleteContact(int id) {
    for (auto it = contacts.begin(); it != contacts.end(); ++it) {
        if (it->getContactID() == id) {
            cout << "Deleted contact: " << it->getFullName() << "\n";
            contacts.erase(it);
            return true;
        }
    }
    cout << "Error: Contact ID " << id << " not found!\n";
    return false;
}

void ContactManager::deleteMultipleContacts(const vector<int>& ids) {
    int count = 0;
    for (int id : ids) {
        if (deleteContact(id)) {
            count++;
        }
    }
    cout << "Total deleted: " << count << " contact(s).\n";
}

void ContactManager::searchContacts(const string& keyword) const {
    string kw = toLower(keyword);
    vector<Contact> results;

    for (const auto& c : contacts) {
        if (toLower(c.getFullName()).find(kw) != string::npos ||
            c.getPhoneNumber().find(kw) != string::npos ||
            toLower(c.getEmail()).find(kw) != string::npos) {
            results.push_back(c);
        }
    }

    if (results.empty()) {
        cout << "No contacts found matching '" << keyword << "'.\n";
        return;
    }

    ContactSorter::sortDefault(results);
    cout << "\nSearch results for '" << keyword << "' (" << results.size() << " found):\n";
    for (const auto& c : results) {
        c.display();
    }
}

void ContactManager::filterFavorites() const {
    vector<Contact> favList;
    for (const auto& c : contacts) {
        if (c.isFavorite()) {
            favList.push_back(c);
        }
    }

    if (favList.empty()) {
        cout << "No favorite contacts found.\n";
        return;
    }

    ContactSorter::sortByNameAZ(favList);
    cout << "\n--- FAVORITE CONTACTS ---\n";
    for (const auto& c : favList) {
        c.display();
    }
}

bool ContactManager::toggleFavorite(int id) {
    Contact* c = findContactByID(id);
    if (!c) {
        cout << "Error: Contact ID " << id << " not found!\n";
        return false;
    }
    c->toggleFavorite();
    cout << "Contact '" << c->getFullName() << "' is now "
         << (c->isFavorite() ? "marked as Favorite." : "removed from Favorites.") << "\n";
    return true;
}

Contact* ContactManager::findContactByID(int id) {
    for (auto& c : contacts) {
        if (c.getContactID() == id) {
            return &c;
        }
    }
    return nullptr;
}

const vector<Contact>& ContactManager::getAllContacts() const {
    return contacts;
}

void ContactManager::setContacts(const vector<Contact>& loadedList) {
    contacts = loadedList;
    int maxID = 0;
    for (const auto& c : contacts) {
        if (c.getContactID() > maxID) {
            maxID = c.getContactID();
        }
    }
    nextID = maxID + 1;
}