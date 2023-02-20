#pragma once
#include "document.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

using namespace std::literals;

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) :begin_(begin), end_(end) {} 

    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

    size_t size() const {
        size_ = std::distance(begin_, end_);
        return size_;
    }

    Iterator begin_;
    Iterator end_;
    size_t size_;
};

template <typename const_iterator>
class Paginator {
public:
    Paginator(const_iterator begin, const_iterator end, size_t size)
    {
        for (size_t i = distance(begin, end); i > 0; ) {
            const size_t page_size = std::min(size, i);
            const const_iterator step = std::next(begin, page_size);
            DocsInPage.push_back({ begin, step });
            i -= page_size;
            begin = step;
        }
    }

    auto begin() const {
        return DocsInPage.begin();
    }

    auto end() const {
        return DocsInPage.end();
    }

    size_t size() {
        return DocsInPage.size();
    }

private:
    std::vector<IteratorRange<const_iterator>> DocsInPage;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& q) {
    for (Iterator it = q.begin(); it != q.end(); ++it) {
        out << *it;
    }
    return out;
}

std::ostream& operator<<(std::ostream& cout, const Document& doc) {
    return cout << "{ "s << "document_id = "s
        << doc.id << ", "s
        << "relevance = "s
        << doc.relevance << ", "s
        << "rating = "s << doc.rating << " }"s;

}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}