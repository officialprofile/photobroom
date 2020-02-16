

#include <core/ilogger.hpp>

class EmptyLogger: public ILogger
{
    public:
        void log(Severity, const QString &) override {}

        void info(const QString &) override {}
        void warning(const QString &) override {}
        void error(const QString &) override {}
        void debug(const QString &) override {}

        std::unique_ptr<ILogger> subLogger(const QString &)
        {
            return std::make_unique<EmptyLogger>();
        }
};
