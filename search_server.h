#pragma once
#include<string>
#include<set>
#include<vector>
#include<map>
#include<algorithm>
#include<stdexcept>
#include<cmath>

#include "string_processing.h"
#include "document.h"
#include "log_duration.h"

using namespace std::string_literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
	explicit SearchServer() = default;

	explicit SearchServer(const std::string&);

	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words)
	: stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
	{
		if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
			throw std::invalid_argument("Some of stop words are invalid"s);
		}
	}

	void AddDocument(int, const std::string&, DocumentStatus, const std::vector<int>&);

	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {
		const auto query = ParseQuery(raw_query);

		auto matched_documents = FindAllDocuments(query, document_predicate);

		sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
			if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
				return lhs.rating > rhs.rating;
			} else {
				return lhs.relevance > rhs.relevance;
			}
		});
		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}

		return matched_documents;
	}

	std::vector<Document> FindTopDocuments(const std::string&, DocumentStatus) const;

	std::vector<Document> FindTopDocuments(const std::string&) const;

	int GetDocumentCount() const;

	int GetDocumentId(int id) const;

	auto begin()const{
		return document_ids_.begin();
	}

	auto end()const{
		return document_ids_.end();
	}

	const std::map<std::string, double>& GetWordFrequencies(int) const;

	std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string&, int) const;

	void RemoveDocument(int);
private:
	struct DocumentData {
		int rating;
		DocumentStatus status;
	};
	const std::set<std::string> stop_words_;
	std::map<std::string, std::map<int, double>> word_to_document_freqs_;
	std::map<int, std::map<std::string, double>> document_words_freqs_;
	std::map<int, DocumentData> documents_;
	std::set<int> document_ids_;

	bool IsStopWord(const std::string&) const;

	static bool IsValidWord(const std::string&);

	std::vector<std::string> SplitIntoWordsNoStop(const std::string&) const;

	static int ComputeAverageRating(const std::vector<int>&);

	struct QueryWord {
		std::string data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(const std::string&) const;

	struct Query {
		std::set<std::string> plus_words;
		std::set<std::string> minus_words;
	};

	Query ParseQuery(const std::string&) const;

	double ComputeWordInverseDocumentFreq(const std::string&) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
		std::map<int, double> document_to_relevance;
		for (const std::string& word : query.plus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
			for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
				const auto& document_data = documents_.at(document_id);
				if (document_predicate(document_id, document_data.status, document_data.rating)) {
					document_to_relevance[document_id] += term_freq * inverse_document_freq;
				}
			}
		}

		for (const std::string& word : query.minus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
				document_to_relevance.erase(document_id);
			}
		}

		std::vector<Document> matched_documents;
		for (const auto [document_id, relevance] : document_to_relevance) {
			matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
		}
		return matched_documents;
	}
};

void PrintMatchDocumentResult(int, const std::vector<std::string>&, DocumentStatus);

void AddDocument(SearchServer&, int, const std::string&, DocumentStatus, const std::vector<int>&);

void FindTopDocuments(const SearchServer&, const std::string&);

void MatchDocuments(const SearchServer&, const std::string&);
