#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QSet>
#include "arduino.h"
class QButtonGroup;
class QLabel;
class LoginPage;
class PecheModuleWidget;
class QTime;
class QMovie;
class QTimer;

class LogistiqueModuleWidget;
#include "arduino.h"


#include <QSqlError>
#include <QSqlQuery>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool zoneexiste(QString zone);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void setupNavigation();
    void createPages();
    void setupDashboardVisuals();
    void applyCardStyle(QWidget *card);
    void applyShadow(QWidget *card);
    void setScaledIcon(QLabel *label, const QString &path, const QSize &size);
    void applyFonts();
    void setPage(const QString &title, int pageIndex);
    void setModuleTab(int pageIndex, int tabIndex);
    void activateDashboardTarget(QWidget *widget);
    void setupDashboardInteraction(QWidget *widget,
                                   const QString &title,
                                   int pageIndex,
                                   int tabIndex = -1);
    void showDashboardActivityDialog();
    void showLogin();
    void showDashboard();
    void setLoginMode(bool enabled);
    void setPechePage(int subIndex, const QString &title);
    void togglePecheSubmenu();
    void setLogistiquePage(int subIndex, const QString &title);
    void toggleLogistiqueSubmenu();
    void resetArduinoInputState();
    void sendArduinoCode(const QByteArray &code);
    void logArduinoState(const QString &step) const;
    int alertLevelForDelay(int estimatedMinutes, const QTime &now) const;
    int parseEstimatedTimeToMinutes(const QString &value) const;

private slots:
    void update_label(); // Slot for arduino data
    void monitorActiveDepartures();

private:
    Ui::MainWindow *ui;
    QButtonGroup *sidebarGroup = nullptr;
    LoginPage *loginPage = nullptr;
    int loginIndex = -1;
    int dashboardIndex = -1;
    int employesIndex = -1;
    int naviresIndex = -1;
    int chambresIndex = -1;
    int logistiqueIndex = -1;
    int equipementsIndex = -1;
    int PecheIndex = -1;
    PecheModuleWidget *PecheModule = nullptr;
    bool PecheSubmenuVisible = false;
    LogistiqueModuleWidget *LogistiqueModule = nullptr;
    bool LogistiqueSubmenuVisible = false;
    QSet<QWidget *> dashboardHoverWidgets;

    QSet<QWidget *> dashboardClickableWidgets;

    Arduino A;
    QByteArray data; // contains data read from serial

    int etat2 = 0;
    int etat =0;
    QString idEmployer;
    QString zone ;
    QString heure;
    QTimer *safeDockMonitorTimer = nullptr;
    QSet<QString> criticalDelayAlertsSent;

    QMovie *dashBgMovie = nullptr;

};
#endif // MAINWINDOW_H

