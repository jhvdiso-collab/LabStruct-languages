#include "Database.h"
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

namespace {
int number(const std::string& s) {
    int value;
    auto result = std::from_chars(s.data(), s.data() + s.size(), value);
    if (result.ec != std::errc{} || result.ptr != s.data() + s.size())
        throw std::invalid_argument("Expected a 32-bit integer: " + s);
    return value;
}
bool validName(const std::string& s) {
    if (s.empty()) return false;
    for (unsigned char c : s)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
    return true;
}
void inorder(const Node* p, std::vector<int>& out) {
    if (!p) return;
    inorder(p->left, out); out.push_back(p->key); inorder(p->right, out);
}
std::string format(const std::vector<int>& values) {
    std::ostringstream out;
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) out << ' ';
        out << values[i];
    }
    out << ']';
    return out.str();
}
const std::map<std::string, int>& commands() {
    static const std::map<std::string, int> result = [] {
        std::map<std::string, int> m = {
            {"PRINT", 2}, {"MPUSH", 3}, {"MINSERT", 4}, {"MGET", 3},
            {"MDEL", 3}, {"MSET", 4}, {"MLEN", 2},
            {"SPUSH", 3}, {"SPOP", 2}, {"SGET", 2},
            {"QPUSH", 3}, {"QPOP", 2}, {"QGET", 2},
            {"TINSERT", 3}, {"TDEL", 3}, {"TGET", 3}, {"ISMEMBER", 3}};
        for (char c : std::string("MFLSQT")) {
            m[std::string(1, c) + "CREATE"] = 2;
        }
        for (char c : std::string("FL")) {
            std::string p(1, c);
            for (const auto& op : {"PUSH", "PUSHFRONT", "PUSHBACK", "DEL", "DELVAL", "GET", "FIND"}) m[p + op] = 3;
            for (const auto& op : {"PUSHBEFORE", "PUSHAFTER"}) m[p + op] = 4;
            for (const auto& op : {"DELBEFORE", "DELAFTER"}) m[p + op] = 3;
            for (const auto& op : {"DELFRONT", "DELBACK", "LEN", "REVERSE"}) m[p + op] = 2;
        }
        return m;
    }();
    return result;
}
}
Container::Container(char kind) : type(kind) {
    if (std::string("MFLSQT").find(type) == std::string::npos) throw std::invalid_argument("Unknown type");
    if (type == 'M') initArray(array);
}
Container::~Container() {
    freeArray(array); freeList(singly); freeDoublyList(doubly);
    freeStack(stack); freeQueue(queue); freeMemory(tree);
}
std::vector<int> Container::values(bool reverse) const {
    std::vector<int> out;
    switch (type) {
    case 'M': for (int i = 0; i < array.size; ++i) out.push_back(array.data[i]); break;
    case 'F': for (auto p = singly; p; p = p->next) out.push_back(p->data); break;
    case 'L':
        if (reverse) {
            for (auto p = doubly.tail; p; p = p->prev) out.push_back(p->data);
            return out;
        }
        for (auto p = doubly.head; p; p = p->next) out.push_back(p->data);
        break;
    case 'S': for (auto p = stack; p; p = p->next) out.push_back(p->data); break;
    case 'Q': for (auto p = queue.head; p; p = p->next) out.push_back(p->data); break;
    case 'T': inorder(tree, out); break;
    }
    if (reverse) std::reverse(out.begin(), out.end());
    return out;
}
void Database::load(const std::string& path) {
    std::map<std::string, std::unique_ptr<Container>> loaded;
    if (!std::filesystem::exists(path)) { containers.clear(); return; }
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot read data file");
    std::string line;
    if (!std::getline(file, line) || line != "LABSTRUCT 1") throw std::runtime_error("Invalid file header");
    while (std::getline(file, line)) {
        std::istringstream row(line);
        std::string type, name, countText, token;
        if (!(row >> type >> name >> countText) || type.size() != 1 || !validName(name) || loaded.count(name))
            throw std::runtime_error("Invalid container record");
        int count = number(countText);
        if (count < 0) throw std::runtime_error("Negative element count");
        auto item = std::make_unique<Container>(type[0]);
        std::vector<int> stackValues;
        SinglyNode** tail = &item->singly;
        for (int i = 0; i < count; ++i) {
            if (!(row >> token)) throw std::runtime_error("Missing element");
            int value = number(token);
            switch (item->type) {
            case 'M': pushBack(item->array, value); break;
            case 'F': *tail = new SinglyNode{value, nullptr}; tail = &(*tail)->next; break;
            case 'L': pushBackDoubly(item->doubly, value); break;
            case 'S': stackValues.push_back(value); break;
            case 'Q': pushQueue(item->queue, value); break;
            case 'T':
                if (searchAVL(item->tree, value)) throw std::runtime_error("Duplicate AVL key");
                item->tree = insert(item->tree, value); break;
            }
        }
        if (row >> token) throw std::runtime_error("Extra data in record");
        for (auto i = stackValues.rbegin(); i != stackValues.rend(); ++i) push(item->stack, *i);
        loaded.emplace(name, std::move(item));
    }
    if (file.bad()) throw std::runtime_error("Data file read failed");
    containers.swap(loaded);
}
void Database::save(const std::string& path) const {
    // A unique sibling temporary file keeps the old database intact on write errors.
    std::string pattern = path + ".tmp.XXXXXX";
    std::vector<char> temp(pattern.begin(), pattern.end()); temp.push_back('\0');
    int fd = mkstemp(temp.data());
    if (fd < 0) throw std::runtime_error("Cannot create temporary data file");
    close(fd);
    std::string temporary(temp.data());
    try {
        std::ofstream file(temporary);
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file << "LABSTRUCT 1\n";
        for (const auto& entry : containers) {
            auto values = entry.second->values();
            file << entry.second->type << ' ' << entry.first << ' ' << values.size();
            for (int value : values) file << ' ' << value;
            file << '\n';
        }
        file.close();
        std::filesystem::rename(temporary, path);
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw;
    }
}
QueryResult Database::execute(const std::string& query) {
    std::istringstream in(query);
    std::vector<std::string> args;
    for (std::string s; in >> s;) args.push_back(s);
    if (args.empty()) throw std::invalid_argument("Empty query");
    const std::string& op = args[0];
    auto spec = commands().find(op);
    if (spec == commands().end()) throw std::invalid_argument("Unknown command: " + op);
    if (args.size() != static_cast<std::size_t>(spec->second)) throw std::invalid_argument("Wrong argument count");
    if (!validName(args[1])) throw std::invalid_argument("Invalid container name");
    int a = args.size() > 2 ? number(args[2]) : 0;
    int b = args.size() > 3 ? number(args[3]) : 0;
    auto found = containers.find(args[1]);
    if (op == "PRINT") {
        if (found == containers.end()) throw std::invalid_argument("Container not found");
        return {format(found->second->values()), false};
    }
    char type = op == "ISMEMBER" ? 'T' : op[0];
    std::string action = op.substr(1);
    bool create = action == "CREATE";
    bool autoCreate = action == "PUSH" || action == "PUSHFRONT" || action == "PUSHBACK" || op == "TINSERT";
    if (found != containers.end() && found->second->type != type) throw std::invalid_argument("Container type mismatch");
    if (create && found != containers.end()) throw std::invalid_argument("Container already exists");
    if (found == containers.end()) {
        if (!create && !autoCreate) throw std::invalid_argument("Container not found; use CREATE or PUSH");
        found = containers.emplace(args[1], std::make_unique<Container>(type)).first;
    }
    Container& c = *found->second;
    if (create) return {"OK", true};
    if (type == 'M') {
        if (op == "MPUSH") { pushBack(c.array, a); return {std::to_string(a), true}; }
        if (op == "MINSERT") { insertAt(c.array, a, b); return {std::to_string(b), true}; }
        if (op == "MGET") return {std::to_string(getAt(c.array, a)), false};
        if (op == "MDEL") return {std::to_string(eraseAt(c.array, a)), true};
        if (op == "MSET") { setAt(c.array, a, b); return {std::to_string(b), true}; }
        if (op == "MLEN") return {std::to_string(arrayLength(c.array)), false};
    }
    if (type == 'F' || type == 'L') {
        auto get = [&](int index) { return type == 'F' ? getSingly(c.singly, index) : getDoubly(c.doubly, index); };
        auto add = [&](int index, int value) { if (type == 'F') insertSingly(c.singly, index, value); else insertDoubly(c.doubly, index, value); };
        auto erase = [&](int index) { return type == 'F' ? eraseSingly(c.singly, index) : eraseDoubly(c.doubly, index); };
        auto length = [&] { return type == 'F' ? singlyLength(c.singly) : doublyLength(c.doubly); };
        if (action == "PUSH" || action == "PUSHBACK") {
            if (type == 'F') pushBack(c.singly, a); else pushBackDoubly(c.doubly, a);
            return {std::to_string(a), true};
        }
        if (action == "PUSHFRONT") { add(0, a); return {std::to_string(a), true}; }
        if (action == "PUSHBEFORE" || action == "PUSHAFTER") {
            get(a); add(a + (action == "PUSHAFTER"), b); return {std::to_string(b), true};
        }
        if (action == "GET") return {std::to_string(get(a)), false};
        if (action == "FIND") return {std::to_string(type == 'F' ? findSingly(c.singly, a) : findDoubly(c.doubly, a)), false};
        if (action == "LEN") return {std::to_string(length()), false};
        if (action == "REVERSE") return {format(c.values(true)), false};
        if (action == "DELVAL") {
            bool removed = type == 'F' ? eraseSinglyValue(c.singly, a) : eraseDoublyValue(c.doubly, a);
            return {removed ? "TRUE" : "FALSE", removed};
        }
        int index = a;
        if (action == "DELFRONT") index = 0;
        if (action == "DELBACK") {
            if (type == 'L') return {std::to_string(popBackDoubly(c.doubly)), true};
            index = length() - 1;
        }
        if (action == "DELBEFORE" || action == "DELAFTER") {
            get(a); index = a + (action == "DELAFTER" ? 1 : -1);
        }
        return {std::to_string(erase(index)), true};
    }
    if (type == 'S') {
        if (op == "SPUSH") { push(c.stack, a); return {std::to_string(a), true}; }
        if (op == "SPOP") return {std::to_string(pop(c.stack)), true};
        if (!c.stack) throw std::out_of_range("Empty stack");
        return {std::to_string(c.stack->data), false};
    }
    if (type == 'Q') {
        if (op == "QPUSH") { pushQueue(c.queue, a); return {std::to_string(a), true}; }
        if (op == "QPOP") return {std::to_string(popQueue(c.queue)), true};
        if (!c.queue.head) throw std::out_of_range("Empty queue");
        return {std::to_string(c.queue.head->data), false};
    }
    bool exists = searchAVL(c.tree, a);
    if (op == "TINSERT") { c.tree = insert(c.tree, a); return {exists ? "FALSE" : "TRUE", !exists}; }
    if (op == "TDEL") { c.tree = remove(c.tree, a); return {exists ? "TRUE" : "FALSE", exists}; }
    return {exists ? "TRUE" : "FALSE", false};
}
