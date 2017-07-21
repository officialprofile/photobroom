/*
 * Widget for photo properties
 * Copyright (C) 2017  Michał Walenciak <Kicer86@gmail.com>
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


#include "photo_properties.hpp"

#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>

#include <system/filesystem.hpp>

#include "utils/selection_extractor.hpp"


PhotoProperties::PhotoProperties(QWidget* p):
    QScrollArea(p),
    m_selectionExtractor(nullptr),
    m_locationLabel(new QLabel(this)),
    m_sizeLabel(new QLabel(this)),
    m_geometryLabel(new QLabel(this)),
    m_locationValue(new QLabel(this)),
    m_sizeValue(new QLabel(this)),
    m_geometryValue(new QLabel(this))
{
    QWidget* area = new QWidget(this);
    QGridLayout* l = new QGridLayout(area);

    l->addWidget(m_locationLabel, 0, 0);
    l->addWidget(m_sizeLabel, 1, 0);
    l->addWidget(m_geometryLabel, 2, 0);

    l->addWidget(m_locationValue, 0, 1);
    l->addWidget(m_sizeValue, 1, 1);
    l->addWidget(m_geometryValue, 2, 1);

    setWidgetResizable(true);
    setWidget(area);
}


PhotoProperties::~PhotoProperties()
{

}


void PhotoProperties::set(const SelectionExtractor* selection)
{
    m_selectionExtractor = selection;

    connect(m_selectionExtractor, &SelectionExtractor::selectionChanged, this, &PhotoProperties::refreshView);
}


void PhotoProperties::refreshView() const
{
    std::vector<IPhotoInfo::Ptr> photos = m_selectionExtractor->getSelection();

    refreshLabels(photos);
    refreshValues(photos);
}


void PhotoProperties::refreshLabels(const std::vector<IPhotoInfo::Ptr>& photos) const
{
    const std::size_t s = photos.size();

    if (s < 2)
    {
        m_locationLabel->setText(tr("Photo location:"));
        m_sizeLabel->setText(tr("Photo size:"));
        m_geometryLabel->setText(tr("Photo geometry:"));
    }
    else
    {
        m_locationLabel->setText(tr("Photos location:"));
        m_sizeLabel->setText(tr("Photos size:"));
        m_geometryLabel->setText("Photos geometry:");
    }
}


void PhotoProperties::refreshValues(const std::vector<IPhotoInfo::Ptr>& photos) const
{
    const std::size_t s = photos.size();

    // calcualte photos size
    std::size_t size = 0;
    for(std::size_t i = 0; i < s; i++)
    {
        const QFileInfo info(photos[i]->getPath());
        size += info.size();
    }

    const QString size_human = sizeHuman(size);

    // handle various cases
    if (s == 0)
    {
        m_locationValue->setText("---");
        m_sizeValue->setText("---");
        m_geometryValue->setText("---");
    }
    else if (s == 1)
    {
        const IPhotoInfo::Ptr& photo = photos.front();
        const QString filePath = photo->getPath();
        const QFileInfo filePathInfo(filePath);
        const QSize geometry = photo->getGeometry();
        const QString geometry_str = tr("%1x%2").arg(geometry.width()).arg(geometry.height());

        // update values
        m_locationValue->setText(filePathInfo.absoluteFilePath());
        m_sizeValue->setText(size_human);
        m_geometryValue->setText(geometry_str);
    }
    else
    {
        // 'merge' paths
        const QFileInfo firstPathInfo(photos.front()->getPath());
        QString result = firstPathInfo.path();          // do not include file name. It will prevent situations when merged path contains part of file names (if they had common part)

        for(std::size_t i = 1; i < s; i++)
        {
            const QFileInfo anotherPhotoPath(photos[i]->getPath());
            result = FileSystem().commonPath(result, anotherPhotoPath.path());
        }

        const QString decorated = result + (result.right(1) == QDir::separator()? QString("") : QDir::separator()) + "...";

        // update values
        m_locationValue->setText(decorated);
        m_sizeValue->setText(size_human);
        m_geometryValue->setText("---");
    }
}


QString PhotoProperties::sizeHuman(int size) const
{
    int i = 0;
    for(; i < 4 && size > 20480; i++)
        size /= 1024.0;

    QString units;
    switch (i)
    {
        case 0:  units = tr("%n byte(s)",  "", size); break;
        case 1:  units = tr("%n kbyte(s)", "", size); break;
        case 2:  units = tr("%n Mbyte(s)", "", size); break;
        default: units = tr("%n Gbyte(s)", "", size); break;
    }

    return units;
}
