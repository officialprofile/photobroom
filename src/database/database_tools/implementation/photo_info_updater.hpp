
#ifndef GUI_PHOTO_INFO_UPDATER_HPP
#define GUI_PHOTO_INFO_UPDATER_HPP

#include <mutex>
#include <condition_variable>
#include <QCache>
#include <QTimer>

#include <core/exif_reader_factory.hpp>
#include <core/itask_executor.hpp>
#include <core/media_information.hpp>
#include <database/idatabase.hpp>

struct ICoreFactoryAccessor;

struct UpdaterTask;

//TODO: construct photo manualy. Add fillers manualy on demand
class PhotoInfoUpdater final: public QObject
{
    public:
        explicit PhotoInfoUpdater(ITaskExecutor &, IMediaInformation &, ICoreFactoryAccessor *, Database::IDatabase& db);
        ~PhotoInfoUpdater();

        PhotoInfoUpdater(const PhotoInfoUpdater &) = delete;
        PhotoInfoUpdater& operator=(const PhotoInfoUpdater &) = delete;

        void updateGeometry(const Photo::SharedDataDelta &);
        void updateTags(const Photo::SharedDataDelta &);
        void updatePHash(const Photo::SharedDataDelta &);

    private:
        friend struct UpdaterTask;

        QCache<QString, FileInformation> m_fileInfos;
        std::mutex m_fileInfosMutex;
        std::unique_ptr<ILogger> m_logger;
        IMediaInformation& m_mediaInformation;
        ICoreFactoryAccessor* m_coreFactory;
        Database::IDatabase& m_db;
        ITaskExecutor& m_tasksExecutor;

        void addTask(std::unique_ptr<UpdaterTask>);
        void apply(std::function<void(Database::IBackend &)>);
        FileInformation getFileInformation(const QString &);
};

#endif
