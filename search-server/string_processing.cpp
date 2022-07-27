#include "string_processing.h"
#include <vector>
#include <string>
using namespace std;

vector<string> SplitIntoWords(const string_view text) {
    vector<string> words;
    words.reserve(500);
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}