/*
    Default Configuration class
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


#ifndef DEFAULT_CONFIGURATION_HPP
#define DEFAULT_CONFIGURATION_HPP

#include <memory>

#include "iconfiguration.hpp"
#include "configuration_export.h"

struct ILogger;
struct DefaultConfigurationPrivate;

class CONFIGURATION_EXPORT DefaultConfiguration: public IConfiguration
{
    public:
        DefaultConfiguration(ILogger *);
        virtual ~DefaultConfiguration();

        virtual Optional<Configuration::EntryData> findEntry(const Configuration::ConfigurationKey &) const override;
        const std::vector<Configuration::EntryData> getEntries() override;

        virtual void addEntry(const Configuration::EntryData &) override;
        virtual void registerDefaultEntries(const std::vector<Configuration::EntryData> &) override;
        virtual void registerKey(const Configuration::ConfigurationKey &) override;
        virtual void registerXml(const QString &) override;
        virtual bool load() override;

    private:
        std::unique_ptr<DefaultConfigurationPrivate> m_impl;

        virtual bool useXml(const QString &);
};

#endif // DEFAULT_CONFIGURATION_HPP
