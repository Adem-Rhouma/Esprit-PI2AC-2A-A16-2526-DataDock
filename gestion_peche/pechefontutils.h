#ifndef PECHEFONTUTILS_H
#define PECHEFONTUTILS_H

#include <QFont>
#include <QString>

class QWidget;

namespace PecheFontUtils {

QString ensureCalSansLoaded();
QString fontCssStack();
QFont moduleFont(qreal pointSize = -1.0,
                 QFont::Weight weight = QFont::Bold);
void applyModuleFont(QWidget *widget);

} // namespace PecheFontUtils

#endif // PECHEFONTUTILS_H
