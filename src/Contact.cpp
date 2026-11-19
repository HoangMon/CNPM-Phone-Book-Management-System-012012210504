#include "../include/Contact.h"
#include <iostream>
#include <iomanip>

Contact::Contact() {
    contactID = 0;
    favorite = false;
}

Contact::Contact(int id, string name, string phone, string mail, string addr, bool fav) {
    contactID = id;
    fullName = name;
    phoneNumber = phone;
    email = mail;
    address = addr;
    favorite = fav;
}

int Contact::getContactID() const {
    return contactID;
}

string Contact::getFullName() const {
    return fullName;
}

string Contact::getPhoneNumber() const {
    return phoneNumber;
}

string Contact::getEmail() const {
    return email;
}

string Contact::getAddress() const {
    return address;
}

bool Contact::isFavorite() const {
    return favorite;
}

void Contact::setContactID(int id) {
    contactID = id;
}

void Contact::setFullName(string name) {
    fullName = name;
}

void Contact::setPhoneNumber(string phone) {
    phoneNumber = phone;
}

void Contact::setEmail(string mail) {
    email = mail;
}

void Contact::setAddress(string addr) {
    address = addr;
}

void Contact::setFavorite(bool fav) {
    favorite = fav;
}

void Contact::toggleFavorite() {
    favorite = !favorite;
}

void Contact::display() const {
    cout << left << setw(5) << contactID
         << setw(6) << (favorite ? "[*]" : "[ ]")
         << setw(22) << fullName
         << setw(15) << phoneNumber
         << setw(25) << (email.empty() ? "N/A" : email)
         << (address.empty() ? "N/A" : address) << "\n";
}