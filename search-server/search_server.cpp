#include "search_server.h"


using namespace std;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0 || documents_.count(document_id) > 0) {
        throw std::invalid_argument("Попытка добавить невалидный документ"s);
    }
    if (!IsValidWord(document)) {
        throw std::invalid_argument("Попытка добавить документ c недопустимыми символами"s);
    }

    documents_storage.emplace_back(document);
    const std::vector<std::string_view> words = SplitIntoWordsNoStop(documents_storage.back());
    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        id_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    docs_id_.insert(document_id);
}


int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    if (documents_.count(document_id) == 0) {
        throw std::out_of_range("No document with id "s + std::to_string(document_id));
    }
    bool need_sort = true;
    const static Query query = ParseQuery(raw_query, need_sort);
    if (std::any_of(query.minus_words.begin(), query.minus_words.end(), [this, document_id](auto& minus_word) {return id_word_freqs_.at(document_id).count(minus_word); })) {
        return { {},  documents_.at(document_id).status };
    }
    const auto& word_freqs = id_word_freqs_.at(document_id);
    std::vector<std::string_view> matched_words(query.plus_words.size());
    const auto match_words_end = std::copy_if(query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&word_freqs](auto& plus_word)
        {return  word_freqs.count(plus_word) != 0; });
    matched_words.erase(match_words_end, matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, const std::string_view raw_query, int document_id) const {
    return SearchServer::MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&, const std::string_view raw_query, int document_id) const {
    bool need_sort = false;
    const static Query query = ParseQuery(raw_query, need_sort);
    if (std::any_of(query.minus_words.begin(), query.minus_words.end(), [this, document_id](auto& minus_word) {return id_word_freqs_.at(document_id).count(minus_word); })) {
        return { {},  documents_.at(document_id).status };
    }
    const auto& word_freqs = id_word_freqs_.at(document_id);
    std::vector<std::string_view> matched_words(query.plus_words.size());
    const auto match_words_end = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&word_freqs](auto& plus_word)
        {return  word_freqs.count(plus_word) != 0; });
    matched_words.erase(match_words_end, matched_words.end());
    std::sort(std::execution::par, matched_words.begin(), matched_words.end());
    auto matched_words_new_end = std::unique(std::execution::par, matched_words.begin(), matched_words.end());
    matched_words.erase(matched_words_new_end, matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}

/*
int SearchServer::GetDocumentId(int index) const {
    if (index < 0 || index > documents_.size()) {
        throw std::out_of_range("Нет документа с таким индексом"s);
    }
    return all_documents_[index];
}*/


std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {return document_status == status; });
}

bool SearchServer::IsStopWord(const std::string_view& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const {
    std::vector<std::string_view> words;
    for (const auto& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-') {
        throw std::invalid_argument("Невалидный поисковый запрос"s);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text, bool need_sort) const {
    Query query;
    for (const auto word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("В поисковом запросе недопустимые символы"s);
        }
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            }
            else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    if (!need_sort) {
        return query;
    }

    sort(query.minus_words.begin(), query.minus_words.end());
    auto unique_minus_words = std::unique(query.minus_words.begin(), query.minus_words.end());
    query.minus_words.erase(unique_minus_words, query.minus_words.end());
    sort(query.plus_words.begin(), query.plus_words.end());
    auto unique_plus_words = std::unique(query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(unique_plus_words, query.plus_words.end());
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::IsValidWord(const std::string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static map<std::string_view, double> empty = {};
    if (id_word_freqs_.count(document_id)) {
        return id_word_freqs_.at(document_id);
    }
    return empty;
}

void SearchServer::RemoveDocument(int document_id) {
    for (auto [word, _] : id_word_freqs_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
        if (word_to_document_freqs_.at(word).empty()) {
            word_to_document_freqs_.erase(word);
        }
    }
    documents_.erase(document_id);
    id_word_freqs_.erase(document_id);
    docs_id_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    SearchServer::RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    if (!documents_.count(document_id)) {
        return;
    }
    vector<std::string_view> words(id_word_freqs_[document_id].size());
    std::transform(std::execution::par, id_word_freqs_.at(document_id).begin(), id_word_freqs_.at(document_id).end(), words.begin(),
        [this](const auto word) {return word.first; });
    for_each(std::execution::par, words.begin(), words.end(), [this, &document_id](const auto word) {word_to_document_freqs_.at(word).erase(document_id); });
    documents_.erase(document_id);
    id_word_freqs_.erase(document_id);
    docs_id_.erase(document_id);
}

std::set<int>::const_iterator SearchServer::begin() {
    return docs_id_.begin();
}


std::set<int>::const_iterator SearchServer::end() {
    return docs_id_.end();
}