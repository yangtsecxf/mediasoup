#pragma once
#include <string>
#include <functional>
#include <sstream>
#include <map>
#include <iomanip>
#include <chrono>
#include <mutex>
#include <thread>
#include <cstring>
 

enum TLogLevel
{
	kLevelVerbose = 0,
	kLevelDebug,    // Detailed information on the flow through the system.
	kLevelInfo,     // Interesting runtime events (startup/shutdown), should be conservative and keep to a minimum.
	kLevelWarn,     // Other runtime situations that are undesirable or unexpected, but not necessarily "wrong".
	kLevelError,    // Other runtime errors or unexpected conditions.
	kLevelFatal,    // Severe errors that cause premature termination.
	kLevelNone,     // Special level used to disable all log messages.
};

typedef std::function<void(const TLogLevel log_level, const std::string& log)> typedef_log_cb;

#define LOG Log::ins()
class Log
{
private:
	typedef_log_cb log_cb_;
	TLogLevel log_level_;
	std::mutex mtx_;

public:
	Log(const TLogLevel log_level = kLevelInfo)
		:log_cb_(nullptr)
		, log_level_(log_level)
	{

	}
	~Log(){}

	static Log* ins()
	{
		static Log s_o;
		return &s_o;
	}

	void set_log_cb(typedef_log_cb log_cb) 
	{
		std::lock_guard<std::mutex> lock(mtx_);
		log_cb_ = log_cb;
	}
	typedef_log_cb get_log_cb() 
	{ 
		std::lock_guard<std::mutex> lock(mtx_);
		return log_cb_; 
	}

	void set_log_level(const TLogLevel log_level) { log_level_ = log_level; }
	TLogLevel get_log_level() { return log_level_; }
	
};

inline std::string get_nowtime_ms()
{
	std::stringstream ss;

	auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	ss << std::put_time(std::localtime(&t), "%X");  // HH:MM:SS

	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
	ss << '.' << std::setfill('0') << std::setw(3) << ms.count();  // .MMM

	return ss.str();
}

// template<size_t I = 0, typename FuncT, typename ...Tp>
// inline typename std::enable_if_t<I == sizeof ...(Tp)> for_each(std::tuple<Tp ...>&, FuncT)
// {
// }

// template<size_t I = 0, typename FuncT, typename ...Tp>
// inline typename std::enable_if_t < I < sizeof ...(Tp)> for_each(std::tuple<Tp ...>& t, FuncT f)
// {
// 	f(std::get<I>(t));
// 	for_each<I + 1, FuncT, Tp...>(t, f);
// }

template<typename T>
inline std::stringstream& print(std::stringstream& os, const T& t)
{
	os << t << " ";
	return os;
}

template<typename T,  typename... Args>
inline std::stringstream& print(std::stringstream& os, const T& t, const Args&... rest)
{
	os << t << " ";
	return print(os, rest...);
}

template<typename...Args>
inline void log_ex(TLogLevel level, const char* file, const int line, const char* function, const Args&... rest)
{
	if (level < LOG->get_log_level() || level > kLevelFatal 
		|| nullptr == file || nullptr == function)
	{
		return;
	}

	const char* end1 = ::strrchr(file, '/');
	const char* end2 = ::strrchr(file, '\\');
	if (!end1 || !end2)
	{
		file = (end1 > end2) ? end1 + 1 : end2 + 1;
	}

	std::stringstream ss;
	std::map<TLogLevel, std::string> map_level{ {kLevelDebug, "[Debug] "} , {kLevelInfo, "[INFO ] "},
		{kLevelWarn, "[WARN ] "}, {kLevelError, "[ERROR] "}, {kLevelFatal, "[FATAL] "} };

	ss << "[" << get_nowtime_ms() << "] " << map_level[level] << "[" << file << " " << function << " " << line << "] - ";

	print(ss, rest...);

	ss << "[tid:" << std::this_thread::get_id() << "]";

	if (LOG->get_log_cb())
	{
		LOG->get_log_cb()(level, ss.str());
	}
}

#define DEBUG(...) log_ex(kLevelDebug, __FILE__, __LINE__, __FUNCTION__,  __VA_ARGS__)
#define INFO(...) log_ex(kLevelInfo, __FILE__, __LINE__, __FUNCTION__,  __VA_ARGS__)
#define WARN(...) log_ex(kLevelWarn, __FILE__, __LINE__, __FUNCTION__,  __VA_ARGS__)
#define ERR(...) log_ex(kLevelError, __FILE__, __LINE__, __FUNCTION__,  __VA_ARGS__)
#define FATAL(...) log_ex(kLevelFatal, __FILE__, __LINE__, __FUNCTION__,  __VA_ARGS__)

// ex:
// 		DEBUG("str1", 2, "str3", 4.0, ...);
// 		INFO("str1", 2, "str3", 4.0, ...);
// 		WARN("str1", 2, "str3", 4.0, ...);
// 		ERR("str1", 2, "str3", 4.0, ...);
// 		FATAL("str1", 2, "str3", 4.0, ...);

//////////////////////////////////////////////////////////////////////////
#define CHECK_VOID(x, log)    \
        if(!(x))\
        {\
			WARN(log);\
            return;\
        }

#define CHECK_RT(x, rt, log)    \
        if(!(x))\
        {\
			WARN(log);\
            return rt;\
        }

#define CHECK_CONTINUE(x, log)    \
        if(!(x))\
        {\
			ERR(log);\
            continue;\
        }

#define CHECK_BREAK(x, log)    \
        if(!(x))\
        {\
			ERR(log);\
            break;\
        }

# define VALID_VOID(x, log) \
        if(!(x))\
        {\
			ERR(log);\
			return;\
        }

# define VALID_RT(x, rt, log) \
        if(!(x))\
        {\
			ERR(log);\
			return (rt);\
        }

# define VALID_CONTINUE(x, log) \
        if(!(x))\
        {\
			ERR(log);\
			continue;\
        }

# define VALID_BREAK(x, log) \
        if(!(x))\
        {\
			ERR(log);\
			break;\
        }

# define ASSERT_NELO(x, log) \
        if(!(x))\
        {\
			ERR(log);\
        }