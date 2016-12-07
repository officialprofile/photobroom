
/*
* Base for exif readers
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

#ifndef A_EXIF_READER_HPP
#define A_EXIF_READER_HPP

#include "iexif_reader.hpp"

struct IPhotosManager;
class QByteArray;

class AExifReader: public IExifReader
{
    public:
        AExifReader(IPhotosManager *);
        AExifReader (const AExifReader &) = delete;
        virtual ~AExifReader();

        AExifReader& operator=(const AExifReader &) = delete;

    protected:
        enum TagTypes
        {
            SequenceNumber,

            DateTimeOriginal,
        };

        virtual void collect(const QByteArray &) = 0;
        virtual std::string read (TagTypes) = 0;

    private:
        IPhotosManager* m_photosManager;

        // ITagFeeder:
        Tag::TagsList getTagsFor(const QString& path) override;
        boost::any get(const QString& path, const ExtraData &) override;
        //

        void feedDateAndTime(Tag::TagsList &);
};

#endif // A_EXIF_READER_HPP
