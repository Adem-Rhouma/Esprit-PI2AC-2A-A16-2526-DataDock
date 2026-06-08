#ifndef CHAMBRESFROIDESEXPORTPAGE_H
#define CHAMBRESFROIDESEXPORTPAGE_H

#include <QWidget>
#include <QMap>
#include <QVariant>
#include <QDate>
#include <QImage>

namespace Ui {
class ChambresFroidesExportPage;
}

class QTableWidget;
class QPainter;
class QPdfWriter;
class QDateEdit;
class QVBoxLayout;
class QLabel;
class QChart;

class ChambresFroidesExportPage : public QWidget
{
    Q_OBJECT

public:
    explicit ChambresFroidesExportPage(QWidget *parent = nullptr);
    ~ChambresFroidesExportPage();

signals:
    void backRequested();

private slots:
    void on_btnGenerate_clicked();
    void on_back_requested();
    void on_checkAll_stateChanged(int state);
    void updatePreview();

private:
    struct ReportSection {
        QString title;
        bool enabled;
        bool allUnits;
        QList<QString> selectedIds;
        QDate fromDate;
        QDate toDate;
        QMap<QString, QVariant> extraParams;
    };

    void setupUI();
    void applyStyles();
    void loadChambresList();
    QList<QMap<QString, QVariant>> gatherSelectedData();
    void generatePDF(const QString &filePath);
    void generateExcel(const QString &filePath);
    
    void drawHeader(QPainter &painter, QPdfWriter &pdfWriter, int pageNum, const QString &subtitle);
    void drawSummaryPage(QPainter &painter, QPdfWriter &pdfWriter);
    
    QList<QMap<QString, QString>> getMaintenanceHistory(const QString &chambreId);
    QList<QMap<QString, QString>> getFishHistory(const QString &chambreId);

    QWidget* createSectionWidget(const QString &title, const QString &id);

    Ui::ChambresFroidesExportPage *ui;
    
    QTableWidget *m_chambresTable;
    QVBoxLayout *m_configLayout;
    QLabel *m_previewLabel;
    QMap<QString, ReportSection> m_sections;
    QDateEdit *m_globalFrom;
    QDateEdit *m_globalTo;
};

#endif // CHAMBRESFROIDESEXPORTPAGE_H
