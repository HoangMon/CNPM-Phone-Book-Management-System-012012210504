#ifndef CANDIDATEMANAGER_H
#define CANDIDATEMANAGER_H

#include <vector>
#include <string>
#include "Candidate.h"
#include "CandidateSorter.h"

using namespace std;

class CandidateManager {
private:
    vector<Candidate> candidates;
    int nextID;

    bool isValidPhone(const string& phone) const;
    bool isValidEmail(const string& email) const;
    bool isDuplicatePhone(const string& phone, int ignoreID = -1) const;
    string toLower(const string& str) const;

public:
    CandidateManager();

    bool addCandidate(const string& name, const string& phone, const string& email = "",
                      const string& address = "", bool favorite = false);
    void viewAllCandidates() const;
    bool updateCandidate(int id, const string& newName, const string& newPhone,
                         const string& newEmail, const string& newAddress);
    bool deleteCandidate(int id);
    void deleteMultipleCandidates(const vector<int>& ids);

    void searchCandidates(const string& keyword) const;
    void filterFavorites() const;
    bool toggleFavorite(int id);

    Candidate* findCandidateByID(int id);
    const vector<Candidate>& getAllCandidates() const;
    void setCandidates(const vector<Candidate>& loadedList);
};

#endif