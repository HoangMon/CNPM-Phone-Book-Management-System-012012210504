
#ifndef GROUP_H
#define GROUP_H

#include <string>
#include <vector>
using namespace std;

class Group {
private:
    int groupID;
    string groupName;
    string description;
    vector<int> contactIDs;

public:
    Group() {
        groupID = 0;
    }

    Group(int id, string name, string desc = "") {
        groupID = id;
        groupName = name;
        description = desc;
    }

    int getGroupID() const {
        return groupID;
    }

    string getGroupName() const {
        return groupName;
    }

    string getDescription() const {
        return description;
    }

    int getContactCount() const {
        return contactIDs.size();
    }

    vector<int> getContactIDs() const {
        return contactIDs;
    }

    void setGroupName(string name) {
        groupName = name;
    }

    void setDescription(string desc) {
        description = desc;
    }

    bool addContact(int contactID) {
        for (int id : contactIDs) {
            if (id == contactID) {
                return false;
            }
        }

        contactIDs.push_back(contactID);
        return true;
    }

    bool removeContact(int contactID) {
        for (int i = 0; i < contactIDs.size(); i++) {
            if (contactIDs[i] == contactID) {
                contactIDs.erase(contactIDs.begin() + i);
                return true;
            }
        }

        return false;
    }
};

#endif
