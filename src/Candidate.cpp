#include "../include/Candidate.h"
#include <iostream>
#include <iomanip>

Candidate::Candidate() {
    candidateID = 0;
    favorite = false;
}

Candidate::Candidate(int id, string name, string phone, string mail, string addr, bool fav) {
    candidateID = id;
    fullName = name;
    phoneNumber = phone;
    email = mail;
    address = addr;
    favorite = fav;
}

int Candidate::getCandidateID() const {
    return candidateID;
}

string Candidate::getFullName() const {
    return fullName;
}

string Candidate::getPhoneNumber() const {
    return phoneNumber;
}

string Candidate::getEmail() const {
    return email;
}

string Candidate::getAddress() const {
    return address;
}

bool Candidate::isFavorite() const {
    return favorite;
}

void Candidate::setCandidateID(int id) {
    candidateID = id;
}

void Candidate::setFullName(string name) {
    fullName = name;
}

void Candidate::setPhoneNumber(string phone) {
    phoneNumber = phone;
}

void Candidate::setEmail(string mail) {
    email = mail;
}

void Candidate::setAddress(string addr) {
    address = addr;
}

void Candidate::setFavorite(bool fav) {
    favorite = fav;
}

void Candidate::toggleFavorite() {
    favorite = !favorite;
}

void Candidate::display() const {
    cout << left << setw(5) << candidateID
         << setw(6) << (favorite ? "[*]" : "[ ]")
         << setw(22) << fullName
         << setw(15) << phoneNumber
         << setw(25) << (email.empty() ? "N/A" : email)
         << (address.empty() ? "N/A" : address) << "\n";
}