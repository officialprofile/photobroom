
#ifndef ID_TO_DATA_CONVERTER_HPP_INCLUDED
#define ID_TO_DATA_CONVERTER_HPP_INCLUDED

#include <QObject>

#include <core/function_wrappers.hpp>
#include <database/photo_data.hpp>
#include <database/idatabase.hpp>

#include "database_export.h"


class DATABASE_EXPORT IdToDataConverter: public QObject
{
        Q_OBJECT

    public:
        explicit IdToDataConverter(Database::IDatabase &);
        ~IdToDataConverter();

        void fetchIds(const std::vector<Photo::Id> &);

    private:
        safe_callback_ctrl m_callbackCtrl;
        Database::IDatabase& m_db;

        void storePhotoData(const std::vector<Photo::Data> &);

    signals:
        void photoDataFetched(const std::vector<Photo::Data> &) const;
};


#endif // ID_TO_DATA_CONVERTER_HPP_INCLUDED
