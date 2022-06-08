#pragma once
#include "search_server.h"
#include <vector>
#include <string>

void PrintMatchForDocument(const std::tuple<std::vector<std::string>, DocumentStatus>& matchInfo, int documentId);
void MatchDocuments(SearchServer& server, const std::string& query);
void FindTopDocuments(SearchServer& server, const std::string& query);
void AddDocument(SearchServer& server, int documentId, const std::string& query, DocumentStatus documentStatus, std::vector<int> ratings);
std::ostream& operator<< (std::ostream& out, const std::vector<std::string>& collection);
