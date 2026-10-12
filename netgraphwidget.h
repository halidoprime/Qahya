#ifndef NETGRAPHWIDGET_H
#define NETGRAPHWIDGET_H

#include <QList>
#include <QWidget>

class QChartView;
class QLineSeries;
class QValueAxis;

class NetGraphWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NetGraphWidget(QWidget *parent = nullptr);

public slots:
    void addSample(double downBytesPerSecond, double upBytesPerSecond);

private:
    QChartView *m_chartView = nullptr;
    QLineSeries *m_downSeries = nullptr;
    QLineSeries *m_upSeries = nullptr;
    QValueAxis *m_axisY = nullptr;
    QList<double> m_downValues;
    QList<double> m_upValues;
    double m_yMax = 1024.0;
};

#endif // NETGRAPHWIDGET_H
