/*
 * Configuration's private implementation
 * Copyright (C) 2015  Michał Walenciak <MichalWalenciak@gmail.com>
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

#include "configuration.hpp"
#include "private/configuration_p.hpp"

#include <fstream>

#include <QFile>
#include <QJsonDocument>
#include <QVariant>

#include <json/reader.h>
#include <json/writer.h>

#include <system/system.hpp>

#include "constants.hpp"


static_assert(sizeof(qlonglong) == sizeof(Json::Int64), "Json and Qt's sizes do not match");

ConfigurationPrivate::ConfigurationPrivate(Configuration* _q):
    m_json(),
    m_dumpTimer(),
    q(_q)
{
    m_dumpTimer.setSingleShot(true);
    m_dumpTimer.setInterval(500);

    connect(&m_dumpTimer, &QTimer::timeout, this, &ConfigurationPrivate::saveData);

    loadData();
}


ConfigurationPrivate::~ConfigurationPrivate()
{
    if (m_dumpTimer.isActive())
    {
        m_dumpTimer.stop();
        saveData();
    }
}


QVariant ConfigurationPrivate::getEntry(const QString& entry)
{
    QVariant v_result;

    solve(entry, [&](Json::Value& value)
    {
        if (value.isString())
            v_result = value.asCString();
        else if(value.isInt())
            v_result = value.asInt();
        else if(value.isInt64())
            v_result = value.asInt64();
        else if(value.isNull())
        {}
        else
            assert(!"not implemented");
    });

    return v_result;
}


void ConfigurationPrivate::setEntry(const QString& entry, const QVariant& entry_value)
{
    solve(entry, [&](Json::Value& value)
    {
        if (entry_value.type() == QVariant::String)
            value = entry_value.toString().toStdString();
        else if(entry_value.type() == QVariant::Int)
            value = entry_value.toInt();
        else if(entry_value.type() == QVariant::ByteArray)
            value = entry_value.toByteArray().data();
        else if(entry_value.type() == QVariant::LongLong)
            value = entry_value.toLongLong();
        else
            assert(!"unsupported type");
    });

    markDataDirty();
}


void ConfigurationPrivate::loadData()
{
    const QString path = System::getApplicationConfigDir();
    const QString configFile = path + "/" + "config.json";

    if (QFile::exists(configFile))
    {
        std::ifstream config(configFile.toStdString(), std::ifstream::binary);

        auto locked_config = m_json.lock();
        config >> *locked_config;
    }
    else
    {
        //load default data
        setEntry(ConfigConsts::BasicKeys::thumbnailWidth, 120);
    }
}


void ConfigurationPrivate::markDataDirty()
{
    m_dumpTimer.start();
}


void ConfigurationPrivate::saveData()
{
    const QString path = System::getApplicationConfigDir();
    const QString configFile = path + "/" + "config.json";

    std::ofstream config(configFile.toStdString(), std::ofstream::binary);

    auto locked_config = m_json.lock();
    config << *locked_config;
}


void ConfigurationPrivate::solve(const QString& entry, std::function<void(Json::Value &)> f)
{
    if (entry.isEmpty() == false)
    {
        auto config = m_json.lock();

        const QStringList levels = entry.split("::");

        Json::Value& obj = config.get();
        Json::Value* value = &obj;

        for(QStringList::const_iterator it = levels.begin(); it != levels.end(); ++it)
        {
            const std::string level_value = it->toStdString();
            Json::Value& value_ref = *value;
            Json::Value& sub_value = value_ref[level_value];

            value = &sub_value;
        }

        f(*value);
    }
}


////////////////////////////////////////////////////////////////////////////////


Configuration::Configuration():
    d(new ConfigurationPrivate(this))
{

}


Configuration::~Configuration()
{
    delete d;
}


QVariant Configuration::getEntry(const QString& entry)
{
    return d->getEntry(entry);
}


void Configuration::setEntry(const QString& entry, const QVariant& value)
{
    d->setEntry(entry, value);
}


void Configuration::setDefaultValue(const QString& entry, const QVariant& value)
{
    const QVariant curren_value = getEntry(entry);
    if (curren_value.isNull())
        setEntry(entry, value);
}
