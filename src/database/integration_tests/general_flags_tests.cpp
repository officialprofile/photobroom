
#include <gmock/gmock.h>

#include "common.hpp"


struct GeneralFlagsTest: Tests::DatabaseTest
{
    GeneralFlagsTest(): Tests::DatabaseTest()
    {

    }

    ~GeneralFlagsTest()
    {

    }
};


TEST_F(GeneralFlagsTest, flagsIntroduction)
{
    for_all([](Database::IDatabase* db)
    {
        db->performCustomAction([](Database::IBackendOperator* op)
        {
            // store 2 photos
            Photo::DataDelta pd1, pd2;
            pd1.data[Photo::Field::Path] = QString("photo1.jpeg");
            pd2.data[Photo::Field::Path] = QString("photo2.jpeg");

            std::vector<Photo::Id> ids;
            std::vector<Photo::DataDelta> photos = { pd1, pd2 };
            op->addPhotos(photos);

            ids.push_back(photos.front().id);
            ids.push_back(photos.back().id);

            op->set(ids[0], "test1", 1);
            op->set(ids[0], "test2", 2);
            op->set(ids[1], "test3", 3);
            op->set(ids[1], "test4", 4);

            EXPECT_EQ(op->get(ids[0], "test2"), 2);
            EXPECT_EQ(op->get(ids[0], "test1"), 1);
            EXPECT_EQ(op->get(ids[1], "test4"), 4);
            EXPECT_EQ(op->get(ids[1], "test3"), 3);
        });
    });
}
