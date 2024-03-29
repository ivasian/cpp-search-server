#include "search_server.h"
#include <string>
#include "document.h"
#include <iostream>
#include "string_processing.h"
#include <algorithm>
#include <numeric>
#include <cmath>

using namespace std;

SearchServer::SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  {
}

_Rb_tree_const_iterator<int> SearchServer::begin() const{
    return document_ids_.begin();
}

_Rb_tree_const_iterator<int> SearchServer::end() const{
    return document_ids_.end();
}

const std::map<std::string_view , double>& SearchServer::GetWordFrequencies(int document_id) const{

    if(!document_to_word_frequency_.count(document_id)){
        static map <std::string_view , double> empty_map;
        return empty_map;
    }
    return document_to_word_frequency_.at(document_id);
}


[[nodiscard]] std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const{
    return FindTopDocuments(
            execution::seq,
            raw_query,
            [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
}

[[nodiscard]] std::vector<Document> SearchServer::FindTopDocuments(std::string_view  raw_query) const{
    return FindTopDocuments(execution::seq, raw_query, DocumentStatus::ACTUAL);
}


void SearchServer::RemoveDocument(int document_id){
    for(auto &[word, freq] : document_to_word_frequency_.at(document_id)){
        word_to_document_frequency_.at(word).erase(document_id);
    }
    document_to_word_frequency_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status,
                               const vector<int>& ratings) {
    if ((document_id < 0)) {
        throw invalid_argument("DocumentID "s + to_string(document_id) + " is negative."s);
    }
    if(documents_.count(document_id) > 0) {
        throw invalid_argument("DocumentID "s + to_string(document_id) + " already exists."s);
    }
    vector<string> words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / static_cast<double> (words.size());
    set<string, less<>> words_in_document;
    for (const string& word : words) {
        auto [It, success2] = words_in_document.insert(word);
        word_to_document_frequency_[*It][document_id] += inv_word_count;
        document_to_word_frequency_[document_id][*It] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{move(words_in_document),ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}


int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view raw_query, int document_id) const {
    Query query = ParseQuery(raw_query);
    const auto& words_in_document_with_id = documents_.at(document_id).words_;
    vector<string_view> matched_words;
    matched_words.reserve(words_in_document_with_id.size());
    for (const string& word : query.minus_words) {
        if (document_to_word_frequency_.at(document_id).count(word)) {
            return {matched_words, documents_.at(document_id).status};
        }
    }

    for (const string& word : query.plus_words) {
        if(words_in_document_with_id.count(word)){
            matched_words.push_back(*words_in_document_with_id.find(word));
        }
    }

    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

void SearchServer::IsValidWord(const string& word) {
    if(!none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    })){
        throw invalid_argument("Stop word - "s + word + " includes non allowed symbols");
    }
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        IsValidWord(word);
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return accumulate(ratings.begin(), ratings.end(),0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
    if (text.empty()) {
        throw invalid_argument("Invalid request. Search term not found."s);
    }
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    IsValidWord(text);
    if (text.empty() || text[0] == '-') {
        throw invalid_argument("Invalid request. Search term includes two minus or only one minus without other symbols.");
    }
    return QueryWord{text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const string_view text) const {
    Query result;
    for (const string& word : SplitIntoWords(text)) {
        QueryWord query_word = ParseQueryWord(word) ;
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            } else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / static_cast<double>(word_to_document_frequency_.at(word).size()));
}



