
#include <iostream>
#include <limits>

#include "../include/GroupManager.h"
#include "../include/FileManager.h"

using namespace std;

int main() {
    GroupManager manager;

    // Đọc dữ liệu nhóm
    FileManager::loadGroups(manager, "data/groups.txt");

    int choice;

    do {
        cout << "\n";
        cout << "====================================\n";
        cout << "       PHONE BOOK MANAGEMENT\n";
        cout << "====================================\n";
        cout << "1. Create Group\n";
        cout << "2. Rename Group\n";
        cout << "3. Delete Group\n";
        cout << "4. Add Contact To Group\n";
        cout << "5. Remove Contact From Group\n";
        cout << "6. Display Groups\n";
        cout << "0. Save and Exit\n";
        cout << "====================================\n";
        cout << "Enter choice: ";

        if (!(cin >> choice)) {
            cout << "Invalid input!\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        int groupID, contactID;
        string name, description;

        switch (choice) {
        case 1:
            cout << "Enter group name: ";
            getline(cin, name);

            cout << "Enter description: ";
            getline(cin, description);

            manager.createGroup(name, description);
            break;

        case 2:
            cout << "Enter group ID: ";
            cin >> groupID;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            cout << "Enter new group name: ";
            getline(cin, name);

            manager.renameGroup(groupID, name);
            break;

        case 3:
            cout << "Enter group ID: ";
            cin >> groupID;

            manager.deleteGroup(groupID);
            break;

        case 4:
            cout << "Enter group ID: ";
            cin >> groupID;

            cout << "Enter contact ID: ";
            cin >> contactID;

            manager.addContactToGroup(groupID, contactID);
            break;

        case 5:
            cout << "Enter group ID: ";
            cin >> groupID;

            cout << "Enter contact ID: ";
            cin >> contactID;

            manager.removeContactFromGroup(groupID, contactID);
            break;

        case 6:
            manager.displayGroups();
            break;

        case 0:
            FileManager::saveGroups(manager, "data/groups.txt");
            cout << "Goodbye!\n";
            break;

        default:
            cout << "Invalid choice!\n";
        }

    } while (choice != 0);

    return 0;
}
