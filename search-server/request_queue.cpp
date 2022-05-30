#include "request_queue.h"
#include <iostream>
#include "document.h"

using namespace std;

QueryResult::QueryResult(const vector<Document>& result) : isEmpty(result.empty()){
        }

RequestQueue::RequestQueue(SearchServer& search_server): 
                            search_server_(search_server){
    }

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
        return FillingDeque (search_server_.FindTopDocuments(raw_query, status));
    }

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
        return FillingDeque (search_server_.FindTopDocuments(raw_query));
    }

int RequestQueue::GetNoResultRequests() const {
        return empty_request_count;
    }


const vector<Document>& RequestQueue::FillingDeque(const vector<Document>& findTopDocResult){
        QueryResult queryResult(findTopDocResult);
        requests_.push_front(queryResult);
        if(queryResult.isEmpty){
            ++empty_request_count;
        }
        if(requests_.size() > min_in_day_){
            if(requests_.back().isEmpty){
                --empty_request_count;
            }
            requests_.pop_back();
        }
        return findTopDocResult;
    }