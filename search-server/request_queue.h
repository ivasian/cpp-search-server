#pragma once
#include "search_server.h"
#include "document.h"
#include <string>
#include <vector>
#include <deque>

struct QueryResult {
        std::vector<Document> result_;
        QueryResult(const std::vector<Document>& result);
        bool IsEmpty() const;
    };


class RequestQueue {
public:
    explicit RequestQueue(SearchServer& search_server);
    
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        return FillingDeque (QueryResult(search_server_.FindTopDocuments(raw_query, document_predicate)));
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
    
    
private:
    
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    SearchServer& search_server_;
    int empty_request_count = 0;

    const std::vector<Document>& FillingDeque(const QueryResult& result);
};