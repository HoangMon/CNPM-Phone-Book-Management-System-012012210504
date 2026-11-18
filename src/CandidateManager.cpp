#include "../include/CandidateManager.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>

CandidateManager::CandidateManager() {
    nextID = 1;
}

string CandidateManager::toLower(const string& str) const {
    string res = str;
    transform(res.begin(), res.end(), res.begin(),
              [](unsigned char c) { return tolower(c); });
    return res;
}

bool CandidateManager::isValidPhone(const string& phone) const {
    if (phone.length() < 9 || phone.length() > 11) return false;
    for (size_t i = 0; i < phone.length(); ++i) {
        if (i == 0 && phone[i] == '+') continue;
        if (!isdigit(static_cast<unsigned char>(phone[i]))) return false;
    }
    return true;
}

bool CandidateManager::isValidEmail(const string& email) const {
    if (email.empty()) return true;
    size_t atPos = email.find('@');
    size_t dotPos = email.find('.', atPos);
    return (atPos != string::npos && dotPos != string::npos && atPos > 0 && dotPos < email.length() - 1);
}

bool CandidateManager::isDuplicatePhone(const string& phone, int ignoreID) const {
    for (const auto& c : candidates) {
        if (c.getCandidateID() != ignoreID && c.getPhoneNumber() == phone) {
            return true;
        }
    }
    return false;
}

bool CandidateManager::addCandidate(const string& name, const string& phone,
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

    Candidate newCand(nextID++, name, phone, email, address, favorite);
    candidates.push_back(newCand);
    cout << "Candidate added successfully (ID: " << newCand.getCandidateID() << ").\n";
    return true;
}

void CandidateManager::viewAllCandidates() const {
    if (candidates.empty()) {
        cout << "You have no contacts in your address book.\n";
        return;
    }

    vector<Candidate> displayList = candidates;
    CandidateSorter::sortDefault(displayList);

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

bool CandidateManager::updateCandidate(int id, const string& newName, const string& newPhone,
                                       const string& newEmail, const string& newAddress) {
    Candidate* c = findCandidateByID(id);
    if (!c) {
        cout << "Error: Candidate ID " << id << " not found!\n";
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
    cout << "Candidate updated successfully.\n";
    return true;
}

bool CandidateManager::deleteCandidate(int id) {
    for (auto it = candidates.begin(); it != candidates.end(); ++it) {
        if (it->getCandidateID() == id) {
            cout << "Deleted candidate: " << it->getFullName() << "\n";
            candidates.erase(it);
            return true;
        }
    }
    cout << "Error: Candidate ID " << id << " not found!\n";
    return false;
}

void CandidateManager::deleteMultipleCandidates(const vector<int>& ids) {
    int count = 0;
    for (int id : ids) {
        if (deleteCandidate(id)) {
            count++;
        }
    }
    cout << "Total deleted: " << count << " candidate(s).\n";
}

void CandidateManager::searchCandidates(const string& keyword) const {
    string kw = toLower(keyword);
    vector<Candidate> results;

    for (const auto& c : candidates) {
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

    CandidateSorter::sortDefault(results);
    cout << "\nSearch results for '" << keyword << "' (" << results.size() << " found):\n";
    for (const auto& c : results) {
        c.display();
    }
}

void CandidateManager::filterFavorites() const {
    vector<Candidate> favList;
    for (const auto& c : candidates) {
        if (c.isFavorite()) {
            favList.push_back(c);
        }
    }

    if (favList.empty()) {
        cout << "No favorite contacts found.\n";
        return;
    }

    CandidateSorter::sortByNameAZ(favList);
    cout << "\n--- FAVORITE CONTACTS ---\n";
    for (const auto& c : favList) {
        c.display();
    }
}

bool CandidateManager::toggleFavorite(int id) {
    Candidate* c = findCandidateByID(id);
    if (!c) {
        cout << "Error: Candidate ID " << id << " not found!\n";
        return false;
    }
    c->toggleFavorite();
    cout << "Candidate '" << c->getFullName() << "' is now "
         << (c->isFavorite() ? "marked as Favorite." : "removed from Favorites.") << "\n";
    return true;
}

Candidate* CandidateManager::findCandidateByID(int id) {
    for (auto& c : candidates) {
        if (c.getCandidateID() == id) {
            return &c;
        }
    }
    return nullptr;
}

const vector<Candidate>& CandidateManager::getAllCandidates() const {
    return candidates;
}

void CandidateManager::setCandidates(const vector<Candidate>& loadedList) {
    candidates = loadedList;
    int maxID = 0;
    for (const auto& c : candidates) {
        if (c.getCandidateID() > maxID) {
            maxID = c.getCandidateID();
        }
    }
    nextID = maxID + 1;
}