/*
 * Access information about people from db and photo itself.
 * Copyright (C) 2018  Michał Walenciak <Kicer86@gmail.com>
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

#ifndef PEOPLEOPERATOR_HPP
#define PEOPLEOPERATOR_HPP

#include <QObject>
#include <QVector>
#include <QRect>

#include <database/photo_data.hpp>
#include <database/person_data.hpp>


namespace Database
{
    struct IDatabase;
}

struct ICoreFactoryAccessor;


class PeopleOperator final: public QObject
{
        Q_OBJECT

    public:
        typedef std::pair<QRect, QString> FaceInfo;
        typedef std::pair<Photo::Id, QRect> FaceLocation;

        PeopleOperator(const QString& storage, Database::IDatabase *, ICoreFactoryAccessor *);
        ~PeopleOperator();

        // Locate faces on given photo.
        void fetchFaces(const Photo::Id &) const;

        // Try to recognize person with given face.
        // Parameter is a face located by fetchFaces()
        void recognize(const FaceLocation &) const;

        // list all people which were not assigned
        void getUnassignedPeople(const Photo::Id &) const;

        // Store information about people on photo
        void store(const Photo::Id &,
                   const std::vector<FaceInfo>& known_people,
                   const QStringList& unknown_people) const;

    signals:
        void faces(const QVector<QRect> &) const;
        void recognized(const QRect &, const PersonName &) const;
        void unassigned(const Photo::Id &, const QStringList &) const;

    private:
        const QString m_storage;
        Database::IDatabase* m_db;
        ICoreFactoryAccessor* m_coreFactory;
};

#endif // PEOPLEOPERATOR_HPP