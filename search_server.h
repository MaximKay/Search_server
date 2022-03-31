#pragma once
#include <string>
#include <set>
#include <vector>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <iostream>
#include <execution>
#include <type_traits>
#include <string_view>
#include <tuple>
#include <future>
#include <mutex>

#include "string_processing.h"
#include "document.h"
#include "log_duration.h"
#include "concurrent_map.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double MAX_DIFF = 1e-6;

class SearchServer {
public:
	explicit SearchServer() = default;

	explicit SearchServer(const std::string&);

	explicit SearchServer(const std::string_view&);

	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words)
		: stop_words_(MakeUniqueNonEmptyStrings(stop_words)) { // Extract non-empty stop words
		//if any word from stops words is not valid, throw exception
		if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
			throw std::invalid_argument("Some of stop words are invalid"s);
		};
	}

	void AddDocument(int, const std::string_view&, DocumentStatus, const std::vector<int>&);

	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const {
		const auto query = ParseQuery(raw_query);

		//getting all matched documents using custom or dafault predicate
		auto matched_documents = FindAllDocuments(query, document_predicate);

		//first sorting by relevance and if relevances are almost the same, then sort by rating
		sort(std::execution::seq, matched_documents.begin(), matched_documents.end(),
			[](const Document& lhs, const Document& rhs) {
				return (std::abs(lhs.relevance - rhs.relevance) < MAX_DIFF) ?
					lhs.rating > rhs.rating : lhs.relevance > rhs.relevance;
			});

		//if matched documents amount is bigger than MAX_RESULT_DOCUMENT_COUNT constant - delete unnecessary docs
		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}
		return matched_documents;
	}

	std::vector<Document> FindTopDocuments(const std::string_view&, const DocumentStatus&) const;

	std::vector<Document> FindTopDocuments(const std::string_view&) const;

	template <typename ExecutionPolicy, typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy,
		const std::string_view& raw_query, DocumentPredicate document_predicate) const {
		//non-parallel version
		if constexpr (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
			return FindTopDocuments(raw_query, document_predicate);
		}
		else {
			//using parallel methods for finding docs and their sorting

			const auto query = ParseQuery(raw_query);

			//getting all matched documents using custom or dafault predicate and parallel execution policy
			auto matched_documents = FindAllDocuments(policy, query, document_predicate);

			//first sorting by relevance and if relevances are almost the same, then sort by rating
			sort(std::execution::par, matched_documents.begin(), matched_documents.end(),
				[](const Document& lhs, const Document& rhs) {
					return (std::abs(lhs.relevance - rhs.relevance) < MAX_DIFF) ?
						lhs.rating > rhs.rating : lhs.relevance > rhs.relevance;
				});

			//if matched documents amount is bigger than MAX_RESULT_DOCUMENT_COUNT constant - delete unnecessary docs
			if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
				matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
			}
			return matched_documents;
		};
	}

	template <typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy,
		const std::string_view& raw_query, const DocumentStatus& status) const {
		if constexpr (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
			//non-parallel version
			return FindTopDocuments(raw_query, status);
		}
		else {
			//creating predicate with document status filtering
			return FindTopDocuments(policy, raw_query, [status](int, DocumentStatus document_status, int) { return document_status == status; });
		};
	}

	template <typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query) const {
		if constexpr (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
			//non-parallel version
			return FindTopDocuments(raw_query);
		}
		else {
			//default documents filter now is DocumentStatus::ACTUAL
			return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
		};
	}

	int GetDocumentCount() const;

	int GetDocumentId(int id) const;

	auto begin()const {
		return document_ids_.begin();
	}

	auto end()const {
		return document_ids_.end();
	}

	const std::map<std::string_view, double> GetWordFrequencies(int) const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view&, int) const;

	template <typename ExecutionPolicy>
	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
		ExecutionPolicy, const std::string_view& raw_query, int document_id) const {
		//non-parallel version
		if constexpr (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
			return MatchDocument(raw_query, document_id);
		};

		//throw exception if document with recieved id does not exist
		if (document_ids_.count(document_id) == 0) {
			throw std::out_of_range("Invalid id"s);
		};

		//version using parallel algorithms
		if constexpr (std::is_same_v<ExecutionPolicy, std::execution::parallel_policy>) {
			const auto query = ParseQuery(raw_query);

			//creating vector for matched words with max size as maximum possible matching words(plus_words)
			std::vector<std::string_view> matched_words(query.plus_words.size());

			//add all matched words to vector
			std::transform(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
				matched_words.begin(), [&, this, document_id](const std::string& word) {
					auto iter = word_to_document_freqs_.find(word);
					return (iter != word_to_document_freqs_.end() && iter->second.count(document_id))
						? std::string_view(iter->first) : ""sv;
				});

			//if any of minus words exists in the document we are looking for matches, then clear all matched words
			for (const std::string& word : query.minus_words) {
				if (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id)) {
					matched_words.clear();
					break;
				}
			}
			return { matched_words, documents_.at(document_id).status };
		};
	}

	void RemoveDocument(int);

	template <typename ExecutionPolicy>
	void RemoveDocument(ExecutionPolicy, int document_id) {
		if (!document_ids_.count(document_id)) {
			return; //if there is no document with recieved id, then do nothing
		};
		if constexpr (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
			RemoveDocument(document_id); //removing with non-parallel methods
			return;
		};
		if constexpr (std::is_same_v<ExecutionPolicy, std::execution::parallel_policy>) {
			document_ids_.erase(document_ids_.find(document_id));
			documents_.erase(documents_.find(document_id));

			//using mutex and lock_guard to do removing correctly step-by-step
			std::mutex map_mutex;
			const auto& words_to_erase = document_words_freqs_.find(document_id)->second;
			std::for_each(std::execution::par, words_to_erase.begin(), words_to_erase.end(),
				[this, &map_mutex, document_id](const auto& word_and_freq) {
					auto& id_freq_map = word_to_document_freqs_.find(word_and_freq.first)->second;
					auto iter_to_erase = id_freq_map.find(document_id);
					std::lock_guard guard(map_mutex);
					id_freq_map.erase(iter_to_erase);
				});

			document_words_freqs_.erase(document_words_freqs_.find(document_id));
		};
	}
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

	std::vector<std::string> SplitIntoWordsNoStop(const std::string_view&) const;

	static int ComputeAverageRating(const std::vector<int>&);

	struct QueryWord {
		std::string data;
		bool is_minus;
		bool is_stop;
	};

	struct Query {
		std::set<std::string> plus_words;
		std::set<std::string> minus_words;
	};

	QueryWord ParseQueryWord(const std::string&) const;

	Query ParseQuery(const std::string_view&) const;

	double ComputeWordInverseDocumentFreq(const std::string&) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
		std::map<int, double> document_to_relevance; //document id and relevance
		for (const std::string& word : query.plus_words) {
			if (word_to_document_freqs_.count(word)) {
				//computing freq for each word
				const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

				for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
					const auto& document_data = documents_.at(document_id);

					//filter unnecessary docs using predicate
					if (document_predicate(document_id, document_data.status, document_data.rating)) {
						document_to_relevance[document_id] += term_freq * inverse_document_freq;
					};
				};
			};
		};

		//erase all documents which contain minus words from result
		for (const std::string& word : query.minus_words) {
			if (word_to_document_freqs_.count(word)) {
				for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
					document_to_relevance.erase(document_id);
				};
			};
		};

		//creating vector of Document data for all valid results
		std::vector<Document> matched_documents;
		for (const auto& [document_id, relevance] : document_to_relevance) {
			matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
		}
		return matched_documents;
	}

	template <typename ExecutionPolicy, typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const ExecutionPolicy& policy,
		const Query& query, DocumentPredicate document_predicate) const {
		if constexpr (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
			return FindAllDocuments(query, document_predicate); //non-parallel method
		};

		//creating ConcurrentMap, it guaranties correct adding and removing elements while multithreading 
		ConcurrentMap<int, double> conc_map(4);
		{
			std::vector<std::future<void>> operations;
			//creating kernel to process word
			auto kernel = [this, &conc_map, document_predicate](const std::string& word) {
				if (word_to_document_freqs_.count(word)) {
					const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
					for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
						const auto& document_data = documents_.at(document_id);
						if (document_predicate(document_id, document_data.status, document_data.rating)) {
							conc_map[document_id] += term_freq * inverse_document_freq;
						};
					};
				};
			};

			//starting kernels
			for (const std::string& word : query.plus_words) {
				operations.push_back(std::async(kernel, word));
			};
		} //when we will exit this namespace, all results of kernels would be gotten automatically

		//erase all documents which contain minus words from result
		std::for_each(policy, query.minus_words.begin(), query.minus_words.end(),
			[this, &conc_map, &policy](const std::string& word) {
				if (word_to_document_freqs_.count(word)) {
					auto& words_freq_map = word_to_document_freqs_.at(word);
					std::for_each(policy, words_freq_map.begin(), words_freq_map.end(),
						[&conc_map](const std::pair<const int, double>& pair_obj) {
							conc_map.erase(pair_obj.first);
						});
				};
			});
		//creating map using method of ConcurrentMap
		std::map<int, double> document_to_relevance = conc_map.BuildOrdinaryMap();
		std::mutex matched_documents_mutex;
		std::vector<Document> matched_documents;
		std::for_each(policy, document_to_relevance.begin(), document_to_relevance.end(),
			[this, &matched_documents, &matched_documents_mutex](const std::pair<const int, double>& pair_obj) {
				Document* ref{};
				{
					//locking only for creating place for element to get maximum perfomance
					std::lock_guard guard(matched_documents_mutex);
					ref = &matched_documents.emplace_back();
				}
				*ref = { pair_obj.first, pair_obj.second, documents_.at(pair_obj.first).rating };
			});
		return matched_documents;
	}
};

void PrintMatchDocumentResult(int, const std::vector<std::string>&, DocumentStatus);

void AddDocument(SearchServer&, int, const std::string&, DocumentStatus, const std::vector<int>&);

void FindTopDocuments(const SearchServer&, const std::string&);

void MatchDocuments(const SearchServer&, const std::string&);
