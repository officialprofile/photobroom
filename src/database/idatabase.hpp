/*
    interface for databases
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

#ifndef IDATABASE_HPP
#define IDATABASE_HPP

#include <vector>
#include <functional>
#include <memory>
#include <set>
#include <string>

#include <QObject>

#include <core/tag.hpp>

#include "action.hpp"
#include "database_status.hpp"
#include "filter.hpp"
#include "group.hpp"
#include "ibackend.hpp"
#include "iphoto_info.hpp"
#include "person_data.hpp"

#include "database_export.h"

struct ILogger;
struct IConfiguration;

namespace Database
{

    struct IPhotoInfoCache;
    struct IPhotoInfoCreator;
    struct IDatabaseClient;
    struct ProjectInfo;

    // for direct operating on backend (IDatabase::performCustomAction)
    struct IBackendOperator: IBackend
    {
        virtual ~IBackendOperator() = default;

        virtual IPhotoInfo::Ptr getPhotoFor(const Photo::Id &) = 0;                                // get IPhotoInfo for given id
        virtual std::vector<Photo::Id> insertPhotos(const std::vector<Photo::DataDelta> &) = 0;    // store photo
    };


    //Database interface.
    //A bridge between clients and backend.
    // TODO: divide into smaller interfaces and use repository pattern (see github issue #272)
    struct DATABASE_EXPORT IDatabase: public QObject
    {
        template <typename... Args>
        using Callback = std::function<void(Args...)>;

        virtual ~IDatabase() = default;

        // store data
        virtual void update(const Photo::DataDelta &) = 0;
        virtual void store(const std::vector<Photo::DataDelta> &,              // only path, flags and tags will be used to feed database
                           const Callback<const std::vector<Photo::Id> &>& = Callback<const std::vector<Photo::Id> &>()) = 0;

        virtual void createGroup(const Photo::Id &, GroupInfo::Type, const Callback<Group::Id> &) = 0;

        // read data
        virtual void countPhotos(const std::vector<IFilter::Ptr> &, const Callback<int> &) = 0;                                       // count photos matching filters
        virtual void getPhotos(const std::vector<Photo::Id> &, const Callback<const std::vector<IPhotoInfo::Ptr> &> &) = 0;           // get particular photos
        virtual void listTagNames(const Callback<const std::vector<TagNameInfo> &> & ) = 0;                                           // list all stored tag names
        virtual void listTagValues(const TagNameInfo &, const Callback<const TagNameInfo &, const std::vector<TagValue> &> &) = 0;    // list all tag values
        virtual void listTagValues(const TagNameInfo &, const std::vector<IFilter::Ptr> &, const Callback<const TagNameInfo &, const std::vector<TagValue> &> &) = 0;  // list values of provided tag on photos matching filter
        virtual void listPhotos(const std::vector<IFilter::Ptr> &, const Callback<const IPhotoInfo::List &> &) = 0;                   // list all photos matching filter

        // modify data

        // drop data

        // other
        template<typename Callable>
        void exec(Callable&& f)
        {
            auto task = std::make_unique<Task<Callable>>(std::forward<Callable>(f));
            execute(std::move(task));
        }

        //init backend - connect to database or create new one
        virtual void init(const ProjectInfo &, const Callback<const BackendStatus &> &) = 0;

        //close database connection
        virtual void closeConnections() = 0;

        struct ITask
        {
            virtual ~ITask() = default;
            virtual void run(IBackendOperator *) = 0;
        };

    signals:
        void photosAdded(const std::vector<IPhotoInfo::Ptr> &);  // emited after new photos were added to database
        void photoModified(const IPhotoInfo::Ptr &);             // emited when photo updated
        void photosRemoved(const std::vector<Photo::Id> &);      // emited after photos removal

    protected:
        template<typename Callable>
        struct Task: ITask
        {
            Task(Callable&& f): m_f(std::forward<Callable>(f)) {}

            void run(IBackendOperator* backend) override
            {
                m_f(backend);
            }

            typedef typename std::remove_reference<Callable>::type Callable_T;  // be sure we store copy of object, not reference or something
            Callable_T m_f;
        };

        virtual void execute(std::unique_ptr<ITask> &&) = 0;

        Q_OBJECT
    };

}

#endif
