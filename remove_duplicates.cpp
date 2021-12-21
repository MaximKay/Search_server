#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server){
	std::map<std::set<std::string_view>, int> documents_words_sets;
	std::set<int> ids_to_delete;
	for (const int id : search_server){
		auto words_for_document = search_server.GetWordFrequencies(id);
		std::set<std::string_view> words;
		for (const auto& [word, _] : words_for_document){
			words.insert(word);
		};
		if (!documents_words_sets.count(words)){
			documents_words_sets[words] = id;
		} else {
			ids_to_delete.insert(id);
		};
	};
	for (const auto id : ids_to_delete){
		search_server.RemoveDocument(id);
		std::cout << "Found duplicate document id "s << id << std::endl;
	};
}
