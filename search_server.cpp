#include "search_server.h"

SearchServer::SearchServer(const std::string& stop_words_text)
	: SearchServer(SplitIntoWords(stop_words_text)) {}

SearchServer::SearchServer(const std::string_view& stop_words_view)
	: SearchServer(SplitIntoWords(stop_words_view)) {}

void SearchServer::AddDocument(int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings) {
	if (document_id < 0 || documents_.count(document_id)) {
		//if recieved id is negative or already exists, throw exception
		throw std::invalid_argument("Invalid document_id"s);
	}
	const auto words = SplitIntoWordsNoStop(document);

	const double inv_word_count = 1.0 / words.size();
	for (const std::string& word : words) {
		word_to_document_freqs_[word][document_id] += inv_word_count;
		document_words_freqs_[document_id][word] += inv_word_count;
	}
	documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
	document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, const DocumentStatus& status) const {
	return FindTopDocuments(raw_query, [status](int, DocumentStatus document_status, int) { return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query) const {
	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
	return static_cast<int>(document_ids_.size());
}

int SearchServer::GetDocumentId(int id) const {
	const auto id_iter = document_ids_.find(id);
	if (id_iter == document_ids_.end()) { //if requested id doesnt exist, throw exception
		throw std::out_of_range("Invalid document id!"s);
	};
	return *(id_iter);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
	const std::string_view& raw_query, int document_id) const {

	//throw exception if document with recieved id does not exist
	if (document_ids_.count(document_id) == 0) {
		throw std::out_of_range("Invalid id"s);
	};

	auto temp_query = ParseQuery(raw_query);

	std::vector<std::string_view> matched_words;
	//find and add all matched words to result vector
	for (const std::string& word : temp_query.plus_words) {
		if (word_to_document_freqs_.count(word) &&
			word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.push_back(std::string_view((*word_to_document_freqs_.find(word)).first));
		}
	}

	//if any of minus words exists in the document we are looking for matches, then clear all matched words
	for (const std::string& word : temp_query.minus_words) {
		if (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.clear();
			break;
		}
	}
	return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const std::string& word) const {
	return stop_words_.count(word);
}

bool SearchServer::IsValidWord(const std::string& word) {
	// A valid word must not contain special characters
	return none_of(word.begin(), word.end(), [](char c) {
		return c >= '\0' && c < ' ';
		});
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const {
	std::vector<std::string> words;
	for (const std::string& word : SplitIntoWords(text)) {
		if (!IsValidWord(word)) { //word should be valid
			throw std::invalid_argument("Word "s + word + " is invalid"s);
		}
		if (!IsStopWord(word)) { //word should not be a stop word
			words.push_back(word);
		}
	}
	return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
	if (ratings.empty()) {
		return 0;
	};

	int rating_sum = 0;
	for (const int rating : ratings) {
		rating_sum += rating;
	};
	return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
	if (text.empty()) { //query word should not be an empty word
		throw std::invalid_argument("Query word is empty"s);
	};

	std::string word = text;
	bool is_minus = false;
	if (word[0] == '-') { //if word has minus character is front, it is a minus word
		is_minus = true;
		word = word.substr(1);
	};

	//if second character is minus too or there is no characters or word is not valid, then throw exception
	if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
		throw std::invalid_argument("Query word "s + text + " is invalid"s);
	}

	return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text) const {
	Query result;
	for (const std::string& word : SplitIntoWords(text)) {
		const auto query_word = ParseQueryWord(word);
		if (!query_word.is_stop) { //word should not be a stop word
			query_word.is_minus ?
				result.minus_words.insert(query_word.data) : result.plus_words.insert(query_word.data);
		}
	}
	return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

const std::map<std::string_view, double> SearchServer::GetWordFrequencies(int document_id) const {
	std::map<std::string_view, double> result;

	auto found_doc = document_words_freqs_.find(document_id);
	if (found_doc != document_words_freqs_.end()) { //if document with requested id had been found
		auto& temp_map = found_doc->second;
		for (const auto& [word, freq] : temp_map) {
			result.insert({ std::string_view(word), freq });
		};
	};

	return result;
}

void SearchServer::RemoveDocument(int document_id) {
	if (document_ids_.count(document_id)) { //if there is no document with recieved id, then do nothing
		document_ids_.erase(document_ids_.find(document_id));
		documents_.erase(documents_.find(document_id));

		for (const auto& [word, _] : document_words_freqs_.find(document_id)->second) {
			auto& id_freq_map = word_to_document_freqs_.find(word)->second;
			id_freq_map.erase(id_freq_map.find(document_id));
		};

		document_words_freqs_.erase(document_words_freqs_.find(document_id));
	};
}

//helps print MatchDocument method result 
void PrintMatchDocumentResult(const int document_id, const std::vector<std::string_view>& words, DocumentStatus status) {
	std::cout << "{ "s
		<< "document_id = "s << document_id << ", "s
		<< "status = "s << static_cast<int>(status) << ", "s
		<< "words = "s;
	for (const std::string_view& word : words) {
		std::cout << word << " "s;
	}
	std::cout << "}"s << std::endl;
}

//adding document with exceptions processing
void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
	const std::vector<int>& ratings) {
	try {
		search_server.AddDocument(document_id, document, status, ratings);
	}
	catch (const std::exception& e) {
		std::cout << "Error adding a document "s << document_id << ": "s << e.what() << std::endl;
	}
}

//doing search request with exceptions processing, logging and printing duration
void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
	LOG_DURATION_STREAM("Operation time"s, std::cout); //start counting duration
	std::cout << "Search results for: "s << raw_query << std::endl;
	try {
		for (const Document& document : search_server.FindTopDocuments(raw_query)) {
			std::cout << document << std::endl;
		}
	}
	catch (const std::exception& e) {
		std::cout << "Search error: "s << e.what() << std::endl;
	}
} //end of namespace, log duration stream will print duration

//looking for mathing documents with exceptions processing, logging and printing duration
void MatchDocuments(const SearchServer& search_server, const std::string& query) {
	LOG_DURATION_STREAM("Operation time"s, std::cout); //start counting duration
	try {
		std::cout << "Matched documents for: "s << query << std::endl;
		for (const int id : search_server) {
			const auto& [words, status] = search_server.MatchDocument(query, id);
			PrintMatchDocumentResult(id, words, status);
		};
	}
	catch (const std::exception& e) {
		std::cout << "Error matching documents for "s << query << ": "s << e.what() << std::endl;
	}
} //end of namespace, log duration stream will print duration
