
#ifndef NOTIFICATIONSACCUMULATOR_HPP
#define NOTIFICATIONSACCUMULATOR_HPP

#include <QObject>
#include "photo_types.hpp"

namespace Database
{
    class NotificationsAccumulator final: public QObject
    {
            Q_OBJECT

        public:
            NotificationsAccumulator() = default;

            void photosAdded(const std::vector<Photo::Id> &);

            void fireChanges();
            void ignoreChanges();

        private:
            std::vector<Photo::Id> m_photosAdded;

        signals:
            void photosAddedSignal(const std::vector<Photo::Id> &) const;
    };
}

#endif
