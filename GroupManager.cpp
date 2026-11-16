
#include "../include/GroupManager.h"
#include <iostream>

using namespace std;

// Constructor
GroupManager::GroupManager() {
    nextGroupID = 1;
}

// Tạo nhóm mới
bool GroupManager::createGroup(string name, string description) {
    if (name.empty()) {
        cout << "Group name cannot be empty!\n";
        return false;
    }

    // Kiểm tra tên nhóm đã tồn tại
    for (const Group& group : groups) {
        if (group.getGroupName() == name) {
            cout << "Group already exists!\n";
            return false;
        }
    }

    Group newGroup(nextGroupID, name, description);

    groups.push_back(newGroup);

    cout << "Group created successfully!\n";

    nextGroupID++;

    return true;
}

// Tìm nhóm theo ID
Group* GroupManager::findGroupByID(int groupID) {
    for (Group& group : groups) {
        if (group.getGroupID() == groupID) {
            return &group;
        }
    }

    return nullptr;
}

// Đổi tên nhóm
bool GroupManager::renameGroup(int groupID, string newName) {
    if (newName.empty()) {
        cout << "New group name cannot be empty!\n";
        return false;
    }

    Group* group = findGroupByID(groupID);

    if (group == nullptr) {
        cout << "Group not found!\n";
        return false;
    }

    // Kiểm tra tên mới có bị trùng không
    for (const Group& item : groups) {
        if (item.getGroupName() == newName &&
            item.getGroupID() != groupID) {
            cout << "Group name already exists!\n";
            return false;
        }
    }

    group->setGroupName(newName);

    cout << "Group renamed successfully!\n";

    return true;
}

// Xóa nhóm
bool GroupManager::deleteGroup(int groupID) {
    for (int i = 0; i < groups.size(); i++) {
        if (groups[i].getGroupID() == groupID) {
            groups.erase(groups.begin() + i);

            cout << "Group deleted successfully!\n";

            return true;
        }
    }

    cout << "Group not found!\n";

    return false;
}

// Thêm liên hệ vào nhóm
bool GroupManager::addContactToGroup(int groupID, int contactID) {
    Group* group = findGroupByID(groupID);

    if (group == nullptr) {
        cout << "Group not found!\n";
        return false;
    }

    if (contactID <= 0) {
        cout << "Invalid contact ID!\n";
        return false;
    }

    if (!group->addContact(contactID)) {
        cout << "Contact already exists in this group!\n";
        return false;
    }

    cout << "Contact added to group successfully!\n";

    return true;
}

// Xóa liên hệ khỏi nhóm
bool GroupManager::removeContactFromGroup(int groupID, int contactID) {
    Group* group = findGroupByID(groupID);

    if (group == nullptr) {
        cout << "Group not found!\n";
        return false;
    }

    if (!group->removeContact(contactID)) {
        cout << "Contact not found in this group!\n";
        return false;
    }

    cout << "Contact removed from group successfully!\n";

    return true;
}

// Hiển thị danh sách nhóm
void GroupManager::displayGroups() const {
    if (groups.empty()) {
        cout << "No groups available!\n";
        return;
    }

    cout << "\n========== CONTACT GROUPS ==========\n";

    for (const Group& group : groups) {
        cout << "Group ID: " << group.getGroupID() << endl;
        cout << "Group Name: " << group.getGroupName() << endl;
        cout << "Description: " << group.getDescription() << endl;
        cout << "Number of Contacts: "
             << group.getContactCount() << endl;

        cout << "------------------------------------\n";
    }
}

// Lấy danh sách nhóm
const vector<Group>& GroupManager::getGroups() const {
    return groups;
}
