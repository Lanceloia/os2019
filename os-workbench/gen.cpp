#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
using namespace std;

map<string, string> m;

void put(string key, string value) {
  m[key] = value;
  cout << "kvdb_put(db, \"" << key << "\",\"" << value << "\");" << endl;
}

void get(string key) {
  cout << "printf(\"%s\\n\", kvdb_get(db, \"" << key << "\"));" << endl;
  if (m.count(key)) {
    cerr << key << ": " << m[key] << endl;
    m.erase(key);
  } else {
    cerr << key << ": "
         << "(null)" << endl;
  }
}

string keys[16] = {"AAA", "BBB", "CCC", "DDD", "EEE", "FFF", "GGG", "HHH",
                   "III", "JJJ", "KKK", "LLL", "MMM", "NNN", "OOO", "PPP"};

string values[256];

void gen_values() {
  for (int i = 0; i < 256; i++) {
    int len = ((rand() % 4) * 32) + (rand() % 32);
    values[i] = "";
    for (int j = 0; j < len; j++) {
      values[i] += rand() % 26 + 'a';
    }
  }
}

int main(int argc, char *argv[]) {
  int seed = 1024;
  if (argc == 2) sscanf(argv[1], "%d", &seed);
  // printf("%d\n", seed);
  srand(996);
  gen_values();

  srand(seed);
  cerr << "first run: " << endl;
  for (int i = 0; i < 32; i++) {
    if (rand() % 4) {
      put(keys[rand() % 16], values[rand() % 256]);
    } else {
      get(keys[rand() % 16]);
    }
  }

  cerr << endl;
  srand(seed);
  cerr << "second run: " << endl;
  for (int i = 0; i < 32; i++) {
    if (rand() % 4) {
      put(keys[rand() % 16], values[rand() % 256]);
    } else {
      get(keys[rand() % 16]);
    }
  }

  return 0;
}