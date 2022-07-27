#pragma once
#include "document.h"
#include <string>
#include <vector>
#include <algorithm>
#include <set>
#include <map>
#include "string_processing.h"
#include <execution>
#include "log_duration.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
            : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for(const std::string& word: stop_words_){
            IsValidWord(word);
        }
    }

    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, const std::string& document, DocumentStatus status,
                     const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    [[nodiscard]] std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    [[nodiscard]] std::vector<Document> FindTopDocuments(const std::execution::parallel_policy& policy, std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    [[nodiscard]] std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy& policy, std::string_view  raw_query, DocumentPredicate document_predicate) const;


    [[nodiscard]] std::vector<Document> FindTopDocuments(std::string_view  raw_query, DocumentStatus status) const;

    [[nodiscard]] std::vector<Document> FindTopDocuments(const std::execution::parallel_policy& policy, std::string_view  raw_query, DocumentStatus status) const;
    [[nodiscard]] std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy& policy, std::string_view  raw_query, DocumentStatus status) const;

    [[nodiscard]] std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    [[maybe_unused]] [[nodiscard]] std::vector<Document> FindTopDocuments(const std::execution::parallel_policy& policy, std::string_view  raw_query) const;
    [[nodiscard]] std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy& policy, std::string_view  raw_query) const;

    [[nodiscard]] int GetDocumentCount() const;

    [[nodiscard]] std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
    [[nodiscard]] std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy& policy, std::string_view raw_query, int document_id) const;
    [[nodiscard]] std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy& policy, std::string_view raw_query, int document_id) const;

    [[nodiscard]] std::_Rb_tree_const_iterator<int> begin() const;

    [[nodiscard]] std::_Rb_tree_const_iterator<int> end() const;

    [[maybe_unused]] [[nodiscard]] const std::map<std::string_view , double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    [[maybe_unused]] void RemoveDocument(const std::execution::sequenced_policy& policy, int document_id);

    [[maybe_unused]] void RemoveDocument(const std::execution::parallel_policy& policy, int document_id);


private:
    struct DocumentData {
        std::set <std::string, std::less<>> words_;
        int rating;
        DocumentStatus status;
    };

    static constexpr double DOUBLE_COMPARISON_ERROR = 1e-6;
    const std::set<std::string> stop_words_;
    std::map<std::string_view , std::map<int, double>> word_to_document_frequency_;
    std::map<int, std::map<std::string_view , double>> document_to_word_frequency_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    [[nodiscard]] bool IsStopWord(const std::string& word) const;

    static void IsValidWord(const std::string& word);

    [[nodiscard]] std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    [[nodiscard]] QueryWord ParseQueryWord(std::string text) const;

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    [[nodiscard]] Query ParseQuery(std::string_view text) const;

    [[nodiscard]] double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy& policy, const Query& query, DocumentPredicate document_predicate) const;

};



template <typename DocumentPredicate>
[[nodiscard]] std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
    Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);

    std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < DOUBLE_COMPARISON_ERROR) {
            return lhs.rating > rhs.rating;
        } else {
            return lhs.relevance > rhs.relevance;
        }
    });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
[[nodiscard]] std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& police, const std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(raw_query, document_predicate);
}

template <typename DocumentPredicate>
[[nodiscard]] std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy, const std::string_view  raw_query, DocumentPredicate document_predicate) const {

    Query query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    std::sort(policy, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < DOUBLE_COMPARISON_ERROR) {
            return lhs.rating > rhs.rating;
        } else {
            return lhs.relevance > rhs.relevance;
        }
    });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_frequency_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_frequency_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_frequency_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_frequency_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.emplace_back(document_id, relevance, documents_.at(document_id).rating);
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy& policy, const Query& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(100);
    std::vector<std::string_view> minus_words(query.minus_words.begin(), query.minus_words.end());
    std::vector<std::string> plus_words(query.plus_words.begin(), query.plus_words.end());

    std::for_each(policy, plus_words.begin(), plus_words.end(),
                    [&](const std::string& word){
                        if (word_to_document_frequency_.count(word)) {
                            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                            for (const auto [document_id, term_freq] : word_to_document_frequency_.at(word)) {
                                const auto& document_data = documents_.at(document_id);
                                if(std::none_of(policy, minus_words.begin(), minus_words.end(),
                                                [&document_data](const std::string_view minus_word){
                                                    return document_data.words_.count(minus_word);
                                                })){
                                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                                        document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                                    }
                                }
                            }
                        }
      });

    std::vector<Document> matched_documents;
    std::map<int, double> ordinaryMap_document_to_relevance = move(document_to_relevance.BuildOrdinaryMap());

    for (const auto [document_id, relevance] : ordinaryMap_document_to_relevance) {
        matched_documents.emplace_back(document_id, relevance, documents_.at(document_id).rating);
    }
    return matched_documents;
}

