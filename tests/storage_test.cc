// SPDX-License-Identifier: MIT
#include "../src/storage.h"
#include <cassert>
#include <map>
#include <string>
#include <iostream>
struct Storage {
    std::map<std::string, std::string> files;
    bool failWrite = false, failMove = false;
    bool exists(const char *p) { return files.count(p); }
    std::string read(const char *p) { return exists(p) ? files.at(p) : ""; }
    long long timestamp(const std::string &v) {
        if (v.empty() || v.find_first_not_of("0123456789") != std::string::npos) return -1;
        return std::stoll(v);
    }
    bool write(const char *p, const std::string &v) {
        if (failWrite) return false;
        files[p] = v; return true;
    }
    void remove(const char *p) { files.erase(p); }
    void move(const char *a, const char *b) {
        if (failMove || !exists(a) || exists(b)) return;
        files[b] = files[a]; remove(a);
    }
};
int main() {
    Storage s;
    s.files = {{"last-success.txt", "200"}, {"status.txt", "DONE"}, {"LICENSES.txt", "MIT"}};
    const auto original = s.files;
    migrateLegacyState(s, false); // Exported USB: no reads/writes or migration.
    assert(s.files == original);
    migrateLegacyState(s, true); // Works independently of enabled/paused state.
    assert(s.read("last-success.dat") == "200" && !s.exists("last-success.txt"));
    assert(s.read("status.log") == "DONE" && !s.exists("status.txt"));
    assert(s.read("LICENSES") == "MIT" && !s.exists("LICENSES.txt"));
    const auto migrated = s.files;
    migrateLegacyState(s, true); assert(s.files == migrated);
    s.files["last-success.txt"] = "300";
    s.failWrite = true; migrateLegacyState(s, true);
    assert(s.read("last-success.txt") == "300" && s.read("last-success.dat") == "200");
    s.failWrite = false; migrateLegacyState(s, true);
    assert(s.read("last-success.dat") == "300" && !s.exists("last-success.txt"));
    s.files["last-success.txt"] = "100"; migrateLegacyState(s, true);
    assert(s.read("last-success.dat") == "300" && !s.exists("last-success.txt"));
    s.files["last-success.txt"] = "400"; s.files["last-success.dat"] = "bad";
    migrateLegacyState(s, true); assert(s.read("last-success.dat") == "400");
    s.files["last-success.txt"] = "bad legacy"; s.files["last-success.dat"] = "bad current";
    migrateLegacyState(s, true);
    assert(s.read("last-success-legacy.dat") == "bad legacy");
    assert(s.read("last-success.dat") == "bad current");
    s.files.clear(); s.files["last-success.txt"] = "500"; s.failMove = true;
    migrateLegacyState(s, true);
    assert(s.read("last-success.txt") == "500" && !s.exists("last-success.dat"));
    std::cout << "storage: USB exclusion, migration, conflicts, malformed state, failed I/O and repeated migration passed\n";
}
