#ifndef CANDIDATESORTER_H
#define CANDIDATESORTER_H

#include <vector>
#include <string>
#include "Candidate.h"

using namespace std;

class CandidateSorter {
private:
    static string toLower(const string& str);

public:
    static void sortDefault(vector<Candidate>& candidates);
    static void sortByNameAZ(vector<Candidate>& candidates);
    static void sortByNameZA(vector<Candidate>& candidates);
    static void sortByID(vector<Candidate>& candidates);
};

#endif