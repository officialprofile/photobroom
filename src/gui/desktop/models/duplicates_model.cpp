
#include <core/function_wrappers.hpp>
#include <core/qmodel_utils.hpp>
#include <database/iphoto_operator.hpp>
#include <database/photo_utils.hpp>

#include "duplicates_model.hpp"


enum Roles
{
    DuplicatesRole = Qt::UserRole + 1,
};


ENUM_ROLES_SETUP(Roles);


QVariant DuplicatesModel::data(const QModelIndex& index, int role) const
{
    const auto row = index.row();

    if (role == DuplicatesRole && index.column() == 0 && row < m_duplicates.size())
        return QVariant::fromValue(m_duplicates[row]);
    else
        return {};
}


int DuplicatesModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid()? 0: static_cast<int>(m_duplicates.size());
}


QHash<int, QByteArray> DuplicatesModel::roleNames() const
{
    auto roles = QAbstractListModel::roleNames();
    const auto extra = parseRoles<Roles>();
    const QHash<int, QByteArray> extraRoles(extra.begin(), extra.end());
    roles.insert(extraRoles);

    return roles;
}


void DuplicatesModel::setDB(Database::IDatabase* db)
{
    assert(m_workInProgress == false);

    m_db = db;
    clear();

    emit dbChanged();
}


bool DuplicatesModel::isWorking() const
{
    return m_workInProgress;
}



Database::IDatabase * DuplicatesModel::db() const
{
    return m_db;
}


void DuplicatesModel::reloadDuplicates()
{
    if (m_db && m_workInProgress == false)
    {
        clear();

        setWorkInProgress(true);

        auto resultCallback = make_cross_thread_function<std::vector<Photo::DataDelta>>(this, &DuplicatesModel::compileDuplicates);

        m_db->exec([resultCallback](Database::IBackend& backend)
        {
            const auto ids = backend.photoOperator().onPhotos(Database::FilterSimilarPhotos{}, Database::Actions::Sort(Database::Actions::Sort::By::PHash));

            std::vector<Photo::DataDelta> data;
            data.reserve(ids.size());

            for(const auto& id: ids)
                data.push_back(backend.getPhotoDelta(id, {Photo::Field::PHash, Photo::Field::Path}));

            resultCallback(data);
        },
        "Looking for photo duplicates"
        );
    }
}


void DuplicatesModel::compileDuplicates(const std::vector<Photo::DataDelta>& duplicatePhotos)
{
    std::vector<std::vector<Photo::DataDelta>> grouped;

    for(auto it = duplicatePhotos.begin(); it != duplicatePhotos.end();)
    {
        const auto currentPHash = it->get<Photo::Field::PHash>();

        auto nextIt = std::find_if_not(it, duplicatePhotos.end(), [&currentPHash](const Photo::DataDelta& data) {
            return data.get<Photo::Field::PHash>() == currentPHash;
        });

        grouped.push_back(std::vector(it, nextIt));

        it = nextIt;
    }

    beginInsertRows({}, 0, grouped.size() - 1);
    m_duplicates.swap(grouped);
    endInsertRows();

    setWorkInProgress(false);
}


void DuplicatesModel::setWorkInProgress(bool work)
{
    m_workInProgress = work;

    emit workStatusChanged(work);
}


void DuplicatesModel::clear()
{
    beginResetModel();
    m_duplicates.clear();
    endResetModel();
}
