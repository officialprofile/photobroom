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

#include "series_detection.hpp"

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QQuickWidget>

#include <core/icore_factory_accessor.hpp>
#include <core/iexif_reader.hpp>
#include <core/ilogger.hpp>
#include <core/ilogger_factory.hpp>
#include <database/idatabase.hpp>

#include "ui/photos_grouping_dialog.hpp"
#include "quick_views/qml_utils.hpp"


Q_DECLARE_METATYPE(GroupCandidate)

using namespace std::placeholders;


SeriesDetection::SeriesDetection(Database::IDatabase* db,
                                 ICoreFactoryAccessor* core,
                                 IThumbnailsManager* thbMgr,
                                 Project* project):
    QDialog(),
    m_tabModel(*db, *core),
    m_core(core),
    m_db(db),
    m_project(project),
    m_qmlView(nullptr),
    m_thumbnailsManager4QML(thbMgr)
{
    // dialog top layout setup
    resize(320, 480);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QDialogButtonBox* dialog_buttons = new QDialogButtonBox(QDialogButtonBox::Close);

    m_qmlView = new QQuickWidget(this);
    m_qmlView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_qmlView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    QmlUtils::registerObject(m_qmlView, "thumbnailsManager", &m_thumbnailsManager4QML);
    QmlUtils::registerObject(m_qmlView, "groupsModelId", &m_tabModel);
    m_qmlView->setSource(QUrl("qrc:/ui/Dialogs/SeriesDetection.qml"));

    layout->addWidget(m_qmlView);
    layout->addWidget(dialog_buttons);

    // wiring
    QObject* seriesDetectionMainId = QmlUtils::findQmlObject(m_qmlView, "seriesDetectionMain");
    connect(seriesDetectionMainId, SIGNAL(group(int)), this, SLOT(group(int)));

    connect(dialog_buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
}


SeriesDetection::~SeriesDetection()
{
    // delete qml view before all other objects it referes to will be deleted
    delete m_qmlView;
}


void SeriesDetection::group(int row)
{
    const QModelIndex firstItemInRow = m_tabModel.index(row, 0);
    const GroupCandidate groupDetails = firstItemInRow.data(SeriesModel::DetailsRole).value<GroupCandidate>();
    launch_groupping_dialog(groupDetails);
}


void SeriesDetection::launch_groupping_dialog(const GroupCandidate& candidate)
{
    auto logger = m_core->getLoggerFactory().get("PhotosGrouping");

    PhotosGroupingDialog pgd(
        candidate.members,
        m_core->getExifReaderFactory(),
        m_core->getTaskExecutor(),
        m_core->getConfiguration(),
        logger.get(),
        candidate.type
    );

    const int exit_code = pgd.exec();

    if (exit_code == QDialog::Accepted)
    {
        PhotosGroupingDialogUtils::GroupDetails details;
        details.photos = pgd.photos();
        details.representativePhoto = pgd.getRepresentative();
        details.type = pgd.groupType();

        PhotosGroupingDialogUtils::createGroup(details, m_project, m_db);
    }
}


int SeriesDetection::selected_row() const
{
    return 0;
}
