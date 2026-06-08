#pragma once

#include <QPixmap>
#include <QTimer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLayout;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
class QProgressBar;
class QPushButton;
class QTextEdit;
class QLineEdit;
QT_END_NAMESPACE

class QHBoxLayout;
class QVBoxLayout;
class QStackedWidget;
class QScrollArea;
class QFrame;

struct FishVisionIndicator
{
    QString label;
    QString status;
};

struct FishVisionRecommendation
{
    QString icon;
    QString titre;
    QString detail;
};

struct FishVisionPositionnement
{
    QString label;
    QString valeur;
    QString status;
};

struct FishVisionResult
{
    QString espece;
    QString nomScientifique;
    QString poidsEstime;
    QStringList tags;
    QString grade;
    int scoreFraicheur = 0;
    int scoreOeil = 0;
    int scoreBranchies = 0;
    int scorePeau = 0;
    QString verdict;
    QString rapportIA;
    QList<FishVisionIndicator> indicateurs;
    double valeurKg = 0.0;
    double fourchetteMin = 0.0;
    double fourchetteMax = 0.0;
    QStringList canauxVente;
    QList<FishVisionRecommendation> recommandations;
    QList<FishVisionPositionnement> positionnement;
    bool loaded = false;
};

struct SavedAnalysis {
    QString id;
    QString date;
    int linkedPecheId = 0;
    QString imagePath;
    FishVisionResult result;
};

class ScoreRingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ScoreRingWidget(QWidget *parent = nullptr);

    void setScore(int score);
    void setCaption(const QString &caption);
    int score() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_score = 0;
    QString m_caption;
};

class GradientValueWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GradientValueWidget(QWidget *parent = nullptr);

    void setValue(int value);
    int value() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize minimumSizeHint() const override;

private:
    int m_value = 0;
};

class FishVisionPage : public QWidget
{
    Q_OBJECT

public:
    explicit FishVisionPage(QWidget *parent = nullptr);
    ~FishVisionPage() override;

signals:
    void backRequested();

private slots:
    void onImporterImageClicked();
    void onLancerDemoClicked();
    void onRetourClicked();
    void onTabFraicheurClicked();
    void onTabRapportClicked();
    void onTabMarcheClicked();
    void onTabRecommandationsClicked();
    void onTabAssistantClicked();
    void onSauvegarderClicked();
    void onNouvelleAnalyseClicked();
    void onRattacherClicked();
    void onExporterPdfClicked();
    void onHistoriqueClicked();
    void onEnvoyerChatClicked();
    void animerScore();
    void onGeminiReplyFinished(QNetworkReply *reply);
    void processGeminiChatReply(QNetworkReply *reply);

private:
    void setupUi();
    void setupStyleSheet();
    void afficherEtatVide();
    void afficherChargement(const QString &sourceLabel);
    void afficherErreur(const QString &message);
    void afficherResultat(const FishVisionResult &result);
    void mettreAJourOngletActif(int tabIndex);
    void setFishImage(const QPixmap &pixmap);
    void clearLayoutWidgets(QLayout *layout);
    QWidget *creerCarteRecente(const QString &espece, int score, const QString &etat, const QString &couleurAccent);
    QWidget *creerBadge(const QString &texte, const QString &type);
    QWidget *creerCanalBadge(const QString &texte);
    QWidget *creerIndicatorCard(const FishVisionIndicator &indicator);
    QWidget *creerItemRecommandation(const QString &icone, const QString &titre, const QString &detail);
    QWidget *creerItemTimeline(const QString &couleur, const QString &texte);
    QWidget *creerPositionnementItem(const FishVisionPositionnement &positionnement);
    QString demanderImageEtAnalyser();
    void analyserImage(const QString &filePath);
    QString detecterMimeType(const QString &filePath) const;
    QByteArray construireGeminiPayload(const QByteArray &imageBytes, const QString &mimeType) const;
    QString extraireTexteGemini(const QByteArray &responseBytes, QString *errorMessage) const;
    bool parserResultatGemini(const QString &jsonText, FishVisionResult *result, QString *errorMessage) const;
    void remplirLayoutsAnalyse(const FishVisionResult &result);
    void setLoadingUiEnabled(bool enabled);
    void updateBarTone(QProgressBar *bar, int score);
    void applyGradeStyle(const QString &grade, const QString &text = QString());
    void updateVerdictWidgets(int score, const QString &verdict, const QString &subtext);
    void polishWidget(QWidget *widget) const;
    void appendChatBotMessage(const QString &text, bool fromUser);
    bool isFishVisionStorageReady(QString *errorMessage) const;
    bool saveCurrentAnalysisToDatabase(int linkedPecheId, QString *savedId, QString *errorMessage);
    bool loadSavedAnalysesFromDatabase(QString *errorMessage = nullptr);
    bool loadSavedAnalysisDetails(int analysisId, SavedAnalysis *analysis, QString *errorMessage) const;
    QList<QPair<int, QString>> fetchAvailablePeches(QString *errorMessage) const;

    // GUI Elements
    QWidget *pageVide = nullptr;
    QWidget *pageResultat = nullptr;
    
    // Header
    QPushButton *btnRetour = nullptr;
    QLabel *lblTitrePage = nullptr;
    QLabel *lblSoustitrePage = nullptr;
    QPushButton *btnHistorique = nullptr;
    QPushButton *btnNouvelleAnalyse = nullptr;
    
    // Upload page
    QFrame *frameUpload = nullptr;
    QLabel *lblIconePoisson = nullptr;
    QLabel *labelUploadTitre = nullptr;
    QLabel *labelUploadSousTitre = nullptr;
    QPushButton *btnImporterImage = nullptr;
    QPushButton *btnLancerDemo = nullptr;
    QLabel *lblSectionTitle = nullptr;
    QHBoxLayout *layoutRecents = nullptr;
    
    // Result Main
    QStackedWidget *stackedContent = nullptr;
    QScrollArea *scrollResultat = nullptr;
    QWidget *scrollAreaWidgetContents = nullptr;
    
    // Result Header
    QFrame *carteResultatHeader = nullptr;
    QLabel *lblImagePoisson = nullptr;
    QLabel *lblNomEspece = nullptr;
    QLabel *lblNomScientifique = nullptr;
    QLabel *badgeAI = nullptr;
    QLabel *lblGrade = nullptr;
    QHBoxLayout *layoutChipsHeader = nullptr;
    
    // Tabs
    QWidget *barreOnglets = nullptr;
    QPushButton *btnOngletFraicheur = nullptr;
    QPushButton *btnOngletRapport = nullptr;
    QPushButton *btnOngletMarche = nullptr;
    QPushButton *btnOngletRecommandations = nullptr;
    QPushButton *btnOngletAssistant = nullptr;
    QStackedWidget *stackOnglets = nullptr;
    
    QWidget *pageFraicheur = nullptr;
    QWidget *pageRapport = nullptr;
    QWidget *pageMarche = nullptr;
    QWidget *pageRecommandations = nullptr;
    QWidget *pageAssistant = nullptr;
    
    // Fraicheur
    QFrame *carteAnalyseIndicateurs = nullptr;
    QLabel *labelIndicateurs = nullptr;
    QLabel *labelOeil = nullptr;
    QProgressBar *barreOeil = nullptr;
    QLabel *lblValOeil = nullptr;
    QLabel *labelBranchies = nullptr;
    QProgressBar *barreBranchies = nullptr;
    QLabel *lblValBranchies = nullptr;
    QLabel *labelPeau = nullptr;
    QProgressBar *barrePeau = nullptr;
    QLabel *lblValPeau = nullptr;
    QLabel *labelEcailles = nullptr;
    QProgressBar *barreEcailles = nullptr;
    QLabel *lblValEcailles = nullptr;
    QLabel *labelChair = nullptr;
    QProgressBar *barreChair = nullptr;
    QLabel *lblValChair = nullptr;
    QLabel *lblNote = nullptr;
    
    QFrame *carteVerdict = nullptr;
    QLabel *labelVerdict = nullptr;
    QHBoxLayout *verdictHeader = nullptr;
    QLabel *lblIconeVerdict = nullptr;
    QLabel *lblVerdictTitre = nullptr;
    QLabel *lblVerdictGrade = nullptr;
    QLabel *lblAgeCapture = nullptr;
    QLabel *lblFenetreVente = nullptr;
    QVBoxLayout *layoutTimeline = nullptr;
    
    // Rapport
    QFrame *carteRapportAI = nullptr;
    QFrame *blocRapportAI = nullptr;
    QLabel *labelRapportTitre = nullptr;
    QLabel *lblRapportTexte = nullptr;
    QHBoxLayout *layoutRapportChips = nullptr;
    QFrame *carteFlags = nullptr;
    QLabel *labelFlags = nullptr;
    QHBoxLayout *layoutDetectionBadges = nullptr;
    
    // Marche
    QFrame *carteValeurEstimee = nullptr;
    QLabel *labelValeur = nullptr;
    QLabel *lblPrixKg = nullptr;
    QLabel *labelValeurSousTitre = nullptr;
    QLabel *labelFourchette = nullptr;
    QFrame *cartePositionnement = nullptr;
    QLabel *labelPositionnement = nullptr;
    QLabel *labelPositionnementNote = nullptr;
    QVBoxLayout *layoutPositionnement = nullptr;
    QFrame *carteAcheteurs = nullptr;
    QLabel *labelCanaux = nullptr;
    QHBoxLayout *layoutCanauxVente = nullptr;
    
    // Recommandations
    QFrame *carteRecommandations = nullptr;
    QLabel *labelRecommandations = nullptr;
    QVBoxLayout *layoutRecommandations = nullptr;
    
    // Assistant IA
    QTextEdit *chatHistory = nullptr;
    QLineEdit *chatInput = nullptr;
    QPushButton *btnSendChat = nullptr;
    
    QPushButton *btnSauvegarder = nullptr;
    QPushButton *btnRattacher = nullptr;
    QPushButton *btnExporterPdf = nullptr;
    QPushButton *btnNouvelleAnalyseBas = nullptr;
    
    QList<SavedAnalysis> m_savedAnalyses;
    
    FishVisionResult m_result;
    QTimer *m_scoreTimer = nullptr;
    ScoreRingWidget *m_scoreRing = nullptr;
    GradientValueWidget *m_verdictScore = nullptr;
    QLabel *m_headerBadge = nullptr;
    QNetworkAccessManager *m_networkManager = nullptr;
    int m_scoreAnimCurrent = 0;
    int m_activeTab = 0;
    QPixmap m_loadedPixmap;
    QString m_currentImagePath;
    bool m_isLoading = false;
    bool m_resetInProgress = false;
    int m_currentAnalysisId = 0;
    int m_currentLinkedPecheId = 0;
};
