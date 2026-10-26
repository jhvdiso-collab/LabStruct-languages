#include "Database.h"
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

using namespace std;

namespace {
int number(const string& s) {
    int value;
    auto result = from_chars(s.data(), s.data() + s.size(), value);
    if (result.ec != errc{} || result.ptr != s.data() + s.size())
        throw invalid_argument("Expected a 32-bit integer: " + s);
    return value;
}
bool validName(const string& s) {
    if (s.empty()) return false;
    for (unsigned char c : s)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
    return true;
}
void inorder(const Node* p, vector<int>& out) {
    if (!p) return;
    inorder(p->left, out); out.push_back(p->key); inorder(p->right, out);
}
string format(const vector<int>& values) {
    ostringstream out;
    out << '[';
    for (size_t i = 0; i < values.size(); ++i) {
        if (i) out << ' ';
        out << values[i];
    }
    out << ']';
    return out.str();
}
const map<string, int>& commands() {
    static const map<string, int> result = [] {
        map<string, int> m = {
            {"PRINT", 2}, {"MPUSH", 3}, {"MINSERT", 4}, {"MGET", 3},
            {"MDEL", 3}, {"MSET", 4}, {"MLEN", 2},
            {"SPUSH", 3}, {"SPOP", 2}, {"SGET", 2},
            {"QPUSH", 3}, {"QPOP", 2}, {"QGET", 2},
            {"TINSERT", 3}, {"TDEL", 3}, {"TGET", 3}, {"ISMEMBER", 3}};
        for (char c : string("MFLSQT")) {
            m[string(1, c) + "CREATE"] = 2;
        }
        for (char c : string("FL")) {
            string p(1, c);
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
    if (string("MFLSQT").find(type) == string::npos) throw invalid_argument("Unknown type");
    if (type == 'M') initArray(array);
}
Container::~Container() {
    freeArray(array); freeList(singly); freeDoublyList(doubly);
    freeStack(stack); freeQueue(queue); freeMemory(tree);
}
vector<int> Container::values(bool reverseOrder) const {
    vector<int> out;
    switch (type) {
    case 'M': for (int i = 0; i < array.size; ++i) out.push_back(array.data[i]); break;
    case 'F': for (auto p = singly; p; p = p->next) out.push_back(p->data); break;
    case 'L':
        if (reverseOrder) {
            for (auto p = doubly.tail; p; p = p->prev) out.push_back(p->data);
            return out;
        }
        for (auto p = doubly.head; p; p = p->next) out.push_back(p->data);
        break;
    case 'S': for (auto p = stack; p; p = p->next) out.push_back(p->data); break;
    case 'Q': for (auto p = queue.head; p; p = p->next) out.push_back(p->data); break;
    case 'T': inorder(tree, out); break;
    }
    if (reverseOrder) reverse(out.begin(), out.end());
    return out;
}
void Database::load(const string& path) {
    map<string, unique_ptr<Container>> loaded;
    if (!filesystem::exists(path)) { containers.clear(); return; }
    ifstream file(path);
    if (!file) throw runtime_error("Cannot read data file");
    string line;
    if (!getline(file, line) || line != "LABSTRUCT 1") throw runtime_error("Invalid file header");
    while (getline(file, line)) {
        istringstream row(line);
        string type, name, countText, token;
        if (!(row >> type >> name >> countText) || type.size() != 1 || !validName(name) || loaded.count(name))
            throw runtime_error("Invalid container record");
        int count = number(countText);
        if (count < 0) throw runtime_error("Negative element count");
        auto item = make_unique<Container>(type[0]);
        vector<int> stackValues;
        SinglyNode** tail = &item->singly;
        for (int i = 0; i < count; ++i) {
            if (!(row >> token)) throw runtime_error("Missing element");
            int value = number(token);
            switch (item->type) {
            case 'M': pushBack(item->array, value); break;
            case 'F': *tail = new SinglyNode{value, nullptr}; tail = &(*tail)->next; break;
            case 'L': pushBackDoubly(item->doubly, value); break;
            case 'S': stackValues.push_back(value); break;
            case 'Q': pushQueue(item->queue, value); break;
            case 'T':
                if (searchAVL(item->tree, value)) throw runtime_error("Duplicate AVL key");
                item->tree = insert(item->tree, value); break;
            }
        }
        if (row >> token) throw runtime_error("Extra data in record");
        for (auto i = stackValues.rbegin(); i != stackValues.rend(); ++i) push(item->stack, *i);
        loaded.emplace(name, move(item));
    }
    if (file.bad()) throw runtime_error("Data file read failed");
    containers.swap(loaded);
}
void Database::save(const string& path) const {
    // A unique sibling temporary file keeps the old database intact on write errors.
    string pattern = path + ".tmp.XXXXXX";
    vector<char> temp(pattern.begin(), pattern.end()); temp.push_back('\0');
    int fd = mkstemp(temp.data());
    if (fd < 0) throw runtime_error("Cannot create temporary data file");
    close(fd);
    string temporary(temp.data());
    try {
        ofstream file(temporary);
        file.exceptions(ios::failbit | ios::badbit);
        file << "LABSTRUCT 1\n";
        for (const auto& entry : containers) {
            auto values = entry.second->values();
            file << entry.second->type << ' ' << entry.first << ' ' << values.size();
            for (int value : values) file << ' ' << value;
            file << '\n';
        }
        file.close();
        filesystem::rename(temporary, path);
    } catch (...) {
        error_code ignored;
        filesystem::remove(temporary, ignored);
        throw;
    }
}
QueryResult Database::execute(const string& query) {
    istringstream in(query);
    vector<string> args;
    for (string s; in >> s;) args.push_back(s);
    if (args.empty()) throw invalid_argument("Empty query");
    const string& op = args[0];
    auto spec = commands().find(op);
    if (spec == commands().end()) throw invalid_argument("Unknown command: " + op);
    if (args.size() != static_cast<size_t>(spec->second)) throw invalid_argument("Wrong argument count");
    if (!validName(args[1])) throw invalid_argument("Invalid container name");
    int a = args.size() > 2 ? number(args[2]) : 0;
    int b = args.size() > 3 ? number(args[3]) : 0;
    auto found = containers.find(args[1]);
    if (op == "PRINT") {
        if (found == containers.end()) throw invalid_argument("Container not found");
        return {format(found->second->values()), false};
    }
    char type = op == "ISMEMBER" ? 'T' : op[0];
    string action = op.substr(1);
    bool create = action == "CREATE";
    bool autoCreate = action == "PUSH" || action == "PUSHFRONT" || action == "PUSHBACK" || op == "TINSERT";
    if (found != containers.end() && found->second->type != type) throw invalid_argument("Container type mismatch");
    if (create && found != containers.end()) throw invalid_argument("Container already exists");
    if (found == containers.end()) {
        if (!create && !autoCreate) throw invalid_argument("Container not found; use CREATE or PUSH");
        found = containers.emplace(args[1], make_unique<Container>(type)).first;
    }
    Container& c = *found->second;
    if (create) return {"OK", true};
    if (type == 'M') {
        if (op == "MPUSH") { pushBack(c.array, a); return {to_string(a), true}; }
        if (op == "MINSERT") { insertAt(c.array, a, b); return {to_string(b), true}; }
        if (op == "MGET") return {to_string(getAt(c.array, a)), false};
        if (op == "MDEL") return {to_string(eraseAt(c.array, a)), true};
        if (op == "MSET") { setAt(c.array, a, b); return {to_string(b), true}; }
        if (op == "MLEN") return {to_string(arrayLength(c.array)), false};
    }
    if (type == 'F' || type == 'L') {
        auto get = [&](int index) { return type == 'F' ? getSingly(c.singly, index) : getDoubly(c.doubly, index); };
        auto add = [&](int index, int value) { if (type == 'F') insertSingly(c.singly, index, value); else insertDoubly(c.doubly, index, value); };
        auto erase = [&](int index) { return type == 'F' ? eraseSingly(c.singly, index) : eraseDoubly(c.doubly, index); };
        auto length = [&] { return type == 'F' ? singlyLength(c.singly) : doublyLength(c.doubly); };
        if (action == "PUSH" || action == "PUSHBACK") {
            if (type == 'F') pushBack(c.singly, a); else pushBackDoubly(c.doubly, a);
            return {to_string(a), true};
        }
        if (action == "PUSHFRONT") { add(0, a); return {to_string(a), true}; }
        if (action == "PUSHBEFORE" || action == "PUSHAFTER") {
            get(a); add(a + (action == "PUSHAFTER"), b); return {to_string(b), true};
        }
        if (action == "GET") return {to_string(get(a)), false};
        if (action == "FIND") return {to_string(type == 'F' ? findSingly(c.singly, a) : findDoubly(c.doubly, a)), false};
        if (action == "LEN") return {to_string(length()), false};
        if (action == "REVERSE") return {format(c.values(true)), false};
        if (action == "DELVAL") {
            bool removed = type == 'F' ? eraseSinglyValue(c.singly, a) : eraseDoublyValue(c.doubly, a);
            return {removed ? "TRUE" : "FALSE", removed};
        }
        int index = a;
        if (action == "DELFRONT") index = 0;
        if (action == "DELBACK") {
            if (type == 'L') return {to_string(popBackDoubly(c.doubly)), true};
            index = length() - 1;
        }
        if (action == "DELBEFORE" || action == "DELAFTER") {
            get(a); index = a + (action == "DELAFTER" ? 1 : -1);
        }
        return {to_string(erase(index)), true};
    }
    if (type == 'S') {
        if (op == "SPUSH") { push(c.stack, a); return {to_string(a), true}; }
        if (op == "SPOP") return {to_string(pop(c.stack)), true};
        if (!c.stack) throw out_of_range("Empty stack");
        return {to_string(c.stack->data), false};
    }
    if (type == 'Q') {
        if (op == "QPUSH") { pushQueue(c.queue, a); return {to_string(a), true}; }
        if (op == "QPOP") return {to_string(popQueue(c.queue)), true};
        if (!c.queue.head) throw out_of_range("Empty queue");
        return {to_string(c.queue.head->data), false};
    }
    bool exists = searchAVL(c.tree, a);
    if (op == "TINSERT") { c.tree = insert(c.tree, a); return {exists ? "FALSE" : "TRUE", !exists}; }
    if (op == "TDEL") { c.tree = remove(c.tree, a); return {exists ? "TRUE" : "FALSE", exists}; }
    return {exists ? "TRUE" : "FALSE", false};
}
