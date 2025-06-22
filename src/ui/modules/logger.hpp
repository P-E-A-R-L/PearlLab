#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <chrono>
#include <imgui.h>
#include <map>
#include <string>
#include <vector>

namespace Logger
{
    enum Level
    {
        INFO = 0,
        WARNING = 1,
        ERROR = 2,
        MESSAGE = 3
    };

    typedef std::chrono::time_point<std::chrono::system_clock> time_t;

    struct Entry
    {
        std::string message;
        Level level;
        ImVec4 color;
        time_t time;
    };

    void init();
    void render();

    // some people may think it's a bad idea to make level & color disjointed,
    // but I don't think to begin with :)
    void log(const std::string &message, Level level, ImVec4 color, time_t time);
    void log(const std::string &message, Level level);

    void info(const std::string &message);
    void warning(const std::string &message);
    void error(const std::string &message);
    void message(const std::string &message);

    void clear();
    void setAutoScroll(bool autoScroll);
    std::vector<Level> &shownTypes();
}

#endif // LOGGER_HPP
