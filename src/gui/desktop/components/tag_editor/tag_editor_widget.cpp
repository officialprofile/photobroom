/*
    Widget for tags manipulation
    Copyright (C) 2013  Michał Walenciak <MichalWalenciak@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "tag_editor_widget.hpp"

#include <QVBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

#include <core/base_tags.hpp>

#include "helpers/tags_view.hpp"
#include "helpers/tags_model.hpp"


TagEditorWidget::TagEditorWidget(QWidget* p, Qt::WindowFlags f):
    QWidget(p, f),
    m_view(nullptr),
    m_model(nullptr),
    m_tagsOperator(),
    m_tagName(nullptr),
    m_tagValue(nullptr),
    m_addButton(nullptr),
    m_tags()
{
    m_view = new TagsView(this);
    m_model = new TagsModel(this);
    m_tagName = new QComboBox(this);
    m_tagValue = new QLineEdit(this);
    m_addButton = new QPushButton(QIcon(":/gui/add-img.svg"), "", this);

    m_view->setModel(m_model);
    m_model->set(&m_tagsOperator);

    QHBoxLayout* hl = new QHBoxLayout;
    hl->addWidget(m_tagName);
    hl->addWidget(m_tagValue);
    hl->addWidget(m_addButton);

    QVBoxLayout* l = new QVBoxLayout(this);
    l->addWidget(m_view);
    l->addLayout(hl);

    connect(m_model, SIGNAL(modelChanged(bool)), this, SLOT(refreshTagNamesList(bool)));
    connect(m_addButton, SIGNAL(clicked(bool)), this, SLOT(addButtonPressed()));

    //initial refresh
    refreshTagNamesList(false);
}


TagEditorWidget::~TagEditorWidget()
{

}


void TagEditorWidget::set(QItemSelectionModel* selectionModel)
{
    m_model->set(selectionModel);
}


void TagEditorWidget::set(DBDataModel* dbDataModel)
{
    m_model->set(dbDataModel);
}


void TagEditorWidget::refreshTagNamesList(bool selection)
{
    bool enable_gui = selection;

    m_tagName->clear();
    m_tags.clear();

    if (selection)
    {
        const auto all_tags = BaseTags::getAll();
        const auto photos_tags = m_model->getTags();

        //remove used tags from list of all tags
        for(const TagNameInfo& info: all_tags)
        {
            const auto it = photos_tags.find(info);

            if (it == photos_tags.end())    //not used?
            {
                const int idx = m_tags.size();
                m_tags.push_back(info);
                m_tagName->addItem(info.getDisplayName(), idx);
            }
        }

        enable_gui = m_tags.empty() == false;
    }

    m_tagName->setEnabled(enable_gui);
    m_tagValue->setEnabled(enable_gui);
    m_addButton->setEnabled(enable_gui);
}


void TagEditorWidget::addButtonPressed()
{
    const int idx = m_tagName->currentIndex();

    assert(idx >= 0 && static_cast<size_t>(idx) < m_tags.size());

    const TagNameInfo& name = m_tags[idx];
    const QString value = m_tagValue->text();

    m_model->addTag(name, value);
}