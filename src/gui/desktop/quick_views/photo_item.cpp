/*
 * Copyright (C) 2020  Michał Walenciak <Kicer86@gmail.com>
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

#include "photo_item.hpp"

#include <QPainter>


using namespace std::placeholders;

PhotoItem::PhotoItem(QQuickItem* parent)
    : QQuickPaintedItem(parent)
    , m_image()
    , m_source()
    , m_thbMgr(nullptr)
    , m_state(State::NotFetched)
    , m_photoWidth(0)
    , m_photoHeight(0)
{

}


PhotoItem::~PhotoItem()
{
    m_callback_ctrl.invalidate();
}


void PhotoItem::paint(QPainter *painter)
{
    if (m_thbMgr == nullptr || m_photoHeight == 0 || m_photoWidth == 0 || m_source.isEmpty())
        return;

    if (m_state == State::NotFetched)
        fetchImage();

    if (m_image.isNull() == false)
        paintImage(*painter);
}


void PhotoItem::setThumbnailsManager(IThumbnailsManager* mgr)
{
    m_thbMgr = mgr;
}


void PhotoItem::setSource(const QString& source)
{
    m_source = source;

    update();
}


void PhotoItem::setPhotoWidth(int w)
{
    m_photoWidth = w;

    update();
}


void PhotoItem::setPhotoHeight(int h)
{
    m_photoHeight = h;

    update();
}


IThumbnailsManager* PhotoItem::thumbnailsManager() const
{
    return m_thbMgr;
}


QString PhotoItem::source() const
{
    return m_source;
}


int PhotoItem::photoWidth() const
{
    return m_photoWidth;
}


int PhotoItem::photoHeight() const
{
    return m_photoHeight;
}


bool PhotoItem::ready() const
{
    return m_state == State::Fetched;
}


void PhotoItem::gotThumbnail(const QImage& image)
{
    setImage(image);
    setState(State::Fetched);
    update();
}


void PhotoItem::setImage(const QImage& image)
{
    if (image.isNull())
        m_image.load(":/gui/error.svg");
    else
        m_image = image;
}


void PhotoItem::setState(PhotoItem::State state)
{
    const bool changed = state != m_state;
    m_state = state;

    if (changed)
        emit stateChanged();
}


void PhotoItem::paintImage(QPainter& painter) const
{
    assert(m_image.isNull() == false);

    const QRectF canvas(0.0, 0.0, width(), height());
    const QRectF photo(QPointF(0.0, 0.0), m_image.size());

    QRectF photoPart = canvas;
    photoPart.moveCenter(photo.center());

    painter.drawImage(canvas.topLeft(), m_image, photoPart);
}


void PhotoItem::fetchImage()
{
    const QSize thbSize = calculateThumbnailSize();
    const int h = thbSize.height();

    auto image = m_thbMgr->fetch(m_source, h);

    if (image.has_value())
    {
        setImage(image.value());
        setState(State::Fetched);
    }
    else
    {
        setState(State::Fetching);
        auto callback = m_callback_ctrl.make_safe_callback<const QImage &>(std::bind(&PhotoItem::gotThumbnail, this, _1));
        m_thbMgr->fetch(m_source, h, callback);
    }
}


QSize PhotoItem::calculateThumbnailSize() const
{
    const int h = static_cast<int>(height());
    const int w = static_cast<int>(width());

    QSize thbSize(m_photoWidth, m_photoHeight);
    thbSize.scale(w, h, Qt::KeepAspectRatioByExpanding);

    return thbSize;
}
