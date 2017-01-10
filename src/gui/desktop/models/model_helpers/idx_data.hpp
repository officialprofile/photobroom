/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#ifndef IDXDATA_HPP
#define IDXDATA_HPP

#include <QMap>

#include <database/iphoto_info.hpp>
#include <database/filter.hpp>

#include "models/db_data_model.hpp"
#include "models/model_types.hpp"

class QVariant;

class IdxDataManager;

struct IIdxData
{
    typedef std::unique_ptr<IIdxData> Ptr;

    virtual ~IIdxData() = default;

    virtual void setParent(IdxData *) = 0;
    virtual IIdxData* addChild(IIdxData::Ptr&& child) = 0;
    virtual void removeChild(IIdxData* child) = 0;
    virtual void removeChildren() = 0;
    virtual IIdxData::Ptr takeChild(IIdxData* child) = 0;
    virtual void setNodeFilter(const Database::IFilter::Ptr& filter) = 0;
    virtual void setNodeSorting(const Hierarchy::Level &) = 0;

    virtual long findPositionFor(const IIdxData* child) const = 0;
    virtual IdxData* parent() const = 0;
    virtual bool isNode() const = 0;
    virtual const std::vector<Ptr>& getChildren() const = 0;
    virtual QVariant getData(int) const = 0;
    virtual const Database::IFilter::Ptr& getFilter() const = 0;
    virtual std::size_t getLevel() const = 0;
    virtual Tag::TagsList getTags() const = 0;

    virtual int getRow() const = 0;
    virtual int getCol() const = 0;
    virtual NodeStatus status() const = 0;
};


class IdxData: public IIdxData
{
    public:
        QMap<int, QVariant> m_data;
        Hierarchy::Level m_order;                // defines how to sort children
        IdxDataManager* m_model;

        // node constructor
        IdxData(IdxDataManager *, const QVariant& name);

        //leaf constructor
        IdxData(IdxDataManager *, const IPhotoInfo::Ptr &);

        virtual ~IdxData();

        IdxData(const IdxData &) = delete;
        IdxData& operator=(const IdxData &) = delete;

        void setNodeFilter(const Database::IFilter::Ptr& filter) override;
        void setNodeSorting(const Hierarchy::Level &) override;
        long findPositionFor(const IIdxData* child) const override;     // returns position where child matches
        long getPositionOf(const IIdxData* child) const;                // returns position of children
        IIdxData* addChild(IIdxData::Ptr&& child) override;             // returns pointer to child
        void removeChild(IIdxData* child) override;                     // removes child (memory is released)
        void removeChildren() override;
        IIdxData::Ptr takeChild(IIdxData* child) override;              // function acts as removeChild but does not delete children
        void reset();
        void setParent(IdxData *) override;
        void setStatus(NodeStatus);
        IdxData* parent() const override;
        bool isPhoto() const;
        bool isNode() const override;

        const std::vector<Ptr>& getChildren() const override;
        QVariant getData(int) const override;
        const Database::IFilter::Ptr& getFilter() const override;
        std::size_t getLevel() const override;
        IPhotoInfo::Ptr getPhoto() const;
        Tag::TagsList getTags() const override;

        int getRow() const;
        int getCol() const;

        NodeStatus status() const override;

        IIdxData* findChildWithBadPosition() const;            // returns first child which lies in a wrong place
        bool sortingRequired() const;

    private:
        std::vector<Ptr> m_children;
        IPhotoInfo::Ptr m_photo;                 // null for nodes, photo for photos
        Database::IFilter::Ptr m_filter;         // define which children match
        size_t m_level;
        IdxData* m_parent;

        IdxData(IdxDataManager *);
        void initLeafData();
        void init();
};

#endif // IDXDATA_HPP
