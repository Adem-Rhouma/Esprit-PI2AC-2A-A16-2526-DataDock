# Project configuration refreshed after system crash
TEMPLATE = app
TARGET = DataDock
QT       += core gui sql multimedia network charts printsupport quickwidgets quick qml location positioning testlib serialport


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Expose the source-tree directories to C++ at compile time
DEFINES += NAVIRES_SCRIPTS_DIR=\\\"$$PWD/gestion_navires/scripts\\\"
DEFINES += PROJECT_SOURCE_DIR=\\\"$$PWD\\\"

SOURCES += \
    arduino.cpp \
    connection.cpp \
    gestion_navires/navire.cpp \
    gestion_navires/navirecruddialog.cpp \
    gestion_employes/email.cpp \
    gestion_employes/employe.cpp \
    gestion_employes/loginpage.cpp \
    gestion_employes/passwordresetserver.cpp \
    main.cpp \
    mainwindow.cpp \
    gestion_chambres_froides/chambresfroidespage.cpp \
    gestion_chambres_froides/chambresfroides.cpp \
    gestion_chambres_froides/chambresfroidesstatspage.cpp \
    gestion_chambres_froides/chambresfroidesmodulewidget.cpp \
    gestion_chambres_froides/chambresfroidesdialog.cpp \
    gestion_chambres_froides/chambresfroidesalertinbox.cpp \
    gestion_chambres_froides/chambresfroidesoptimizationpage.cpp \
    gestion_chambres_froides/scenariodetailsdialog.cpp \
    gestion_chambres_froides/chambresfroidesconfigdialog.cpp \
    gestion_chambres_froides/chambresfroidesexportpage.cpp \
    gestion_equipements/equipementsmodulewidget.cpp \
    gestion_equipements/equipementspage.cpp \
    gestion_equipements/materiel.cpp \
    gestion_equipements/equipementsstatspage.cpp \
    gestion_equipements/equipementsqrcodepage.cpp \
    gestion_equipements/equipementssmsalertspage.cpp \
    gestion_equipements/qrcodegen.cpp \
    gestion_employes/employesmodulewidget.cpp \
    gestion_employes/employespage.cpp \
    gestion_employes/employestatspage.cpp \
    gestion_logistique/logistiquemodulewidget.cpp \
    gestion_logistique/logistiquepage.cpp \
    gestion_logistique/views/logistiquestatspage.cpp \
    gestion_logistique/views/camionspage.cpp \
    gestion_logistique/views/operationscamionspage.cpp \
    gestion_logistique/views/camionmappage.cpp \
    gestion_logistique/views/camionefficiencypage.cpp \
    gestion_logistique/views/camionactivitypage.cpp \
    gestion_logistique/models/camion.cpp \
    gestion_logistique/models/operationcamion.cpp \
    gestion_navires/navirespage.cpp \
    gestion_navires/naviredetailpage.cpp \
    gestion_navires/meteoglobalservice.cpp \
    gestion_navires/meteostripwidget.cpp \
    gestion_navires/naviresstatspage.cpp \
    gestion_navires/navirepredictivepage.cpp \
    gestion_navires/naviresmodulewidget.cpp \
    gestion_navires/meteoclient.cpp \
    gestion_navires/maintenanceengine.cpp \
    gestion_navires/alertservice.cpp \
    gestion_navires/meteowidget.cpp \
    gestion_navires/maintenancepage.cpp \
    gestion_peche/dynamicpricingdialog.cpp \
    gestion_peche/pechecruddialog.cpp \
    gestion_peche/pechedetailsdialog.cpp \
    gestion_peche/pechearchiveddialog.cpp \
    gestion_peche/pechepage.cpp \
    gestion_peche/pechemodulewidget.cpp \
    gestion_peche/pechestatspage.cpp \
    gestion_peche/pechesmartservicespage.cpp \
    gestion_peche/pecherecommendationspage.cpp \
    gestion_peche/pechefontutils.cpp \
    gestion_peche/fishvision/fishvisionpage.cpp \
    gestion_peche/model/peche.cpp \
    gestion_peche/model/pechedao.cpp \
    gestion_peche/model/pecheespece.cpp \
    gestion_peche/model/pecheemploye.cpp \
    gestion_peche/pechebasedialog.cpp \
    gestion_peche/dynamic_pricing/repository/dynamicpricingrepository.cpp \
    gestion_peche/dynamic_pricing/service/gfwintelligenceservice.cpp \
    gestion_peche/dynamic_pricing/service/marineweatherservice.cpp \
    gestion_peche/dynamic_pricing/ui/dynamicpricingpage.cpp \
    gestion_peche/dynamic_pricing/ui/pricehistorytablemodel.cpp
HEADERS += \
    arduino.h \
    connection.h \
    gestion_navires/navire.h \
    gestion_navires/navirecruddialog.h \
    gestion_employes/email.h \
    gestion_employes/employe.h \
    gestion_employes/loginpage.h \
    gestion_employes/passwordresetserver.h \
    mainwindow.h \
    gestion_chambres_froides/chambresfroidespage.h \
    gestion_chambres_froides/chambresfroidesstatspage.h \
    gestion_chambres_froides/chambresfroides.h \
    gestion_chambres_froides/chambresfroidesmodulewidget.h \
    gestion_chambres_froides/chambresfroidesdialog.h \
    gestion_chambres_froides/chambresfroidesalertinbox.h \
    gestion_chambres_froides/chambresfroidesoptimizationpage.h \
    gestion_chambres_froides/scenariodetailsdialog.h \
    gestion_chambres_froides/chambresfroidesconfigdialog.h \
    gestion_chambres_froides/chambresfroidesexportpage.h \
    gestion_equipements/equipementsmodulewidget.h \
    gestion_equipements/equipementspage.h \
    gestion_equipements/materiel.h \
    gestion_equipements/equipementsstatspage.h \
    gestion_equipements/equipementsqrcodepage.h \
    gestion_equipements/equipementssmsalertspage.h \
    gestion_equipements/qrcodegen.hpp \
    gestion_employes/employesmodulewidget.h \
    gestion_employes/employespage.h \
    gestion_employes/employestatspage.h \
    gestion_logistique/logistiquemodulewidget.h \
    gestion_logistique/logistiquepage.h \
    gestion_logistique/views/logistiquestatspage.h \
    gestion_logistique/views/camionspage.h \
    gestion_logistique/views/operationscamionspage.h \
    gestion_logistique/views/camionmappage.h \
    gestion_logistique/models/camion.h \
    gestion_logistique/models/operationcamion.h \
    gestion_navires/navirespage.h \
    gestion_navires/naviredetailpage.h \
    gestion_navires/meteoglobalservice.h \
    gestion_navires/meteostripwidget.h \
    gestion_navires/naviresstatspage.h \
    gestion_navires/navirepredictivepage.h \
    gestion_navires/naviresmodulewidget.h \
    gestion_navires/NavireConstants.h \
    gestion_navires/NavireDatabaseHelper.h \
    gestion_navires/meteo_data.h \
    gestion_navires/maintenancerule.h \
    gestion_navires/vesselusage.h \
    gestion_navires/maintenancemodel.h \
    gestion_navires/meteoclient.h \
    gestion_navires/meteotranslator.h \
    gestion_navires/maintenanceengine.h \
    gestion_navires/emailnotifier.h \
    gestion_navires/alertservice.h \
    gestion_navires/meteowidget.h \
    gestion_navires/maintenancepage.h \
    gestion_navires/maintenancegantt.h \
    gestion_navires/naviretests.h \
    gestion_peche/dynamicpricingdialog.h \
    gestion_peche/pechecruddialog.h \
    gestion_peche/pechedetailsdialog.h \
    gestion_peche/pechearchiveddialog.h \
    gestion_peche/pechepage.h \
    gestion_peche/pechemodulewidget.h \
    gestion_peche/pechestatspage.h \
    gestion_peche/pechesmartservicespage.h \
    gestion_peche/pecherecommendationspage.h \
    gestion_peche/pechefontutils.h \
    gestion_peche/fishvision/fishvisionpage.h \
    gestion_peche/model/peche.h \
    gestion_peche/model/pechedao.h \
    gestion_peche/model/pecheespece.h \
    gestion_peche/model/pecheemploye.h \
    gestion_peche/pechebasedialog.h \
    gestion_peche/dynamic_pricing/models/dynamicpricingmodels.h \
    gestion_peche/dynamic_pricing/models/marinedto.h \
    gestion_peche/dynamic_pricing/repository/dynamicpricingrepository.h \
    gestion_peche/dynamic_pricing/service/gfwintelligenceservice.h \
    gestion_peche/dynamic_pricing/service/marineweatherservice.h \
    gestion_peche/dynamic_pricing/ui/dynamicpricingpage.h \
    gestion_logistique/views/camionefficiencypage.h \
    gestion_logistique/views/camionactivitypage.h \
    gestion_peche/dynamic_pricing/ui/pricehistorytablemodel.h


FORMS += \
    gestion_employes/loginpage.ui \
    mainwindow.ui \
    gestion_chambres_froides/chambresfroidespage.ui \
    gestion_chambres_froides/chambresfroidesstatspage.ui \
    gestion_chambres_froides/chambresfroidesdialog.ui \
    gestion_chambres_froides/chambresfroidesoptimizationpage.ui \
    gestion_equipements/equipementspage.ui \
    gestion_equipements/equipementsstatspage.ui \
    gestion_employes/employespage.ui \
    gestion_employes/employestatspage.ui \
    gestion_logistique/logistiquepage.ui \
    gestion_logistique/logistiquestatspage.ui \
    gestion_navires/navirespage.ui \
    gestion_navires/naviresstatspage.ui \
    gestion_navires/naviredetailpage.ui \
    gestion_navires/navirepredictivepage.ui \
    gestion_navires/meteowidget.ui \
    gestion_navires/maintenancepage.ui \
    gestion_navires/navirecruddialog.ui \
    gestion_peche/dynamicpricingdialog.ui \
    gestion_peche/pechecruddialog.ui \
    gestion_peche/pechedetailsdialog.ui \
    gestion_peche/pechearchiveddialog.ui \
    gestion_peche/pechepage.ui \
    gestion_peche/pechestatspage.ui \
    gestion_peche/pechesmartservicespage.ui


RESOURCES += \
    assets.qrc \
    gestion_peche/assets/assets_peche.qrc

# Deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    emaill/HashInfo.txt \
    emaill/OpenSSL License.txt \
    emaill/ReadMe.txt \
    emaill/libeay32.dll \
    emaill/openssl.exe \
    emaill/ssleay32.dll \
    faceidd/README.md \
    faceidd/app.py \
    faceidd/requirements.txt
