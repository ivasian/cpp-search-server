#include "request_queue.h"
#include <iostream>
#include "document.h"

using namespace std;

QueryResult::QueryResult(const vector<Document>& result) : result_(result){
        }

bool QueryResult::IsEmpty() const{
            return result_.empty();
        }



RequestQueue::RequestQueue(SearchServer& search_server): 
                            search_server_(search_server){
    }
    

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
        return FillingDeque (QueryResult(search_server_.FindTopDocuments(raw_query, status)));
    }

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
        return FillingDeque (QueryResult(search_server_.FindTopDocuments(raw_query)));
    }

int RequestQueue::GetNoResultRequests() const {
        return empty_request_count;
    }


const vector<Document>& RequestQueue::FillingDeque(const QueryResult& result){
        requests_.push_front(result);
        if(result.IsEmpty()){
            ++empty_request_count;
        }
        if(requests_.size() > min_in_day_){
            if(requests_.back().IsEmpty()){
                --empty_request_count;
            }
            requests_.pop_back();
        }
        return result.result_;
    }