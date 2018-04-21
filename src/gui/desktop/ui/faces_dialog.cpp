
#include "faces_dialog.hpp"

#include <cassert>

#include <QPainter>

#include <core/icore_factory_accessor.hpp>
#include <core/ipython_thread.hpp>
#include <database/photo_data.hpp>
#include <project_utils/project.hpp>

#include "ui_faces_dialog.h"

using namespace std::placeholders;

FacesDialog::FacesDialog(const Photo::Data& data, ICoreFactoryAccessor* coreAccessor, Project* prj, QWidget *parent):
    QDialog(parent),
    m_people(prj->getProjectInfo().getInternalLocation(ProjectInfo::FaceRecognition), prj->getDatabase(), coreAccessor),
    m_faces(),
    m_photoPath(data.path),
    ui(new Ui::FacesDialog),
    m_pythonThread(coreAccessor->getPythonThread()),
    m_facesToAnalyze(0)
{
    ui->setupUi(this);

    qRegisterMetaType<QVector<QRect>>("QVector<QRect>");

    connect(ui->scaleSlider, &QSlider::valueChanged,
            this, &FacesDialog::updateImage);

    connect(&m_people, &PeopleOperator::faces,
            this, &FacesDialog::applyFacesLocations);

    connect(&m_people, &PeopleOperator::recognized,
            this, &FacesDialog::applyFaceName);

    ui->statusLabel->setText(tr("Locating faces..."));

    m_people.fetchFaces(data.id);
    updateImage();
}


FacesDialog::~FacesDialog()
{
    delete ui;
}


std::vector<std::pair<QRect, QString>> FacesDialog::people() const
{
    std::vector<std::pair<QRect, QString>> result;

    const int count = ui->peopleList->rowCount();
    assert(count == m_faces.size());

    for(int i = 0; i < count; i++)
    {
        auto person = ui->peopleList->item(i, 0);

        if (person != nullptr)
        {
            const QString name = person->text();

            if (name.isEmpty() == false)
            {
                auto face_data = std::make_pair(m_faces[i], name);
                result.push_back(face_data);
            }
        }
    }

    return result;
}


void FacesDialog::applyFacesLocations(const Photo::Id& id, const QVector<QRect>& faces)
{
    const QString status = faces.isEmpty()? tr("Found %1 face(s).").arg(faces.size()) :
                                            tr("Found %1 face(s). Recognizing people...").arg(faces.size());

    ui->statusLabel->setText(status);

    m_faces = faces;
    updateImage();
    updatePeopleList();

    m_facesToAnalyze = faces.size();

    for(const QRect& face: faces)
        m_people.recognize(id, face);
}


void FacesDialog::applyFaceName(const Photo::Id &, const QRect& face, const PersonData& person)
{
    const QString name = person.name();

    auto it = std::find(m_faces.cbegin(), m_faces.cend(), face);

    if (it != m_faces.cend())
    {
        const std::size_t idx = std::distance(m_faces.cbegin(), it);
        ui->peopleList->setItem(idx, 0, new QTableWidgetItem(name));
    }

    m_facesToAnalyze--;

    if (m_facesToAnalyze == 0)
        ui->statusLabel->setText(tr("All known faces recognized."));
}


void FacesDialog::updateImage()
{
    QImage image(m_photoPath);
    const QSize currentSize = image.size();
    const double scale = ui->scaleSlider->value() / 100.0;

    const QSize scaledSize = currentSize * scale;
    image = image.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QVector<QRect> scaledFaces;
    for(QRect face: m_faces)
    {
        const QPoint tl = face.topLeft();
        const QSize size = face.size();
        const QRect scaledFace = QRect( tl * scale, size * scale );

        scaledFaces.append(scaledFace);
    }

    if (scaledFaces.isEmpty() == false)
    {
        QPainter painter(&image);

        QPen pen;
        pen.setColor(Qt::red);
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawRects(scaledFaces);

        pen.setWidth(1);
        painter.setBackground( Qt::white );
        painter.setBackgroundMode( Qt::OpaqueMode );
        painter.setPen(pen);

        for(int i = 0; i < scaledFaces.size(); i++)
        {
            const QRect& face = scaledFaces[i];
            const QPoint tl = face.topLeft();
            painter.drawText(tl, QString::number(i + 1));
        }
    }

    QPixmap new_pixmap = QPixmap::fromImage(image);
    ui->imageView->setPixmap(new_pixmap);
}


void FacesDialog::updatePeopleList()
{
    const int rowCount = ui->peopleList->rowCount();
    const int peopleCount = m_faces.size();

    if (rowCount < peopleCount)
        ui->peopleList->setRowCount(peopleCount);
}
