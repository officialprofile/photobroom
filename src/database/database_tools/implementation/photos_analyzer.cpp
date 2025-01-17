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

#include <core/function_wrappers.hpp>
#include <core/icore_factory_accessor.hpp>
#include <core/itask_executor.hpp>
#include <core/slicer.hpp>
#include <database/iphoto_operator.hpp>
#include <database/general_flags.hpp>
#include <database/database_executor_traits.hpp>

#include "photos_analyzer_p.hpp"
#include "photos_analyzer_constants.hpp"
#include "../photos_analyzer.hpp"


using namespace std::chrono_literals;
using namespace std::placeholders;
using namespace PhotosAnalyzerConsts;

PhotosAnalyzerImpl::PhotosAnalyzerImpl(ICoreFactoryAccessor* coreFactory, Database::IDatabase& database):
    m_taskQueue(&coreFactory->getTaskExecutor()),
    m_mediaInformation(coreFactory),
    m_updater(m_taskQueue, m_mediaInformation, coreFactory, database),
    m_updateQueue(1000, 5s, std::bind(&PhotosAnalyzerImpl::flushQueue, this, _1, _2)),
    m_database(database),
    m_tasksView(nullptr),
    m_viewTask(nullptr),
    m_totalTasks(0),
    m_doneTasks(0)
{
    //check for not fully initialized photos in database
    //TODO: use independent updaters here (issue #102)

    // GeometryLoaded < 1
    Database::FilterPhotosWithFlags geometryFilter;
    geometryFilter.comparison[Photo::FlagsE::GeometryLoaded] = Database::ComparisonOp::Less;
    geometryFilter.flags[Photo::FlagsE::GeometryLoaded] = GeometryFlagVersion;

    // ExifLoaded < 1
    Database::FilterPhotosWithFlags exifFilter;
    exifFilter.comparison[Photo::FlagsE::ExifLoaded] = Database::ComparisonOp::Less;
    exifFilter.flags[Photo::FlagsE::ExifLoaded] = ExifFlagVersion;

    // group flag filters
    Database::GroupFilter flagsFilter = {geometryFilter, exifFilter};
    flagsFilter.mode = Database::LogicalOp::Or;

    // only normal photos
    const Database::FilterPhotosWithGeneralFlag generalFlagsFilter(Database::CommonGeneralFlags::State,
                                                                   static_cast<int>(Database::CommonGeneralFlags::StateType::Normal));

    const Database::GroupFilter noExifOrGeometryFilter = {flagsFilter, generalFlagsFilter};

    // photos with no phash
    Database::FilterPhotosWithPHash phashFilter;
    Database::FilterNotMatchingFilter noPhashFilter(phashFilter);

    // photos with phash_state == 0 (Normal) flag
    Database::FilterPhotosWithGeneralFlag phashGeneralFlagFilter(Database::CommonGeneralFlags::PHashState,
                                                                 static_cast<int>(Database::CommonGeneralFlags::PHashStateType::Normal));

    // group phash filters
    const Database::GroupFilter noPhashFilterGroup = { noPhashFilter, phashGeneralFlagFilter };

    // bind all fitlers together
    Database::GroupFilter filters = {noExifOrGeometryFilter, noPhashFilterGroup};
    filters.mode = Database::LogicalOp::Or;

    m_database.exec([this, filters](Database::IBackend& backend)
    {
        auto photos = backend.photoOperator().getPhotos(filters);

        if (photos.empty() == false)
            invokeMethod(this, &PhotosAnalyzerImpl::addPhotos, photos);

        // as all uninitialized photos were found.
        // start watching for any new photos added later.
        m_backendConnection = connect(&backend, &Database::IBackend::photosAdded,
                                      this, &PhotosAnalyzerImpl::addPhotos);
    },
    "PhotosAnalyzerImpl: fetching nonanalyzed photos"
    );
}


PhotosAnalyzerImpl::~PhotosAnalyzerImpl()
{
    stop();

    if (m_viewTask)
        m_viewTask->finished();
}


void PhotosAnalyzerImpl::set(ITasksView* tasksView)
{
    m_tasksView = tasksView;
}


void PhotosAnalyzerImpl::addPhotos(const std::vector<Photo::Id>& ids)
{
    IViewTask* loadTask = m_tasksView->add(tr("Loading photos needing update"));

    runOn(m_taskQueue, [this, ids, loadTask]()
    {
        const auto count = ids.size();

        try
        {
            int progress = 0;
            loadTask->getProgressBar()->setMinimum(0);
            loadTask->getProgressBar()->setMaximum(static_cast<int>(count));

            slice(ids.begin(), ids.end(), 200, [this, loadTask, &progress](auto first, auto last)
            {
                const std::vector<Photo::DataDelta> photoData =
                    evaluate<std::vector<Photo::DataDelta>(Database::IBackend &)>(m_database, [first, last](Database::IBackend& backend)
                {
                    std::vector<Photo::DataDelta> photos;
                    photos.reserve(static_cast<unsigned>(last - first));

                    std::transform(first, last, std::back_inserter(photos), [&backend](const Photo::Id& id) mutable
                    {
                        return backend.getPhotoDelta(id, {Photo::Field::Flags, Photo::Field::Path, Photo::Field::Tags});
                    });

                    return photos;
                });

                invokeMethod(this, &PhotosAnalyzerImpl::updatePhotos, photoData);
                progress += last - first;

                loadTask->getProgressBar()->setValue(progress);

                m_workState.throwIfAbort();
            });
        }
        catch(const abort_exception &)
        {

        }

        loadTask->finished();
    },
    "PhotosAnalyzerImpl: fetching photo details"
    );
}


void PhotosAnalyzerImpl::updatePhotos(const std::vector<Photo::DataDelta>& photos)
{
    for(const auto& photo: photos)
    {
        auto storage = make_cross_thread_function<Photo::SafeDataDelta *>(this, std::bind(&PhotosAnalyzerImpl::photoUpdated, this, _1));
        Photo::SharedDataDelta sharedDataDelta(new Photo::SafeDataDelta(photo), storage);
        m_totalTasks++;

        if (photo.get<Photo::Field::Flags>().at(Photo::FlagsE::GeometryLoaded) < GeometryFlagVersion)
            m_updater.updateGeometry(sharedDataDelta);

        if (photo.get<Photo::Field::Flags>().at(Photo::FlagsE::ExifLoaded) < ExifFlagVersion)
            m_updater.updateTags(sharedDataDelta);

        if (photo.has(Photo::Field::PHash) == false)
            m_updater.updatePHash(sharedDataDelta);
    }

    refreshView();
}


void PhotosAnalyzerImpl::photoUpdated(Photo::SafeDataDelta* safeData)
{
    const auto delta = *safeData->lock();

    m_updateQueue.push(delta);
    m_doneTasks++;

    refreshView();

    delete safeData;
}


void PhotosAnalyzerImpl::flushQueue(PhotosQueue::ContainerIt first, PhotosQueue::ContainerIt last)
{
    std::vector<Photo::DataDelta> toFlush(first, last);

    m_database.exec([toFlush](Database::IBackend& backend)
    {
        backend.update(toFlush);
    });
}


void PhotosAnalyzerImpl::stop()
{
    disconnect(m_backendConnection);
    m_taskQueue.clear();
    m_workState.abort();
    m_taskQueue.waitForPendingTasks();
}


void PhotosAnalyzerImpl::setupRefresher()
{
    const auto work = m_totalTasks - m_doneTasks;

    if (work > 0 && m_viewTask == nullptr)         //there are tasks but no view task
        m_viewTask = m_tasksView->add(tr("Extracting data from photos"));
    else if (work == 0 && m_viewTask != nullptr)
    {
        m_viewTask->finished();
        m_viewTask = nullptr;
        m_totalTasks = 0;
        m_doneTasks = 0;
    }
}


void PhotosAnalyzerImpl::refreshView()
{
    setupRefresher();

    if (m_viewTask != nullptr)
    {
        IProgressBar* progressBar = m_viewTask->getProgressBar();
        progressBar->setMaximum(m_totalTasks);
        progressBar->setValue(m_doneTasks);
    }
}


///////////////////////////////////////////////////////////////////////////////


PhotosAnalyzer::PhotosAnalyzer(ICoreFactoryAccessor* coreFactory,
                               Database::IDatabase& database)
    : m_data(new PhotosAnalyzerImpl(coreFactory, database))
{

}


PhotosAnalyzer::~PhotosAnalyzer()
{

}


void PhotosAnalyzer::set(ITasksView* tasksView)
{
    m_data->set(tasksView);
}
