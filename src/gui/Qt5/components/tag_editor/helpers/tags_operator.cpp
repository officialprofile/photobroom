/*
 * Tags operator
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

#include "tags_operator.hpp"


TagsOperator::TagsOperator(): m_tagUpdaters()
{

}


void TagsOperator::operateOn(const std::vector<IPhotoInfo::Ptr>& photos)
{
    m_tagUpdaters.clear();

    for (const IPhotoInfo::Ptr& photo: photos)
        m_tagUpdaters.emplace_back(photo);
}


Tag::TagsList TagsOperator::getTags() const
{
    Tag::TagsList tags;

    for (const TagUpdater& tagUpdater: m_tagUpdaters)
    {
        const Tag::TagsList l_tags = tagUpdater.getTags();

        for (auto it = l_tags.begin(); it != l_tags.end(); ++it)
        {
            auto f_it = tags.find(it->first);      //check if this tag already exists in main set of tags

            if (f_it != tags.end())  //it does
            {
                //check if values are the same
                Tag::Info info_it(it);
                Tag::Info info_f_it(f_it);

                if (info_it.valuesString() != info_f_it.valuesString())
                {
                    TagValue new_value( { "<multiple values>" } );
                    f_it->second = new_value;
                }
            }
            else
                tags.insert(*it);
        }
    }

    return tags;
}


void TagsOperator::setTag(const TagNameInfo& name, const TagValue& values)
{
    for (TagUpdater& updater: m_tagUpdaters)
        updater.setTag(name, values);
}


void TagsOperator::setTags(const Tag::TagsList& tags)
{
    for (TagUpdater& updater: m_tagUpdaters)
        updater.setTags(tags);
}


void TagsOperator::updateTag(const QString& name, const QString& rawList)
{
    //find tag for given name
    Tag::TagsList tags = getTags();
    bool updated = false;

    for(Tag::Info info: tags)
    {
        if (info.name() == name)
        {
            info.setRawValues(rawList);
            setTag(info.getTypeInfo(), info.values());

            updated = true;
            break;
        }
    }

    assert(updated);
}