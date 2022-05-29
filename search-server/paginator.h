#pragma once
#include <iostream>
#include <vector>

template <typename InputIt>
class IteratorRange {
public:
    IteratorRange(const InputIt begin,const InputIt end):
            begin_(begin), end_(end) {
    }
    auto begin() const{
        return begin_;
    }

    auto end() const{
        return end_;
    }
private:
    InputIt begin_, end_;

};


template <typename InputIt>
class Paginator{
public:
    Paginator (InputIt begin, InputIt end, size_t page_size){
        size_t cnt = distance (begin, end);
        for(; cnt > page_size; cnt -= page_size){
            auto currentRangeBegin = begin;
            auto currentRangeEnd = next(begin, page_size);
            pages_.push_back(IteratorRange(currentRangeBegin, currentRangeEnd));
            begin = currentRangeEnd;
        }
        if(cnt > 0){
            auto currentRangeBegin = begin;
            auto currentRangeEnd = next(begin, cnt);
            pages_.push_back(IteratorRange(currentRangeBegin, currentRangeEnd));
        }
    }
    auto begin() const {
        return pages_.begin();
    }
    auto end() const {
        return pages_.end();
    }
private:
    std::vector <IteratorRange<InputIt>> pages_;
};


template <typename InputIt>
std::ostream& operator << (std::ostream& out, const IteratorRange<InputIt>& range){
    using namespace std;
    for(const auto elem : range){
        out << "{ "s << elem << " }"s;
    }
    return out;
}
template <typename InputIt>
std::ostream& operator << (std::ostream& out, const Paginator<InputIt>& pages){
    using namespace std;
    for(const auto page : pages){
        out << page << endl;
    }
    return out;
}

template <typename Container>
auto Paginate (Container& container, size_t page_size){
    return Paginator(std::begin(container), std::end(container), page_size);
}


