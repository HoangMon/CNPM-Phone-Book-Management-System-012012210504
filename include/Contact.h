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
    Contact();
    Contact(int id, string name, string phone, string mail = "", string addr = "", bool fav = false);

    // Getters
    int getContactID() const;
    string getFullName() const;
    string getPhoneNumber() const;
    string getEmail() const;
    string getAddress() const;
    bool isFavorite() const;

    // Setters
    void setContactID(int id);
    void setFullName(string name);
    void setPhoneNumber(string phone);
    void setEmail(string mail);
    void setAddress(string addr);
    void setFavorite(bool fav);
    void toggleFavorite();

    // Display
    void display() const;
};

#endif