#include "request_queue.h"

RequestQueue::RequestQueue(SearchServer& search_server) : search_server_(search_server){}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
	const auto search_result = search_server_.FindTopDocuments(raw_query, status);
	NewQuery(raw_query, search_result);
	return search_result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
	const auto search_result = search_server_.FindTopDocuments(raw_query);
	NewQuery(raw_query, search_result);
	return search_result;
}

int RequestQueue::GetNoResultRequests() const {
	return static_cast<int>(count_if(begin(requests_), end(requests_),
		[](const QueryResult& result) {return result.successful_search_ == false; }));
}

void RequestQueue::NewQuery(const std::string& query, const std::vector<Document>& search_result){
	bool success;
	!search_result.empty() ? success = true : success = false;
	if (requests_.size() >= sec_in_day_){
		requests_.pop_front();
	};
	requests_.push_back({query, success});
}
