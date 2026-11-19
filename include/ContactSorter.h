#ifndef CONTACTSORTER_H
#define CONTACTSORTER_H

#include <vector>
#include <string>
#include "Contact.h"

using namespace std;

class ContactSorter {
private:
    static string toLower(const string& str);

public:
    static void sortDefault(vector<Contact>& contacts);
    static void sortByNameAZ(vector<Contact>& contacts);
    static void sortByNameZA(vector<Contact>& contacts);
    static void sortByID(vector<Contact>& contacts);
};

#endif