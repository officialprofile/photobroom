/*
 * Base for SQL-based backends
 * This class is meant to be included to each project using it.
 *
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

#include "sql_backend.hpp"

#include <iostream>
#include <set>
#include <thread>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QPixmap>
#include <QBuffer>
#include <QString>

#include <OpenLibrary/utils/optional.hpp>

#include <core/tag.hpp>
#include <core/task_executor.hpp>
#include <database/filter.hpp>

#include "table_definition.hpp"
#include "sqldbquery.hpp"
#include "sql_query_constructor.hpp"
#include "tables.hpp"
#include "query_structs.hpp"


namespace Database
{
    namespace
    {
        QByteArray toPrintable(const QPixmap& pixmap)
        {
            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);

            pixmap.save(&buffer, "JPEG");

            return bytes.toBase64();
        }

        QPixmap fromPrintable(const QByteArray& data)
        {
            const QByteArray bytes = QByteArray::fromBase64(data);
            QPixmap pixmap;

            const bool status = pixmap.loadFromData(bytes, "JPEG");

            if (status == false)
            {
                //TODO: load thumbnail from file
            }

            return pixmap;
        }


        struct SqlFiltersVisitor: IFilterVisitor
        {
                SqlFiltersVisitor(): m_temporary_result() {}
                virtual ~SqlFiltersVisitor() {}

                QString parse(const std::deque<IFilter::Ptr>& filters)
                {
                    QString result;
                    const size_t s = filters.size();
                    bool nest_previous = true;
                    m_temporary_result = "";

                    for (size_t i = 0; i < s; i++)
                    {
                        if (i > 0) // not first filter? Nest previous one
                        {
                            if (nest_previous) //really nest?
                            {
                                result = "SELECT * FROM "
                                         "( " + result + ") AS level_%1";

                                result = result.arg(i);
                            }
                        }
                        else
                        {
                            result = "SELECT %1.id AS photos_id FROM %1";
                            result = result.arg(TAB_PHOTOS);
                        }

                        const size_t index = s - i - 1;
                        m_temporary_result = "";
                        filters[index]->visitMe(this);

                        nest_previous = m_temporary_result.isEmpty() == false;
                        result += m_temporary_result;
                        m_temporary_result = "";
                    }

                    return result;
                }

            private:
                // IFilterVisitor interface
                void visit(FilterEmpty *) override
                {
                }

                void visit(FilterDescription* desciption) override
                {
                    QString result;

                    result = " JOIN (" TAB_TAGS ", " TAB_TAG_NAMES ")"
                             " ON (" TAB_TAGS ".photo_id = photos_id AND " TAB_TAG_NAMES ".id = " TAB_TAGS ".name_id)"
                             " WHERE " TAB_TAG_NAMES ".name = '%1' AND " TAB_TAGS ".value = '%2'";

                    result = result.arg(desciption->tagName);
                    result = result.arg(desciption->tagValue);

                    m_temporary_result = result;
                }

                void visit(FilterFlags* flags) override
                {
                    QString result;

                    result =  " JOIN " TAB_FLAGS " ON " TAB_FLAGS ".photo_id = photos_id";
                    result += " WHERE " TAB_FLAGS ".staging_area = '%1'";

                    result = result.arg(flags->stagingArea? "TRUE": "FALSE");

                    m_temporary_result = result;
                }

                QString m_temporary_result;
        };

    }

    struct StorePhoto;

    /*****************************************************************************/


    struct ASqlBackend::Data
    {
            ASqlBackend* m_backend;
            std::thread::id m_database_thread_id;
            std::string m_databaseName;

            Data(ASqlBackend* backend);
            ~Data();
            Data(const Data &) = delete;
            Data& operator=(const Data &) = delete;

            bool exec(const QString& query, QSqlQuery* result) const;
            bool exec(const SqlQuery& query, QSqlQuery* result) const;
            bool createDB(const QString& dbName) const;
            Optional<unsigned int> store(const TagNameInfo& nameInfo) const;
            bool store(const PhotoInfo::Ptr& data);
            PhotoInfo::Ptr getPhoto(const PhotoInfo::Id &);
            std::vector<TagNameInfo> listTags() const;
            std::set<TagValueInfo> listTagValues(const TagNameInfo& tagName);
            std::deque<TagValueInfo> listTagValues(const TagNameInfo &, const std::deque<IFilter::Ptr> &);
            QueryList getPhotos(const std::deque<IFilter::Ptr>& filter);

        private:
            friend struct StorePhoto;

            Optional<unsigned int> findTagByName(const QString& name) const;
            QString generateFilterQuery(const std::deque<IFilter::Ptr>& filter);
            bool storeThumbnail(int photo_id, const QPixmap &) const;
            bool storeHash(int photo_id, const PhotoInfo::Hash &) const;
            bool storeTags(int photo_id, const std::shared_ptr<ITagData> &) const;
            bool storeFlags(int photo_id, const PhotoInfo::Ptr &) const;
            TagData getTagsFor(const PhotoInfo::Id &);
            Optional<QPixmap> getThumbnailFor(const PhotoInfo::Id &);
            Optional<PhotoInfo::Hash> getHashFor(const PhotoInfo::Id &);
            QString getPathFor(const PhotoInfo::Id &);

            //for friends:
            bool storePhoto(const PhotoInfo::Ptr& data);
    };


    struct StorePhoto: ITaskExecutor::ITask
    {
        ASqlBackend::Data* m_data;
        PhotoInfo::Ptr m_photo;

        StorePhoto(ASqlBackend::Data* data, const PhotoInfo::Ptr& photo): m_data(data), m_photo(photo)
        {
        }

        StorePhoto(const StorePhoto &) = delete;
        StorePhoto& operator=(const StorePhoto &) = delete;

        virtual std::string name() const override
        {
            return "Photo storing";
        }

        virtual void perform() override
        {
            const bool status = m_data->storePhoto(m_photo); //call store from ASqlBackend::Data

            if (status == false)
                std::cerr << "error while storing photo in database" << std::endl;
        }
    };


    ASqlBackend::Data::Data(ASqlBackend* backend): m_backend(backend), m_database_thread_id(), m_databaseName("")
    {

    }


    ASqlBackend::Data::~Data()
    {

    }


    bool ASqlBackend::Data::exec(const QString& query, QSqlQuery* result) const
    {
        // threads cannot be used with sql connections:
        // http://qt-project.org/doc/qt-5/threads-modules.html#threads-and-the-sql-module
        // make sure the same thread is used as at construction time.
        assert(std::this_thread::get_id() == m_database_thread_id);

        std::clog << query.toStdString() << std::endl;

        const bool status = result->exec(query);

        if (status == false)
            std::cerr << "SQLBackend: error: " << result->lastError().text().toStdString()
                      << " while performing query: " << query.toStdString() << std::endl;

        return status;
    }


    bool ASqlBackend::Data::exec(const SqlQuery& query, QSqlQuery* result) const
    {
        auto& queries = query.getQueries();
        bool status = true;

        for(size_t i = 0; i < queries.size() && status; i++)
            status = exec(queries[i], result);

        return status;
    }


    bool ASqlBackend::Data::createDB(const QString& dbName) const
    {
        //check if database exists
        QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());
        QSqlQuery query(db);
        bool status = exec(QString("SHOW DATABASES LIKE '%1';").arg(dbName), &query);

        //create database if doesn't exists
        bool empty = query.next() == false;

        if (status && empty)
            status = exec(QString("CREATE DATABASE `%1`;").arg(dbName), &query);

        //switch to database
        if (status)
            status = exec(QString("USE %1;").arg(dbName), &query);

        return status;
    }


    Optional<unsigned int> ASqlBackend::Data::store(const TagNameInfo& nameInfo) const
    {
        const QString& name = nameInfo.getName();
        const int type = nameInfo.getType();
        QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());
        QSqlQuery query(db);

        //check if tag exists
        Optional<unsigned int> tagId = findTagByName(name);

        if (! tagId)  //tag not yet in database
        {
            const QString queryStr = QString("INSERT INTO %1 (id, name, type) VALUES (NULL, '%2', '%3');")
                                     .arg(TAB_TAG_NAMES)
                                     .arg(name)
                                     .arg(type);

            const bool status = exec(queryStr, &query);

            if (status)
                tagId = query.lastInsertId().toUInt();
        }

        return tagId;
    }


    //TODO: threads cannot be used with sql connections:
    //      http://qt-project.org/doc/qt-5/threads-modules.html#threads-and-the-sql-module
    bool ASqlBackend::Data::store(const PhotoInfo::Ptr& data)
    {
        //auto task = std::make_shared<StorePhoto>(this, data);
        //TaskExecutorConstructor::get()->add(task);

        storePhoto(data);

        return true;
    }


    std::vector<TagNameInfo> ASqlBackend::Data::listTags() const
    {
        QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());
        QSqlQuery query(db);
        const QString query_str("SELECT name, type FROM " TAB_TAG_NAMES ";");

        const bool status = exec(query_str, &query);
        std::vector<TagNameInfo> result;

        while (status && query.next())
        {
            const QString name       = query.value(0).toString();
            const unsigned int value = query.value(1).toUInt();

            TagNameInfo tagName(name, value);
            result.push_back(tagName);
        }

        return result;
    }


    std::set<TagValueInfo> ASqlBackend::Data::listTagValues(const TagNameInfo& tagName)
    {
        const Optional<unsigned int> tagId = findTagByName(tagName);

        std::set<TagValueInfo> result;

        if (tagId)
        {
            QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());
            QSqlQuery query(db);
            const QString query_str = QString("SELECT value FROM " TAB_TAGS " WHERE name_id=\"%1\";")
                                      .arg(*tagId);

            const bool status = exec(query_str, &query);

            while (status && query.next())
            {
                const QString value = query.value(0).toString();

                TagValueInfo valueInfo(value);
                result.insert(valueInfo);
            }
        }

        return result;
    }


    std::deque<TagValueInfo> ASqlBackend::Data::listTagValues(const TagNameInfo& tagName, const std::deque<IFilter::Ptr>& filter)
    {
        const QString filterQuery = generateFilterQuery(filter);

        //from filtered photos, get info about tags used there
        QString queryStr = "SELECT DISTINCT %2.value FROM ( %1 ) JOIN %2, %3 ON photos_id=%2.photo_id AND %3.id=%2.name_id WHERE name='%4'";

        queryStr = queryStr.arg(filterQuery);
        queryStr = queryStr.arg(TAB_TAGS);
        queryStr = queryStr.arg(TAB_TAG_NAMES);
        queryStr = queryStr.arg(tagName.getName());

        QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());
        QSqlQuery query(db);

        std::deque<TagValueInfo> result;
        const bool status = exec(queryStr, &query);

        if (status)
            while (status && query.next())
            {
                const QString value = query.value(0).toString();

                TagValueInfo valueInfo(value);
                result.push_back(valueInfo);
            }

        return result;
    }


    QueryList ASqlBackend::Data::getPhotos(const std::deque<IFilter::Ptr>& filter)
    {
        const QString queryStr = generateFilterQuery(filter);

        QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());
        QSqlQuery query(db);

        exec(queryStr, &query);
        SqlDBQuery* dbQuery = new SqlDBQuery(query, m_backend);
        InterfaceContainer<IQuery> container(dbQuery);

        QueryList result(container);

        return result;
    }


    Optional<unsigned int> ASqlBackend::Data::findTagByName(const QString& name) const
    {
        QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());
        QSqlQuery query(db);
        const QString find_tag_query = QString("SELECT id FROM " TAB_TAG_NAMES " WHERE name =\"%1\";").arg(name);
        const bool status = exec(find_tag_query, &query);

        Optional<unsigned int> result;

        if (status && query.next())
            result = query.value(0).toInt();

        return result;
    }


    QString ASqlBackend::Data::generateFilterQuery(const std::deque<IFilter::Ptr>& filters)
    {
        SqlFiltersVisitor visitor;
        const QString result = visitor.parse(filters);

        return result;
    }


    bool ASqlBackend::Data::storeThumbnail(int photo_id, const QPixmap& pixmap) const
    {
        InsertQueryData data(TAB_THUMBS);

        data.setColumns("photo_id", "data");
        data.setValues(QString::number(photo_id), QString(toPrintable(pixmap)) );

        SqlQuery queryStrs = m_backend->getQueryConstructor()->insertOrUpdate(data);

        QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());
        QSqlQuery query(db);
        bool status = exec(queryStrs, &query);

        return status;
    }
    
    
    bool ASqlBackend::Data::storeHash(int photo_id, const PhotoInfo::Hash& hash) const
    {
        InsertQueryData data(TAB_HASHES);

        data.setColumns("photo_id", "hash");
        data.setValues(QString::number(photo_id), hash.c_str());

        SqlQuery queryStrs = m_backend->getQueryConstructor()->insertOrUpdate(data);

        QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());
        QSqlQuery query(db);
        bool status = exec(queryStrs, &query);

        return status;
    }


    bool ASqlBackend::Data::storeTags(int photo_id, const std::shared_ptr<ITagData>& tags) const
    {
        QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());
        QSqlQuery query(db);
        bool status = true;

        //remove all tags already attached to photo. TODO: maybe some inteligence here?
        const QString deleteQuery = QString("DELETE FROM " TAB_TAGS " WHERE photo_id=\"%1\"").arg(photo_id);
        status = exec(deleteQuery, &query);

        ITagData::TagsList tagsList = tags->getTags();

        for (auto it = tagsList.begin(); status && it != tagsList.end(); ++it)
        {
            //store tag name
            const Optional<unsigned int> tag_id = store(it->first);

            if (tag_id)
            {
                //store tag values
                const ITagData::ValuesSet& values = it->second;

                for (auto it_v = values.cbegin(); it_v != values.cend(); ++it_v)
                {
                    const TagValueInfo& valueInfo = *it_v;

                    const QString query_str =
                        QString("INSERT INTO " TAB_TAGS
                                "(id, value, photo_id, name_id) VALUES(NULL, \"%1\", \"%2\", \"%3\");")
                        .arg(valueInfo.value())
                        .arg(photo_id)
                        .arg(*tag_id);

                    status = exec(query_str, &query);
                }
            }
            else
                status = false;
        }

        return status;
    }


    bool ASqlBackend::Data::storeFlags(int photo_id, const PhotoInfo::Ptr& photoInfo) const
    {
        InsertQueryData queryData(TAB_FLAGS);
        queryData.setColumns("id", "photo_id", "staging_area", "tags_loaded", "hash_loaded", "thumbnail_loaded");
        queryData.setValues( InsertQueryData::Value::Null,
                             QString::number(photo_id),
                             photoInfo->getFlags().stagingArea? "TRUE": "FALSE",
                             photoInfo->getFlags().exifLoaded? "TRUE": "FALSE",
                             photoInfo->getFlags().hashLoaded? "TRUE": "FALSE",
                             photoInfo->getFlags().thumbnailLoaded? "TRUE": "FALSE" );

        auto queryStrs = m_backend->getQueryConstructor()->insertOrUpdate(queryData);

        QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());
        QSqlQuery query(db);
        bool status = exec(queryStrs, &query);

        return status;
    }


    bool ASqlBackend::Data::storePhoto(const PhotoInfo::Ptr& data)
    {
        QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());
        QSqlQuery query(db);

        bool status = db.transaction();

        //store path and hash
        PhotoInfo::Id id = data->getID();
        const bool updating = id.valid();
        const bool inserting = !updating;

        InsertQueryData insertData(TAB_PHOTOS);
        insertData.setColumns("path", "store_date");
        insertData.setValues(data->getPath(), InsertQueryData::Value::CurrentTime);

        SqlQuery queryStrs;

        if (inserting)
        {
            insertData.setColumns("id");
            insertData.setValues(InsertQueryData::Value::Null);
            queryStrs = m_backend->getQueryConstructor()->insert(insertData);
        }
        else
        {
            UpdateQueryData updateData(insertData);
            updateData.setCondition( "id", QString::number(id.value()) );
            queryStrs = m_backend->getQueryConstructor()->update(updateData);
        }

        //execute update/insert
        if (status)
            status = exec(queryStrs, &query);

        //update id
        if (status && inserting)                       //Get Id from database after insert
        {
            QVariant photo_id  = query.lastInsertId(); //TODO: WARNING: may not work (http://qt-project.org/doc/qt-5.1/qtsql/qsqlquery.html#lastInsertId)
            status = photo_id.isValid();

            if (status)
                id = PhotoInfo::Id(photo_id.toInt());
        }

        //make sure id is set
        if (status)
            status = id.valid();

        //store used tags
        std::shared_ptr<ITagData> tags = data->getTags();

        if (status)
            status = storeTags(id, tags);

        if (status && data->isThumbnailLoaded())
            status = storeThumbnail(id, data->getThumbnail());
        
        if (status && data->isHashLoaded())
            status = storeHash(id, data->getHash());

        if (status)
            status = storeFlags(id, data);

        if (status)
            status = db.commit();
        else
            db.rollback();

        //store id in photo
        if (status && inserting)
            data->initID(id);

        return status;
    }


    PhotoInfo::Ptr ASqlBackend::Data::getPhoto(const PhotoInfo::Id& id)
    {
        //basic data
        PhotoInfo::Ptr photoInfo = std::make_shared<PhotoInfo>(getPathFor(id));
        photoInfo->initID(id);

        //load tags
        const TagData tagData = getTagsFor(id);

        std::shared_ptr<ITagData> tags = photoInfo->getTags();
        tags->setTags(tagData.getTags());

        //load thumbnail
        const Optional<QPixmap> thumbnail = getThumbnailFor(id);
        if (thumbnail)
            photoInfo->initThumbnail(*thumbnail);
        
        //load hash
        const Optional<PhotoInfo::Hash> hash = getHashFor(id);
        if (hash)
            photoInfo->initHash(*hash);

        return photoInfo;
    }


    TagData ASqlBackend::Data::getTagsFor(const PhotoInfo::Id& photoId)
    {
        QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());
        QSqlQuery query(db);

        QString queryStr = QString("SELECT "
                                   "%1.id, %2.name, %1.value, %1.name_id "
                                   "FROM "
                                   "%1 "
                                   "JOIN "
                                   "%2 "
                                   "ON %2.id = %1.name_id "
                                   "WHERE %1.photo_id = '%3';")
                           .arg(TAB_TAGS)
                           .arg(TAB_TAG_NAMES)
                           .arg(photoId.value());

        const bool status = exec(queryStr, &query);
        TagData tagData;

        while(status && query.next())
        {
            const QString name  = query.value(1).toString();
            const QString value = query.value(2).toString();
            const unsigned int tagType = query.value(3).toInt();

            tagData.setTag(TagNameInfo(name, tagType), value);
        }

        return tagData;
    }


    Optional<QPixmap> ASqlBackend::Data::getThumbnailFor(const PhotoInfo::Id& id)
    {
        QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());

        Optional<QPixmap> pixmap;
        QSqlQuery query(db);

        QString queryStr = QString("SELECT data FROM %1 WHERE %1.photo_id = '%2'");

        queryStr = queryStr.arg(TAB_THUMBS);
        queryStr = queryStr.arg(id.value());

        const bool status = exec(queryStr, &query);

        if(status && query.next())
        {
            const QVariant variant = query.value(0);
            QByteArray data(variant.toByteArray());
            pixmap = fromPrintable(data);
        }

        return pixmap;
    }
    
    Optional<PhotoInfo::Hash> ASqlBackend::Data::getHashFor(const PhotoInfo::Id& id)
    {
        QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());
        QSqlQuery query(db);
        QString queryStr = QString("SELECT hash FROM %1 WHERE %1.photo_id = '%2'");

        queryStr = queryStr.arg(TAB_HASHES);
        queryStr = queryStr.arg(id.value());

        const bool status = exec(queryStr, &query);

        Optional<PhotoInfo::Hash> result;
        if(status && query.next())
        {
            const QVariant variant = query.value(0);
            
            result = variant.toString().toStdString();
        }

        return result;
    }


    QString ASqlBackend::Data::getPathFor(const PhotoInfo::Id& id)
    {
        QSqlDatabase db = QSqlDatabase::database(m_databaseName.c_str());
        QSqlQuery query(db);

        QString queryStr = QString("SELECT path FROM %1 WHERE %1.id = '%2'");

        queryStr = queryStr.arg(TAB_PHOTOS);
        queryStr = queryStr.arg(id.value());

        const bool status = exec(queryStr, &query);

        QString result;
        if(status && query.next())
        {
            const QVariant path = query.value(0);

            result = path.toString();
        }

        return result;
    }


    ///////////////////////////////////////////////////////////////////////


    ASqlBackend::ASqlBackend(): m_data(new Data(this))
    {

    }


    ASqlBackend::~ASqlBackend()
    {

    }


    void ASqlBackend::closeConnections()
    {
        QSqlDatabase db = QSqlDatabase::database(m_data->m_databaseName.c_str());

        if (db.isValid() && db.isOpen())
        {
            std::cout << "ASqlBackend: closing database connections." << std::endl;
            db.close();
        }
    }


    bool ASqlBackend::createDB(const char* name)
    {
        return m_data->createDB(name);
    }


    bool ASqlBackend::onAfterOpen()
    {
        return true;
    }


    bool ASqlBackend::assureTableExists(const TableDefinition& definition) const
    {
        QSqlDatabase db = QSqlDatabase::database(m_data->m_databaseName.c_str());

        QSqlQuery query(db);
        const QString showQuery = getQueryConstructor()->prepareFindTableQuery(definition.name);

        bool status = m_data->exec(showQuery, &query);

        //create table 'name' if doesn't exist
        bool empty = query.next() == false;

        if (status && empty)
        {
            QString columnsDesc;
            const int size = definition.columns.size();

            for(int i = 0; i < size; i++)
            {
                const bool notlast = i + 1 < size;
                columnsDesc += getQueryConstructor()->prepareColumnDescription(definition.columns[i]) + (notlast? ", ": "");
            }

            status = m_data->exec( getQueryConstructor()->prepareCreationQuery(definition.name, columnsDesc), &query );

            if (status && definition.keys.empty() == false)
            {
                const int keys = definition.keys.size();

                for(int i = 0; status && i < keys; i++)
                {
                    QString indexDesc;

                    indexDesc += "CREATE " + definition.keys[i].type;
                    indexDesc += " " + definition.keys[i].name + "_idx";
                    indexDesc += " ON " + definition.name;
                    indexDesc += " " + definition.keys[i].def + ";";

                    status = m_data->exec(indexDesc, &query);
                }
            }
        }

        return status;
    }


    bool ASqlBackend::exec(const QString& query, QSqlQuery* status) const
    {
        return m_data->exec(query, status);
    }


    bool ASqlBackend::init(const std::string& dbName)
    {
        //store thread id for further validation
        m_data->m_database_thread_id = std::this_thread::get_id();
        m_data->m_databaseName = dbName;

        QSqlDatabase db = QSqlDatabase::database(m_data->m_databaseName.c_str());
        bool status = prepareDB(&db, dbName.c_str());

        if (status)
            status = db.open();

        if (status)
            status = onAfterOpen();

        if (status)
            status = checkStructure();
        else
            std::cerr << "SQLBackend: error opening database: " << db.lastError().text().toStdString() << std::endl;

        //TODO: crash when status == false;
        return status;
    }


    bool ASqlBackend::store(const PhotoInfo::Ptr& entry)
    {
        bool status = false;

        if (m_data)
            status = m_data->store(entry);
        else
            std::cerr << "ASqlBackend: database object does not exist." << std::endl;

        return status;
    }


    std::vector<TagNameInfo> ASqlBackend::listTags()
    {
        std::vector<TagNameInfo> result;

        if (m_data)
            result = m_data->listTags();
        else
            std::cerr << "ASqlBackend: database object does not exist." << std::endl;

        return result;
    }


    std::set<TagValueInfo> ASqlBackend::listTagValues(const TagNameInfo& tagName)
    {
        std::set<TagValueInfo> result;

        if (m_data)
            result = m_data->listTagValues(tagName);
        else
            std::cerr << "ASqlBackend: database object does not exist." << std::endl;

        return result;
    }


    std::deque<TagValueInfo> ASqlBackend::listTagValues(const TagNameInfo& tagName, const std::deque<IFilter::Ptr>& filter)
    {
        std::deque<TagValueInfo> result = m_data->listTagValues(tagName, filter);

        return result;
    }


    QueryList ASqlBackend::getAllPhotos()
    {
        std::deque<IFilter::Ptr> emptyFilter;
        return m_data->getPhotos(emptyFilter);  //like getPhotos but without any filters
    }


    QueryList ASqlBackend::getPhotos(const std::deque<IFilter::Ptr>& filter)
    {
        return m_data->getPhotos(filter);
    }


    PhotoInfo::Ptr ASqlBackend::getPhoto(const PhotoInfo::Id& id)
    {
        return m_data->getPhoto(id);
    }


    bool ASqlBackend::checkStructure()
    {
        QSqlDatabase db = QSqlDatabase::database(m_data->m_databaseName.c_str());
        bool status = db.transaction();

        //check tables existance
        if (status)
            for (TableDefinition& table: tables)
            {
                status = assureTableExists(table);

                if (!status)
                    break;
            }

        QSqlQuery query(db);

        //at least one row must be present in table 'version_history'
        if (status)
            status = m_data->exec("SELECT COUNT(*) FROM " TAB_VER_HIST ";", &query);

        if (status)
            status = query.next() == true;

        const QVariant rows = status? query.value(0): QVariant(0);

        //insert first entry
        if (status && rows == 0)
            status = m_data->exec(QString("INSERT INTO " TAB_VER_HIST "(version, date)"
                                          " VALUES(%1, CURRENT_TIMESTAMP);")
                                  .arg(db_version), &query);

        //TODO: check last entry with current version

        if (status)
            status = db.commit();
        else
            db.rollback();

        return status;
    }

}
