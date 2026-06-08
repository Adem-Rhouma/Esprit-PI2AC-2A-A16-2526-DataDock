#include "navirepredictivepage.h"
#include "ui_navirepredictivepage.h"
#include "connection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDateTime>
#include <QDebug>
#include <QVBoxLayout>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QCategoryAxis>

NavirePredictivePage::NavirePredictivePage(int shipId, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NavirePredictivePage),
    m_shipId(shipId),
    m_regressorProcess(new QProcess(this)),
    m_classifierProcess(new QProcess(this))
{
    ui->setupUi(this);
    
    connect(m_regressorProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &NavirePredictivePage::onRegressorFinished);
    connect(m_classifierProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &NavirePredictivePage::onClassifierFinished);
            
    connect(m_regressorProcess, &QProcess::errorOccurred, this, &NavirePredictivePage::onPythonError);
    connect(m_classifierProcess, &QProcess::errorOccurred, this, &NavirePredictivePage::onPythonError);

    runPrediction();
}

NavirePredictivePage::~NavirePredictivePage() {
    delete ui;
}

void NavirePredictivePage::runPrediction() {
    // 1. Fetch data for this ship
    QJsonObject root;
    QJsonArray logs;

    QSqlQuery q(Connection::createInstance().db);
    q.prepare("SELECT DISTANCE_NM, FUEL_CONSUMED, CARGO_LOAD_TONS, DESCRIPTION "
              "FROM VESSEL_LOGS WHERE IDNAVIRE = :id ORDER BY RETURN_TIME ASC");
    q.bindValue(":id", m_shipId);
    
    if (q.exec()) {
        while (q.next()) {
            QJsonObject log;
            log["distance"] = q.value(0).toDouble();
            log["fuel"] = q.value(1).toDouble();
            log["cargo"] = q.value(2).toDouble();
            log["description"] = q.value(3).toString();
            logs.append(log);
        }
    }
    root["logs"] = logs;

    QJsonDocument doc(root);
    QByteArray jsonData = doc.toJson();

    // 2. Run Python scripts independently
    // Paths are resolved relative to the project source tree via the
    // NAVIRES_SCRIPTS_DIR macro defined in DataDock.pro (= <project>/gestion_navires/scripts)
    const QString scriptsDir    = QString(NAVIRES_SCRIPTS_DIR);
    const QString pythonPath    = scriptsDir + "/venv/bin/python3";
    const QString regressorScript = scriptsDir + "/numerical_regressor.py";
    const QString classifierScript = scriptsDir + "/symptom_classifier.py";

    m_regressorProcess->setProcessChannelMode(QProcess::MergedChannels);
    m_regressorProcess->start(pythonPath, QStringList() << regressorScript);
    m_regressorProcess->write(jsonData);
    m_regressorProcess->closeWriteChannel();

    m_classifierProcess->setProcessChannelMode(QProcess::MergedChannels);
    m_classifierProcess->start(pythonPath, QStringList() << classifierScript);
    m_classifierProcess->write(jsonData);
    m_classifierProcess->closeWriteChannel();
}

void NavirePredictivePage::onRegressorFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitCode != 0) {
        ui->lblMaintValue->setText("Erreur Regressor");
        return;
    }
    QByteArray output = m_regressorProcess->readAllStandardOutput();
    QJsonDocument doc = QJsonDocument::fromJson(output);
    if (!doc.isNull() && doc.isObject()) updateMaintenanceUI(doc.object());
}

void NavirePredictivePage::onClassifierFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitCode != 0) {
        ui->lblSuggestions->setText("Erreur Classifier");
        return;
    }
    QByteArray output = m_classifierProcess->readAllStandardOutput();
    QJsonDocument doc = QJsonDocument::fromJson(output);
    if (!doc.isNull() && doc.isObject()) updateSymptomUI(doc.object());
}

void NavirePredictivePage::onPythonError(QProcess::ProcessError error) {
    qDebug() << "Python process error:" << error;
    ui->lblMaintValue->setText("Erreur Python");
}

void NavirePredictivePage::updateMaintenanceUI(const QJsonObject &predictions) {
    if (predictions.contains("error")) {
        ui->lblMaintValue->setText(predictions["error"].toString());
        return;
    }

    ui->lblReadinessValue->setText(QString::number(predictions["predicted_readiness"].toDouble()) + "%");
    ui->lblMaintValue->setText(predictions["next_maintenance_date"].toString());

    // Update Chart
    QJsonArray chartData = predictions["chart_data"].toArray();
    QLineSeries *series = new QLineSeries();
    QStringList categories;

    for (int i = 0; i < chartData.size(); ++i) {
        QJsonObject point = chartData[i].toObject();
        series->append(i, point["value"].toDouble());
        categories << point["day"].toString();
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Évolution de la Disponibilité Prédite (10 jours)");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QCategoryAxis *axisX = new QCategoryAxis();
    for (int i = 0; i < categories.size(); ++i) {
        axisX->append(categories[i], i);
    }
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, 100);
    axisY->setTitleText("Prêt pour Voyage (%)");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    if (ui->chartContainer->layout() == nullptr) {
        QVBoxLayout *layout = new QVBoxLayout(ui->chartContainer);
        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        layout->addWidget(chartView);
    } else {
        // Replace existing chart view if any
        QLayoutItem *item = ui->chartContainer->layout()->takeAt(0);
        if (item) delete item->widget();
        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        ui->chartContainer->layout()->addWidget(chartView);
    }
}

void NavirePredictivePage::updateSymptomUI(const QJsonObject &predictions) {
    if (predictions.contains("error")) {
        ui->lblSuggestions->setText(predictions["error"].toString());
        return;
    }

    QJsonArray suggestions = predictions["suggestions"].toArray();
    QString sugText = "Aucun problème majeur détecté.";
    if (!suggestions.isEmpty()) {
        sugText = "<b>Problèmes identifiés :</b><br/>";
        for (const QJsonValue &v : suggestions) {
            sugText += "• " + v.toString() + "<br/>";
        }
    }
    ui->lblSuggestions->setText(sugText);
}
