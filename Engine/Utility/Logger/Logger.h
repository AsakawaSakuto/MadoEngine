#pragma once
#include <string>
#include <source_location>

namespace Logger {

    enum class Level {
        Info,
        Warning,
        Error,
        Debug
    };

    void Output(
        const std::string& message,
        Level level = Level::Info,
        const std::source_location& location = std::source_location::current()
    );

    void Info(const std::string& message);
    void Warning(const std::string& message);
    void Error(const std::string& message);
    void Debug(const std::string& message);

}
