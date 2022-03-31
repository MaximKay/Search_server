#pragma once
#include <vector>
#include <set>
#include <string>
#include <string_view>

std::vector<std::string> SplitIntoWords(const std::string_view&);

std::vector<std::string> SplitIntoWords(const std::string&);

//returning set of string which creates from any container with .begin() and .end() methods
template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
	std::set<std::string> non_empty_strings;
	for (const std::string& str : strings) {
		if (!str.empty()) {
			non_empty_strings.insert(str);
		};
	};
	return non_empty_strings;
}
