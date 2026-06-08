#ifndef PECHEBASEDIALOG_H
#define PECHEBASEDIALOG_H

#include <QDialog>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>

class PecheBaseDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PecheBaseDialog(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void applyTheme();
};

#endif // PECHEBASEDIALOG_H
