#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string_view& text_view) {
	std::vector<std::string> words{};
	std::string word{};
	for (const char c : text_view) {
		if (c == ' ') {
			if (!word.empty()) {
				words.push_back(word);
				word.clear();
			}
		}
		else {
			word += c;
		}
	}
	if (!word.empty()) {
		words.push_back(word);
	}
	return words;
}

std::vector<std::string> SplitIntoWords(const std::string& text) {
	return SplitIntoWords(std::string_view(text));
}