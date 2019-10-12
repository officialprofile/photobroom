/*
 *    interface for backends
 *    Copyright (C) 2013  Michał Walenciak <MichalWalenciak@gmail.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef IBACKEND_HPP
#define IBACKEND_HPP

#include <string>
#include <set>
#include <vector>
#include <optional>

#include <core/tag.hpp>

#include "database_status.hpp"
#include "filter.hpp"
#include "group.hpp"
#include "person_data.hpp"
#include "photo_data.hpp"

#include "database_export.h"
#include "database_status.strings.hpp"


struct IConfiguration;
struct ILoggerFactory;

#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

#define DB_ERROR_ON_FALSE(CALL, ERRCODE)       \
    {                                          \
        if ( !(CALL) )                         \
            throw db_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " + __PRETTY_FUNCTION__, ERRCODE);  \
    }

#define DB_ERR_ON_FALSE(CALL) DB_ERROR_ON_FALSE(CALL, StatusCodes::GeneralError)


namespace Database
{
    struct IGroupOperator;
    struct IPhotoChangeLogOperator;
    struct IPhotoOperator;
    struct ProjectInfo;

    // for internal usage
    class db_error: std::exception
    {
            std::string m_err;
            StatusCodes m_status;

        public:
            db_error(const std::string& err, StatusCodes status = StatusCodes::GeneralError):
                m_err(),
                m_status(status)
            {
                m_err += err + ": " + get_entry(m_status);
            }

            StatusCodes status() const noexcept
            {
                return m_status;
            }

            const char* what() const noexcept override
            {
                return m_err.c_str();
            }
    };

    /** \brief Low level database interface.
     *
     * It defines way of communication with database backend.\n
     * \todo Divide into smaller interfaces and use repository pattern (see github issue #272)\n
     *       Backend should be splitted into operator and itself should become a facade.
     */
    struct DATABASE_EXPORT IBackend: public QObject
    {
        virtual ~IBackend() = default;

        /** \brief Add photos to database
         *  \arg photos details of photos to be stored. At least id and path need to be valid. \see Photo::DataDelta
         *  \return true if successful.
         *
         *  If operation is sucessful photosAdded() signal is emited.
         */
        virtual bool addPhotos(std::vector<Photo::DataDelta>& photos) = 0;

        //update data
        virtual bool update(const Photo::DataDelta &) = 0;

        //read data

        /// list all stored tag names
        virtual std::vector<TagNameInfo> listTags() = 0;

        /// list all values of tag for photos matching provided filter
        virtual std::vector<TagValue>    listTagValues(const TagNameInfo &,
                                                       const std::vector<IFilter::Ptr> &) = 0;

        /// list all photos
        virtual std::vector<Photo::Id>   getAllPhotos() = 0;

        /// find all photos matching filter
        virtual std::vector<Photo::Id>   getPhotos(const std::vector<IFilter::Ptr> &) = 0;

        /// get particular photo
        virtual Photo::Data              getPhoto(const Photo::Id &) = 0;

        /// Count photos matching filter
        virtual int                      getPhotosCount(const std::vector<IFilter::Ptr> &) = 0;

        /// list all people names
        virtual std::vector<PersonName>  listPeople() = 0;

        /// list people on photo
        virtual std::vector<PersonInfo>  listPeople(const Photo::Id &) = 0;

        ///< person data
        virtual PersonName               person(const Person::Id &) = 0;

        /**
         * \brief Store or update person
         * \arg pn Details about person name to be stored.
         * \return id of created or updated person.
         *
         * If \a pn has valid id then person name will be updated in database   \n
         * If id is not valid and \a pn holds name already existing in database \n
         * then method will return id of that person.                             \n
         * If id is not valid and name was not found in database then             \n
         * new person will be added to database and its id will be returned.      \n
         */
        virtual Person::Id               store(const PersonName& pn) = 0;
        virtual PersonInfo::Id           store(const PersonInfo &) = 0;                          // store or update person details
        virtual void                     set(const Photo::Id &, const QString &, int value) = 0; // set flag for photo to given value
        virtual std::optional<int>       get(const Photo::Id &, const QString &) = 0;            // get flag value

        // reading extra data
        //virtual QByteArray getThumbnail(const Photo::Id &) = 0;                               // get thumbnail for photo

        // modify data
        virtual std::vector<Photo::Id> markStagedAsReviewed() = 0;

        // write extra data
        //virtual bool setThumbnail(const Photo::Id &, const QByteArray &) = 0;                  // set thumbnail for photo

        /// \brief init backend - connect to database or create new one
        virtual BackendStatus init(const ProjectInfo &) = 0;

        /// \brief close database connection
        virtual void closeConnections() = 0;

        // TODO: a set of 'operators' which are about to replace methods above
        //       in the name of interface segregation and repository pattern (see #272 on github)
        virtual IGroupOperator* groupOperator() = 0;
        virtual IPhotoOperator* photoOperator() = 0;
        virtual IPhotoChangeLogOperator* photoChangeLogOperator() = 0;

    signals:
        /// emited after new photos were added to database
        void photosAdded(const std::vector<Photo::Id> &);

        ///< emited when photo updated
        void photoModified(const Photo::Id &);

        ///< emited after photos removal
        void photosRemoved(const std::vector<Photo::Id> &);

        ///< emited when done with photos marking
        void photosMarkedAsReviewed(const std::vector<Photo::Id> &);

    private:
        Q_OBJECT
    };
}

#endif

