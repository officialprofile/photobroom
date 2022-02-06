
#ifndef SELECTIONTOPHOTOIDTRANSLATOR_HPP
#define SELECTIONTOPHOTOIDTRANSLATOR_HPP

#include <QObject>

#include <core/function_wrappers.hpp>
#include <database/photo_data.hpp>
#include <database/idatabase.hpp>


class SelectionToPhotoDataTranslator: public QObject
{
        Q_OBJECT

    public:
        explicit SelectionToPhotoDataTranslator(Database::IDatabase &);
        ~SelectionToPhotoDataTranslator();

        void selectedPhotos(const std::vector<Photo::Id> &);

        std::vector<Photo::Data> getSelectedDatas() const;

    private:
        safe_callback_ctrl m_callbackCtrl;
        mutable ol::ThreadSafeResource<std::vector<Photo::Data>> m_selected;
        Database::IDatabase& m_db;

        void setSelected(const std::vector<Photo::Data> &);

    signals:
        void selectionChanged(const std::vector<Photo::Data> &) const;
};


#endif
