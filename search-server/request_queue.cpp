#include "request_queue.h"


RequestQueue::RequestQueue(const SearchServer& search_server)
        : double_server(search_server)
    {
    }
    
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        std::vector<Document> query = double_server.FindTopDocuments(raw_query, status);
        AddResult(query);
        return query;
    }

 std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
        std::vector<Document> query = double_server.FindTopDocuments(raw_query);
        AddResult(query);
        return query;
    }

 int RequestQueue::GetNoResultRequests() const {
        return zero_;
    }

 void RequestQueue::AddResult(const std::vector<Document>& query) {
        QueryResult result;
        if (query.size() == 0) {
            ++zero_;
        }
        result.query_result_ = static_cast<int>(query.size());
        requests_.push_back(result);
        ++result_request;
        if (result_request > min_in_day_) {
            if (requests_[0].query_result_ == 0) {
                --zero_;
            }
            requests_.pop_front();
        }
    }