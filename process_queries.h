#pragma once

#include "document.h"
#include "search_server.h"
#include <vector>
#include <algorithm>
#include <execution>
#include <functional>
#include <utility>

std::vector<std::vector<Document>> ProcessQueries( 
	const SearchServer&, const std::vector<std::string>&);

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server, const std::vector<std::string>& queries);