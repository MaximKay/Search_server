#pragma once
#include <string>

using namespace std::string_literals;

struct Document {
	Document();
	Document(int id, double relevance, int rating);
	int id{0};
	double relevance{0.0};
	int rating{0};
};

enum class DocumentStatus {
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};

template <typename Ostream>
Ostream& operator<<(Ostream& output, const Document& doc) {
	output << "{ document_id = "s << doc.id << ", relevance = "s <<
			doc.relevance << ", rating = "s << doc.rating << " }"s;
	return output;
}