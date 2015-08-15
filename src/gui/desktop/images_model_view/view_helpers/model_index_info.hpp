/*
 * Model for view
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

#ifndef MODELINDEXINFO_HPP
#define MODELINDEXINFO_HPP

#include <QModelIndex>
#include <QRect>

#include <core/tree.hpp>

struct ModelIndexInfo
{
        bool expanded;

        void setPosition(const QPoint &);
        void setSize(const QSize &);
        void setRect(const QRect& r);
        void setOverallSize(const QSize& r);

        QRect getOverallRect() const;
        const QRect getRect() const;
        const QSize& getOverallSize() const;
        const QPoint& getPosition() const;
        const QSize& getSize() const;

        void cleanRects();
        void markPositionInvalid();
        void markSizeInvalid();

        bool isSizeValid() const;
        bool isPositionValid() const;
        bool valid() const;

        ModelIndexInfo(const QModelIndex &);
        
        operator std::string() const;

    private:
        struct Postition
        {
            QPoint position;
            QSize size;
            bool valid;             //'position' field is valid?

            Postition(): position(), size(), valid(false) {}

            QRect getRect() const;
            void setRect(const QRect &);

        } position;

        QSize overallRect;
};

#endif // MODELINDEXINFO_H
