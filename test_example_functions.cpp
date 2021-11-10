#include "test_example_functions.h"

void Assert(const bool& bool_value, const std::string& bool_str, const std::string& file_name, const int& line,
		const std::string& func_name, const std::string& hint){
	if (!bool_value){
		std::cerr << file_name << "("s << line << "): "s << func_name << ": "s
				<< "ASSERT("s << bool_str << ") failed."s;
		if (!hint.empty()) {
			std::cerr << " Hint: "s << hint;
		}
		std::cerr << std::endl;
		abort();
	};
}

void TestExcludeStopWordsFromAddedDocumentContent() {
	const int doc_id = 42;
	const std::string content = "cat in the city"s;
	const std::vector<int> ratings = {1, 2, 3};
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Incorrect amount of found documents"s);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL_HINT(doc0.id, doc_id, "Incorrect document found"s);
	}

	{
		SearchServer server("in the"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
	}
}

void TestMinusWords(){
	const int doc_id = 42;
	const std::string content = "cat in the city"s;
	const std::vector<int> ratings = {1, 2, 3};
	const int doc_id_2 = 12;
	const std::string content_2 = "dog in the house"s;
	const std::vector<int> ratings_2 = {5, 2, 4};
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
		const auto found_docs = server.FindTopDocuments("in the -dog"s);
		ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Documents with minus words must be excluded"s);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL_HINT(doc0.id, doc_id, "Minus words excluded wrong document"s);
	}
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
		ASSERT_HINT(server.FindTopDocuments("-in cat dog"s).empty(), "Stop words must be excluded from documents"s);
	}
}

void RelevanceCalculatingAndSortingTests(){
	const int doc_id = 42;
	const std::string content = "cat in the city"s;
	const std::vector<int> ratings = {1, 2, 3};
	const int doc_id_2 = 12;
	const std::string content_2 = "city house"s;
	const std::vector<int> ratings_2 = {1, 2, 1};
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
		const auto found_docs = server.FindTopDocuments("cat"s);
		const double rel = 0.25 * (log(2.0 / 1));
		ASSERT_HINT(abs(found_docs[0].relevance - rel) < 1e-6, "Incorrect relevance calculating"s);
	}
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
		server.AddDocument(10, "some important text"s, DocumentStatus::ACTUAL, {5, 5, 5});
		const auto found_docs = server.FindTopDocuments("in city"s);
		ASSERT_HINT(found_docs[0].relevance > found_docs[1].relevance, "Incorrect relevance sorting"s);
	}
}

void AverageRatingTest() {
	const int doc_id = 42;
	const std::string content = "cat in the city"s;
	const std::vector<int> ratings = {1, 2, 3};
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("cat"s);
		const int correct_rating = (1 + 2 + 3)/3;
		ASSERT_EQUAL_HINT(found_docs[0].rating, correct_rating, "Incorrect average rating calculations"s);
	}
}

void CustomFiltersTests() {
	const int doc_id = 42;
	const std::string content = "cat in the city"s;
	const std::vector<int> ratings = {1, 2, 3};
	const int doc_id_2 = 12;
	const std::string content_2 = "dog in the house"s;
	const std::vector<int> ratings_2 = {5, 5, 4};
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);
		const auto filter = [](int document_id, DocumentStatus document_status, int rating)
																										{return document_status == DocumentStatus::BANNED;};
		const auto found_docs = server.FindTopDocuments("in"s, filter);
		ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Status filter found wrong amount of documents"s);
		ASSERT_EQUAL_HINT(found_docs[0].id, 12, "Status filter found wrong document"s);
	}
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
		const auto filter = [](int document_id, DocumentStatus document_status, int rating)
																												{return rating > 3;};
		const auto found_docs = server.FindTopDocuments("in"s, filter);
		ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Rating filter found wrong amount of documents"s);
		ASSERT_EQUAL_HINT(found_docs[0].id, 12, "Rating filter found wrong document"s);
	}
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
		const auto filter = [](int document_id, DocumentStatus document_status, int rating)
																													{return document_id > 20;};
		const auto found_docs = server.FindTopDocuments("in"s, filter);
		ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Id filter found wrong amount of documents"s);
		ASSERT_EQUAL_HINT(found_docs[0].id, 42, "Id filter found wrong document"s);
	}

}

void StatusFilterTest() {
	const int doc_id = 42;
	const std::string content = "cat in the city"s;
	const std::vector<int> ratings = {1, 2, 3};
	const int doc_id_2 = 12;
	const std::string content_2 = "dog in the house"s;
	const std::vector<int> ratings_2 = {5, 5, 4};
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);
		const auto found_docs = server.FindTopDocuments("in"s, DocumentStatus::BANNED);
		ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Document status as a second parameter found wrong amount of documents"s);
		ASSERT_EQUAL_HINT(found_docs[0].id, 12, "Document status as a second parameter found wrong document"s);
	}
}

void RemoveDocumentTest(){
	const int doc_id = 42;
	const std::string content = "cat in the city"s;
	const std::vector<int> ratings = {1, 2, 3};
	const int doc_id_2 = 12;
	const std::string content_2 = "dog in the house"s;
	const std::vector<int> ratings_2 = {5, 5, 4};
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
		server.RemoveDocument(doc_id);
		std::map<std::string, double> empty_map;
		ASSERT_EQUAL_HINT(server.GetDocumentId(doc_id), -999, "Wrong document was removed from ids set"s);
		ASSERT_EQUAL_HINT(server.GetWordFrequencies(doc_id), empty_map, "Wrong document was removed from word freqs map"s);
		const auto found_docs = server.FindTopDocuments("city"s);
		ASSERT_EQUAL_HINT(found_docs.size(), 0u, "Document was not removed"s);
	}
}

void TestSearchServer() {
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestMinusWords);
	RUN_TEST(RelevanceCalculatingAndSortingTests);
	RUN_TEST(AverageRatingTest);
	RUN_TEST(CustomFiltersTests);
	RUN_TEST(StatusFilterTest);
	RUN_TEST(RemoveDocumentTest);
}
