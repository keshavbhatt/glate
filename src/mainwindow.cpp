#include "mainwindow.h"
#include "ui_mainwindow.h"

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
  QShortcut *shortcut =
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
      QScreen *pScreen =
          QGuiApplication::screenAt(this->mapToGlobal({this->width() / 2, 0}));
      QRect availableScreenSize = pScreen->availableGeometry();
      this->move(availableScreenSize.center() - this->rect().center());
    }
  }

  // init media player
  m_player = new QMediaPlayer(this, QMediaPlayer::StreamPlayback);
  m_player->setVolume(100);
  connect(m_player, &QMediaPlayer::stateChanged, this,
          [=](QMediaPlayer::State state) {
            QPushButton *playBtn = this->findChild<QPushButton *>(
                m_player->objectName().split("_").last());
            if (playBtn != nullptr && state == QMediaPlayer::StoppedState) {
              playBtn->setIcon(QIcon(":/icons/volume-up-line.png"));
              m_playSelected = false;
              m_selectedText.clear();
            }
            if (playBtn != nullptr && state == QMediaPlayer::PlayingState) {
              playBtn->setIcon(QIcon(":/icons/stop-line.png"));
            }
          });

  connect(m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),
          this, [=](QMediaPlayer::Error error) {
            Q_UNUSED(error);
            m_playSelected = false;
            m_selectedText.clear();
            QPushButton *playBtn = this->findChild<QPushButton *>(
                m_player->objectName().split("_").last());
            if (playBtn != nullptr)
              playBtn->setIcon(QIcon(":/icons/volume-up-line.png"));
            showError(m_player->errorString());
          });

  connect(m_player, &QMediaPlayer::mediaStatusChanged, this,
          [=](QMediaPlayer::MediaStatus mediastate) {
            QPushButton *playBtn = this->findChild<QPushButton *>(
                m_player->objectName().split("_").last());
            if (playBtn != nullptr &&
                (mediastate == QMediaPlayer::LoadingMedia ||
                 mediastate == QMediaPlayer::BufferingMedia)) {
              playBtn->setIcon(QIcon(":/icons/loader-2-fill.png"));
            }
            if (playBtn != nullptr &&
                (mediastate == QMediaPlayer::BufferedMedia)) {
              playBtn->setIcon(QIcon(":/icons/stop-line.png"));
            }
          });

  // load src1
  ui->counter->setText("5000/5000");
  int minWid =
      ui->counter->fontMetrics().boundingRect(ui->counter->text()).width();
  ui->counter->setText(QString::number(ui->src1->toPlainText().count()) +
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
  if (m_nativeHotkey->isRegistered() == false) {
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
      QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
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

  m_trayManager->updateMenu(false);

  if (QSystemTrayIcon::isSystemTrayAvailable()) {
    this->hide();
    event->ignore();
    return;
  }

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
    QString langLine = QString(lang.readLine());
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
    QString ttsCode = QString(tts.readLine());
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

MainWindow::~MainWindow() { delete ui; }

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
  jsonFile.open(QFile::WriteOnly);
  jsonFile.write(reply.toUtf8());
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

void MainWindow::on_play2_clicked() {
  bool player1Playing = m_player->objectName().split("_").last() == "play1" &&
                        m_player->state() == QMediaPlayer::PlayingState;

  if (m_player->state() != QMediaPlayer::PlayingState) {
    if (m_supportedTts.contains(getTransLang(), Qt::CaseInsensitive) == false) {
      showError("Selected language '" + getTransLang() +
                "' is not supported by TTS Engine.\nPlease choose different "
                "Language.");
      return;
    }
    m_player->setObjectName("player_play2");
    QString text = ui->src2->toPlainText();
    if (m_playSelected)
      text = m_selectedText;

    QStringList src2Parts;
    QMediaPlaylist *src2playlist = new QMediaPlaylist(m_player);
    src2playlist->setPlaybackMode(QMediaPlaylist::Sequential);
    connect(src2playlist, &QMediaPlaylist::currentIndexChanged, [=](int pos) {
      qDebug() << "Playist for player2 pos changed" << pos;
    });
    if (utils::splitString(text, 1500, src2Parts)) {
      foreach (QString src2Url, src2Parts) {
        QUrl url("https://www.google.com/speech-api/v1/synthesize");
        QUrlQuery params;
        params.addQueryItem("ie", "UTF-8");
        params.addQueryItem("lang", getTransLang());
        params.addQueryItem("text", QUrl::toPercentEncoding(src2Url.toUtf8()));
        url.setQuery(params);
        src2playlist->addMedia(QMediaContent(url));
      }
    }
    m_player->setPlaylist(src2playlist);
    m_player->play();
    ui->play2->setIcon(QIcon(":/icons/loader-2-fill.png"));
  } else {
    m_player->blockSignals(true);
    m_player->stop();
    ui->play1->setIcon(QIcon(":/icons/volume-up-line.png"));
    ui->play2->setIcon(QIcon(":/icons/volume-up-line.png"));
    m_player->blockSignals(false);
  }

  if (player1Playing) {
    on_play2_clicked();
  }
}

void MainWindow::on_play1_clicked() {
  bool player2Playing = m_player->objectName().split("_").last() == "play2" &&
                        m_player->state() == QMediaPlayer::PlayingState;

  if (m_player->state() != QMediaPlayer::PlayingState) {
    if (m_supportedTts.contains(getSourceLang(), Qt::CaseInsensitive) ==
        false) {
      showError("Selected language '" + getSourceLang() +
                "' is not supported by TTS Engine.\nPlease choose different "
                "Language.");
      return;
    }
    m_player->setObjectName("player_play1");
    QString text = ui->src1->toPlainText();
    if (m_playSelected)
      text = m_selectedText;

    QStringList src1Parts;
    QMediaPlaylist *src1playlist = new QMediaPlaylist(m_player);
    src1playlist->setPlaybackMode(QMediaPlaylist::Sequential);
    connect(src1playlist, &QMediaPlaylist::currentIndexChanged, [=](int pos) {
      qDebug() << "Playist for player1 pos changed" << pos;
    });
    if (utils::splitString(text, 1500, src1Parts)) {
      foreach (QString src1Url, src1Parts) {
        QUrl url("https://www.google.com/speech-api/v1/synthesize");
        QUrlQuery params;
        params.addQueryItem("ie", "UTF-8");
        params.addQueryItem("lang", getSourceLang());
        params.addQueryItem("text", QUrl::toPercentEncoding(src1Url.toUtf8()));
        url.setQuery(params);
        src1playlist->addMedia(QMediaContent(url));
      }
    }
    m_player->setPlaylist(src1playlist);
    m_player->play();
    ui->play1->setIcon(QIcon(":/icons/loader-2-fill.png"));
  } else {
    m_player->blockSignals(true);
    m_player->stop();
    ui->play1->setIcon(QIcon(":/icons/volume-up-line.png"));
    ui->play2->setIcon(QIcon(":/icons/volume-up-line.png"));
    m_player->blockSignals(false);
  }

  if (player2Playing) {
    on_play1_clicked();
  }
}

void MainWindow::on_src1_textChanged() {
  ui->counter->setText(QString::number(ui->src1->toPlainText().count()) +
                       "/5000");
  if (ui->src1->toPlainText().count() > 5000) {
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
  m_share->setTranslation(ui->src2->toPlainText(), m_translationId);
  if (m_share->isVisible() == false) {
    m_share->showNormal();
  }
}

void MainWindow::on_settings_clicked() {
  if (m_settingsWidget->isVisible() == false) {
    m_settingsWidget->adjustSize();
    QScreen *pScreen = QGuiApplication::screenAt(
        this->mapToGlobal({m_settingsWidget->width() / 2, 0}));
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
        this->mapToGlobal({m_historyWidget->width() / 2, 0}));
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
        QFile jsonFile(utils::returnPath("cache") + "/" + m_translationId +
                       ".glate");
        jsonFile.open(QFile::ReadOnly);
        m_currentTranslationData.clear();
        processTranslation(jsonFile.readAll());
        m_lineByLine->setData(m_currentTranslationData,
                              getLangName(getSourceLang()),
                              getLangName(getTransLang()), m_translationId);
      }
    }
    QScreen *pScreen = QGuiApplication::screenAt(
        this->mapToGlobal({m_lineByLine->width() / 2, 0}));
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
