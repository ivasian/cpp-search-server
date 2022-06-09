#include "remove_duplicates.h"
using namespace std;
void RemoveDuplicates(SearchServer& search_server) {
    set <int> document_ids_to_delete;
    map <set<string>, int> words_to_id;
    for(const int document_id : search_server) {
        set <string> words;
        for(const auto &[word, frequency] :search_server.GetWordFrequencies(document_id)){
            words.insert(word);
        }
        if(words_to_id.count(words)==0){
            words_to_id[words] = document_id;
        } else {
            int cur_id = words_to_id[words];
            document_ids_to_delete.insert(max(cur_id, document_id));
            words_to_id[words] = min(cur_id, document_id);
        }
    }
    for(const int document_id : document_ids_to_delete){
        search_server.RemoveDocument(document_id);
        cout << "Found duplicate document id "s << document_id << endl;
    }
}