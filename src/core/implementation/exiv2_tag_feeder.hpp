
#ifndef TAG_FEEDER_HPP
#define TAG_FEEDER_HPP

#include <memory>
#include <string>

#include <exiv2/exiv2.hpp>

#include "atagfeeder.hpp"

class Exiv2TagFeeder: public ATagFeeder
{
    public:
        Exiv2TagFeeder();

    private:
        virtual void collect(const QByteArray&);
        virtual std::string get(TagTypes);

        Exiv2::Image::AutoPtr m_exif_data;
};

#endif