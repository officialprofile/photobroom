
/* GUI interface */

#ifndef GUI_HPP
#define GUI_HPP

#include <memory>

#include "gui_export.h"

struct ILoggerFactory;
struct ILogger;
struct ITaskExecutor;
struct IPluginLoader;
struct IProjectManager;
struct IConfiguration;

struct GUI_EXPORT Gui
{
    Gui();
    ~Gui();
    Gui(const Gui &) = delete;
    Gui& operator=(const Gui &) = delete;

    void set(IProjectManager *);
    void set(IPluginLoader *);
    void set(ITaskExecutor *);
    void set(IConfiguration *);
    void set(ILoggerFactory *);
    void run(int argc, char **argv);

    private:
        IProjectManager* m_prjManager;
        IPluginLoader* m_pluginLoader;
        ITaskExecutor* m_taskExecutor;
        IConfiguration* m_configuration;
        std::unique_ptr<ILogger> m_logger;
};

#endif

