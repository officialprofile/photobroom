
#include "photo_info.hpp"

#include <assert.h>

#include <QPixmap>
#include <QImage>


//TODO: use interface
#include "photo_loader.hpp"

namespace
{
    //TODO: remove, use config
    const int photoWidth = 120;

    PhotoLoader photoLoader;
}


PhotoInfo::PhotoInfo(const std::string &path, ThreadMultiplexer::IGetter* getter):
    APhotoInfo(path),
    m_thumbnail(new QPixmap),
    m_thumbnailRaw(nullptr),
    m_multpilexer()
{
    m_multpilexer.setGetter(getter);
    load();
}


PhotoInfo::~PhotoInfo()
{
    delete m_thumbnail;
    delete m_thumbnailRaw;
}


RawPhotoData PhotoInfo::rawPhotoData()
{
    //TODO: introduce some cache
    const QPixmap photo = getPhoto();
    QImage image = photo.toImage();

    RawPhotoData data;

    data.data = image.bits();
    data.size = image.byteCount();

    return data;
}


RawPhotoData PhotoInfo::rawThumbnailData()
{
    RawPhotoData data;

    if (m_thumbnailRaw == nullptr)
    {
        m_thumbnailRaw = new QImage;
        *m_thumbnailRaw = m_thumbnail->toImage();
    }

    data.data = m_thumbnailRaw->bits();
    data.size = m_thumbnailRaw->byteCount();

    return data;
}


const QPixmap PhotoInfo::getPhoto() const
{
    const std::string path = APhotoInfo::getPath();
    const QPixmap pixmap(path.c_str());

    return pixmap;
}


const QPixmap &PhotoInfo::getThumbnail() const
{
    return *m_thumbnail;
}


void PhotoInfo::load()
{
    photoLoader.generateThumbnail(APhotoInfo::getPath().c_str(), this);
    m_thumbnail->load(":/gui/images/clock64.png");
}


void PhotoInfo::thumbnailReady(const QString& path)
{
    *m_thumbnail = photoLoader.getThumbnailFor(path);

    m_multpilexer.send(this);
}