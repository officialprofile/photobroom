/*
 * Photo Broom - photos management tool.
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

#ifndef FFMPEGVIDEODETAILSREADER_HPP
#define FFMPEGVIDEODETAILSREADER_HPP

#include <optional>

#include <QDateTime>
#include <QSize>
#include <QStringList>

// Class is reetrant.
// All its methods may take a while as ffmpeg is called inside

class FFMpegVideoDetailsReader
{
    public:
        explicit FFMpegVideoDetailsReader(const QString& ffprobePath, const QString& path);
        FFMpegVideoDetailsReader(const FFMpegVideoDetailsReader &) = delete;
        FFMpegVideoDetailsReader(FFMpegVideoDetailsReader &&) = delete;

        FFMpegVideoDetailsReader& operator=(const FFMpegVideoDetailsReader &) = delete;
        FFMpegVideoDetailsReader& operator=(FFMpegVideoDetailsReader &&) = delete;

        virtual ~FFMpegVideoDetailsReader() = default;

        bool hasDetails() const;                        // checks if given file contains any data to be read

        std::optional<QSize> resolutionOf() const;
        int durationOf() const;                         // video duration in seconds
        std::optional<QDateTime> creationTime() const;  // creation time from video metadata

    private:
        const QString m_ffprobePath;
        const QStringList m_output;

        QStringList outputFor(const QString &) const;
        int rotation() const;
};

#endif // FFMPEGVIDEODETAILSREADER_HPP
