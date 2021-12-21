#include "log_duration.h"

LogDuration::LogDuration() = default;

LogDuration::LogDuration(const std::string& operation) : operation_(operation + ": "s){}

LogDuration::LogDuration(const std::string& operation, std::ostream& output) : operation_(operation + ": "s),
		output_(output){}

LogDuration::~LogDuration(){
const auto end_time = std::chrono::steady_clock::now();
output_ << operation_ << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() <<
		" ms"s << std::endl;
}
