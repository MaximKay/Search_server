#include "read_input_functions.h"
#include "string_processing.h"
#include "document.h"
#include "search_server.h"
#include "paginator.h"
#include "request_queue.h"
#include "test_example_functions.h"
#include "remove_duplicates.h"
#include "process_queries.h"

using namespace std::string_literals;

void PrintDocument(const Document& document) {
	std::cout << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating << " }"s << std::endl;
}

int main() {
	{
		TestSearchServer();
		std::cout << "All tests OK"s << std::endl;
	}

	{
		SearchServer search_server("and in at"s);
		RequestQueue request_queue(search_server);

		search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
		search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
		search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
		search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
		search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

		// 1439 empty requests
		for (int i = 0; i < 1439; ++i) {
			request_queue.AddFindRequest("empty request"s);
		}
		// still 1439 empty requests
		request_queue.AddFindRequest("curly dog"s);
		// 1438 empty requests
		request_queue.AddFindRequest("big collar"s);
		// 1437 empty requests
		request_queue.AddFindRequest("sparrow"s);
		std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
	}

	{
	   SearchServer search_server("и в на"s);

	   AddDocument(search_server, 1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
	   AddDocument(search_server, 1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
	   AddDocument(search_server, -1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
	   AddDocument(search_server, 3, "большой пёс скво\x12рец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
	   AddDocument(search_server, 4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 1, 1});

	   FindTopDocuments(search_server, "пушистый -пёс"s);
	   FindTopDocuments(search_server, "пушистый --кот"s);
	   FindTopDocuments(search_server, "пушистый -"s);

	   MatchDocuments(search_server, "пушистый пёс"s);
	   MatchDocuments(search_server, "модный -кот"s);
	   MatchDocuments(search_server, "модный --пёс"s);
	   MatchDocuments(search_server, "пушистый - хвост"s);
   }

   {
	   SearchServer search_server("and with"s);

	   search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
	   search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
	   search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
	   search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, {1, 3, 2});
	   search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});

	   const auto search_results = search_server.FindTopDocuments("curly dog"s);
	   int page_size = 2;
	   const auto pages = Paginate(search_results, page_size);

	   // pagination
	   for (auto page = pages.begin(); page != pages.end(); ++page) {
		   std::cout << *page << std::endl;
		   std::cout << "Page break"s << std::endl;
	   }
   }

   {
	   SearchServer search_server("and with"s);

	   AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
	   AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

	   // дубликат документа 2, будет удалён
	   AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

	   // отличие только в стоп-словах, считаем дубликатом
	   AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

	   // множество слов такое же, считаем дубликатом документа 1
	   AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

	   // добавились новые слова, дубликатом не является
	   AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

	   // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
	   AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

	   // есть не все слова, не является дубликатом
	   AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

	   // слова из разных документов, не является дубликатом
	   AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

	   std::cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
	   RemoveDuplicates(search_server);
	   std::cout << "After duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
   }

   {
	   SearchServer search_server("and with"s);

	   int id = 0;
	   for (
		   const std::string& text : {
			   "funny pet and nasty rat"s,
			   "funny pet with curly hair"s,
			   "funny pet and not very nasty rat"s,
			   "pet with rat and rat and rat"s,
			   "nasty rat with curly hair"s,
		   }
		   ) {
		   search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
	   }

	   const std::vector<std::string> queries = {
		   "nasty rat -not"s,
		   "not very funny nasty pet"s,
		   "curly hair"s
	   };
	   id = 0;
	   for (
		   const auto& documents : ProcessQueries(search_server, queries)
		   ) {
		   std::cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << std::endl;
	   }
   }

   {
	   SearchServer search_server("and with"s);

	   int id = 0;
	   for (
		   const std::string& text : {
			   "funny pet and nasty rat"s,
			   "funny pet with curly hair"s,
			   "funny pet and not very nasty rat"s,
			   "pet with rat and rat and rat"s,
			   "nasty rat with curly hair"s,
		   }
		   ) {
		   search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
	   }

	   const std::vector<std::string> queries = {
		   "nasty rat -not"s,
		   "not very funny nasty pet"s,
		   "curly hair"s
	   };
	   for (const Document& document : ProcessQueriesJoined(search_server, queries)) {
		   std::cout << "Document "s << document.id << " matched with relevance "s << document.relevance << std::endl;
	   }
   }

   {
	   SearchServer search_server("and with"s);

	   int id = 0;
	   for (
		   const std::string& text : {
			   "funny pet and nasty rat"s,
			   "funny pet with curly hair"s,
			   "funny pet and not very nasty rat"s,
			   "pet with rat and rat and rat"s,
			   "nasty rat with curly hair"s,
		   }
		   ) {
		   search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
	   }

	   const std::string query = "curly and funny"s;

	   auto report = [&search_server, &query] {
		   std::cout << search_server.GetDocumentCount() << " documents total, "s
			   << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << std::endl;
	   };

	   report();
	   // однопоточная версия
	   search_server.RemoveDocument(5);
	   report();
	   // однопоточная версия
	   search_server.RemoveDocument(std::execution::seq, 1);
	   report();
	   // многопоточная версия
	   search_server.RemoveDocument(std::execution::par, 2);
	   report();
   }

   {
	   SearchServer search_server("and with"s);

	   int id = 0;
	   for (
		   const std::string& text : {
			   "funny pet and nasty rat"s,
			   "funny pet with curly hair"s,
			   "funny pet and not very nasty rat"s,
			   "pet with rat and rat and rat"s,
			   "nasty rat with curly hair"s,
		   }
		   ) {
		   search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
	   }

	   const std::string query = "curly and funny -not"s;

	   {
		   const auto [words, status] = search_server.MatchDocument(query, 1);
		   std::cout << words.size() << " words for document 1"s << std::endl;
		   // 1 words for document 1
	   }

	   {
		   const auto [words, status] = search_server.MatchDocument(std::execution::seq, query, 2);
		   std::cout << words.size() << " words for document 2"s << std::endl;
		   // 2 words for document 2
	   }

	   {
		   const auto [words, status] = search_server.MatchDocument(std::execution::par, query, 3);
		   std::cout << words.size() << " words for document 3"s << std::endl;
		   // 0 words for document 3
	   }
   }

	{
		SearchServer search_server("and with"s);

		int id = 0;
		for (
			const std::string& text : {
				"white cat and yellow hat"s,
				"curly cat curly tail"s,
				"nasty dog with big eyes"s,
				"nasty pigeon john"s,
			}
			) {
			search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
		}

		std::cout << "ACTUAL by default:"s << std::endl;
		// последовательная версия
		for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
			PrintDocument(document);
		}
		std::cout << "BANNED:"s << std::endl;
		// последовательная версия
		for (const Document& document : search_server.FindTopDocuments(std::execution::seq,
			"curly nasty cat"s, DocumentStatus::BANNED)) {
			PrintDocument(document);
		}

		std::cout << "Even ids:"s << std::endl;
		// параллельная версия
		for (const Document& document : search_server.FindTopDocuments(std::execution::par,
			"curly nasty cat"s, [](int document_id, DocumentStatus, int)
			{ return document_id % 2 == 0; })) {
			PrintDocument(document);
		}
	}

	return 0;
}
