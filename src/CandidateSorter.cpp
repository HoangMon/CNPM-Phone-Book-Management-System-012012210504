#include "../include/CandidateSorter.h"
#include <algorithm>
#include <cctype>

string CandidateSorter::toLower(const string& str) {
    string res = str;
    transform(res.begin(), res.end(), res.begin(),
              [](unsigned char c) { return tolower(c); });
    return res;
}

void CandidateSorter::sortDefault(vector<Candidate>& candidates) {
    sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        if (a.isFavorite() != b.isFavorite()) {
            return a.isFavorite() > b.isFavorite();
        }
        return toLower(a.getFullName()) < toLower(b.getFullName());
    });
}

void CandidateSorter::sortByNameAZ(vector<Candidate>& candidates) {
    sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return toLower(a.getFullName()) < toLower(b.getFullName());
    });
}

void CandidateSorter::sortByNameZA(vector<Candidate>& candidates) {
    sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return toLower(a.getFullName()) > toLower(b.getFullName());
    });
}

void CandidateSorter::sortByID(vector<Candidate>& candidates) {
    sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.getCandidateID() < b.getCandidateID();
    });
}