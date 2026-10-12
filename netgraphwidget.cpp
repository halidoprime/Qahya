#include "netgraphwidget.h"

#include <QChart>
#include <QChartView>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineSeries>
#include <QMargins>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QValueAxis>

namespace {
constexpr int kSampleCount = 60;
constexpr int kChartHeight = 52;
constexpr int kChartWidth = 200;
const QColor kBackgroundColor = QColor(QStringLiteral("#1e1f24"));
const QColor kDownColor = QColor(QStringLiteral("#63d297"));
const QColor kUpColor = QColor(QStringLiteral("#ff7777"));
}

NetGraphWidget::NetGraphWidget(QWidget *parent)
    : QWidget(parent)
{
    m_downSeries = new QLineSeries(this);
    m_downSeries->setPen(QPen(kDownColor, 2));
    m_upSeries = new QLineSeries(this);
    m_upSeries->setPen(QPen(kUpColor, 2));

    auto *chart = new QChart;
    chart->legend()->hide();
    chart->setMargins(QMargins(0, 0, 0, 0));
    chart->setBackgroundRoundness(0);
    chart->setDropShadowEnabled(false);
    chart->setBackgroundBrush(kBackgroundColor);
    chart->addSeries(m_downSeries);
    chart->addSeries(m_upSeries);

    auto *axisX = new QValueAxis(chart);
    axisX->setRange(0, kSampleCount - 1);
    axisX->setVisible(false);
    chart->addAxis(axisX, Qt::AlignBottom);
    m_downSeries->attachAxis(axisX);
    m_upSeries->attachAxis(axisX);

    m_axisY = new QValueAxis(chart);
    m_axisY->setRange(0, m_yMax);
    m_axisY->setVisible(false);
    chart->addAxis(m_axisY, Qt::AlignLeft);
    m_downSeries->attachAxis(m_axisY);
    m_upSeries->attachAxis(m_axisY);

    m_chartView = new QChartView(chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing, true);
    m_chartView->setFrameShape(QFrame::NoFrame);
    m_chartView->setBackgroundBrush(kBackgroundColor);
    m_chartView->setFixedSize(kChartWidth, kChartHeight);

    auto *legend = new QLabel(this);
    legend->setObjectName(QStringLiteral("netGraphLegend"));
    legend->setTextFormat(Qt::RichText);
    legend->setAlignment(Qt::AlignCenter);
    legend->setText(QStringLiteral("<span style=\"color:%1;\">↓ %2</span><br>"
                                   "<span style=\"color:%3;\">↑ %4</span>")
                        .arg(kDownColor.name(), tr("Download"), kUpColor.name(), tr("Upload")));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(m_chartView);
    layout->addWidget(legend, 0, Qt::AlignVCenter);
}

void NetGraphWidget::addSample(double downBytesPerSecond, double upBytesPerSecond)
{
    m_downValues.append(downBytesPerSecond);
    m_upValues.append(upBytesPerSecond);
    if (m_downValues.size() > kSampleCount) {
        m_downValues.removeFirst();
        m_upValues.removeFirst();
    }

    QList<QPointF> downPoints;
    QList<QPointF> upPoints;
    downPoints.reserve(m_downValues.size());
    upPoints.reserve(m_downValues.size());
    double peak = 0.0;
    for (int i = 0; i < m_downValues.size(); ++i) {
        downPoints.append(QPointF(i, m_downValues.at(i)));
        upPoints.append(QPointF(i, m_upValues.at(i)));
        peak = qMax(peak, qMax(m_downValues.at(i), m_upValues.at(i)));
    }
    m_downSeries->replace(downPoints);
    m_upSeries->replace(upPoints);

    const double target = qMax(peak * 1.25, 1024.0);
    m_yMax = qMax(target, m_yMax * 0.85);
    m_axisY->setRange(0, m_yMax);
}
