#include "mainwindow.h"

#include <QFile>
#include <QNetworkRequest>
#include <QShortcut>
#include <QUrlQuery>

#include "ui_mainwindow.h"
#include "utils.h"

namespace {
constexpr int kMaxTtsChunkLength = 1400;
constexpr int kFallbackTtsChunkLength = 700;
constexpr auto kPlay1IdleTooltip = "Play source text";
constexpr auto kPlay2IdleTooltip = "Play translation";
constexpr auto kPreparingTooltip = "Preparing audio...";
constexpr auto kBufferingTooltip = "Buffering audio...";
constexpr auto kStopTooltip = "Stop playback";
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  m_trayManager = new SystemTrayManager(this);

  connect(m_trayManager, &SystemTrayManager::showWindow, this, [=]() {
    this->showNormal();
    m_trayManager->updateMenu(this->isVisible());
  });
  connect(m_trayManager, &SystemTrayManager::hideWindow, this, [=]() {
    this->hide();
    m_trayManager->updateMenu(this->isVisible());
  });
  connect(m_trayManager, &SystemTrayManager::quitRequested, this, [=]() {
    m_forceQuit = true;
    this->close();
  });

  m_trayManager->updateMenu(true);

  this->setWindowTitle(QApplication::applicationName() + " v" +
                       QApplication::applicationVersion());
  this->setWindowIcon(QIcon(":/icons/app/icon-64.png"));

  setStyle(":/qbreeze/" + m_settings.value("theme", "dark").toString() +
           ".qss");

  // set layout
  m_translate = new controlButton(ui->translationWidget);
  m_translate->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  m_translate->setFixedSize(36, 36);
  m_translate->setMouseTracking(true);
  m_translate->setToolTip("Translate (Shift + Enter)");
  m_translate->setStyleSheet("border:none;border-radius:18px;");
  m_translate->setIconSize(QSize(28, 28));
  auto shortcut =
      new QShortcut(QKeySequence(tr("Shift+Return", "Translate")), m_translate,
                    SLOT(click()));
  shortcut->setContext(Qt::ApplicationShortcut);
  m_translate->setIcon(QIcon(":/icons/arrow-right-circle-line.png"));
  QTimer::singleShot(100, this, SLOT(resizeFix()));
  connect(m_translate, &QAbstractButton::clicked, this,
          &MainWindow::translate_clicked);

  ui->src1->installEventFilter(this);

  // read language codes and initialize vars
  readLangCode();

  // populate lang combos
  ui->lang1->blockSignals(true);
  ui->lang2->blockSignals(true);

  ui->lang1->addItem(QIcon(":/icons/translate-2.png"), "Auto Detect");
  ui->lang1->addItems(m_langName);
  ui->lang2->addItems(m_langName);
  for (int i = 0; i < ui->lang1->count(); i++) {
    if (i == 0) {
      ui->lang1->setItemIcon(i, QIcon(":/icons/magic-line.png"));
    } else {
      ui->lang1->setItemIcon(i, QIcon(":/icons/translate-2.png"));
    }
  }
  for (int i = 0; i < ui->lang2->count(); i++) {
    ui->lang2->setItemIcon(i, QIcon(":/icons/translate-2.png"));
  }

  // load settings
  ui->lang1->setCurrentIndex(m_settings.value("lang1", 0).toInt());
  ui->lang2->setCurrentIndex(m_settings.value("lang2", 20).toInt());

  ui->switch_lang->setEnabled(ui->lang1->currentIndex() != 0);

  ui->lang1->blockSignals(false);
  ui->lang2->blockSignals(false);

  ui->lang1->setMinimumWidth(150);
  ui->lang2->setMinimumWidth(150);

  // loader is the child of wall_view
  m_loader = new WaitingSpinnerWidget(m_translate, true, true);
  m_loader->setRoundness(70.0);
  m_loader->setMinimumTrailOpacity(15.0);
  m_loader->setTrailFadePercentage(70.0);
  m_loader->setNumberOfLines(30);
  m_loader->setLineLength(14);
  m_loader->setLineWidth(2);
  m_loader->setInnerRadius(2);
  m_loader->setRevolutionsPerSecond(3);
  m_loader->setColor(QColor(67, 124, 198));

  if (m_settings.value("geometry").isValid()) {
    restoreGeometry(m_settings.value("geometry").toByteArray());
    if (m_settings.value("windowState").isValid()) {
      restoreState(m_settings.value("windowState").toByteArray());
    } else {
      QScreen *pScreen = QGuiApplication::screenAt(
          this->mapToGlobal(QPoint(this->width() / 2, 0)));
      QRect availableScreenSize = pScreen->availableGeometry();
      this->move(availableScreenSize.center() - this->rect().center());
    }
  }

  // init media player
  m_player = new QMediaPlayer(this);
  m_ttsNetwork = new QNetworkAccessManager(this);
  auto audioOutput = new QAudioOutput(this);
  audioOutput->setVolume(1.0);
  m_player->setAudioOutput(audioOutput);
  connect(m_player, &QMediaPlayer::playbackStateChanged, this,
          [=](QMediaPlayer::PlaybackState state) {
            const QString activeButton = activePlayButtonName();
            if (!activeButton.isEmpty() &&
                state == QMediaPlayer::StoppedState) {
              setPlayButtonState(activeButton, QIcon(":/icons/volume-up-line.png"),
                                 activeButton == "play1"
                                     ? kPlay1IdleTooltip
                                     : kPlay2IdleTooltip);
              m_playSelected = false;
              m_selectedText.clear();
            }
            if (!activeButton.isEmpty() &&
                state == QMediaPlayer::PlayingState) {
              setPlayButtonState(activeButton, QIcon(":/icons/stop-line.png"),
                                 kStopTooltip);
            }
          });

  connect(m_player, &QMediaPlayer::errorOccurred, this,
          [=](QMediaPlayer::Error error, const QString &errorString) {
            Q_UNUSED(error);
            stopTtsSession(false);
            showError(errorString.isEmpty() ? m_player->errorString()
                                            : errorString);
          });

  connect(m_player, &QMediaPlayer::mediaStatusChanged, this,
          [=](QMediaPlayer::MediaStatus mediastate) {
            if (mediastate == QMediaPlayer::EndOfMedia) {
              if (m_ttsSessionActive) {
                ++m_ttsCurrentIndex;
                if (m_ttsCurrentIndex < m_ttsLocalFiles.size()) {
                  playCurrentTtsChunk();
                } else {
                  stopTtsSession(false);
                }
                return;
              }
            }

            const QString activeButton = activePlayButtonName();
            if (!activeButton.isEmpty() &&
                (mediastate == QMediaPlayer::LoadingMedia ||
                 mediastate == QMediaPlayer::BufferingMedia)) {
              setPlayButtonState(activeButton,
                                 QIcon(":/icons/loader-2-fill.png"),
                                 kBufferingTooltip);
            }
            if (!activeButton.isEmpty() &&
                (mediastate == QMediaPlayer::BufferedMedia)) {
              setPlayButtonState(activeButton, QIcon(":/icons/stop-line.png"),
                                 kStopTooltip);
            }
          });

  resetPlayIcons();

  // load src1
  ui->counter->setText("5000/5000");
  int minWid =
      ui->counter->fontMetrics().boundingRect(ui->counter->text()).width();
  ui->counter->setText(QString::number(ui->src1->toPlainText().length()) +
                       "/5000");

  ui->counter->setMinimumWidth(minWid);
  ui->counter_2->setMinimumWidth(minWid);

  m_clipboard = QApplication::clipboard();

  // init share
  m_share = new Share(this);
  m_share->setWindowTitle(QApplication::applicationName() +
                          " | Share translation");
  m_share->setWindowFlag(Qt::Dialog);
  m_share->setWindowModality(Qt::NonModal);

  m_lineByLine = new LineByLine(this, m_clipboard);
  m_lineByLine->setWindowTitle(QApplication::applicationName() +
                               " | LineByLine translation");
  m_lineByLine->setWindowFlag(Qt::Dialog);
  m_lineByLine->setWindowModality(Qt::NonModal);

  // init settings
  init_settings();

  // init history
  init_history();

  if (m_settings.value("lastTranslation").isValid()) {
    QFileInfo check(QFile(utils::returnPath("history") + "/" +
                          m_settings.value("lastTranslation").toString() +
                          ".json"));
    if (check.exists()) {
      m_translationId = m_settings.value("lastTranslation").toString();
      m_historyWidget->loadHistoryItem(utils::returnPath("history") + "/" +
                                       m_translationId + ".json");
    }
  }

  if (m_settings.value("firstRun").isValid() == false) {
    on_help_clicked();
    m_settings.setValue("firstRun", false);
  }
}

// TODO skip duplicate history item by checking translationId

void MainWindow::init_history() {
  utils::returnPath("history");
  m_historyWidget = new History(this);
  m_historyWidget->setWindowTitle(QApplication::applicationName() +
                                  " | History");
  m_historyWidget->setWindowFlag(Qt::Dialog);
  m_historyWidget->setWindowModality(Qt::NonModal);
  connect(m_historyWidget, &History::setTranslationId, this,
          [=](QString transId) { this->m_translationId = transId; });
  connect(m_historyWidget, &History::historyItemMeta, this,
          [=](QStringList historyItemMeta) {
            if (historyItemMeta.count() >= 4) {
              m_currentTranslationData.clear();
              m_settings.setValue(
                  "lastTranslation",
                  m_translationId); // translation id is set before this functor
                                    // is called via signal
              QString from, to, src1, src2;
              from = historyItemMeta.at(0);
              to = historyItemMeta.at(1);
              src1 = historyItemMeta.at(2);
              src2 = historyItemMeta.at(3);
              ui->src1->setText(src1);
              ui->src2->setText(src2);
              ui->lang1->setCurrentText(from);
              ui->lang2->setCurrentText(to);
            }
          });
}

void MainWindow::changeEvent(QEvent *e) {
  QMainWindow::changeEvent(e);
  //  if (e->type() == QEvent::WindowStateChange) {
  //    m_trayManager->updateMenu(this->isVisible());
  //  }
}

void MainWindow::init_settings() {
  m_nativeHotkey = new QHotkey(this);
  m_settingsWidget = new Settings(this, m_nativeHotkey);
  m_settingsWidget->setWindowTitle(QApplication::applicationName() +
                                   " | Settings");
  m_settingsWidget->setWindowFlag(Qt::Dialog);
  m_settingsWidget->setWindowModality(Qt::NonModal);
  connect(m_settingsWidget, &Settings::translationRequest, this,
          [=](const QString selected) {
            ui->src1->setText(selected);
            ui->lang1->setCurrentIndex(0);
            m_translate->click();
            this->setWindowState((this->windowState() & ~Qt::WindowMinimized) |
                                 Qt::WindowActive);
            this->show();
          });
  connect(m_settingsWidget, &Settings::themeToggled, this, [=]() {
    setStyle(":/qbreeze/" + m_settings.value("theme", "dark").toString() +
             ".qss");
  });
  const QString platform = QGuiApplication::platformName().toLower();
  const bool hotkeyUnsupported = platform.contains("wayland");
  if (!hotkeyUnsupported && m_settingsWidget->quickResultCheckBoxChecked() &&
      m_nativeHotkey->isRegistered() == false) {
    showError("Unable to register Global Hotkey.\nIs another instance of " +
              QApplication::applicationName() +
              "running ?\nIf yes close it and restart the application.");
  }
}

void MainWindow::textSelectionChanged(QTextEdit *editor) {
  QString selection = editor->textCursor().selectedText().trimmed();
  if (!selection.isEmpty()) {
    m_textOptionWidget->setObjectName("selection_" + editor->objectName());
    m_textOptionWidget->move(QCursor::pos().x(), QCursor::pos().y() + 10);
    m_textOptionWidget->resize(m_textOptionWidget->minimumSizeHint());
    m_textOptionWidget->adjustSize();
    m_textOptionWidget->show();

    textOptionForm.readPushButton->disconnect();
    textOptionForm.copyPushButton->disconnect();

    connect(textOptionForm.readPushButton, &QPushButton::clicked, this, [=]() {
      if (this->findChild<QTextEdit *>(m_textOptionWidget->objectName()
                                           .split("selection_")
                                           .last()
                                           .trimmed()) != nullptr) {
        m_playSelected = true;
        m_selectedText = selection;
        if (m_textOptionWidget->objectName()
                .split("selection_")
                .last()
                .trimmed() == "src1") {
          on_play1_clicked();
        } else {
          on_play2_clicked();
        }
        m_textOptionWidget->hide();
      }
    });

    connect(textOptionForm.copyPushButton, &QPushButton::clicked, this, [=]() {
      m_clipboard->setText(selection);
      m_textOptionWidget->hide();
    });
  } else {
    m_playSelected = false;
    m_selectedText.clear();
  }
}

bool MainWindow::eventFilter(QObject *o, QEvent *e) {
  // simulate translate shortcut instead of going to new line when cursor is on
  // textEdit widgets.
  if (e->type() == QEvent::KeyRelease || e->type() == QEvent::KeyPress ||
      e->type() == QEvent::ShortcutOverride) {
    if (o == ui->src1 || o == ui->src2) {
      auto keyEvent = static_cast<QKeyEvent *>(e);
      if ((keyEvent->key() == Qt::Key_Return) &&
          keyEvent->modifiers() & (Qt::ShiftModifier)) {
        m_translate->click();
        return true;
      } else {
        return false;
      }
    }
  }
  return QMainWindow::eventFilter(o, e);
}

void MainWindow::resizeFix() {
  this->resize(this->width() + 1, this->height());
  ui->left_buttons_widget->setMinimumWidth(ui->right_button_widget->width());
  ui->left_buttons_widget->resize(ui->right_button_widget->width(),
                                  ui->left_buttons_widget->height());
}

void MainWindow::closeEvent(QCloseEvent *event) {

  m_settings.setValue("geometry", saveGeometry());
  m_settings.setValue("windowState", saveState());

  // save quick trans shortcut
  m_settings.setValue("quicktrans_shortcut",
                      m_settingsWidget->keySequenceEditKeySequence());

  // save quicktrans settings
  m_settings.setValue("quicktrans",
                      m_settingsWidget->quickResultCheckBoxChecked());

  // Use live UI state for this close action, then persist it.
  const bool closeToTray =
      m_settingsWidget != nullptr
          ? m_settingsWidget->closeToTrayEnabled()
          : m_settings.value("closeToTray", true).toBool();
  m_settings.setValue("closeToTray", closeToTray);

  if (!m_forceQuit && m_trayManager->isTrayAvailable() && closeToTray) {
    m_trayManager->updateMenu(false);
    this->hide();
    event->ignore();
    return;
  }
  qApp->quit();

  QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
  int x =
      ui->translationWidget->geometry().center().x() - m_translate->width() / 2;
  int y = ui->translationWidget->geometry().center().y() -
          (m_translate->height() + m_translate->height() / 2);
  m_translate->move(x, y);
  ui->switch_lang->move(x, ui->switch_lang->y());
  ui->left_buttons_widget->resize(ui->right_button_widget->width(),
                                  ui->left_buttons_widget->height());
  QMainWindow::resizeEvent(event);
}

void MainWindow::readLangCode() {
  QFile lang(":/resources/lang.glate");
  if (!lang.open(QIODevice::ReadOnly)) {
    qWarning("Unable to open file");
    return;
  }
  while (!lang.atEnd()) {
    auto langLine = QString(lang.readLine());
    QStringList langItem = langLine.split("<=>");
    m_langName.append(langItem.at(0).trimmed());
    m_langCode.append(langItem.at(1).trimmed());
  }
  lang.close();

  // TTS
  QFile tts(":/resources/tts.glate");
  if (!tts.open(QIODevice::ReadOnly)) {
    qWarning("Unable to open file");
    return;
  }
  while (!tts.atEnd()) {
    auto ttsCode = QString(tts.readLine());
    m_supportedTts.append(ttsCode.trimmed());
  }
  tts.close();
}

void MainWindow::setStyle(const QString &fname) {
  QFile styleSheet(fname);
  if (!styleSheet.open(QIODevice::ReadOnly)) {
    qWarning("Unable to open file");
    return;
  }
  qApp->setStyleSheet(styleSheet.readAll());
  styleSheet.close();
}

MainWindow::~MainWindow() {
  stopTtsSession(true);
  delete ui;
}

void MainWindow::translate_clicked() {
  // clear processor vars
  m_currentTranslationData.clear();
  ui->src2->clear();

  if (m_request != nullptr) {
    m_request->blockSignals(true);
    m_request->deleteLater();
    m_request->blockSignals(false);
    m_request = nullptr;
    m_loader->stop();
  }

  if (m_request == nullptr) {
    m_request = new Request(this);
    connect(m_request, &Request::requestStarted, this,
            [=]() { m_loader->start(); });
    connect(m_request, &Request::requestFinished, this, [=](QString reply) {
      // save cache for line by line use
      saveByTransId(m_translationId, reply);
      // load to view
      processTranslation(reply);
      m_loader->stop();
      m_settings.setValue("lastTranslation", m_translationId);
    });
    connect(m_request, &Request::downloadError, this, [=](QString errorString) {
      m_loader->stop();
      showError(errorString);
    });

    QString src1 = ui->src1->toPlainText();

    QUrl url("https://translate.googleapis.com/translate_a/single");
    QUrlQuery urlQuery;

    urlQuery.addQueryItem("client", "gtx");
    urlQuery.addQueryItem("sl", getSourceLang());
    urlQuery.addQueryItem("tl", getTransLang());
    urlQuery.addQueryItem("dt", "t");
    urlQuery.addQueryItem("q", src1);

    urlQuery.setQueryDelimiters('=', '&');

    url.setQuery(urlQuery);

    QString urlStr = url.toString(QUrl::FullyEncoded);
    qDebug() << urlStr;

    if (urlStr.isEmpty()) {
      showError("Invalid Input.");
    } else {
      m_request->get(QUrl(urlStr));
      m_translationId = utils::generateRandomId(20);
    }
  }
}

void MainWindow::saveByTransId(const QString &translationId,
                               const QString &reply) {
  QFile jsonFile(utils::returnPath("cache") + "/" + translationId + ".glate");
  if (!jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    qWarning() << "Unable to open cache file for writing:" << jsonFile.fileName()
               << jsonFile.errorString();
    return;
  }

  const qint64 written = jsonFile.write(reply.toUtf8());
  if (written < 0) {
    qWarning() << "Unable to write cache file:" << jsonFile.fileName()
               << jsonFile.errorString();
  }

  jsonFile.close();
}

void MainWindow::processTranslation(const QString &reply) {
  m_currentTranslationData.clear();
  QJsonDocument jsonResponse = QJsonDocument::fromJson(reply.toUtf8());
  if (jsonResponse.isEmpty()) {
    showError(
        "Empty response returned.\n Please report to keshavnrj@gmail.com");
    return;
  }
  QJsonArray jsonArray = jsonResponse.array();
  foreach (const QJsonValue &val, jsonArray) {
    if (val.isArray() && !val.toArray().isEmpty()) {
      foreach (const QJsonValue &val2, val.toArray()) {
        if (val2.isArray() && !val2.toArray().isEmpty() &&
            val2.toArray().count() >= 2) {
          QStringList translationFregments;
          translationFregments << val2.toArray().at(0).toString();
          translationFregments << val2.toArray().at(1).toString();
          m_currentTranslationData.append(translationFregments);
        }
      }
    }
  }
  foreach (QStringList translationFregments, m_currentTranslationData) {
    ui->src2->append(translationFregments.at(0));
  }
  // scroll to top
  ui->src2->verticalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);
  // history save
  m_historyWidget->insertItem(
      QStringList() << getLangName(getSourceLang())
                    << getLangName(getTransLang()) << ui->src1->toPlainText()
                    << ui->src2->toPlainText()
                    << QDateTime::currentDateTime().toLocalTime().toString(),
      false, m_translationId);
}

QString MainWindow::getSourceLang() {
  int indexLang1 = ui->lang1->currentIndex();
  if (indexLang1 == 0) {
    return "auto";
  } else {
    return m_langCode.at(indexLang1 - 1).trimmed();
  }
}

QString MainWindow::getTransLang() {
  int indexLang2 = ui->lang2->currentIndex();
  return m_langCode.at(indexLang2).trimmed();
}

void MainWindow::showError(const QString &message) {
  QString temp = message;
  if (temp.contains("host", Qt::CaseInsensitive) ||
      temp.contains("translate.googleapis.com")) {
    temp.replace("translate.googleapis.com", "host");
  }
  // init error
  m_error = new Error(this);
  m_error->setAttribute(Qt::WA_DeleteOnClose, true);
  m_error->setWindowTitle(QApplication::applicationName() + " | Error dialog");
  m_error->setWindowFlag(Qt::Dialog);
  m_error->setWindowModality(Qt::NonModal);
  m_error->setError("An Error ocurred while processing your request!", temp);
  m_error->show();
}

void MainWindow::on_clear_clicked() {
  stopTtsSession(true);

  ui->src1->clear();
  ui->src2->clear();

  if (m_request != nullptr) {
    m_request->blockSignals(true);
    m_request->deleteLater();
    m_request->blockSignals(false);
    m_request = nullptr;
    m_loader->stop();
  }
  m_currentTranslationData.clear();
  m_translationId.clear();
  m_lineByLine->clearData();
}

void MainWindow::on_paste_clicked() {
  const QMimeData *mimeData = m_clipboard->mimeData();
  if (mimeData->hasText()) {
    ui->src1->setPlainText(mimeData->text());
  }
}

void MainWindow::on_switch_lang_clicked() {
  int index1 = ui->lang1->currentIndex();
  int index2 = ui->lang2->currentIndex();

  ui->lang1->setCurrentIndex(index2 + 1);
  ui->lang2->setCurrentIndex(index1 - 1);

  if (ui->src2->toPlainText().trimmed().isEmpty() == false) {
    QString src1 = ui->src1->toPlainText();
    QString src2 = ui->src2->toPlainText();

    ui->src2->setText(src1);
    ui->src1->setText(src2);
  }
}

void MainWindow::on_lang1_currentIndexChanged(int index) {
  ui->switch_lang->setEnabled(index != 0);
  m_settings.setValue("lang1", index);
  m_translate->setToolTip(
      "Translate from " + ui->lang1->itemText(index) + " to " +
      ui->lang2->itemText(ui->lang2->currentIndex()) + "(Shift + Enter)");
}

void MainWindow::on_lang2_currentIndexChanged(int index) {
  m_settings.setValue("lang2", index);
  m_translate->setToolTip("Translate from " +
                          ui->lang1->itemText(ui->lang1->currentIndex()) +
                          " to " + ui->lang2->itemText(index));
}

QString MainWindow::activePlayButtonName() const {
  const QStringList parts = m_player->objectName().split("_");
  return parts.isEmpty() ? QString() : parts.last();
}

void MainWindow::setPlayButtonState(const QString &buttonName, const QIcon &icon,
                                    const QString &toolTip) {
  if (buttonName == "play1") {
    ui->play1->setIcon(icon);
    ui->play1->setToolTip(toolTip);
  } else if (buttonName == "play2") {
    ui->play2->setIcon(icon);
    ui->play2->setToolTip(toolTip);
  }
}

void MainWindow::resetPlayIcons() {
  setPlayButtonState("play1", QIcon(":/icons/volume-up-line.png"),
                     kPlay1IdleTooltip);
  setPlayButtonState("play2", QIcon(":/icons/volume-up-line.png"),
                     kPlay2IdleTooltip);
}

bool MainWindow::retryCurrentTtsChunkWithFallback() {
  if (!m_ttsSessionActive || m_ttsCurrentIndex < 0 ||
      m_ttsCurrentIndex >= m_ttsChunks.size()) {
    return false;
  }

  const QString currentChunk = m_ttsChunks.at(m_ttsCurrentIndex);
  if (currentChunk.size() <= kFallbackTtsChunkLength) {
    return false;
  }

  QStringList fallbackChunks;
  if (!utils::splitString(currentChunk, kFallbackTtsChunkLength,
                          fallbackChunks) ||
      fallbackChunks.isEmpty()) {
    return false;
  }

  m_ttsChunks.removeAt(m_ttsCurrentIndex);
  for (int i = fallbackChunks.size() - 1; i >= 0; --i) {
    m_ttsChunks.insert(m_ttsCurrentIndex, fallbackChunks.at(i));
  }

  return true;
}

void MainWindow::stopTtsSession(bool stopPlayer) {
  if (m_ttsReply != nullptr) {
    m_ttsReply->abort();
    m_ttsReply->deleteLater();
    m_ttsReply = nullptr;
  }

  if (stopPlayer && m_player->playbackState() != QMediaPlayer::StoppedState) {
    m_player->stop();
  }

  m_ttsSessionActive = false;
  m_ttsChunks.clear();
  m_ttsLocalFiles.clear();
  m_ttsLang.clear();
  m_ttsGender.clear();
  m_ttsCurrentIndex = -1;
  m_playSelected = false;
  m_selectedText.clear();
  resetPlayIcons();

  if (m_ttsTempDir != nullptr) {
    delete m_ttsTempDir;
    m_ttsTempDir = nullptr;
  }
}

void MainWindow::playCurrentTtsChunk() {
  if (!m_ttsSessionActive || m_ttsCurrentIndex < 0 ||
      m_ttsCurrentIndex >= m_ttsLocalFiles.size()) {
    return;
  }

  const QString filePath = m_ttsLocalFiles.at(m_ttsCurrentIndex);
  if (filePath.isEmpty() || !QFile::exists(filePath)) {
    showError("Unable to play downloaded TTS audio chunk.");
    stopTtsSession(false);
    return;
  }

  m_player->setSource(QUrl::fromLocalFile(filePath));
  m_player->play();
}

void MainWindow::downloadCurrentTtsChunk() {
  if (!m_ttsSessionActive || m_ttsCurrentIndex < 0 ||
      m_ttsCurrentIndex >= m_ttsChunks.size()) {
    stopTtsSession(false);
    return;
  }

  if (m_ttsTempDir == nullptr) {
    m_ttsTempDir = new QTemporaryDir();
    if (!m_ttsTempDir->isValid()) {
      showError("Unable to create temporary directory for TTS audio.");
      stopTtsSession(false);
      return;
    }
  }

  QUrl url("https://www.google.com/speech-api/v1/synthesize");
  QUrlQuery params;
  params.addQueryItem("ie", "UTF-8");
  params.addQueryItem("lang", m_ttsLang);
  params.addQueryItem("gender", m_ttsGender);
  params.addQueryItem("text", m_ttsChunks.at(m_ttsCurrentIndex));
  url.setQuery(params);

  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    "Mozilla/5.0 (X11; Linux x86_64) Glate/4.0");

  QNetworkReply *reply = m_ttsNetwork->get(request);
  m_ttsReply = reply;

  connect(reply, &QNetworkReply::finished, this, [this, reply]() {
    if (reply != m_ttsReply) {
      return;
    }

    m_ttsReply = nullptr;

    if (!m_ttsSessionActive) {
      reply->deleteLater();
      return;
    }

    const int statusCode =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError || statusCode < 200 ||
        statusCode >= 300) {
      const QString err = reply->errorString().isEmpty()
                              ? QString("HTTP %1").arg(statusCode)
                              : reply->errorString();
      reply->deleteLater();

      if (retryCurrentTtsChunkWithFallback()) {
        downloadCurrentTtsChunk();
        return;
      }

      showError("Unable to download TTS audio: " + err);
      stopTtsSession(false);
      return;
    }

    const QByteArray data = reply->readAll();
    reply->deleteLater();
    if (data.isEmpty()) {
      if (retryCurrentTtsChunkWithFallback()) {
        downloadCurrentTtsChunk();
        return;
      }

      showError("TTS service returned empty audio data.");
      stopTtsSession(false);
      return;
    }

    const QString filePath =
        m_ttsTempDir->filePath(QString("tts_%1.mp3")
                                   .arg(m_ttsCurrentIndex, 4, 10, QChar('0')));
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      showError("Unable to store downloaded TTS audio chunk.");
      stopTtsSession(false);
      return;
    }
    file.write(data);
    file.close();

    while (m_ttsLocalFiles.size() <= m_ttsCurrentIndex) {
      m_ttsLocalFiles.append(QString());
    }
    m_ttsLocalFiles[m_ttsCurrentIndex] = filePath;

    // Pre-download all chunks first to avoid network-induced playback gaps.
    ++m_ttsCurrentIndex;
    if (m_ttsCurrentIndex < m_ttsChunks.size()) {
      downloadCurrentTtsChunk();
      return;
    }

    if (!m_ttsLocalFiles.isEmpty()) {
      m_ttsCurrentIndex = 0;
      playCurrentTtsChunk();
    } else {
      stopTtsSession(false);
    }
  });
}

void MainWindow::startTtsSession(const QString &playerName, const QString &text,
                                 const QString &ttsLang,
                                 const QString &voiceGender) {
  stopTtsSession(true);

  QStringList chunks;
  if (!utils::splitString(text, kMaxTtsChunkLength, chunks) || chunks.isEmpty()) {
    chunks = QStringList{text};
  }

  m_ttsChunks = chunks;
  m_ttsLang = ttsLang;
  m_ttsGender = voiceGender;
  m_ttsCurrentIndex = 0;
  m_ttsSessionActive = true;
  m_player->setObjectName("player_" + playerName);

  if (playerName == "play1") {
    setPlayButtonState("play1", QIcon(":/icons/loader-2-fill.png"),
                       kPreparingTooltip);
    setPlayButtonState("play2", QIcon(":/icons/volume-up-line.png"),
                       kPlay2IdleTooltip);
  } else {
    setPlayButtonState("play2", QIcon(":/icons/loader-2-fill.png"),
                       kPreparingTooltip);
    setPlayButtonState("play1", QIcon(":/icons/volume-up-line.png"),
                       kPlay1IdleTooltip);
  }

  downloadCurrentTtsChunk();
}

bool MainWindow::resolveTtsVoiceConfig(const QString &baseLang, QString &ttsLang,
                                       QString &ttsCheckLang,
                                       QString &voiceGender) const {
  const int voiceIdx = m_settings.value("voiceGender", 0).toInt();
  const auto voices = utils::availableVoices();
  const VoiceOption &voiceConf =
      voiceIdx < voices.size() ? voices.at(voiceIdx) : voices.first();

  voiceGender = voiceConf.gender;
  ttsLang = voiceConf.langOverride.isEmpty() ? baseLang : voiceConf.langOverride;
  ttsCheckLang =
      voiceConf.langOverride.isEmpty() ? baseLang
                                       : voiceConf.langOverride.split('-').first();
  return true;
}

void MainWindow::toggleOrStartTts(const QString &buttonName,
                                  const QString &baseLang,
                                  const QString &displayLang,
                                  const QString &text) {
  const QString activeButton = activePlayButtonName();
  const bool isPlaying =
      m_player->playbackState() == QMediaPlayer::PlayingState;

  if (isPlaying && activeButton == buttonName) {
    stopTtsSession(true);
    return;
  }

  if (isPlaying && activeButton != buttonName) {
    stopTtsSession(true);
  }

  QString ttsLang;
  QString ttsCheckLang;
  QString voiceGender;
  resolveTtsVoiceConfig(baseLang, ttsLang, ttsCheckLang, voiceGender);

  if (!m_supportedTts.contains(ttsCheckLang, Qt::CaseInsensitive)) {
    showError("Selected language '" + displayLang +
              "' is not supported by TTS Engine.\nPlease choose different "
              "Language.");
    return;
  }

  startTtsSession(buttonName, text, ttsLang, voiceGender);
}

void MainWindow::on_play2_clicked() {
  QString text = ui->src2->toPlainText();
  if (m_playSelected)
    text = m_selectedText;

  toggleOrStartTts("play2", getTransLang(), getTransLang(), text);
}

void MainWindow::on_play1_clicked() {
  QString text = ui->src1->toPlainText();
  if (m_playSelected)
    text = m_selectedText;

  toggleOrStartTts("play1", getSourceLang(), getSourceLang(), text);
}

void MainWindow::on_src1_textChanged() {
  ui->counter->setText(QString::number(ui->src1->toPlainText().length()) +
                       "/5000");
  if (ui->src1->toPlainText().length() > 5000) {
    int diff = ui->src1->toPlainText().length() - 5000;
    QString newStr = ui->src1->toPlainText();
    newStr.chop(diff);
    ui->src1->setText(newStr);
    QTextCursor cursor(ui->src1->textCursor());
    cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    ui->src1->setTextCursor(cursor);
  }
  ui->play1->setEnabled(!ui->src1->toPlainText().isEmpty());
}

void MainWindow::on_src2_textChanged() {
  // enable disable play button if text is not empty
  ui->play2->setEnabled(!ui->src2->toPlainText().isEmpty());
}

void MainWindow::on_copy_clicked() {
  const QString text = ui->src2->toPlainText();
  if (!text.isEmpty()) {
    m_clipboard->setText(text);
  }
}

void MainWindow::on_share_clicked() {
  m_share->setTranslation(ui->src2->toPlainText(), m_translationId,
                          getTransLang());
  if (m_share->isVisible() == false) {
    m_share->showNormal();
  }
}

void MainWindow::on_settings_clicked() {
  if (m_settingsWidget->isVisible() == false) {
    m_settingsWidget->adjustSize();
    QScreen *pScreen = QGuiApplication::screenAt(
        this->mapToGlobal(QPoint(m_settingsWidget->width() / 2, 0)));
    QRect availableScreenSize = pScreen->availableGeometry();
    m_settingsWidget->move(availableScreenSize.center() -
                           m_settingsWidget->rect().center());
    m_settingsWidget->showNormal();
  }
}

void MainWindow::on_history_clicked() {
  if (m_historyWidget->isVisible() == false) {
    m_historyWidget->loadHistory();
    QScreen *pScreen = QGuiApplication::screenAt(
        this->mapToGlobal(QPoint(m_historyWidget->width() / 2, 0)));
    QRect availableScreenSize = pScreen->availableGeometry();
    m_historyWidget->move(availableScreenSize.center() -
                          m_historyWidget->rect().center());
    m_historyWidget->showNormal();
  }
}

void MainWindow::on_lineByline_clicked() {
  if (m_translationId.isEmpty()) {
    showError("Translate something first.");
    return;
  }
  if (m_lineByLine->isVisible() == false) {
    if (m_translationId != m_lineByLine->getTId()) {
      m_lineByLine->clearData();
      if (m_currentTranslationData.isEmpty() == false) {
        m_lineByLine->setData(m_currentTranslationData,
                              getLangName(getSourceLang()),
                              getLangName(getTransLang()), m_translationId);
      } else {
        // load translation from cache;
        QFile jsonFile(utils::returnPath("cache") + "/" + m_translationId + ".glate");
        if (!jsonFile.open(QFile::ReadOnly)) {
          showError("Unable to open cached translation file.");
          return;
        }

        m_currentTranslationData.clear();
        processTranslation(QString::fromUtf8(jsonFile.readAll()));
        jsonFile.close();

        m_lineByLine->setData(m_currentTranslationData, getLangName(getSourceLang()),
                              getLangName(getTransLang()), m_translationId);
      }
    }
    QScreen *pScreen = QGuiApplication::screenAt(
        this->mapToGlobal(QPoint(m_lineByLine->width() / 2, 0)));
    QRect availableScreenSize = pScreen->availableGeometry();
    m_lineByLine->move(availableScreenSize.center() -
                       m_lineByLine->rect().center());
    m_lineByLine->showNormal();
  }
}

// convert lang code to lang name.
QString MainWindow::getLangName(const QString &langCode) {
  if (langCode == "auto") {
    return "Auto Detected";
  } else {
    QStringList langNameList = m_langName;
    langNameList.removeAt(0); // remove auto detect
    QString langName = langNameList.at(m_langCode.lastIndexOf(langCode) - 1);
    return langName;
  }
}

// not tested not being used yet
QString MainWindow::getLangCode(const QString &langName) {
  if (langName.contains("auto", Qt::CaseInsensitive)) {
    return "auto";
  } else {
    QStringList langCodeList = m_langCode;
    QString langCode = langCodeList.at(m_langName.lastIndexOf(langName) - 1);
    return langCode;
  }
}

void MainWindow::on_help_clicked() {
  if (m_slider == nullptr) {
    m_slider = new Slider(this);
    m_slider->setWindowTitle(QApplication::applicationName() +
                             " | Introduction");
    m_slider->setWindowFlag(Qt::Dialog);
    m_slider->setWindowModality(Qt::ApplicationModal);
    m_slider->setMinimumSize(680, 420);
  }
  if (m_slider->isVisible() == false) {
    m_slider->showNormal();
    m_slider->start();
  }
}
