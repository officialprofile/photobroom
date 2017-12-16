/*
 * Plugins loader class
 * Copyright (C) 2014  Michał Walenciak <MichalWalenciak@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "plugin_loader.hpp"

#include <deque>
#include <cassert>

#include <QDir>
#include <QPluginLoader>

#include <core/ilogger.hpp>
#include <core/ilogger_factory_accessor.hpp>
#include <core/down_cast.hpp>
#include <system/filesystem.hpp>
#include <database/idatabase_plugin.hpp>


namespace
{
    struct PluginFinder final
    {
        PluginFinder(): m_logger(nullptr), m_db_plugins(), m_found(false)
        {

        }

        PluginFinder(const PluginFinder &) = delete;

        ~PluginFinder()
        {
            for(auto plugin: m_db_plugins)
                delete plugin;
        }

        PluginFinder& operator=(const PluginFinder &) = delete;

        void set(ILoggerFactory* logger_factory)
        {
            m_logger = logger_factory->get("PluginLoader");
        }

        void find_all_db_plugins()
        {
            if (!m_found)
            {
                QDir pluginsDir = FileSystem().getPluginsPath();

                pluginsDir.cd("database");
                QFileInfoList db_plugins = pluginsDir.entryInfoList(QStringList(), QDir::Files);

                InfoStream(m_logger.get()) << "Searching for plugins in: " << pluginsDir.path();

                m_found = true;

                for(const QFileInfo& info: db_plugins)
                {
                    const QString path = info.absoluteFilePath();

                    InfoStream(m_logger.get()) << "Found database plugin: " << path;

                    QObject* raw_plugin = load(path);

                    Database::IPlugin* plugin = down_cast<Database::IPlugin *>(raw_plugin);

                    m_db_plugins.push_back(plugin);
                }
            }
        }

        Database::IPlugin* loadDBPlugin(const QString& name)
        {
            find_all_db_plugins();

            Database::IPlugin* result = nullptr;

            //TODO: load plugins basing on config etc
            for(Database::IPlugin* plugin: m_db_plugins)
            {
                if (plugin->backendName() == name)
                {
                    result = plugin;
                    break;
                }
            }

            return result;
        }

        const std::deque<Database::IPlugin *>& getDBPlugins()
        {
            find_all_db_plugins();

            return m_db_plugins;
        }

    private:
        QObject* load(const QString& path)
        {
            InfoStream(m_logger.get()) << "Loading database plugin: " << path;
            QPluginLoader loader(path);
            QObject* plugin = loader.instance();

            if (plugin == nullptr)
                ErrorStream(m_logger.get()) << "\tError: " << loader.errorString();

            return plugin;
        }

        std::unique_ptr<ILogger> m_logger;
        std::deque<Database::IPlugin *> m_db_plugins;
        bool m_found;
    };


}

struct PluginLoader::Impl
{
    Impl(): m_finder() {}

    PluginFinder m_finder;
};


PluginLoader::PluginLoader(): m_impl(new Impl)
{

}


PluginLoader::~PluginLoader()
{

}


void PluginLoader::set(ILoggerFactory* logger_factor)
{
    m_impl->m_finder.set(logger_factor);
}


Database::IPlugin* PluginLoader::getDBPlugin(const QString& name)
{
    Database::IPlugin* result = m_impl->m_finder.loadDBPlugin(name);

    return result;
}


const std::deque<Database::IPlugin *>& PluginLoader::getDBPlugins() const
{
    return m_impl->m_finder.getDBPlugins();
}

