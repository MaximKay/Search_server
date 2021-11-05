#pragma once
#include<vector>
#include<string>
#include<deque>

#include "document.h"
#include "search_server.h"

class RequestQueue {
public:
	explicit RequestQueue(SearchServer&);

	template <typename DocumentPredicate>
	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
		const auto search_result = search_server_.FindTopDocuments(raw_query, document_predicate);
		NewQuery(raw_query, search_result);
		return search_result;
	}

	std::vector<Document> AddFindRequest(const std::string&, DocumentStatus);

	std::vector<Document> AddFindRequest(const std::string&);

	int GetNoResultRequests() const;

private:
	struct QueryResult {
		std::string query_;
		bool successful_search_;
	};
	std::deque<QueryResult> requests_;
	const static int sec_in_day_ = 1440;
	SearchServer& search_server_;

	void NewQuery(const std::string&, const std::vector<Document>&);
};
