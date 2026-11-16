
#ifndef CONTACT_H
#define CONTACT_H

#include <string>
using namespace std;

class Contact {
private:
    int contactID;
    string fullName;
    string phoneNumber;
    string email;
    string address;
    bool favorite;

public:
    Contact() {
        contactID = 0;
        favorite = false;
    }

    Contact(int id, string name, string phone,
            string mail, string addr, bool fav = false) {
        contactID = id;
        fullName = name;
        phoneNumber = phone;
        email = mail;
        address = addr;
        favorite = fav;
    }

    int getContactID() const {
        return contactID;
    }

    string getFullName() const {
        return fullName;
    }

    string getPhoneNumber() const {
        return phoneNumber;
    }

    string getEmail() const {
        return email;
    }

    string getAddress() const {
        return address;
    }

    bool isFavorite() const {
        return favorite;
    }

    void setFullName(string name) {
        fullName = name;
    }

    void setPhoneNumber(string phone) {
        phoneNumber = phone;
    }

    void setEmail(string mail) {
        email = mail;
    }

    void setAddress(string addr) {
        address = addr;
    }

    void setFavorite(bool fav) {
        favorite = fav;
    }
};

#endif
