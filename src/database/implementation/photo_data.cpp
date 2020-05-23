/*
 * Photo Broom - photos management tool.
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

#include <cassert>

#include "photo_data.hpp"

namespace Photo
{

    int Data::getFlag(const Photo::FlagsE& flag) const
    {
        auto it = flags.find(flag);

        const int result = it == flags.end()? 0: it->second;

        return result;
    }


    Data& Data::apply(const DataDelta& delta)
    {
        id = delta.getId();

        if (delta.has(Photo::Field::Tags))
            tags = delta.get<Photo::Field::Tags>();

        if (delta.has(Photo::Field::Geometry))
            geometry = delta.get<Photo::Field::Geometry>();

        if (delta.has(Photo::Field::Checksum))
            sha256Sum = delta.get<Photo::Field::Checksum>();

        if (delta.has(Photo::Field::Flags))
            flags = delta.get<Photo::Field::Flags>();

        if (delta.has(Photo::Field::GroupInfo))
            groupInfo = delta.get<Photo::Field::GroupInfo>();

        return *this;
    }


    void DataDelta::setId(const Photo::Id& id)
    {
        assert(m_id.valid() == false);      // do we expect id to be set more than once?
        m_id = id;
    }


    void DataDelta::clear()
    {
        m_data.clear();
        m_id = Photo::Id();
    }


    bool DataDelta::has(Photo::Field field) const
    {
        auto it = m_data.find(field);

        return it != m_data.end();
    }


    const Id & DataDelta::getId() const
    {
        return m_id;
    }


    bool DataDelta::operator<(const DataDelta& other) const
    {
        return m_id < other.m_id;
    }


    bool DataDelta::operator==(const DataDelta& other) const
    {
        return std::tie(m_id, m_data) == std::tie(other.m_id, other.m_data);
    }


    const DataDelta::Storage& DataDelta::get(Photo::Field field) const
    {
        assert(has(field));
        auto it = m_data.find(field);

        return it->second;
    }
}
