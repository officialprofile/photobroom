/*
 * Tree View for Images.
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

#include "images_tree_view.hpp"

#include <cassert>

#include <QScrollBar>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>

#include <configuration/constants.hpp>
#include <configuration/configuration.hpp>

#include "view_helpers/data.hpp"
#include "view_helpers/positions_calculator.hpp"
#include "view_helpers/positions_reseter.hpp"
#include "tree_item_delegate.hpp"


ViewStatus::ViewStatus():
    QObject(),
    m_timer(new QTimer(this)),
    m_requiresUpdate(false)
{
    connect(m_timer, SIGNAL(timeout()), this, SLOT(trigger_update()));
    m_timer->setSingleShot(true);
}


void ViewStatus::markDirty()
{
    m_requiresUpdate = true;

    if (m_timer->isActive() == false)
        trigger_update();
}


void ViewStatus::trigger_update()
{
    if (m_requiresUpdate)
    {
        m_requiresUpdate = false;
        m_timer->start(250);                   //disable updates for a 250 ms
        emit update_now();
    }
}


///////////////////////////////////////////////////////////////////////////////

ImagesTreeView::ImagesTreeView(QWidget* _parent): QAbstractItemView(_parent), m_data(new Data), m_viewStatus()
{
    TreeItemDelegate* delegate = new TreeItemDelegate(this);

    setItemDelegate(delegate);

    connect(&m_viewStatus, SIGNAL(update_now()), this, SLOT(updateModel()));
}


ImagesTreeView::~ImagesTreeView()
{

}


void ImagesTreeView::set(IConfiguration* configuration)
{
    m_data->m_configuration = configuration;
}


QModelIndex ImagesTreeView::indexAt(const QPoint& point) const
{
    const QPoint offset = getOffset();
    const QPoint treePoint = point + offset;
    ModelIndexInfoSet::iterator infoIt = m_data->get(treePoint);
    const QModelIndex result = m_data->get(infoIt);

    return result;
}


bool ImagesTreeView::isIndexHidden(const QModelIndex& index) const
{
    (void) index;

    return false;
}


QRect ImagesTreeView::visualRect(const QModelIndex& index) const
{
    return getItemRect(index);
}


QRegion ImagesTreeView::visualRegionForSelection(const QItemSelection& selection) const
{
    QModelIndexList indexes = selection.indexes();
    QRegion result;

    for (const QModelIndex& idx: indexes)
    {
        ModelIndexInfoSet::iterator infoIt = m_data->find(idx);

        if (infoIt.valid())
        {
            const ModelIndexInfo& info = *infoIt;

            result += info.getRect();
        }
    }

    return result;
}


int ImagesTreeView::horizontalOffset() const
{
    return horizontalScrollBar()->value();
}


int ImagesTreeView::verticalOffset() const
{
    return verticalScrollBar()->value();
}


QModelIndex ImagesTreeView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    (void) cursorAction;
    (void) modifiers;

    return QModelIndex();
}


void ImagesTreeView::scrollTo(const QModelIndex& index, QAbstractItemView::ScrollHint hint)
{
    (void) index;
    (void) hint;
}


void ImagesTreeView::setSelection(const QRect& _rect, QItemSelectionModel::SelectionFlags flags)
{
    const QRect treeRect = _rect.translated(getOffset());

    const std::deque<QModelIndex> items = findItemsIn(treeRect);
    QItemSelection selection;

    for (const QModelIndex& item: items)
        selection.select(item, item);

    selectionModel()->select(selection, flags);
}


void ImagesTreeView::setModel(QAbstractItemModel* m)
{
    //disconnect current model
    QAbstractItemModel* current_model = QAbstractItemView::model();
    disconnect(current_model);

    //
    QAbstractItemView::setModel(m);
    m_data->set(m);

    //read model
    rereadModel();

    //connect to model's signals
    connect(m, SIGNAL(modelReset()), this, SLOT(modelReset()), Qt::UniqueConnection);
    connect(m, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
            this, SLOT(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)), Qt::UniqueConnection);
    connect(m, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)), this, SLOT(rowsAboutToBeRemoved(QModelIndex,int,int)), Qt::UniqueConnection);

    connect(m, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(rowsInserted(QModelIndex,int,int)), Qt::UniqueConnection);
    connect(m, SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int)), this, SLOT(rowsMoved(QModelIndex,int,int,QModelIndex,int)), Qt::UniqueConnection);
    connect(m, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(rowsRemoved(QModelIndex,int,int)), Qt::UniqueConnection);
}


void ImagesTreeView::paintEvent(QPaintEvent *)
{
    QPainter painter(viewport());
    const QPoint offset = getOffset();
    QRect visible_area = viewport()->rect();

    visible_area.moveTo(offset);
    painter.translate(-offset);

    std::deque<QModelIndex> items = findItemsIn(visible_area);

    for (const QModelIndex& item: items)
    {
        ModelIndexInfoSet::const_iterator infoIt = m_data->cfind(item);
        assert(infoIt.valid());
        const ModelIndexInfo& info = *infoIt;

        QStyleOptionViewItem styleOption;
        styleOption.rect = info.getRect();
        styleOption.features = m_data->isImage(item)? QStyleOptionViewItem::HasDecoration: QStyleOptionViewItem::HasDisplay;
        styleOption.palette = palette();
        styleOption.state |= selectionModel()->isSelected(item)? QStyle::State_Selected: QStyle::State_None;

        QAbstractItemView::itemDelegate()->paint(&painter, styleOption, item);
    }
}


void ImagesTreeView::mouseReleaseEvent(QMouseEvent* e)
{
    QAbstractItemView::mouseReleaseEvent(e);

    QModelIndex item = indexAt(e->pos());
    ModelIndexInfoSet::iterator infoIt = m_data->find(item);

    if (item.isValid() && infoIt.valid())
    {
        ModelIndexInfo& info = *infoIt;
        info.expanded = !info.expanded;

        //reset some positions
        PositionsReseter reseter(model(), m_data.get());
        reseter.itemChanged(item);

        updateData();

        m_viewStatus.markDirty();
    }
}


void ImagesTreeView::resizeEvent(QResizeEvent* e)
{
    QAbstractItemView::resizeEvent(e);

    //reset all positions
    PositionsReseter reseter(model(), m_data.get());
    reseter.invalidateAll();

    updateData();

    m_viewStatus.markDirty();
}


/// private:


const QRect& ImagesTreeView::getItemRect(const QModelIndex& index) const
{
    ModelIndexInfoSet::const_iterator infoIt = m_data->cfind(index);

    assert(infoIt.valid());
    const ModelIndexInfo& info = *infoIt;

    return info.getRect();
}


std::deque<QModelIndex> ImagesTreeView::findItemsIn(const QRect& _rect) const
{
    //TODO: optimise?
    std::deque<QModelIndex> result;

    m_data->for_each_visible( [&] ( ModelIndexInfoSet::iterator it)
    {
        const ModelIndexInfo& info = *it;
        const QRect& item_rect = info.getRect();
        const bool overlap = _rect.intersects(item_rect);

        if (overlap)
        {
            QModelIndex modelIdx = m_data->get(it);
            if(modelIdx.isValid() == false)
                modelIdx = m_data->get(it);

            result.push_back(modelIdx);
        }

        return true;
    });

    return result;
}


void ImagesTreeView::updateData()
{
    QAbstractItemModel* m = QAbstractItemView::model();

    // is there anything to calculate?
    if (m != nullptr && m->rowCount() > 0)
    {
        PositionsCalculator calculator(m, m_data.get(), viewport()->width());
        calculator.updateItems();
    }
}


void ImagesTreeView::updateGui()
{
    ModelIndexInfoSet::const_iterator infoIt = m_data->cfind(QModelIndex());
    assert(infoIt.valid());

    const ModelIndexInfo& info = *infoIt;
    const QSize areaSize = viewport()->size();
    const QSize treeAreaSize = info.getOverallRect().size();

    verticalScrollBar()->setPageStep(areaSize.height());
    horizontalScrollBar()->setPageStep(areaSize.width());
    verticalScrollBar()->setRange(0, treeAreaSize.height() - areaSize.height());
    horizontalScrollBar()->setRange(0, treeAreaSize.width() - areaSize.width());

    //refresh widget
    viewport()->update();
}


QPoint ImagesTreeView::getOffset() const
{
    const QPoint offset(horizontalOffset(), verticalOffset());

    return offset;
}


void ImagesTreeView::modelReset()
{
    rereadModel();
}


void ImagesTreeView::rowsAboutToBeMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& /*destinationParent*/, int /*destinationRow*/)
{
    rowsAboutToBeRemoved(sourceParent, sourceStart, sourceEnd);
}


void ImagesTreeView::rowsAboutToBeRemoved(const QModelIndex& _parent, int start, int end)
{
    //remove data from internal data model
    for(int i = start; i <= end; i++)
    {
        QModelIndex child = model()->index(i, 0, _parent);

        m_data->forget(child);
    }
}


void ImagesTreeView::rowsInserted(const QModelIndex& _parent, int from, int to)
{
    PositionsReseter reseter(model(), m_data.get());
    reseter.itemsAdded(_parent, from, to);

    updateData();

    m_viewStatus.markDirty();
}


void ImagesTreeView::rowsMoved(const QModelIndex & sourceParent, int sourceStart, int sourceEnd, const QModelIndex & destinationParent, int destinationRow)
{
    const int items = sourceEnd - sourceStart + 1;
    rowsRemoved(sourceParent, sourceStart, sourceEnd);
    rowsInserted(destinationParent, destinationRow, destinationRow + items - 1);
}


void ImagesTreeView::rowsRemoved(const QModelIndex& _parent, int first, int)
{
    //reset sizes and positions of existing items
    PositionsReseter reseter(model(), m_data.get());
    reseter.childrenRemoved(_parent, first);

    updateData();

    //update model
    m_viewStatus.markDirty();
}


void ImagesTreeView::rereadModel()
{
    m_data->clear();
    m_viewStatus.markDirty();
}


void ImagesTreeView::updateModel()
{
    updateData();
    updateGui();
}