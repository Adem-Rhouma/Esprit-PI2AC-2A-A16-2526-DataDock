#ifndef LOGINPAGE_H
#define LOGINPAGE_H

#include <QWidget>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QImageCapture>
#include <QImage>
#include "passwordresetserver.h"

class QLabel;
class QLineEdit;
class QPushButton;
class QAction;

class LoginPage : public QWidget
{
    Q_OBJECT

public:
    explicit LoginPage(QWidget *parent = nullptr);
    ~LoginPage();
    void focusUsername();

signals:
    void loginRequested();
    void faceImageCaptured(const QImage &image);

private slots:
    void onLoginClicked();
    void onFaceIdClicked();
    void onTogglePasswordVisibility();
    void onImageCaptured(int id, const QImage &preview);
    void onImageCaptureError(int id, QImageCapture::Error error, const QString &errorString);
    void onImageSaved(int id, const QString &fileName);
    void onForgotPasswordClicked();
private:
    void setupUi();
    void applyStyles();
    void updatePasswordToggleIcon();

    QLabel *logoLabel = nullptr;
    QLabel *titleLabel = nullptr;
    QLabel *subtitleLabel = nullptr;
    QLineEdit *usernameEdit = nullptr;
    QLineEdit *passwordEdit = nullptr;
    QAction *togglePasswordAction = nullptr;
    bool passwordVisible = false;
    QPushButton *forgotButton;
    QPushButton *loginButton = nullptr;
    QPushButton *faceIdButton = nullptr;

    QCamera *camera = nullptr;
    QMediaCaptureSession *captureSession = nullptr;
    QImageCapture *imageCapture = nullptr;

    QImage capturedFaceImage;
    PasswordResetServer *resetServer = nullptr;
};

#endif // LOGINPAGE_H
