
#ifndef ILOG_HPP
#define ILOG_HPP

#include <string>
#include <vector>
 
struct ILogger
{
    virtual ~ILogger() {}

    enum class Severity
    {
        Info,
        Warning,
        Error,
        Debug,
    };

    virtual void log(Severity, const std::string& message) = 0;
};

#endif
