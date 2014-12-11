#ifndef LOGGER_H
#define LOGGER_H

#include "log.h"

#ifdef LOG_FILE
static logging::logger< logging::file_log_policy > log_inst(LOG_FILE);
#else
std::string getNameByDate()
{
	std::ostringstream stringStream;

	time_t t = time(0);
	struct tm * currentTime = new tm;
	localtime_s(currentTime, &t);

	stringStream << currentTime->tm_year + 1900
		<< std::setfill('0') << std::setw(2) << currentTime->tm_mon + 1
		<< std::setfill('0') << std::setw(2) << currentTime->tm_mday
		<< ".log";

	delete currentTime;
	return stringStream.str();
}

static logging::logger< logging::file_log_policy > log_inst(getNameByDate());

#endif

#ifdef ENABLE_LOGGING
#define LOG log_inst.print< logging::severity_type::info >
#define LOG_DEBUG log_inst.print< logging::severity_type::debug >
#define LOG_ERR log_inst.print< logging::severity_type::error >
#define LOG_WARN log_inst.print< logging::severity_type::warning >
#else
#define LOG(...) 
#define LOG_DEBUG(...) 
#define LOG_ERR(...)
#define LOG_WARN(...)
#endif

#endif