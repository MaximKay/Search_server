#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <execution>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
	string s;
	getline(cin, s);
	return s;
}

int ReadLineWithNumber() {
	int result;
	cin >> result;
	ReadLine();
	return result;
}

vector<string> SplitIntoWords(const string& text) {
	vector<string> words;
	string word;
	for (const char c : text) {
		if (c == ' ') {
			words.push_back(word);
			word = "";
		} else {
			word += c;
		}
	}
	words.push_back(word);

	return words;
}

struct Document {
	int id;
	double relevance;
	int rating;
};

enum class DocumentStatus {
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};

class SearchServer {
public:
	void SetStopWords(const string& text) {
		for (const string& word : SplitIntoWords(text)) {
			stop_words_.insert(word);
		}
	}

	void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
		const vector<string> words = SplitIntoWordsNoStop(document);
		const double inv_word_count = 1.0 / words.size();
		for (const string& word : words) {
			word_to_document_freqs_[word][document_id] += inv_word_count;
		}
		documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
	}

	template <typename Filter>
	vector<Document> FindTopDocuments(const string& raw_query, Filter filter) const {
		const Query query = ParseQuery(raw_query);
		auto raw_documents = FindAllDocuments(query);
		vector<Document> matched_documents;

		for (const auto& doc:raw_documents){
			if (filter(doc.id, documents_.at(doc.id).status, doc.rating)){
				matched_documents.push_back(doc);
			};
		};

		const auto double_equal = [](const Document& lhs, const Document& rhs){
			if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
				return lhs.rating > rhs.rating;
			} else {return lhs.relevance > rhs.relevance;};
		};

		sort(execution::par, matched_documents.begin(), matched_documents.end(), double_equal);

		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}
		return matched_documents;
	}

	vector<Document> FindTopDocuments(const string& raw_query, const DocumentStatus& status) const {
		return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating)
				{return document_status == status;});
	}

	vector<Document> FindTopDocuments(const string& raw_query) const {
		return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
	}

	int GetDocumentCount() const {
		return documents_.size();
	}

	tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
		const Query query = ParseQuery(raw_query);
		vector<string> matched_words;
		for (const string& word : query.plus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			if (word_to_document_freqs_.at(word).count(document_id)) {
				matched_words.push_back(word);
			}
		}
		for (const string& word : query.minus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			if (word_to_document_freqs_.at(word).count(document_id)) {
				matched_words.clear();
				break;
			}
		}
		return {matched_words, documents_.at(document_id).status};
	}

private:
	struct DocumentData {
		int rating;
		DocumentStatus status;
	};

	set<string> stop_words_;
	map<string, map<int, double>> word_to_document_freqs_;
	map<int, DocumentData> documents_;

	bool IsStopWord(const string& word) const {
		return stop_words_.count(word) > 0;
	}

	vector<string> SplitIntoWordsNoStop(const string& text) const {
		vector<string> words;
		for (const string& word : SplitIntoWords(text)) {
			if (!IsStopWord(word)) {
				words.push_back(word);
			}
		}
		return words;
	}

	static int ComputeAverageRating(const vector<int>& ratings) {
		int rating_sum = reduce (execution::par, begin(ratings), end(ratings), 0);
		return rating_sum / static_cast<int>(ratings.size());
	}

	struct QueryWord {
		string data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(string text) const {
		bool is_minus = false;
		// Word shouldn't be empty
		if (text[0] == '-') {
			is_minus = true;
			text = text.substr(1);
		}
		return {
			text,
			is_minus,
			IsStopWord(text)
		};
	}

	struct Query {
		set<string> plus_words;
		set<string> minus_words;
	};

	Query ParseQuery(const string& text) const {
		Query query;
		for (const string& word : SplitIntoWords(text)) {
			const QueryWord query_word = ParseQueryWord(word);
			if (!query_word.is_stop) {
				if (query_word.is_minus) {
					query.minus_words.insert(query_word.data);
				} else {
					query.plus_words.insert(query_word.data);
				}
			}
		}
		return query;
	}

	// Existence required
	double ComputeWordInverseDocumentFreq(const string& word) const {
		return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
	}

	vector<Document> FindAllDocuments(const Query& query) const {
		map<int, double> document_to_relevance;
		for (const string& word : query.plus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
			for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {

				document_to_relevance[document_id] += term_freq * inverse_document_freq;

			}
		}

		for (const string& word : query.minus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
				document_to_relevance.erase(document_id);
			}
		}

		vector<Document> matched_documents;
		for (const auto [document_id, relevance] : document_to_relevance) {
			matched_documents.push_back({
				document_id,
				relevance,
				documents_.at(document_id).rating
			});
		}
		return matched_documents;
	}
};

void PrintDocument(const Document& document) {
	cout << "{ "s
			<< "document_id = "s << document.id << ", "s
			<< "relevance = "s << document.relevance << ", "s
			<< "rating = "s << document.rating
			<< " }"s << endl;
}

//Unit tests
#define RUN_TEST(func)RunningTest((func), #func)
#define ASSERT_EQUAL(a, b) AssertEqual((a), (b), #a, #b, __FILE__, __LINE__, __FUNCTION__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqual((a), (b), #a, #b, __FILE__, __LINE__, __FUNCTION__, (hint))
#define ASSERT(bool_value) Assert((bool_value), #bool_value, __FILE__, __LINE__, __FUNCTION__, ""s)
#define ASSERT_HINT(bool_value, hint) Assert((bool_value), #bool_value, __FILE__, __LINE__, __FUNCTION__, (hint))

void Assert(const bool& bool_value, const string& bool_str, const string& file_name, const int& line,
		const string& func_name, const string& hint){
	if (!bool_value){
		cerr << file_name << "("s << line << "): "s << func_name << ": "s
				<< "ASSERT("s << bool_str << ") failed."s;
		if (!hint.empty()) {
			cerr << " Hint: "s << hint;
		}
		cerr << endl;
		abort();
	};
}

template <typename A, typename B>
void AssertEqual(const A& first, const B& second, const string& first_str,
		const string& second_str, const string& file_name, const int& line,
		const string& func_name, const string& hint){
	if (first != second) {
		cerr << file_name << "("s << line << "): "s << func_name << ": "s
				<< "ASSERT("s << first_str << " == "s << second_str << ") failed."s;
		if (!hint.empty()) {
			cerr << " Hint: "s << hint;
		}
		cerr << endl;
		abort();
	};
}

template<typename Function>
void RunningTest (const Function& func, const string& func_name){
	func();
}

void TestExcludeStopWordsFromAddedDocumentContent() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = {1, 2, 3};
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Incorrect amount of found documents"s);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL_HINT(doc0.id, doc_id, "Incorrect document found"s);
	}

	{
		SearchServer server;
		server.SetStopWords("in the"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
	}
}

void TestMinusWords(){
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = {1, 2, 3};
	const int doc_id_2 = 12;
	const string content_2 = "dog in the house"s;
	const vector<int> ratings_2 = {5, 2, 4};
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
	const string content = "cat in the city"s;
	const vector<int> ratings = {1, 2, 3};
	const int doc_id_2 = 12;
	const string content_2 = "dog in the house"s;
	const vector<int> ratings_2 = {5, 2, 4};
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
	const string content = "cat in the city"s;
	const vector<int> ratings = {1, 2, 3};
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
	const string content = "cat in the city"s;
	const vector<int> ratings = {1, 2, 3};
	const int doc_id_2 = 12;
	const string content_2 = "dog in the house"s;
	const vector<int> ratings_2 = {5, 5, 4};
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);
		const auto filter = [](int document_id, DocumentStatus document_status, int rating){return document_status == DocumentStatus::BANNED;};
		const auto found_docs = server.FindTopDocuments("in"s, filter);
		ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Status filter found wrong amount of documents"s);
		ASSERT_EQUAL_HINT(found_docs[0].id, 12, "Status filter found wrong document"s);
	}
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
		const auto filter = [](int document_id, DocumentStatus document_status, int rating){return rating > 3;};
		const auto found_docs = server.FindTopDocuments("in"s, filter);
		ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Rating filter found wrong amount of documents"s);
		ASSERT_EQUAL_HINT(found_docs[0].id, 12, "Rating filter found wrong document"s);
	}
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
		const auto filter = [](int document_id, DocumentStatus document_status, int rating){return document_id > 20;};
		const auto found_docs = server.FindTopDocuments("in"s, filter);
		ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Id filter found wrong amount of documents"s);
		ASSERT_EQUAL_HINT(found_docs[0].id, 42, "Id filter found wrong document"s);
	}

}

void StatusFilterTest() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = {1, 2, 3};
	const int doc_id_2 = 12;
	const string content_2 = "dog in the house"s;
	const vector<int> ratings_2 = {5, 5, 4};
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);
		const auto found_docs = server.FindTopDocuments("in"s, DocumentStatus::BANNED);
		ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Document status as a second parameter found wrong amount of documents"s);
		ASSERT_EQUAL_HINT(found_docs[0].id, 12, "Document status as a second parameter found wrong document"s);
	}
}

void TestSearchServer() {
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestMinusWords);
	RUN_TEST(RelevanceCalculatingAndSortingTests);
	RUN_TEST(AverageRatingTest);
	RUN_TEST(CustomFiltersTests);
	RUN_TEST(StatusFilterTest);
}

int main() {

	TestSearchServer();
	// Если вы видите эту строку, значит все тесты прошли успешно
	cout << "Search server testing finished"s << endl;

	SearchServer search_server;
	search_server.SetStopWords("и в на"s);

	search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
	search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
	search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

	cout << "ACTUAL by default:"s << endl;
	for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
		PrintDocument(document);
	}

	cout << "ACTUAL:"s << endl;
	for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; })) {
		PrintDocument(document);
	}

	cout << "Even ids:"s << endl;
	for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
		PrintDocument(document);
	}

	return 0;
}
