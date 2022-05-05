
#ifndef PHOTO_DATA_FIELDS_HPP_INCLUDED
#define PHOTO_DATA_FIELDS_HPP_INCLUDED

#include "photo_types.hpp"

namespace Photo
{
    enum class Field
    {
        Tags,
        Flags,
        Path,
        Geometry,
        GroupInfo,
        PHash,
    };

    template<Field>
    struct DeltaTypes {};

    template<>
    struct DeltaTypes<Field::Tags>
    {
        typedef Tag::TagsList Storage;
    };

    template<>
    struct DeltaTypes<Field::Flags>
    {
        typedef Photo::FlagValues Storage;
    };

    template<>
    struct DeltaTypes<Field::Path>
    {
        typedef QString Storage;
    };

    template<>
    struct DeltaTypes<Field::Geometry>
    {
        typedef QSize Storage;
    };

    template<>
    struct DeltaTypes<Field::GroupInfo>
    {
        typedef GroupInfo Storage;
    };

    template<>
    struct DeltaTypes<Field::PHash>
    {
        typedef Photo::PHash Storage;
    };
}

#endif