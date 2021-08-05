/*
 * Photo Broom - photos management tool.
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


#include <unordered_set>
#include <QDateTime>

#include <core/iexif_reader.hpp>
#include <core/media_types.hpp>
#include <core/task_executor_utils.hpp>
#include <core/tags_utils.hpp>
#include <ibackend.hpp>
#include <iphoto_operator.hpp>

#include "database_executor_traits.hpp"
#include "../series_detector.hpp"
#include "photo_utils.hpp"

namespace
{
    template<Group::Type>
    class GroupValidator;

    class abort_exception: public std::exception {};

    int readExposure(const std::any& exposure)
    {
        const float exp = std::any_cast<float>(exposure);

        return static_cast<int>(exp * 100);   // convert original exposure to centi-exposure
    }

    template<>
    class GroupValidator<Group::Type::Animation>
    {
    public:
        GroupValidator(IExifReader& exif, const SeriesDetector::Rules &)
            : m_exifReader(exif)
        {

        }

        void setCurrentPhoto(const Photo::DataDelta& d)
        {
            m_sequence = m_exifReader.get(d.get<Photo::Field::Path>(), IExifReader::TagType::SequenceNumber);
        }

        bool canBePartOfGroup()
        {
            const bool has_exif_data = m_sequence.has_value();

            if (has_exif_data)
            {
                const int s = std::any_cast<int>(m_sequence.value());
                auto s_it = m_sequence_numbers.find(s);

                return s_it == m_sequence_numbers.end();
            }
            else
                return false;
        }

        void accept()
        {
            assert(m_sequence);

            const int s = std::any_cast<int>(m_sequence.value());

            m_sequence_numbers.insert(s);
        }

        std::optional<std::any> m_sequence;

        std::unordered_set<int> m_sequence_numbers;
        IExifReader& m_exifReader;
    };

    template<>
    class GroupValidator<Group::Type::HDR>: GroupValidator<Group::Type::Animation>
    {
        typedef GroupValidator<Group::Type::Animation> Base;

    public:
        GroupValidator(IExifReader& exif, const SeriesDetector::Rules& r)
            : Base(exif, r)
        {

        }

        void setCurrentPhoto(const Photo::DataDelta& d)
        {
            Base::setCurrentPhoto(d);
            m_exposure = m_exifReader.get(d.get<Photo::Field::Path>(), IExifReader::TagType::Exposure);
        }

        bool canBePartOfGroup()
        {
            const bool has_exif_data = Base::canBePartOfGroup() && m_exposure;

            if (has_exif_data)
            {
                const int e = readExposure(m_exposure.value());

                auto e_it = m_exposures.find(e);

                return e_it == m_exposures.end();
            }
            else
                return false;
        }

        void accept()
        {
            assert(m_exposure);

            Base::accept();

            const int e = readExposure(m_exposure.value());
            m_exposures.insert(e);
        }

        std::optional<std::any> m_exposure;
        std::unordered_set<int> m_exposures;
    };

    template<>
    class GroupValidator<Group::Type::Generic>
    {
    public:
        GroupValidator(IExifReader &, const SeriesDetector::Rules& r)
            : m_prev_stamp(0)
            , m_rules(r)
        {

        }

        void setCurrentPhoto(const Photo::DataDelta& d)
        {
            m_current_stamp = Tag::timestamp(d.get<Photo::Field::Tags>());
        }

        bool canBePartOfGroup()
        {
            return m_prev_stamp.count() == 0 || m_current_stamp - m_prev_stamp <= m_rules.manualSeriesMaxGap;
        }

        void accept()
        {
            m_prev_stamp = m_current_stamp;
        }

        std::chrono::milliseconds m_prev_stamp,
                                  m_current_stamp;
        const SeriesDetector::Rules& m_rules;
    };

    class SeriesExtractor
    {
    public:
        SeriesExtractor(IExifReader& exifReader,
                        const std::deque<Photo::DataDelta>& photos,
                        const SeriesDetector::Rules& r,
                        const QPromise<std::vector<GroupCandidate>>* p)
            : m_exifReader(exifReader)
            , m_rules(r)
            , m_photos(photos)
            , m_promise(p)
        {

        }

        template<Group::Type type>
        std::vector<GroupCandidate> extract()
        {
            std::vector<GroupCandidate> results;

            for (auto it = m_photos.begin(); it != m_photos.end();)
            {
                if (m_promise && m_promise->isCanceled())
                    throw abort_exception();

                GroupCandidate group;
                group.type = type;

                GroupValidator<type> validator(m_exifReader, m_rules);

                for (auto it2 = it; it2 != m_photos.end(); ++it2)
                {
                    const Photo::DataDelta& data = *it2;

                    validator.setCurrentPhoto(data);

                    if (validator.canBePartOfGroup())
                    {
                        group.members.push_back(data);
                        validator.accept();
                    }
                    else
                        break;
                }

                const auto members = group.members.size();

                // each photo should have different exposure
                if (members > 1)
                {
                    results.push_back(group);

                    auto first = it;
                    auto last = first + members;

                    it = m_photos.erase(first, last);
                }
                else
                    ++it;
            }

            return results;
        }

    private:
        IExifReader& m_exifReader;
        const SeriesDetector::Rules& m_rules;
        std::deque<Photo::DataDelta> m_photos;
        const QPromise<std::vector<GroupCandidate>>* m_promise;
    };
}


SeriesDetector::Rules::Rules(std::chrono::milliseconds gap)
    : manualSeriesMaxGap(gap)
{

}


SeriesDetector::SeriesDetector(ILogger& logger, Database::IDatabase& db, IExifReader& exif, const QPromise<std::vector<GroupCandidate>>* p)
    : m_logger(logger)
    , m_db(db)
    , m_promise(p)
    , m_exifReader(exif)
{

}


std::vector<GroupCandidate> SeriesDetector::listCandidates(const Rules& rules) const
{
    QElapsedTimer timer;
    timer.start();

    const std::deque<Photo::DataDelta> candidates =
        evaluate<std::deque<Photo::DataDelta>(Database::IBackend &)>(m_db, [](Database::IBackend& backend)
    {
        std::vector<GroupCandidate> result;

        // find photos which are not part of any group
        Database::FilterPhotosWithRole group_filter(Database::FilterPhotosWithRole::Role::Regular);

        const auto photos = backend.photoOperator().onPhotos( {group_filter}, Database::Actions::SortByTimestamp() );

        std::deque<Photo::DataDelta> deltas;
        for (const Photo::Id& id: photos)
        {
            const Photo::DataDelta delta = backend.getPhotoDelta(id, {Photo::Field::Tags, Photo::Field::Path});
            deltas.push_back(delta);
        }

        return deltas;
    });

    const QString log = QString("Collecting photos for grouping took %1s").arg(timer.elapsed() / 1000);

    m_logger.debug(log);

    return analyze_photos(candidates, rules);
}


std::vector<GroupCandidate> SeriesDetector::analyze_photos(const std::deque<Photo::DataDelta>& photos, const Rules& rules) const
{
    using namespace std::chrono_literals;

    QElapsedTimer timer;
    std::deque<Photo::DataDelta> suitablePhotos;

    // grouping works for images only
    timer.start();
    std::copy_if(photos.begin(), photos.end(), std::back_inserter(suitablePhotos), [](const auto& photo) {
        return MediaTypes::isImageFile(Photo::getPath(photo));
    });

    m_logger.debug(QString("Validating media type took %1s").arg(timer.elapsed() / 1000));

    // drop images which are not made close to another one
    timer.start();
    std::deque<Photo::DataDelta> prefiltered;
    for(std::size_t i = 0; i < suitablePhotos.size(); i++)
    {
        std::vector<std::chrono::milliseconds> neighbours;

        const auto currentTimestamp = Tag::timestamp(suitablePhotos[i].get<Photo::Field::Tags>());

        if (currentTimestamp == 0ms)
            continue;

        if (i > 0)
        {
            const auto previousTimestamp = Tag::timestamp(suitablePhotos[i - 1].get<Photo::Field::Tags>());
            neighbours.push_back(previousTimestamp);
        }

        if (i < suitablePhotos.size() - 1)
        {
            const auto nextTimestamp = Tag::timestamp(suitablePhotos[i + 1].get<Photo::Field::Tags>());
            neighbours.push_back(nextTimestamp);
        }

        const bool any = std::any_of(neighbours.begin(), neighbours.end(), [&currentTimestamp, rules](const auto& neighbourTimestamp)
        {
            return std::chrono::abs(currentTimestamp - neighbourTimestamp) <= rules.manualSeriesMaxGap;
        });

        if (any)
            prefiltered.push_back(suitablePhotos[i]);
    }

    try
    {
        SeriesExtractor extractor(m_exifReader, prefiltered, rules, m_promise);

        timer.restart();
        auto hdrs = extractor.extract<Group::Type::HDR>();
        m_logger.debug(QString("HDRs extraction took: %1s").arg(timer.elapsed() / 1000));

        timer.restart();
        auto animations = extractor.extract<Group::Type::Animation>();
        m_logger.debug(QString("Animations extraction took: %1s").arg(timer.elapsed() / 1000));

        timer.restart();
        auto generics = extractor.extract<Group::Type::Generic>();
        m_logger.debug(QString("Generic groups extraction took: %1s").arg(timer.elapsed() / 1000));

        timer.restart();
        std::vector<GroupCandidate> sequences;
        std::copy(hdrs.begin(), hdrs.end(), std::back_inserter(sequences));
        std::copy(animations.begin(), animations.end(), std::back_inserter(sequences));
        std::copy(generics.begin(), generics.end(), std::back_inserter(sequences));
        m_logger.debug(QString("Finalization took: %1s").arg(timer.elapsed() / 1000));

        return sequences;
    }
    catch (const abort_exception &)
    {
        return {};
    }
}
