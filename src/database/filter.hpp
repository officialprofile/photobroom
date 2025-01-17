/*
 * Photo Broom - photos management tool.
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

#ifndef FILTER_HPP
#define FILTER_HPP

#include <memory>
#include <vector>
#include <variant>

#include <QString>

#include <core/data_ptr.hpp>
#include <core/tag.hpp>
#include <core/search_expression_evaluator.hpp>

#include "database_export.h"
#include "person_data.hpp"

namespace Database
{
    //filters

    struct EmptyFilter;
    struct GroupFilter;
    struct FilterPhotosWithTag;
    struct FilterPhotosWithFlags;
    struct FilterNotMatchingFilter;
    struct FilterPhotosWithId;
    struct FilterPhotosMatchingExpression;
    struct FilterPhotosWithPath;
    struct FilterPhotosWithRole;
    struct FilterPhotosWithPerson;
    struct FilterPhotosWithGeneralFlag;
    struct FilterPhotosWithPHash;
    struct FilterSimilarPhotos;


    typedef std::variant<EmptyFilter,
                         GroupFilter,
                         FilterPhotosWithTag,
                         FilterPhotosWithFlags,
                         FilterNotMatchingFilter,
                         FilterPhotosWithId,
                         FilterPhotosMatchingExpression,
                         FilterPhotosWithPath,
                         FilterPhotosWithRole,
                         FilterPhotosWithPerson,
                         FilterPhotosWithGeneralFlag,
                         FilterPhotosWithPHash,
                         FilterSimilarPhotos
    > Filter;

    enum class ComparisonOp
    {
        Equal,
        Less,
        LessOrEqual,
        Greater,
        GreaterOrEqual,
    };

    enum class LogicalOp
    {
        And,
        Or,
    };

    struct DATABASE_EXPORT EmptyFilter
    {

    };

    struct DATABASE_EXPORT GroupFilter
    {
        GroupFilter(const std::vector<Filter> &);
        GroupFilter(const std::initializer_list<Filter> &);

        std::vector<Filter> filters;
        LogicalOp mode = LogicalOp::And;
    };

    struct DATABASE_EXPORT FilterPhotosWithTag
    {
        Tag::Types tagType;
        TagValue tagValue;
        ComparisonOp valueMode;
        bool includeEmpty;

        FilterPhotosWithTag(const Tag::Types &, const TagValue & = TagValue(), ComparisonOp = ComparisonOp::Equal, bool include_empty = false);
    };

    struct DATABASE_EXPORT FilterPhotosWithFlags
    {
        FilterPhotosWithFlags();
        FilterPhotosWithFlags(const std::map<Photo::FlagsE, int> &);

        ComparisonOp comparisonMode(Photo::FlagsE) const;

        std::map<Photo::FlagsE, int> flags;
        std::map<Photo::FlagsE, ComparisonOp> comparison;
        LogicalOp mode;
    };

    struct DATABASE_EXPORT FilterPhotosWithId
    {
        FilterPhotosWithId();

        Photo::Id filter;
    };

    struct DATABASE_EXPORT FilterPhotosMatchingExpression
    {
        FilterPhotosMatchingExpression(const SearchExpressionEvaluator::Expression &);

        SearchExpressionEvaluator::Expression expression;
    };

    struct DATABASE_EXPORT FilterPhotosWithPath
    {
        explicit FilterPhotosWithPath(const QString &);

        QString path;
    };

    struct DATABASE_EXPORT FilterPhotosWithRole
    {
        enum class Role
        {
            Regular,
            GroupRepresentative,
            GroupMember,
        };

        explicit FilterPhotosWithRole(Role);

        Role m_role;
    };

    struct DATABASE_EXPORT FilterPhotosWithPerson
    {
        explicit FilterPhotosWithPerson(const Person::Id &);

        Person::Id person_id;
    };

    struct DATABASE_EXPORT FilterPhotosWithGeneralFlag
    {
        enum class Mode
        {
            Exact,          // filter photos with exact value
            Bit,            // filter photos with given bits set (ie value AND flag.value == value)
        };

        explicit FilterPhotosWithGeneralFlag(const QString& name, int value, Mode mode = Mode::Exact);

        QString name;
        int value;
        Mode mode;
    };

    struct DATABASE_EXPORT FilterPhotosWithPHash { };

    struct DATABASE_EXPORT FilterSimilarPhotos { };

    struct DATABASE_EXPORT FilterNotMatchingFilter
    {
        template<typename T>
        explicit FilterNotMatchingFilter(const T& f): filter(new Filter(f))
        {

        }

        ol::data_ptr<Filter> filter;
    };

    // helpers

    /**
     * @brief return filter which will filter out broken photos (missing, broken, deleted etc)
     */
    Filter getValidPhotosFilter();
}

#endif // FILTER_HPP
