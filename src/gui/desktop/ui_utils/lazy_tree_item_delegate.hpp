/*
 * LazyTreeItemDelegate - loads images from external source
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

#ifndef LAZYTREEITEMDELEGATE_HPP
#define LAZYTREEITEMDELEGATE_HPP

#include "desktop/views/tree_item_delegate.hpp"

struct IThumbnailAcquisitor;

class LazyTreeItemDelegate: public TreeItemDelegate
{
        Q_OBJECT

    public:
        LazyTreeItemDelegate(ImagesTreeView *);
        LazyTreeItemDelegate(const LazyTreeItemDelegate &) = delete;
        ~LazyTreeItemDelegate();

        LazyTreeItemDelegate& operator=(const LazyTreeItemDelegate &) = delete;

        void set(IThumbnailAcquisitor *);

        // TreeItemDelegate:
        QImage getImage(const QModelIndex &, const QSize &) const override;

    private:
        IThumbnailAcquisitor* m_thumbnailAcquisitor;
};

#endif // LAZYTREEITEMDELEGATE_HPP
