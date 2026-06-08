#ifndef MAINTENANCEGANTT_H
#define MAINTENANCEGANTT_H

#include <QWidget>
#include <QPainter>
#include <QDate>
#include <QMap>

class MaintenanceGantt : public QWidget {
    Q_OBJECT
public:
    explicit MaintenanceGantt(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(300);
    }

    void setData(const QMap<QString, QDate> &vesselMaintDates) {
        dates = vesselMaintDates;
        setMinimumHeight(qMax(300, dates.size() * 45));
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        int rowHeight = 40;
        int startX = 120;
        QDate today = QDate::currentDate();

        int i = 0;
        for (auto it = dates.begin(); it != dates.end(); ++it) {
            QString name = it.key();
            QDate maintDate = it.value();

            // Row background
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(241, 245, 249));
            p.drawRoundedRect(0, i * rowHeight + 5, width(), rowHeight - 10, 8, 8);

            // Vessel name
            p.setPen(QColor(30, 41, 59));
            p.drawText(10, i * rowHeight + 25, name);

            // Timeline bar
            int daysFromToday = today.daysTo(maintDate);
            int barX = startX + daysFromToday * 5;
            int barWidth = 20;

            QColor barColor = QColor(83, 144, 217); // Blue
            if (daysFromToday < 7) barColor = QColor(239, 68, 68); // Red
            else if (daysFromToday < 30) barColor = QColor(245, 158, 11); // Amber

            p.setBrush(barColor);
            p.drawRoundedRect(barX, i * rowHeight + 10, barWidth, rowHeight - 20, 4, 4);

            i++;
        }
    }

private:
    QMap<QString, QDate> dates;
};

#endif // MAINTENANCEGANTT_H
