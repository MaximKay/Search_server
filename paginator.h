#pragma once
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
	IteratorRange() = default;

	IteratorRange(Iterator begin, Iterator end) : begin_(begin), end_(end) {}

	auto begin() const {return begin_;}

	auto end() const {return end_;}

	auto size() const {return distance(begin_, end_);}

	template <typename Ostream>
	Ostream& operator<<(Ostream& output) {
		for (auto it = begin(); it != end(); ++it) {
			output << *it;
		}
		return output;
	}

private:
	Iterator begin_;
	Iterator end_;
};

template <typename Iterator>
class Paginator : public IteratorRange<Iterator>{
public:
	Paginator (Iterator it_begin, Iterator it_end, int page_size){
		for (auto it = it_begin; it != it_end; advance(it, page_size)){
			if (distance(it, it_end) <= page_size) {
				pages_.push_back({it,it_end});
				break;
			}
			pages_.push_back({it,next(it,page_size)});
		};
	}

	auto begin()const{
		return pages_.begin();
	}

	auto end()const{
		return pages_.end();
	}

	auto size() const {
		return distance(begin(), end());
	}

private:
	std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Ostream, typename Iterator>
Ostream& operator<<(Ostream& out, IteratorRange<Iterator> page){
	for (auto it = page.begin(); it != page.end(); ++it) {
		out << *it;
	}
	return out;
}

template <typename Container>
auto Paginate(const Container& c, int page_size){
	return Paginator(begin(c), end(c), page_size);
}
