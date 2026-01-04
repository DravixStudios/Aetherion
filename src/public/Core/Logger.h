#pragma once
#include <iostream>
#include <string>
#include <fmt/core.h>

#if defined(LOGGING_USE_SPDLOG)
	#include <spdlog/spdlog.h>
#else 
	/*
		Color codes
		Credits: https://gist.github.com/Kielx/2917687bc30f567d45e15a4577772b02 
	*/
	#define RESET   "\033[0m"
	#define BLACK   "\033[30m"      /* Black */
	#define RED     "\033[31m"      /* Red */
	#define GREEN   "\033[32m"      /* Green */
	#define YELLOW  "\033[33m"      /* Yellow */
	#define BLUE    "\033[34m"      /* Blue */
	#define MAGENTA "\033[35m"      /* Magenta */
	#define CYAN    "\033[36m"      /* Cyan */
	#define WHITE   "\033[37m"      /* White */
	#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
	#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
	#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
	#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
	#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
	#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
	#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
	#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */
#endif

namespace Logger {
#if defined(LOGGING_USE_SPDLOG)
	template<typename... Args>
	inline void 
	Debug(const char* fmt, Args&&... args) {
		spdlog::debug(fmt::runtime(fmt), std::forward<Args>(args)...);
	}

	template<typename... Args>
	inline void
	Info(const char* fmt, Args&&... args) {
		spdlog::info(fmt::runtime(fmt), std::forward<Args>(args)...);
	}

	template<typename... Args>
	inline void Error(const char* fmt, Args&&... args) {
		spdlog::error(fmt::runtime(fmt), std::forward<Args>(args)...);
	}
#else
	template<typename... Args>
	inline void
		Debug(const char* fmt_str, Args&&... args) {
		std::cout << BLUE << "[Debug] " << WHITE
			<< fmt::vformat(fmt::runtime(fmt_str), fmt::make_format_args(std::forward<Args>(args)...))
			<< std::endl;
	}

	template<typename... Args>
	inline void
		Info(const char* fmt_str, Args&&... args) {
		std::cout << GREEN << "[Info] " << WHITE
			<< fmt::vformat(fmt::runtime(fmt_str), fmt::make_format_args(std::forward<Args>(args)...))
			<< std::endl;
	}

	template<typename... Args>
	inline void
		Error(const char* fmt_str, Args&&... args) {
		std::cout << RED << "[Error] " << WHITE
			<< fmt::vformat(fmt::runtime(fmt_str), fmt::make_format_args(std::forward<Args>(args)...))
			<< std::endl;
	}
#endif
}