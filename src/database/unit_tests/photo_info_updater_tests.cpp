
#include <gmock/gmock.h>

#include "database_tools/implementation/photo_info_updater.hpp"
#include "database_tools/implementation/photos_analyzer_constants.hpp"
#include "unit_tests_utils/empty_logger.hpp"
#include "unit_tests_utils/fake_task_executor.hpp"
#include "unit_tests_utils/mock_backend.hpp"
#include "unit_tests_utils/mock_core_factory_accessor.hpp"
#include "unit_tests_utils/mock_configuration.hpp"
#include "unit_tests_utils/mock_database.hpp"
#include "unit_tests_utils/mock_logger_factory.hpp"
#include "unit_tests_utils/mock_media_information.hpp"
#include "unit_tests_utils/printers.hpp"


using testing::_;
using testing::An;
using testing::Invoke;
using testing::Return;
using testing::ReturnRef;
using testing::NiceMock;
using namespace PhotosAnalyzerConsts;

TEST(PhotoInfoUpdaterTest, tagsUpdate)
{
    FakeTaskExecutor taskExecutor;
    NiceMock<MockBackend> backend;
    NiceMock<ILoggerFactoryMock> loggerFactoryMock;
    NiceMock<IConfigurationMock> configurationMock;
    NiceMock<ICoreFactoryAccessorMock> coreFactory;
    NiceMock<MockDatabase> db;
    NiceMock<MediaInformationMock> mediaInformation;

    ON_CALL(mediaInformation, getInformation).WillByDefault(Return(FileInformation{}));
    ON_CALL(coreFactory, getConfiguration).WillByDefault(ReturnRef(configurationMock));
    ON_CALL(coreFactory, getLoggerFactory).WillByDefault(ReturnRef(loggerFactoryMock));
    ON_CALL(coreFactory, getTaskExecutor).WillByDefault(ReturnRef(taskExecutor));
    ON_CALL(loggerFactoryMock, get(An<const QString &>())).WillByDefault(Invoke([](const auto &)
    {
        return std::make_unique<EmptyLogger>();
    }));

    ON_CALL(db, execute).WillByDefault(Invoke([&backend](std::unique_ptr<Database::IDatabase::ITask>&& task)
    {
        task->run(backend);
    }));

    // orignal state of photo
    Photo::DataDelta photo(Photo::Id(123));
    photo.insert<Photo::Field::Path>("/path/to/file.jpeg");
    photo.insert<Photo::Field::Flags>( std::map<Photo::FlagsE, int>{ {Photo::FlagsE::StagingArea, 1}, {Photo::FlagsE::GeometryLoaded, 2} } );
    photo.insert<Photo::Field::Tags>( std::map<Tag::Types, TagValue>{ {Tag::Types::Event, TagValue::fromType<Tag::Types::Event>("qweasd")}, {Tag::Types::Rating, 5} } );

    // expected state after calling updater
    Photo::DataDelta newPhotoData(photo);
    newPhotoData.get<Photo::Field::Flags>()[Photo::FlagsE::ExifLoaded] = ExifFlagVersion;

    PhotoInfoUpdater updater(taskExecutor, mediaInformation, &coreFactory, db);

    auto sharedData = std::make_shared<Photo::SafeDataDelta>(photo);
    updater.updateTags(sharedData);

    const auto lockedData = sharedData->lock();
    EXPECT_EQ(*lockedData, newPhotoData);
}
