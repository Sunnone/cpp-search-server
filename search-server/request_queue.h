#pragma once
#include "search_server.h"
#include <deque>
#include <vector>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    void AddResult(const std::vector<Document>& query);

    struct QueryResult {
    public:
        int query_result_;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int result_request = 0;
    const SearchServer& double_server;
    int zero_ = 0;
};


template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    std::vector<Document> query = double_server.FindTopDocuments(raw_query, document_predicate);
    AddResult(query);
    return query;
}