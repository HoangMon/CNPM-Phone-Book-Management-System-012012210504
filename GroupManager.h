
#ifndef GROUPMANAGER_H
#define GROUPMANAGER_H

#include "Group.h"
#include <vector>
#include <string>

using namespace std;

class GroupManager {
private:
    vector<Group> groups;
    int nextGroupID;

public:
    GroupManager();

    bool createGroup(string name, string description = "");

    bool renameGroup(int groupID, string newName);

    bool deleteGroup(int groupID);

    bool addContactToGroup(int groupID, int contactID);

    bool removeContactFromGroup(int groupID, int contactID);

    void displayGroups() const;

    Group* findGroupByID(int groupID);

    const vector<Group>& getGroups() const;
};

#endif
