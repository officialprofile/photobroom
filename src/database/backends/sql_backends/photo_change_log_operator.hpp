/*
 * Class reponsible for managing photo Change Log management
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

#ifndef PHOTOCHANGELOGOPERATOR_HPP
#define PHOTOCHANGELOGOPERATOR_HPP

#include <QString>

#include <database/photo_types.hpp>

struct ILogger;

namespace Database
{
    struct IBackend;
    struct IGenericSqlQueryGenerator;
    struct ISqlQueryExecutor;

    class PhotoChangeLogOperator final
    {
        public:
            PhotoChangeLogOperator(const QString &, const IGenericSqlQueryGenerator *, Database::ISqlQueryExecutor *, ILogger *, IBackend *);
            ~PhotoChangeLogOperator();


        private:
            QString m_connectionName;
            const IGenericSqlQueryGenerator* m_queryGenerator;
            ISqlQueryExecutor* m_executor;
            ILogger* m_logger;
            IBackend* m_backend;

            enum Operation
            {
                Add     = 1,
                Modify  = 2,
                Remove  = 3,
            };

            enum Field
            {
                Tags    = 1,
            };

            void append(const Photo::Id &, Operation, Field, const QString& data);
    };
}

#endif // PHOTOCHANGELOGOPERATOR_HPP
