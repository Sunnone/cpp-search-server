#include "test_example_functions.h"

void AddDocument(SearchServer& search_server, int doc_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
	search_server.AddDocument(doc_id, document, status, ratings);
}