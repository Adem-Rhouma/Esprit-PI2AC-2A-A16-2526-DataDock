#include "pechebasedialog.h"
#include "pechefontutils.h"
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QColor>
#include <QDir>
#include <QCoreApplication>

PecheBaseDialog::PecheBaseDialog(QWidget *parent)
    : QDialog(parent)
{
    PecheFontUtils::applyModuleFont(this);

    // Apply initial theming for the derived dialogs
    applyTheme();
}

void PecheBaseDialog::paintEvent(QPaintEvent *event)
{
    QDialog::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Load background image
    // The user provided the precise path
    QPixmap bkgnd("C:/Users/moonm/OneDrive/Desktop/2a16-smart-fishing-port-management/assets/img/winbg.png");
    
    if (!bkgnd.isNull()) {
        // Scale to fill the background, keeping aspect ratio by expanding to avoid stretching
        QPixmap scaled = bkgnd.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        
        // Calculate the drawing position to center the image
        int x = (width() - scaled.width()) / 2;
        int y = (height() - scaled.height()) / 2;
        
        painter.drawPixmap(x, y, scaled);
        
        // Add semi-transparent dark overlay for readability
        // Dark blue-ish overlay matching the theme
        painter.fillRect(rect(), QColor(10, 15, 30, 140)); 
    } else {
        painter.fillRect(rect(), QColor("#1a1a2e")); // Fallback color if image is missing
    }
}

void PecheBaseDialog::applyTheme()
{
    // Make sure the dialog background is transparent so the paintEvent is visible
    // Update styling for inner components to be slightly transparent and readable
    QString qss = this->styleSheet() + 
        "QDialog { background: transparent; }\n"
        "QWidget#scrollAreaWidgetContents { background: transparent; }\n"
        "QTableView { background-color: rgba(20, 25, 40, 0.8); border: 1px solid #3d4a6b; border-radius: 6px; gridline-color: #2c3553; color: white; }\n"
        "QHeaderView::section { background-color: rgba(30, 38, 56, 0.95); border: none; border-bottom: 2px solid #5a6b9c; padding: 4px; font-weight: bold; color: white; }\n"
        "QGroupBox { background-color: rgba(20, 25, 40, 0.6); border: 1px solid #3d4a6b; border-radius: 6px; margin-top: 2ex; font-weight: bold; color: #e0e5ff; }\n"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; }\n"
        "QLabel { color: #e0e5ff; }\n"
        "QComboBox, QLineEdit, QDateEdit, QSpinBox, QDoubleSpinBox, QPlainTextEdit { background-color: rgba(15, 20, 35, 0.8); border: 1px solid #3d4a6b; border-radius: 4px; color: white; padding: 4px; }\n"
        "QPushButton { background-color: rgba(50, 65, 100, 0.8); border: 1px solid #5a6b9c; border-radius: 4px; color: white; padding: 6px 12px; }\n"
        "QPushButton:hover { background-color: rgba(70, 85, 130, 0.9); }\n";
    this->setStyleSheet(qss);
}
