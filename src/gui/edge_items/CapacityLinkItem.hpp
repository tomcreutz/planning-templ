#ifndef TEMPL_GUI_EDGE_ITEMS_CAPACITY_LINK_ITEM_HPP
#define TEMPL_GUI_EDGE_ITEMS_CAPACITY_LINK_ITEM_HPP

#include <graph_analysis/gui/EdgeItemBase.hpp>
#include "../../CapacityLink.hpp"
#include <QGraphicsRectItem>

namespace templ {
namespace gui {
namespace edge_items {

/**
 * The graphical representation for CapacityLink
 */
class CapacityLinkItem : public graph_analysis::gui::EdgeItemBase
{
public:
    CapacityLinkItem() {}
    CapacityLinkItem(graph_analysis::gui::GraphWidget* graphWiget,
            const graph_analysis::Edge::Ptr& edge,
            QGraphicsItem* parent);
    virtual ~CapacityLinkItem();
    virtual int type() const;

    void adjustEdgePositioning();

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = 0);
    QRectF boundingRect() const;
    QPainterPath shape() const;

    int mArrowSize;
    QGraphicsTextItem* mpLabel;
    QGraphicsTextItem* mpClassName;
    QGraphicsPolygonItem* mpMultiLine;
    QGraphicsPolygonItem* mpArrowHead;

    /// Representation for the outer shape of a fill bar, to display the status of the capacity
    /// consumption
    QGraphicsRectItem* mpFillBar;
    /// Representation of th filling for the fill bar
    QGraphicsRectItem* mpFillStatus;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);

    EdgeItemBase* createNewItem(graph_analysis::gui::GraphWidget* graphWidget,
                const graph_analysis::Edge::Ptr& edge,
                QGraphicsItem* parent) const;


};

} // end namespace edge_items
} // end namespace gui
} // end namespace templ
#endif // TEMPL_GUI_EDGE_ITEMS_CAPACITY_LINK_ITEM_HPP