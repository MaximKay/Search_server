#include "process_queries.h"

//this function returning vector of results for each query from incoming vector of queries
std::vector<std::vector<Document>> ProcessQueries(
	const SearchServer& search_server, const std::vector<std::string>& queries) {
	std::vector<std::vector<Document>> result(queries.size());
	std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(),
		[&search_server](const std::string& query) { 
			return search_server.FindTopDocuments(query); 
		});
	return result;
}

//transforming vector of results vectors to one vector
std::vector<Document> ProcessQueriesJoined(
	const SearchServer& search_server, const std::vector<std::string>& queries) {
	std::vector<Document> result;
	for (const std::vector<Document>& doc_vect : ProcessQueries(search_server, queries)) {
		result.insert(result.end(), doc_vect.begin(), doc_vect.end());
	};
	return result;
}