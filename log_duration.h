#pragma once

#include<chrono>
#include <iostream>
#include <string>

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT (durationLogging, __LINE__)
#define LOG_DURATION(operation) LogDuration UNIQUE_VAR_NAME_PROFILE(operation)
#define LOG_DURATION_STREAM(operation, stream) LogDuration UNIQUE_VAR_NAME_PROFILE(operation, stream)

using namespace std::string_literals;

//simple class to get durationof anything
//name of operation can be set wih passing it to the constructor
class LogDuration {
public:
	LogDuration();

	LogDuration(const std::string&);

	LogDuration(const std::string&, std::ostream&);

	~LogDuration();

private:
	std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
	std::string operation_;
	std::ostream& output_ = std::cerr;
};
