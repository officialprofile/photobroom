
#include <gtest/gtest.h>

#include <unit_tests_utils/empty_logger.hpp>
#include "face_recognition/dlib_wrapper/dlib_face_recognition_api.hpp"
#include "utils.hpp"



namespace
{
    EmptyLogger logger;

    void faceLocationTest(const QImage& img)
    {
        dlib_api::FaceLocator faceLocator(&logger);

        QVector facesCnn = faceLocator.face_locations_cnn(img, 0);
        QVector facesHog = faceLocator.face_locations_hog(img, 0);

        ASSERT_EQ(facesCnn.size(), 1);
        ASSERT_EQ(facesHog.size(), 1);

        const QRect faceCnn = facesCnn.front();
        const QRect faceHog = facesHog.front();
        const QRect faceCommon = faceCnn.intersected(faceHog);

        const double faceCommonArea = faceCommon.width() * faceCommon.height();
        const double faceCnnArea = faceCnn.width() * faceCnn.height();
        const double faceHogArea = faceHog.width() * faceHog.height();

        // expecting common area to be 70% of each result
        EXPECT_GT(faceCommonArea/faceCnnArea, 0.70);
        EXPECT_GT(faceCommonArea/faceHogArea, 0.70);
    }
}


TEST(FaceLocationTest, hogVsCnn)
{
    const QImage img1(utils::photoPath(1));
    const QImage img2(utils::photoPath(2));

    faceLocationTest(img1);
    faceLocationTest(img2);
}
