/*
    Factory for database
    Copyright (C) 2013  Michał Walenciak <MichalWalenciak@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "databasebuilder.hpp"

#include <assert.h>

#include <memory>
#include <fstream>

#include "configuration/configurationfactory.hpp"
#include "configuration/iconfiguration.hpp"
#include "configuration/entrydata.hpp"

#include "memorydatabase.hpp"
#include "ifs.hpp"
#include "backend_loader.hpp"

//TODO: cleanup this file!

namespace Database
{

    const char* databaseLocation = "Database::Backend::DataLocation";

    namespace
    {

        struct StreamFactory: public IStreamFactory
        {
            virtual ~StreamFactory()
            {

            }

            virtual std::shared_ptr<std::iostream> openStream(const std::string &filename,
                                                              std::ios_base::openmode mode) override
            {
                auto stream = std::make_shared<std::fstream>();

                stream->open(filename.c_str(), mode);

                return stream;
            }
        };

        //object which initializes configuration with db entries
        struct ConfigurationInitializer: public Configuration::IInitializer
        {
            ConfigurationInitializer()
            {
                std::shared_ptr< ::IConfiguration > config = ConfigurationFactory::get();
                config->registerInitializer(this);
            }

            virtual std::string getXml()
            {
                std::shared_ptr< ::IConfiguration > config = ConfigurationFactory::get();
                boost::optional<Configuration::EntryData> entry = config->findEntry(Configuration::configLocation);

                assert(entry.is_initialized());

                const std::string configPath = entry->value();
                const std::string dbPath = configPath + "/database";

                const std::string configuration_xml =
                    "<configuration>                                        "
                    "   <keys>                                              "
                    "   </keys>                                             "
                    "                                                       "
                    "   <defaults>                                          "
                    "       <key name='Database::Backend::DataLocation' value='" + dbPath + "' />"
                    "   </defaults>                                         "
                    "</configuration>                                       ";

                return configuration_xml;
            }
        } initializer;
    }

    struct Builder::Impl
    {
        std::unique_ptr<IFrontend> defaultDatabase;
        std::shared_ptr<IBackend> defaultBackend;
        BackendBuilder backendBuilder;
    };


    Builder::Builder(): m_impl(new Impl)
    {

    }


    Builder::~Builder()
    {

    }


    Builder* Builder::instance()
    {
        static Builder builder;
        return &builder;
    }



    IFrontend* Builder::get()
    {
        if (m_impl->defaultDatabase.get() == nullptr)
        {
            std::shared_ptr<IStreamFactory> fs = std::make_shared<StreamFactory>();
            std::unique_ptr<IFrontend> frontend(new MemoryDatabase(fs));
            std::shared_ptr<Database::IBackend> backend = getBackend();

            if (backend.get() != nullptr)
            {
                m_impl->defaultDatabase = std::move(frontend);
                m_impl->defaultDatabase->setBackend(backend);
            }
            //else TODO: emit signal to signalize there are some problems at initialization
        }

        return m_impl->defaultDatabase.get();
    }


    std::shared_ptr<IBackend> Builder::getBackend(Type type)
    {
        if (m_impl->defaultBackend.get() == nullptr)
        {
            m_impl->defaultBackend = m_impl->backendBuilder.get();
            const bool status = m_impl->defaultBackend->init();

            if (!status)
                m_impl->defaultBackend.reset();
        }

        return m_impl->defaultBackend;
    }


}
