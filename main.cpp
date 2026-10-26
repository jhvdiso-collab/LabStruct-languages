#include "Database.h"
#include <iostream>
#include <stdexcept>
#include <string>
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") {
            std::cout << "Usage: data_structures_app --file PATH --query 'COMMAND name [arguments]'\n"
                         "Types: M array, F singly linked list, L doubly linked list, S stack, Q queue, T AVL.\n"
                         "PRINT name prints any container. See README.md for all commands.\n";
            return 0;
        }
        std::string file, query;
        for (int i = 1; i < argc; i += 2) {
            if (i + 1 >= argc) throw std::invalid_argument("Missing option value");
            std::string option = argv[i];
            if (option == "--file" && file.empty()) file = argv[i + 1];
            else if (option == "--query" && query.empty()) query = argv[i + 1];
            else throw std::invalid_argument("Unknown or duplicate option: " + option);
        }
        if (file.empty() || query.empty()) throw std::invalid_argument("Required: --file PATH --query 'COMMAND name [arguments]'");
        Database db;
        db.load(file);
        auto result = db.execute(query);
        if (result.changed) db.save(file);
        std::cout << result.output << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
