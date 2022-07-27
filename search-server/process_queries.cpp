#include "process_queries.h"
#include <execution>

std::vector<std::vector<Document>> ProcessQueries(
        const SearchServer& search_server,
        const std::vector<std::string>& queries){
    std::vector<std::vector<Document>> result (queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(),
                   [&search_server](const std::string& query){
        return search_server.FindTopDocuments(query);
    });
    return result;
}

std::vector<Document> ProcessQueriesJoined(
        const SearchServer& search_server,
        const std::vector<std::string>& queries){
        std::vector<std::vector<Document>> pred_result (queries.size());
        std::transform(std::execution::par, queries.begin(), queries.end(), pred_result.begin(),
                   [&search_server](const std::string& query){
                       return search_server.FindTopDocuments(query);
                   });
        size_t count = 0;
        for(auto v : pred_result){
            count+=v.size();
        }
        std::vector<Document> result;
        result.reserve(count);
        for(auto v : pred_result){
            for(auto doc : v){
                result.push_back(std::move(doc));
            }

        }
        return result;
}