#include "pechefontutils.h"

#include <QFontDatabase>
#include <QStringList>
#include <QWidget>

namespace {

QString resolvedCalSansFamily()
{
    static const QString family = []() {
        const int fontId =
            QFontDatabase::addApplicationFont(QStringLiteral(":/assets/CalSans-Regular.ttf"));
        if (fontId != -1) {
            const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            if (!families.isEmpty()) {
                return families.first();
            }
        }
        return QStringLiteral("Cal Sans");
    }();

    return family;
}

QStringList fallbackFamilies()
{
    return {
        resolvedCalSansFamily(),
        QStringLiteral("Segoe UI"),
        QStringLiteral("Arial"),
        QStringLiteral("Helvetica"),
        QStringLiteral("Segoe UI Emoji")
    };
}

} // namespace

namespace PecheFontUtils {

QString ensureCalSansLoaded()
{
    return resolvedCalSansFamily();
}

QString fontCssStack()
{
    const QString primary = ensureCalSansLoaded();
    return QStringLiteral("\"%1\", \"Segoe UI\", Arial, Helvetica, sans-serif")
        .arg(primary);
}

QFont moduleFont(qreal pointSize, QFont::Weight weight)
{
    QFont font;
    font.setFamilies(fallbackFamilies());
    font.setStyleStrategy(QFont::PreferDefault);
    font.setWeight(weight);
    if (pointSize > 0.0) {
        font.setPointSizeF(pointSize);
    }
    return font;
}

void applyModuleFont(QWidget *widget)
{
    if (!widget) {
        return;
    }

    widget->setFont(moduleFont());
}

} // namespace PecheFontUtils
