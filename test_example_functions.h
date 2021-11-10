#pragma once

#include <string>
#include <iostream>

#define RUN_TEST(function) RunningTest(function);
#define ASSERT(bool_expression) Assert((bool_expression), (#bool_expression), __FILE__, __LINE__, __FUNCTION__, "");
#define ASSERT_HINT(bool_expression, hint) Assert((bool_expression), (#bool_expression), __FILE__, __LINE__, __FUNCTION__, (hint));
#define ASSERT_EQUAL(value1, value2) AssertEqual((value1), (value2), (#value1), (#value2), __FILE__, __LINE__, __FUNCTION__, "");
#define ASSERT_EQUAL_HINT(value1, value2, hint) AssertEqual((value1), (value2), (#value1), (#value2), __FILE__, __LINE__, __FUNCTION__, hint);

#include "search_server.h"
#include "document.h"

using namespace std::string_literals;

void Assert(const bool&, const std::string&, const std::string&, const int&,
		const std::string&, const std::string&);

template <typename A, typename B>
void AssertEqual(const A& first, const B& second, const std::string& first_str,
		const std::string& second_str, const std::string& file_name, const int& line,
		const std::string& func_name, const std::string& hint){
	if (first != second) {
		std::cerr << file_name << "("s << line << "): "s << func_name << ": "s
				<< "ASSERT("s << first_str << " == "s << second_str << ") failed."s;
		if (!hint.empty()) {
			std::cerr << " Hint: "s << hint;
		}
		std::cerr << std::endl;
		abort();
	};
}

template<typename Function>
void RunningTest (const Function& func){
	func();
}

void TestExcludeStopWordsFromAddedDocumentContent();
void TestMinusWords();
void RelevanceCalculatingAndSortingTests();
void AverageRatingTest();
void CustomFiltersTests();
void StatusFilterTest();
void RemoveDocumentTest();

void TestSearchServer();
