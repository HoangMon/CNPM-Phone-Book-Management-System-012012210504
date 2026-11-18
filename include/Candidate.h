#ifndef CANDIDATE_H
#define CANDIDATE_H

#include <string>
using namespace std;

class Candidate {
private:
    int candidateID;
    string fullName;
    string phoneNumber;
    string email;
    string address;
    bool favorite;

public:
    Candidate();
    Candidate(int id, string name, string phone, string mail = "", string addr = "", bool fav = false);

    // Getters
    int getCandidateID() const;
    string getFullName() const;
    string getPhoneNumber() const;
    string getEmail() const;
    string getAddress() const;
    bool isFavorite() const;

    // Setters
    void setCandidateID(int id);
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