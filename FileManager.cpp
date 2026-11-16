
#include "../include/FileManager.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

// ==========================================
// LUU DANH SACH LIEN HE
// ==========================================
bool FileManager::saveContacts(
    const ContactManager& manager,
    const string& filename
) {
    ofstream file(filename);

    if (!file.is_open()) {
        cout << "Cannot open contacts file!\n";
        return false;
    }

    for (const Contact& contact : manager.getContacts()) {
        file << contact.getContactID() << "|"
             << contact.getFullName() << "|"
             << contact.getPhoneNumber() << "|"
             << contact.getEmail() << "|"
             << contact.getAddress() << "|"
             << contact.isFavorite() << "\n";
    }

    file.close();

    cout << "Contacts saved successfully!\n";
    return true;
}

// ==========================================
// DOC DANH SACH LIEN HE
// ==========================================
bool FileManager::loadContacts(
    ContactManager& manager,
    const string& filename
) {
    ifstream file(filename);

    if (!file.is_open()) {
        cout << "Contacts file not found!\n";
        return false;
    }

    string line;

    while (getline(file, line)) {
        stringstream ss(line);

        string id;
        string name;
        string phone;
        string email;
        string address;
        string favorite;

        getline(ss, id, '|');
        getline(ss, name, '|');
        getline(ss, phone, '|');
        getline(ss, email, '|');
        getline(ss, address, '|');
        getline(ss, favorite);

        if (id.empty()) {
            continue;
        }

        try {
            manager.restoreContact(
                stoi(id),
                name,
                phone,
                email,
                address,
                favorite == "1"
            );
        }
        catch (...) {
            cout << "Invalid contact data skipped!\n";
        }
    }

    file.close();

    cout << "Contacts loaded successfully!\n";
    return true;
}

// ==========================================
// LUU DANH SACH NHOM
// ==========================================
bool FileManager::saveGroups(
    const GroupManager& manager,
    const string& filename
) {
    ofstream file(filename);

    if (!file.is_open()) {
        cout << "Cannot open groups file!\n";
        return false;
    }

    for (const Group& group : manager.getGroups()) {
        file << group.getGroupID() << "|"
             << group.getGroupName() << "|"
             << group.getDescription() << "|";

        vector<int> contactIDs = group.getContactIDs();

        for (int i = 0; i < contactIDs.size(); i++) {
            file << contactIDs[i];

            if (i < contactIDs.size() - 1) {
                file << ",";
            }
        }

        file << "\n";
    }

    file.close();

    cout << "Groups saved successfully!\n";
    return true;
}

// ==========================================
// DOC DANH SACH NHOM
// ==========================================
bool FileManager::loadGroups(
    GroupManager& manager,
    const string& filename
) {
    ifstream file(filename);

    if (!file.is_open()) {
        cout << "Groups file not found!\n";
        return false;
    }

    string line;

    while (getline(file, line)) {
        stringstream ss(line);

        string idText;
        string name;
        string description;
        string contactList;

        getline(ss, idText, '|');
        getline(ss, name, '|');
        getline(ss, description, '|');
        getline(ss, contactList);

        if (idText.empty() || name.empty()) {
            continue;
        }

        try {
            int id = stoi(idText);
            vector<int> contactIDs;

            stringstream contactStream(contactList);
            string contactID;

            while (getline(contactStream, contactID, ',')) {
                if (!contactID.empty()) {
                    contactIDs.push_back(stoi(contactID));
                }
            }

            manager.restoreGroup(
                id,
                name,
                description,
                contactIDs
            );
        }
        catch (...) {
            cout << "Invalid group data skipped!\n";
        }
    }

    file.close();

    cout << "Groups loaded successfully!\n";
    return true;
}
