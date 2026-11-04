#pragma once

#include <QHeaderView>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QVector>

#include <algorithm>
#include <array>

class FilterHeader final : public QHeaderView {
public:
    explicit FilterHeader(QWidget* parent) : QHeaderView(Qt::Horizontal, parent)
    {
        setMouseTracking(true);
        viewport()->setMouseTracking(true);
        connect(this, &QHeaderView::sectionResized, this, [this] { positionFilters(); });
        connect(this, &QHeaderView::sectionMoved, this, [this] { positionFilters(); });
        connect(this, &QHeaderView::geometriesChanged, this, [this] { positionFilters(); });
    }

    template<std::size_t N>
    void setFilters(const std::array<QLineEdit*, N>& filters)
    {
        filters_.clear();
        for (auto* filter : filters) filters_.push_back(filter);
        positionFilters();
    }

    QSize sizeHint() const override
    {
        auto size = QHeaderView::sizeHint();
        size.rheight() += filterHeight();
        return size;
    }

protected:
    void paintSection(QPainter* painter, const QRect& rect, int section) const override
    {
        auto title = rect;
        title.setHeight(std::max(0, rect.height() - filterHeight()));
        QHeaderView::paintSection(painter, title, section);
    }
    void paintEvent(QPaintEvent* event) override
    {
        QHeaderView::paintEvent(event);
        QPainter painter(viewport());
        painter.setPen(QColor(QStringLiteral("#afc3d2")));
        for (int section = 0; section < count(); ++section) {
            if (isSectionHidden(section)) continue;
            const int edge = sectionViewportPosition(section) + sectionSize(section) - 1;
            painter.drawLine(edge, 0, edge, viewport()->height() - 1);
        }
    }
    void mouseMoveEvent(QMouseEvent* event) override
    {
        QHeaderView::mouseMoveEvent(event);
        if (event->buttons() != Qt::NoButton) return;
        const int x = qRound(event->position().x());
        bool resize = false;
        for (int section = 0; section < count(); ++section) {
            if (isSectionHidden(section) || sectionResizeMode(section) != QHeaderView::Interactive)
                continue;
            const int edge = sectionViewportPosition(section) + sectionSize(section);
            if (std::abs(x - edge) <= 3) {
                resize = true;
                break;
            }
        }
        if (resize) {
            setCursor(Qt::SizeHorCursor);
            viewport()->setCursor(Qt::SizeHorCursor);
        } else {
            unsetCursor();
            viewport()->unsetCursor();
        }
    }
    void resizeEvent(QResizeEvent* event) override
    {
        QHeaderView::resizeEvent(event);
        positionFilters();
    }
    void scrollContentsBy(int dx, int dy) override
    {
        QHeaderView::scrollContentsBy(dx, dy);
        positionFilters();
    }

private:
    int filterHeight() const { return fontMetrics().height() + 14; }
    void positionFilters()
    {
        for (int i = 0; i < filters_.size(); ++i)
            if (filters_[i])
                filters_[i]->setGeometry(sectionViewportPosition(i),
                    height() - filterHeight(), sectionSize(i), filterHeight());
    }
    QVector<QLineEdit*> filters_;
};
