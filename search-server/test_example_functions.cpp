#include "test_example_functions.h"

std::ostream& operator<< (std::ostream& out, const std::vector<std::string>& collection){
    using namespace std;
    for(auto &elem : collection){
        out << elem << " "s;
    }
    return out;
}
void PrintMatchForDocument(const std::tuple<std::vector<std::string>, DocumentStatus>& matchInfo, int documentId){
        using namespace std;
        auto &[words, status] =  matchInfo;
        cout << "{ document_id = "s << documentId << ", status = "s << static_cast<int>(status)
             << " words = "s << words << " }"s <<endl;
}
void MatchDocuments(SearchServer& server, const std::string& query){
    std::cout << "Matching documents for query : " << query << std::endl;
    for(const int document_id : server){
        PrintMatchForDocument(server.MatchDocument(query, document_id), document_id);
    }
}
void FindTopDocuments(SearchServer& server, const std::string& query){
    using namespace std;
    cout << "Result search for query : " << query << endl;
    for(const auto document : server.FindTopDocuments(query)){
        cout << "{ "s << document << " }"s << endl;
    }

}
void AddDocument(SearchServer& server, int documentId, const std::string& query, DocumentStatus documentStatus, std::vector<int> ratings){
    server.AddDocument(documentId, query, documentStatus, ratings);
}