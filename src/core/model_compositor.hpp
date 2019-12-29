/*
 * Copyright (C) 2019  Michał Walenciak <Kicer86@gmail.com>
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
 */

#ifndef MODELCOMPOSITOR_HPP
#define MODELCOMPOSITOR_HPP

#include <QAbstractListModel>

#include "imodel_compositor_data_source.hpp"

/**
 * @brief Unite many data sources into one
 */
class ModelCompositor : public QAbstractListModel
{
    public:
        void add(IModelCompositorDataSource *);

        virtual int rowCount(const QModelIndex & = {}) const override;
        virtual QVariant data(const QModelIndex &, int) const override;

    private:
        std::map<IModelCompositorDataSource *, int> m_sources;

        void dataSourceChanged(IModelCompositorDataSource *);
};

#endif // MODELCOMPOSITOR_HPP
