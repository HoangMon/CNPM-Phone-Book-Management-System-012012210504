#ifndef CONTACTMANAGER_H
#define CONTACTMANAGER_H

#include <vector>
#include <string>
#include "Contact.h"
#include "ContactSorter.h"

using namespace std;

class ContactManager {
private:
    vector<Contact> contacts;
    int nextID;

    bool isValidPhone(const string& phone) const;
    bool isValidEmail(const string& email) const;
    bool isDuplicatePhone(const string& phone, int ignoreID = -1) const;
    string toLower(const string& str) const;

public:
    ContactManager();

    bool addContact(const string& name, const string& phone, const string& email = "",
                      const string& address = "", bool favorite = false);
    void viewAllContacts() const;
    bool updateContact(int id, const string& newName, const string& newPhone,
                         const string& newEmail, const string& newAddress);
    bool deleteContact(int id);
    void deleteMultipleContacts(const vector<int>& ids);

    void searchContacts(const string& keyword) const;
    void filterFavorites() const;
    bool toggleFavorite(int id);

    Contact* findContactByID(int id);
    const vector<Contact>& getAllContacts() const;
    void setContacts(const vector<Contact>& loadedList);
};

#endif