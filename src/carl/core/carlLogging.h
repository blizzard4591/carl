/**
 * @file carlLogging.h
 * @author Gereon Kremer <gereon.kremer@cs.rwth-aachen.de>
 */

#pragma once

#include <cassert>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string.h>
#include <utility>

#include "../util/Singleton.h"
#include "../io/streamingOperators.h"
#include "carlLoggingHelper.h"

namespace carl {

/**
 * Contains a custom logging facility.
 * 
 * This logging facility is fairly generic and is used as a simple and header-only alternative to more advanced solutions like log4cplus or boost::log.
 * 
 * The basic components are Sinks, Channels, Filters, RecordInfos, Formatters and the central Logger component.
 * 
 * A Sink represents a logging output like a terminal or a log file.
 * This implementation provides a FileSink and a StreamSink, but the basic Sink class can be extended as necessary.
 * 
 * A Channel is a string that identifies the context of the log message, usually something like the class name where the log message is emitted.
 * Channels are organized hierarchically where the levels are separated by dots. For example, `carl` is considered the parent of `carl.core`.
 * 
 * A Filter is associated with a Sink and makes sure that only a subset of all log messages is forwarded to the Sink.
 * Filter rules are pairs of a Channel and a minimum LogLevel, meaning that messages of this Channel and at least the given LogLevel are forwarded.
 * If a Filter does not contain any rule for some Channel, the parent Channel is considered. Each Filter contains a rule for the empty Channel, initialized with LVL_DEFAULT.
 * 
 * A RecordInfo stores auxiliary information of a log message like the filename, line number and function name where the log message was emitted.
 * 
 * A Formatter is associated with a Sink and produces the actual string that is sent to the Sink.
 * Usually, it adds auxiliary information like the current time, LogLevel, Channel and information from a RecordInfo to the string logged by the user.
 * The Formatter implements a reasonable default behaviour for log files, but it can be subclassed and modified as necessary.
 * 
 * The Logger class finally plugs all these components together.
 * It allows to configure multiple Sink objects which are identified by strings called `id` and offers a central `log()` method.
 * 
 * Initial configuration may look like this:
 * @code{.cpp}
 * carl::logging::logger().configure("logfile", "carl.log");
 * carl::logging::logger().filter("logfile")
 *     ("carl", carl::logging::LogLevel::LVL_INFO)
 *     ("carl.core", carl::logging::LogLevel::LVL_DEBUG)
 * ;
 * carl::logging::logger().resetFormatter();
 * @endcode
 * 
 * Macro facilitate the usage:
 * <ul>
 * <li>`CARLLOG_<LVL>(channel, msg)` produces a normal log message where channel should be string identifying the channel and msg is the message to be logged.</li>
 * <li>`CARLLOG_FUNC(channel, args)` produces a log message tailored for function calls. args should represent the function arguments.</li>
 * <li>`CARLLOG_ASSERT(channel, condition, msg)` checks the condition and if it fails calls `CARLLOG_FATAL(channel, msg)` and asserts the condition.</li>
 * </ul>
 * Any message (`msg` or `args`) can be an arbitrary expression that one would stream to an `std::ostream` like `stream << (msg);`. No final newline is needed.
 */
namespace logging {

/**
 * Indicated which log messages should be forwarded to some sink.
 * 
 * All messages which have a level that is equal or greater than the specified value will be forwarded.
 */
enum class LogLevel : unsigned {
	/// All log messages.
	LVL_ALL,
	/// Log messages used for tracing the program flow in detail.
	LVL_TRACE,
	/// Log messages used for debugging.
	LVL_DEBUG,
	/// Log messages used for information.
	LVL_INFO,
	/// Log messages used to warn about an undesired state.
	LVL_WARN,
	/// Log messages used for errors that can be handled.
	LVL_ERROR,
	/// Log messages used for errors that lead to program termination.
	LVL_FATAL,
	/// Log no messages.
	LVL_OFF,
	/// Default log level.
	LVL_DEFAULT = LVL_WARN
};

/**
 * Streaming operator for LogLevel.
 * @param os Output stream.
 * @param level LogLevel.
 * @return os.
 */
inline std::ostream& operator<<(std::ostream& os, LogLevel level) {
	switch (level) {
		case LogLevel::LVL_ALL:		return os << "ALL  ";
		case LogLevel::LVL_TRACE:	return os << "TRACE";
		case LogLevel::LVL_DEBUG:	return os << "DEBUG";
		case LogLevel::LVL_INFO:	return os << "INFO ";
		case LogLevel::LVL_WARN:	return os << "WARN ";
		case LogLevel::LVL_ERROR:	return os << "ERROR";
		case LogLevel::LVL_FATAL:	return os << "FATAL";
		case LogLevel::LVL_OFF:		return os << "OFF  ";
		default:					return os << "???  ";
	}
}

/**
 * This classes provides an easy way to obtain the current number of milliseconds that the program has been running.
 */
struct Timer {
	/// The clock type used jere.
	typedef std::chrono::high_resolution_clock clock;
	/// The duration type used here.
	typedef std::chrono::duration<unsigned,std::milli> duration;
	/// Start of this timer.
	clock::time_point start;
	Timer(): start(clock::now()) {}
	/**
	 * Calculated the number of milliseconds since this object has been created.
     * @return Milliseconds passed.
     */
	unsigned passed() const {
		clock::duration d(clock::now() - start);
		return std::chrono::duration_cast<duration>(d).count();
	}
	/**
	 * Streaming operator for a Timer.
	 * Prints the result of `t.passed()`.
	 * @param os Output stream.
	 * @param t Timer.
	 * @return os.
	 */
	friend std::ostream& operator<<(std::ostream& os, const Timer& t) {
		return os << t.passed();
	}
};

/**
 * Base class for a logging sink. It only provides an interface to access some std::ostream.
 */
struct Sink {
	/**
	 * Default destructor.
     */
	virtual ~Sink() {}
	/**
	 * Abstract logging interface.
	 * The intended usage is to write any log output to the output stream returned by this function.
     * @return Output stream.
     */
	virtual std::ostream& log() = 0;
};
/**
 * Logging sink that wraps an arbitrary `std::ostream`.
 * It is meant to be used for streams like `std::cout` or `std::cerr`.
 */
struct StreamSink: public Sink {
	/// Output stream.
	std::ostream os;
	/**
	 * Create a StreamSink from some output stream.
     * @param os Output stream.
     */
	StreamSink(std::ostream& os): os(os.rdbuf()) {}
	virtual ~StreamSink() {}
	virtual std::ostream& log() { return os; }
};
/**
 * Logging sink for file output.
 */
struct FileSink: public Sink {
	/// File output stream.
	std::ofstream os;
	/**
	 * Create a FileSink that logs to the specified file.
	 * The file is truncated upon construction.
     * @param filename
     */
	FileSink(const std::string& filename): os(filename, std::ios::out) {}
	virtual ~FileSink() { os.close(); }
	virtual std::ostream& log() { return os; }
};

/**
 * This class checks if some log message shall be forwarded to some sink.
 */
struct Filter {
	/// Mapping from channels to (minimal) log levels.
	std::map<std::string, LogLevel> data;
	
	/**
	 * Constructor.
	 * @param level Default minimal log level.
	 */
	Filter(LogLevel level = LogLevel::LVL_DEFAULT) {
		(*this)("", level);
	}
	/**
	 * Set the minimum log level for some channel.
	 * Returns `*this`, hence calls to this method can be chained arbitrarily.
     * @param channel Channel name.
     * @param level LogLevel.
	 * @return This object.
     */
	Filter& operator()(const std::string& channel, LogLevel level) {
		data[channel] = level;
		return *this;
	}
	/**
	 * Checks if the given log level is sufficient for the log message to be forwarded.
     * @param channel Channel name.
     * @param level LogLevel.
     * @return If the message shall be forwarded.
     */
	bool check(const std::string& channel, LogLevel level) {
		std::string curChan = channel;
		auto it = data.find(curChan);
		while (curChan.size() > 0 && it == data.end()) {
			auto n = curChan.rfind('.');
			curChan = (n == std::string::npos) ? "" : curChan.substr(0, n);
			it = data.find(curChan);
		}
		assert(it != data.end());
		return level >= it->second;
	}
	/**
	 * Streaming operator for a Filter.
	 * All the rules stored in the filter are printed in a human-readable fashion.
	 * @param os Output stream.
	 * @param f Filter.
	 * @return os.
	 */
	friend std::ostream& operator<<(std::ostream& os, const Filter& f) {
		os << "Filter:" << std::endl;
		for (auto it: f.data) os << "\t\"" << it.first << "\" -> " << it.second << std::endl;
		return os;
	}
};

/**
 * Additional information about a log message.
 */
struct RecordInfo {
	/// File name.
	std::string filename;
	/// Function name.
	std::string func;
	/// Line number.
	unsigned line;
	/**
	 * Constructor.
     * @param filename File name.
     * @param func Function name.
     * @param line Line number.
     */
	RecordInfo(const std::string& filename, const std::string& func, unsigned line): 
		filename(filename), func(func), line(line) {}
};

/**
 * Formats a log messages.
 */
struct Formatter {
	/// Width of the longest channel.
	std::size_t channelwidth = 10;
	/**
	 * Extracts the maximum width of a channel to optimize the formatting.
     * @param f Filter.
     */
	virtual void configure(const Filter& f) {
		for (auto t: f.data) {
			if (t.first.size() > channelwidth) channelwidth = t.first.size();
		}
	}
	/**
	 * Prints the prefix of a log message, i.e. everything that goes before the message given by the user, to the output stream.
     * @param os Output stream.
     * @param timer Timer holding program execution time.
     * @param channel Channel name.
     * @param level LogLevel.
     * @param info Auxiliary information.
     */
	virtual void prefix(std::ostream& os, const Timer& timer, const std::string& channel, LogLevel level, const RecordInfo& info) {
		os.fill(' ');
		os << "[" << std::right << std::setw(4) << timer << "] " << level << " ";
		std::string filename(carl::basename(info.filename));
		unsigned long spacing = 1;
		if (channelwidth + 15 > channel.size() + filename.size()) spacing = channelwidth + 15 - channel.size() - filename.size();
		os << channel << std::string(spacing, ' ') << filename << ":" << std::left << std::setw(4) << info.line << " ";
		if (!info.func.empty()) os << info.func << "(): ";
	}
	/**
	 * Prints the suffix of a log message, i.e. everything that goes after the message given by the user, to the output stream.
	 * Usually, this is only a newline.
     * @param os Output stream.
     */
	virtual void suffix(std::ostream& os) {
		os << std::endl;
	}
};

/**
 * Main logger class.
 */
class Logger: public carl::Singleton<Logger> {
	friend carl::Singleton<Logger>;
	/// Mapping from channels to associated logging classes.
	std::map<std::string, std::tuple<std::shared_ptr<Sink>, Filter, std::shared_ptr<Formatter>>> data;
	/// Logging mutex to ensure thread-safe logging.
	std::mutex mutex;
	/// Timer to track program runtime.
	Timer timer;

	/**
	 * Default constructor.
     */
	Logger() {}
public:
	/**
	 * Desctructor.
     */
	~Logger() {
		data.clear();
	}
	/**
	 * Check if a Sink with the given id has been installed.
     * @param id Sink identifier.
     * @return If a Sink with this id is present.
     */
	bool has(const std::string& id) const {
		return data.find(id) != data.end();
	}
	/**
	 * Installs the given sink.
	 * If a Sink with this name is already present, it is overwritten.
     * @param id Sink identifier.
     * @param sink Sink.
     */
	void configure(const std::string& id, std::shared_ptr<Sink> sink) {
		this->data[id] = std::make_tuple(sink, Filter(), std::make_shared<Formatter>());
	}
	/**
	 * Installs a FileSink.
     * @param id Sink identifier.
     * @param filename Filename passed to the FileSink.
     */
	void configure(const std::string& id, const std::string& filename) {
		configure(id, std::make_shared<FileSink>(filename));
	}
	/**
	 * Installs a StreamSink.
     * @param id Sink identifier.
     * @param os Output stream passed to the StreamSink.
     */
	void configure(const std::string& id, std::ostream& os) {
		configure(id, std::make_shared<StreamSink>(os));
	}
	/**
	 * Retrieves the Filter for some Sink.
     * @param id Sink identifier.
     * @return Filter.
     */
	Filter& filter(const std::string& id) {
		auto it = data.find(id);
		assert(it != data.end());
		return std::get<1>(it->second);
	}
	/**
	 * Retrieves the Formatter for some Sink.
     * @param id Sink identifier.
     * @return Formatter.
     */
	std::shared_ptr<Formatter> formatter(const std::string& id) {
		auto it = data.find(id);
		assert(it != data.end());
		return std::get<2>(it->second);
	}
	/**
	 * Overwrites the Formatter for some Sink.
     * @param id Sink identifier.
     * @param fmt New Formatter.
     */
	void formatter(const std::string& id, std::shared_ptr<Formatter> fmt) {
		auto it = data.find(id);
		assert(it != data.end());
		std::get<2>(it->second) = fmt;
		std::get<2>(it->second)->configure(std::get<1>(it->second));
	}
	/**
	 * Reconfigures all Formatter objects.
	 * This should be done once after all configuration is finished.
     */
	void resetFormatter() {
		for (auto& t: data) {
			std::get<2>(t.second)->configure(std::get<1>(t.second));
		}
	}
	/**
	 * Logs a message.
     * @param level LogLevel.
     * @param channel Channel name.
     * @param ss Message to be logged.
     * @param info Auxiliary information.
     */
	void log(LogLevel level, const std::string& channel, const std::stringstream& ss, const RecordInfo& info) {
		std::lock_guard<std::mutex> lock(mutex);
		for (auto t: data) {
			if (!std::get<1>(t.second).check(channel, level)) continue;
			std::get<2>(t.second)->prefix(std::get<0>(t.second)->log(), timer, channel, level, info);
			std::get<0>(t.second)->log() << ss.str();
			std::get<2>(t.second)->suffix(std::get<0>(t.second)->log());
		}
	}
};

/**
 * Returns the single global instance of a Logger.
 * 
 * Calls `Logger::getInstance()`.
 * @return Logger object.
 */
inline Logger& logger() {
	return Logger::getInstance();
}

/// Create a record info.
#define __CARLLOG_RECORD carl::logging::RecordInfo(__FILE__, __func__, __LINE__)
/// Create a record info without function name.
#define __CARLLOG_RECORD_NOFUNC carl::logging::RecordInfo(__FILE__, "", __LINE__)
/// Basic logging macro.
#define __CARLLOG(level, channel, expr) { std::stringstream ss; ss << expr; carl::logging::Logger::getInstance().log(level, channel, ss, __CARLLOG_RECORD); }
/// Basic logging macro without function name.
#define __CARLLOG_NOFUNC(level, channel, expr) { std::stringstream ss; ss << expr; carl::logging::Logger::getInstance().log(level, channel, ss, __CARLLOG_RECORD_NOFUNC); }

/// Intended to be called when entering a function. Format: `<function name>(<args>)`.
#define CARLLOG_FUNC(channel, args) __CARLLOG_NOFUNC(carl::logging::LogLevel::LVL_TRACE, channel, __func__ << "(" << args << ")");

/// Log with level LVL_TRACE.
#define CARLLOG_TRACE(channel, expr) __CARLLOG(carl::logging::LogLevel::LVL_TRACE, channel, expr)
/// Log with level LVL_DEBUG.
#define CARLLOG_DEBUG(channel, expr) __CARLLOG(carl::logging::LogLevel::LVL_DEBUG, channel, expr)
/// Log with level LVL_INFO.
#define CARLLOG_INFO(channel, expr) __CARLLOG(carl::logging::LogLevel::LVL_INFO, channel, expr)
/// Log with level LVL_WARN.
#define CARLLOG_WARN(channel, expr) __CARLLOG(carl::logging::LogLevel::LVL_WARN, channel, expr)
/// Log with level LVL_ERROR.
#define CARLLOG_ERROR(channel, expr) __CARLLOG(carl::logging::LogLevel::LVL_ERROR, channel, expr)
/// Log with level LVL_FATAL.
#define CARLLOG_FATAL(channel, expr) __CARLLOG(carl::logging::LogLevel::LVL_FATAL, channel, expr)

/// Log and assert the given condition, if the condition evaluates to false.
#define CARLLOG_ASSERT(channel, condition, expr) if (!condition) { CARLLOG_FATAL(channel, expr); assert(condition); }

}
}