#include "pricehistorytablemodel.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QIcon>
#include <QPainter>
#include <QPixmap>

namespace {

QString formatPrice(double value)
{
    return value > 0.0
        ? QString::number(value, 'f', 3) + QStringLiteral(" TND/kg")
        : QStringLiteral("-");
}

QString formatScore(double value)
{
    return QString::number(qRound(value)) + QStringLiteral("%");
}

QString truncateText(const QString &value, int maxLength)
{
    const QString trimmed = value.trimmed();
    if (trimmed.size() <= maxLength) {
        return trimmed;
    }
    return trimmed.left(maxLength - 3) + QStringLiteral("...");
}

QIcon makeEyeIcon()
{
    QPixmap pixmap(18, 18);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(QStringLiteral("#5B21B6")), 1.4));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QRectF(2.0, 5.0, 14.0, 8.0));
    painter.setBrush(QColor(QStringLiteral("#C4B5FD")));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QRectF(7.0, 7.0, 4.0, 4.0));

    return QIcon(pixmap);
}

} // namespace

PriceHistoryTableModel::PriceHistoryTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int PriceHistoryTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

int PriceHistoryTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(ColumnCount);
}

QVariant PriceHistoryTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= m_entries.size()
        || index.column() < 0
        || index.column() >= static_cast<int>(ColumnCount)) {
        return QVariant();
    }

    const DpHistoryEntry &entry = m_entries.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColDateTime:
            return entry.createdAt.isValid()
                ? entry.createdAt.toString(QStringLiteral("dd/MM/yyyy hh:mm"))
                : QStringLiteral("-");
        case ColPort:
            return entry.portName.isEmpty() ? QStringLiteral("Port #%1").arg(entry.portId) : entry.portName;
        case ColBasePrice:
            return formatPrice(entry.basePrice);
        case ColRecommendedPrice:
            return formatPrice(entry.recommendedPrice);
        case ColDelta: {
            const double delta = entry.recommendedPrice - entry.basePrice;
            const QString prefix = delta >= 0.0 ? QStringLiteral("+") : QString();
            return prefix + QString::number(delta, 'f', 3) + QStringLiteral(" TND/kg");
        }
        case ColWeather:
            return formatScore(entry.weatherScore);
        case ColRisk:
            return formatScore(entry.riskScore);
        case ColFreshness:
            return formatScore(entry.freshnessScore);
        case ColSource:
            return entry.apiSource.isEmpty() ? QStringLiteral("BASE_LOCALE") : entry.apiSource;
        case ColExplanation:
            return entry.explanation.isEmpty() ? QStringLiteral("-") : truncateText(entry.explanation, 84);
        case ColAction:
            return QString();
        default:
            return QVariant();
        }
    }

    if (role == Qt::DecorationRole && index.column() == ColAction) {
        static const QIcon eyeIcon = makeEyeIcon();
        return eyeIcon;
    }

    if (role == Qt::ToolTipRole && index.column() == ColExplanation) {
        return entry.explanation;
    }

    if (role == Qt::ToolTipRole && index.column() == ColAction) {
        return QStringLiteral("Afficher le detail du log");
    }

    if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
        case ColBasePrice:
        case ColRecommendedPrice:
        case ColDelta:
        case ColWeather:
        case ColRisk:
        case ColFreshness:
        case ColAction:
            return QVariant(Qt::AlignCenter);
        default:
            return QVariant(Qt::AlignVCenter | Qt::AlignLeft);
        }
    }

    if (role == Qt::ForegroundRole) {
        switch (index.column()) {
        case ColDelta: {
            const double delta = entry.recommendedPrice - entry.basePrice;
            if (delta > 0.001) {
                return QBrush(QColor(QStringLiteral("#059669")));
            }
            if (delta < -0.001) {
                return QBrush(QColor(QStringLiteral("#dc2626")));
            }
            return QBrush(QColor(QStringLiteral("#111827")));
        }
        case ColRisk:
            return entry.riskScore >= 60.0
                ? QBrush(QColor(QStringLiteral("#F87171")))
                : QBrush(QColor(QStringLiteral("#E2E8F0")));
        case ColFreshness:
            if (entry.freshnessScore >= 70.0) {
                return QBrush(QColor(QStringLiteral("#059669")));
            }
            if (entry.freshnessScore >= 40.0) {
                return QBrush(QColor(QStringLiteral("#d97706")));
            }
            return QBrush(QColor(QStringLiteral("#dc2626")));
        default:
            return QBrush(QColor(QStringLiteral("#111827")));
        }
    }

    if (role == Qt::FontRole
        && (index.column() == ColRecommendedPrice || index.column() == ColDelta)) {
        QFont font;
        font.setBold(true);
        return font;
    }

    return QVariant();
}

QVariant PriceHistoryTableModel::headerData(int section,
                                           Qt::Orientation orientation,
                                           int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }

    switch (section) {
    case ColDateTime: return QStringLiteral("Date");
    case ColPort: return QStringLiteral("Port");
    case ColBasePrice: return QStringLiteral("Base");
    case ColRecommendedPrice: return QStringLiteral("Recommandé");
    case ColDelta: return QStringLiteral("Ecart");
    case ColWeather: return QStringLiteral("Meteo");
    case ColRisk: return QStringLiteral("Risque");
    case ColFreshness: return QStringLiteral("Fraîcheur");
    case ColSource: return QStringLiteral("Source");
    case ColExplanation: return QStringLiteral("Explication");
    case ColAction: return QStringLiteral("Detail");
    default: return QVariant();
    }
}

void PriceHistoryTableModel::setEntries(const QVector<DpHistoryEntry> &entries)
{
    beginResetModel();
    m_entries = entries;
    endResetModel();
}

void PriceHistoryTableModel::clearEntries()
{
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

const DpHistoryEntry *PriceHistoryTableModel::entryAt(int row) const
{
    if (row < 0 || row >= m_entries.size()) {
        return nullptr;
    }
    return &m_entries.at(row);
}
