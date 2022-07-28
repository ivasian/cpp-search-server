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
using vector_string_view = std::vector<std::string_view>;
using matched_word_with_status = std::tuple<vector_string_view, DocumentStatus>;

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

    template <typename ExecutionPolicy, typename DocumentPredicate>
    [[nodiscard]] std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const;

    [[nodiscard]] std::vector<Document> FindTopDocuments(std::string_view  raw_query, DocumentStatus status) const;

    template<typename ExecutionPolicy>
    [[nodiscard]] std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view  raw_query, DocumentStatus status) const;


    [[nodiscard]] std::vector<Document> FindTopDocuments(std::string_view raw_query) const;
    template<typename ExecutionPolicy>
    [[nodiscard]] std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view  raw_query) const;


    [[nodiscard]] matched_word_with_status MatchDocument(std::string_view raw_query, int document_id) const;
    template<typename ExecutionPolicy>
    [[nodiscard]] matched_word_with_status MatchDocument(const ExecutionPolicy& policy, std::string_view raw_query, int document_id) const;

    [[nodiscard]] int GetDocumentCount() const;

    [[nodiscard]] std::_Rb_tree_const_iterator<int> begin() const;
    [[nodiscard]] std::_Rb_tree_const_iterator<int> end() const;

    [[nodiscard]] const std::map<std::string_view , double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    template<typename ExecutionPolicy>
    void RemoveDocument(const ExecutionPolicy& policy, int document_id);



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

template <typename ExecutionPolicy, typename DocumentPredicate>
[[nodiscard]] std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const {
    if constexpr(std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>){
        return FindTopDocuments(raw_query, document_predicate);
    } else {
        Query query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(static_cast<const std::execution::parallel_policy>(policy), query, document_predicate);

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

}

template <typename ExecutionPolicy>
[[nodiscard]] std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentStatus status) const{
        return FindTopDocuments(
                policy,
                raw_query,
                [status](int document_id, DocumentStatus document_status, int rating) {
                    return document_status == status;
                });
}

template<typename ExecutionPolicy>
[[nodiscard]] std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view  raw_query) const{
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template<typename ExecutionPolicy>
void SearchServer::RemoveDocument(const ExecutionPolicy& policy, int document_id){
    if constexpr(std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>){
        RemoveDocument(document_id);
    } else {
        if (!document_ids_.count(document_id)) {
            return;
        }
        std::vector<std::string_view> words_in_document_with_document_id(documents_.at(document_id).words_.begin(),
                                                               documents_.at(document_id).words_.end());

        for_each(policy, words_in_document_with_document_id.begin(),
                 words_in_document_with_document_id.end(),
                 [&](const std::string_view word){
                     word_to_document_frequency_.at(word).erase(document_id);
                 });

        document_to_word_frequency_.erase(document_id);
        documents_.erase(document_id);
        document_ids_.erase(document_id);
    }
}

template<typename ExecutionPolicy>
[[nodiscard]] matched_word_with_status SearchServer::MatchDocument(const ExecutionPolicy& policy, std::string_view raw_query, int document_id) const{
    if constexpr(std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>){
        return MatchDocument(raw_query, document_id);
    } else {
        Query query = ParseQuery(raw_query);
        const auto& words_in_document = documents_.at(document_id).words_;

        auto word_checker = [&words_in_document](const std::string_view & word){
            return words_in_document.count(word);
        };

        std::vector <std::string_view> minus_word(query.minus_words.begin(), query.minus_words.end());

        if(any_of(policy, minus_word.begin(), minus_word.end(), word_checker)){
            return {std::vector <std::string_view> {}, documents_.at(document_id).status};
        }

        std::vector <std::string_view> pre_matched_words(query.plus_words.size());
        std::vector <std::string_view> plus_words(query.plus_words.begin(), query.plus_words.end());
        auto It_end = copy_if(policy, plus_words.begin(), plus_words.end(), pre_matched_words.begin(), word_checker);
        pre_matched_words.erase(It_end, pre_matched_words.end());

        sort(policy, pre_matched_words.begin(), pre_matched_words.end());
        pre_matched_words.erase(unique(policy, pre_matched_words.begin(), pre_matched_words.end()), pre_matched_words.end());

        std::atomic_int size = 0;
        std::vector<std::string_view> matched_words(pre_matched_words.size());
        for_each(policy, pre_matched_words.begin(), pre_matched_words.end(),
                 [&](const std::string_view& plus_word){
                     matched_words[size++] = *words_in_document.find(plus_word);
                 }
        );

        return {matched_words, documents_.at(document_id).status};
    }
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
    vector_string_view minus_words(query.minus_words.begin(), query.minus_words.end());
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

