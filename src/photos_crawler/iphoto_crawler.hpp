
#ifndef ANALYZER_IPHOTO_CRAWLER_HPP
#define ANALYZER_IPHOTO_CRAWLER_HPP

#include <vector>

#include "photos_crawler_export.h"

class QString;


struct PHOTOS_CRAWLER_EXPORT IMediaNotification
{
    virtual ~IMediaNotification() = default;

    virtual void found(const QString &) = 0;
    virtual void finished() = 0;
};

struct PHOTOS_CRAWLER_EXPORT IPhotoCrawler
{
    virtual ~IPhotoCrawler() = default;

    virtual void crawl(const QString &, IMediaNotification *) = 0;   // find media files for given path. Notify about each result
    virtual void stop() = 0;                                         // stop crawling
};

#endif
