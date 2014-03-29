/*
 * SQLDatabase iterator
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

#include "photo_iterator.hpp"

#include <QVariant>

#include <core/tag.hpp>

#include "implementation/db_photo_info.hpp"

namespace Database
{

    struct PhotoIterator::Impl
    {
        std::shared_ptr<IQuery> m_query;
        IPhotoInfo::Ptr m_photoInfo;

        void calculatePhotoInfo()
        {
            if (m_photoInfo.get() == nullptr)
            {
                APhotoInfoInitData data;

                const unsigned int id = m_query->getField(IQuery::Fields::Id).toInt();
                data.path = m_query->getField(IQuery::Fields::Path).toString().toStdString();
                data.hash = m_query->getField(IQuery::Fields::Hash).toString().toStdString();

                IPhotoInfo* photoInfo = new DBPhotoInfo(m_query, data);
                m_photoInfo.reset(photoInfo);

                //do not modify original query, use clone
                std::shared_ptr<IQuery> query = m_query->clone();
                std::shared_ptr<ITagData> tags = m_photoInfo->getTags();

                bool status = true;
                do
                {
                    const QString tagName  = m_query->getField(IQuery::Fields::TagName).toString();
                    const QString tagValue = m_query->getField(IQuery::Fields::TagValue).toString();
                    const int     tagType  = m_query->getField(IQuery::Fields::TagType).toInt();

                    //append tag
                    tags->setTag(TagNameInfo(tagName, tagType), tagValue);

                    status = query->gotoNext();
                }
                while (status);
            }
        }

        void invalidate()
        {
            m_photoInfo.reset();
        }
    };


    PhotoIterator::PhotoIterator(const std::shared_ptr<IQuery>& query): m_impl(new Impl)
    {
        m_impl->m_query = query;

        //goto first item
        m_impl->m_query->gotoNext();
    }


    PhotoIterator::PhotoIterator(): m_impl(new Impl)
    {

    }


    PhotoIterator::~PhotoIterator()
    {

    }


    PhotoIterator::operator bool ()
    {
        const bool result = m_impl->m_query.get() != nullptr &&
                            m_impl->m_query->valid();
        return result;
    }


    bool PhotoIterator::operator!()
    {
        const bool result = m_impl->m_query.get() != nullptr &&
                            m_impl->m_query->valid();
        return !result;
    }


    PhotoIterator& PhotoIterator::operator++()
    {
        if ( *this )
        {
            //get id of current photo
            const unsigned int id = m_impl->m_query->getField(IQuery::Fields::Id).toInt();
            unsigned int n_id = 0;
            bool n = true;
            do
            {
                n = m_impl->m_query->gotoNext();

                if (n)
                    n_id = m_impl->m_query->getField(IQuery::Fields::Id).toInt();
            }
            while (n && id == n_id);   //next row as long as ids are equal
        }

        return *this;
    }


    PhotoIterator PhotoIterator::operator++(int)
    {
        PhotoIterator other = *this;

        ++(*this);
        return other;
    }


    IPhotoInfo* PhotoIterator::operator->()
    {
        m_impl->calculatePhotoInfo();
        return m_impl->m_photoInfo.get();
    }

}