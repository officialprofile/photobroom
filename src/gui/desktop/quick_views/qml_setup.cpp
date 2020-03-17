

#include "qml_setup.hpp"

#include "photo.hpp"

void register_qml_types()
{
    qmlRegisterType<PhotoItem>("photo_broom.qml", 1, 0, "Photo");
    qmlRegisterType<PhotoPropertiesObject>("photo_broom.qml", 1, 0, "PhotoProperties");
    qmlRegisterInterface<IThumbnailsManager>("IThumbnailsManager");

}
