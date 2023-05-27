#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QMimeData>
#include <QTimer>

#include <QDesktopWidget>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaPlaylist>
#include <QScreen>
#include <QScrollBar>
#include <QVariant>

#include <QKeyEvent>
#include <QKeySequence>
#include <QShortcut>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  this->setWindowTitle(QApplication::applicationName() + " v" +
                       QApplication::applicationVersion());
  this->setWindowIcon(QIcon(":/icons/app/icon-64.png"));

  setStyle(":/qbreeze/" + settings.value("theme", "dark").toString() + ".qss");

  // set layout
  _translate = new controlButton(ui->translationWidget);
  _translate->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  _translate->setFixedSize(36, 36);
  _translate->setMouseTracking(true);
  _translate->setToolTip("Translate (Shift + Enter)");
  _translate->setStyleSheet("border:none;border-radius:18px;");
  _translate->setIconSize(QSize(28, 28));
  QShortcut *shortcut = new QShortcut(
      QKeySequence(tr("Shift+Return", "Translate")), _translate, SLOT(click()));
  shortcut->setContext(Qt::ApplicationShortcut);
  _translate->setIcon(QIcon(":/icons/arrow-right-circle-line.png"));
  QTimer::singleShot(100, this, SLOT(resizeFix()));
  connect(_translate, &QAbstractButton::clicked, this,
          &MainWindow::translate_clicked);

  // read language codes and initialize vars
  readLangCode();

  // populate lang combos
  ui->lang1->blockSignals(true);
  ui->lang2->blockSignals(true);

  ui->lang1->addItem(QIcon(":/icons/translate-2.png"), "Auto Detect");
  ui->lang1->addItems(_langName);
  ui->lang2->addItems(_langName);
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
  ui->lang1->setCurrentIndex(settings.value("lang1", 0).toInt());
  ui->lang2->setCurrentIndex(settings.value("lang2", 20).toInt());

  ui->switch_lang->setEnabled(ui->lang1->currentIndex() != 0);

  ui->lang1->blockSignals(false);
  ui->lang2->blockSignals(false);

  ui->lang1->setMinimumWidth(150);
  ui->lang2->setMinimumWidth(150);

  // loader is the child of wall_view
  _loader = new WaitingSpinnerWidget(_translate, true, true);
  _loader->setRoundness(70.0);
  _loader->setMinimumTrailOpacity(15.0);
  _loader->setTrailFadePercentage(70.0);
  _loader->setNumberOfLines(30);
  _loader->setLineLength(14);
  _loader->setLineWidth(2);
  _loader->setInnerRadius(2);
  _loader->setRevolutionsPerSecond(3);
  _loader->setColor(QColor(67, 124, 198));

  if (settings.value("geometry").isValid()) {
    restoreGeometry(settings.value("geometry").toByteArray());
    if (settings.value("windowState").isValid()) {
      restoreState(settings.value("windowState").toByteArray());
    } else {
      QScreen *pScreen =
          QGuiApplication::screenAt(this->mapToGlobal({this->width() / 2, 0}));
      QRect availableScreenSize = pScreen->availableGeometry();
      this->move(availableScreenSize.center() - this->rect().center());
    }
  }

  // init media player
  _player = new QMediaPlayer(this, QMediaPlayer::StreamPlayback);
  _player->setVolume(100);
  connect(_player, &QMediaPlayer::stateChanged, this,
          [=](QMediaPlayer::State state) {
            QPushButton *playBtn = this->findChild<QPushButton *>(
                _player->objectName().split("_").last());
            if (playBtn != nullptr && state == QMediaPlayer::StoppedState) {
              playBtn->setIcon(QIcon(":/icons/volume-up-line.png"));
              playSelected = false;
              selectedText.clear();
            }
            if (playBtn != nullptr && state == QMediaPlayer::PlayingState) {
              playBtn->setIcon(QIcon(":/icons/stop-line.png"));
            }
          });

  connect(_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),
          this, [=](QMediaPlayer::Error error) {
            Q_UNUSED(error);
            playSelected = false;
            selectedText.clear();
            QPushButton *playBtn = this->findChild<QPushButton *>(
                _player->objectName().split("_").last());
            if (playBtn != nullptr)
              playBtn->setIcon(QIcon(":/icons/volume-up-line.png"));
            showError(_player->errorString());
          });

  connect(_player, &QMediaPlayer::mediaStatusChanged, this,
          [=](QMediaPlayer::MediaStatus mediastate) {
            QPushButton *playBtn = this->findChild<QPushButton *>(
                _player->objectName().split("_").last());
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

  clipboard = QApplication::clipboard();

  // init share
  _share = new Share(this);
  _share->setWindowTitle(QApplication::applicationName() +
                         " | Share translation");
  _share->setWindowFlag(Qt::Dialog);
  _share->setWindowModality(Qt::NonModal);

  _lineByLine = new LineByLine(this, clipboard);
  _lineByLine->setWindowTitle(QApplication::applicationName() +
                              " | LineByLine translation");
  _lineByLine->setWindowFlag(Qt::Dialog);
  _lineByLine->setWindowModality(Qt::NonModal);

  // init settings
  init_settings();

  // init history
  init_history();

  if (settings.value("lastTranslation").isValid()) {
    QFileInfo check(QFile(utils::returnPath("history") + "/" +
                          settings.value("lastTranslation").toString() +
                          ".json"));
    if (check.exists()) {
      translationId = settings.value("lastTranslation").toString();
      historyWidget->loadHistoryItem(utils::returnPath("history") + "/" +
                                     translationId + ".json");
    }
  }

  if (settings.value("firstRun").isValid() == false) {
    on_help_clicked();
    settings.setValue("firstRun", false);
  }
}

// TODO skip duplicate history item by checking translationId

void MainWindow::init_history() {
  utils::returnPath("history");
  historyWidget = new History(this);
  historyWidget->setWindowTitle(QApplication::applicationName() + " | History");
  historyWidget->setWindowFlag(Qt::Dialog);
  historyWidget->setWindowModality(Qt::NonModal);
  connect(historyWidget, &History::setTranslationId, this,
          [=](QString transId) { this->translationId = transId; });
  connect(historyWidget, &History::historyItemMeta, this,
          [=](QStringList historyItemMeta) {
            if (historyItemMeta.count() >= 4) {
              currentTranslationData.clear();
              settings.setValue(
                  "lastTranslation",
                  translationId); // translation id is set before this functor
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

void MainWindow::init_settings() {
  nativeHotkey = new QHotkey(this);
  settingsWidget = new Settings(this, nativeHotkey);
  settingsWidget->setWindowTitle(QApplication::applicationName() +
                                 " | Settings");
  settingsWidget->setWindowFlag(Qt::Dialog);
  settingsWidget->setWindowModality(Qt::NonModal);
  connect(settingsWidget, &Settings::translationRequest, this,
          [=](const QString selected) {
            ui->src1->setText(selected);
            ui->lang1->setCurrentIndex(0);
            _translate->click();
            this->setWindowState((this->windowState() & ~Qt::WindowMinimized) |
                                 Qt::WindowActive);
            this->show();
          });
  connect(settingsWidget, &Settings::themeToggled, this, [=]() {
    setStyle(":/qbreeze/" + settings.value("theme", "dark").toString() +
             ".qss");
  });
  if (nativeHotkey->isRegistered() == false) {
    showError("Unable to register Global Hotkey.\nIs another instance of " +
              QApplication::applicationName() +
              "running ?\nIf yes close it and restart the application.");
  }
}

void MainWindow::textSelectionChanged(QTextEdit *editor) {
  QString selection = editor->textCursor().selectedText().trimmed();
  if (!selection.isEmpty()) {
    textOptionWidget->setObjectName("selection_" + editor->objectName());
    textOptionWidget->move(QCursor::pos().x(), QCursor::pos().y() + 10);
    textOptionWidget->resize(textOptionWidget->minimumSizeHint());
    textOptionWidget->adjustSize();
    textOptionWidget->show();

    textOptionForm.readPushButton->disconnect();
    textOptionForm.copyPushButton->disconnect();

    connect(textOptionForm.readPushButton, &QPushButton::clicked, this, [=]() {
      if (this->findChild<QTextEdit *>(textOptionWidget->objectName()
                                           .split("selection_")
                                           .last()
                                           .trimmed()) != nullptr) {
        playSelected = true;
        selectedText = selection;
        if (textOptionWidget->objectName()
                .split("selection_")
                .last()
                .trimmed() == "src1") {
          on_play1_clicked();
        } else {
          on_play2_clicked();
        }
        textOptionWidget->hide();
      }
    });

    connect(textOptionForm.copyPushButton, &QPushButton::clicked, this, [=]() {
      clipboard->setText(selection);
      textOptionWidget->hide();
    });
  } else {
    playSelected = false;
    selectedText.clear();
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
        _translate->click();
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
  settings.setValue("geometry", saveGeometry());
  settings.setValue("windowState", saveState());

  // save quick trans shortcut
  settings.setValue("quicktrans_shortcut",
                    settingsWidget->keySequenceEditKeySequence());

  // save quicktrans settings
  settings.setValue("quicktrans", settingsWidget->quickResultCheckBoxChecked());

  QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
  int x =
      ui->translationWidget->geometry().center().x() - _translate->width() / 2;
  int y = ui->translationWidget->geometry().center().y() -
          (_translate->height() + _translate->height() / 2);
  _translate->move(x, y);
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
    _langName.append(langItem.at(0).trimmed());
    _langCode.append(langItem.at(1).trimmed());
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
    _supportedTts.append(ttsCode.trimmed());
  }
  tts.close();
}

void MainWindow::setStyle(QString fname) {
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
  currentTranslationData.clear();
  ui->src2->clear();

  if (_request != nullptr) {
    _request->blockSignals(true);
    _request->deleteLater();
    _request->blockSignals(false);
    _request = nullptr;
    _loader->stop();
  }

  if (_request == nullptr) {
    _request = new Request(this);
    connect(_request, &Request::requestStarted, this,
            [=]() { _loader->start(); });
    connect(_request, &Request::requestFinished, this, [=](QString reply) {
      // save cache for line by line use
      saveByTransId(translationId, reply);
      // load to view
      processTranslation(reply);
      _loader->stop();
      settings.setValue("lastTranslation", translationId);
    });
    connect(_request, &Request::downloadError, this, [=](QString errorString) {
      _loader->stop();
      showError(errorString);
    });

    QString src1 = ui->src1->toPlainText().replace("&", "and").replace("#", "");
    QString urlStr =
        "https://translate.googleapis.com/translate_a/single?client=gtx&sl=" +
        getSourceLang() + "&tl=" + getTransLang() + "&dt=t&q=" + src1;
    QString url = QUrl(urlStr).toString(QUrl::FullyEncoded);
    qDebug() << url;
    if (url.isEmpty()) {
      showError("Invalid Input.");
    } else {
      _request->get(QUrl(url));
      translationId = utils::generateRandomId(20);
    }
  }
}

void MainWindow::saveByTransId(QString translationId, QString reply) {
  QFile jsonFile(utils::returnPath("cache") + "/" + translationId + ".glate");
  jsonFile.open(QFile::WriteOnly);
  jsonFile.write(reply.toUtf8());
  jsonFile.close();
}

void MainWindow::processTranslation(QString reply) {
  currentTranslationData.clear();
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
          currentTranslationData.append(translationFregments);
        }
      }
    }
  }
  foreach (QStringList translationFregments, currentTranslationData) {
    ui->src2->append(translationFregments.at(0));
  }
  // scroll to top
  ui->src2->verticalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);
  // history save
  historyWidget->insertItem(
      QStringList() << getLangName(getSourceLang())
                    << getLangName(getTransLang()) << ui->src1->toPlainText()
                    << ui->src2->toPlainText()
                    << QDateTime::currentDateTime().toLocalTime().toString(),
      false, translationId);
}

QString MainWindow::getSourceLang() {
  int indexLang1 = ui->lang1->currentIndex();
  if (indexLang1 == 0) {
    return "auto";
  } else {
    return _langCode.at(indexLang1 - 1).trimmed();
  }
}

QString MainWindow::getTransLang() {
  int indexLang2 = ui->lang2->currentIndex();
  return _langCode.at(indexLang2).trimmed();
}

void MainWindow::showError(QString message) {
  if (message.contains("host", Qt::CaseInsensitive) ||
      message.contains("translate.googleapis.com")) {
    message.replace("translate.googleapis.com", "host");
  }
  // init error
  _error = new Error(this);
  _error->setAttribute(Qt::WA_DeleteOnClose);
  _error->setWindowTitle(QApplication::applicationName() + " | Error dialog");
  _error->setWindowFlag(Qt::Dialog);
  _error->setWindowModality(Qt::NonModal);
  _error->setError("An Error ocurred while processing your request!", message);
  _error->show();
}

void MainWindow::on_clear_clicked() {
  ui->src1->clear();
  ui->src2->clear();

  if (_request != nullptr) {
    _request->blockSignals(true);
    _request->deleteLater();
    _request->blockSignals(false);
    _request = nullptr;
    _loader->stop();
  }
  currentTranslationData.clear();
  translationId.clear();
  _lineByLine->clearData();
}

void MainWindow::on_paste_clicked() {
  const QMimeData *mimeData = clipboard->mimeData();
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
  settings.setValue("lang1", index);
  _translate->setToolTip(
      "Translate from " + ui->lang1->itemText(index) + " to " +
      ui->lang2->itemText(ui->lang2->currentIndex()) + "(Shift + Enter)");
}

void MainWindow::on_lang2_currentIndexChanged(int index) {
  settings.setValue("lang2", index);
  _translate->setToolTip("Translate from " +
                         ui->lang1->itemText(ui->lang1->currentIndex()) +
                         " to " + ui->lang2->itemText(index));
}

void MainWindow::on_play2_clicked() {
  bool player1Playing = _player->objectName().split("_").last() == "play1" &&
                        _player->state() == QMediaPlayer::PlayingState;

  if (_player->state() != QMediaPlayer::PlayingState) {
    if (_supportedTts.contains(getTransLang(), Qt::CaseInsensitive) == false) {
      showError("Selected language '" + getTransLang() +
                "' is not supported by TTS Engine.\nPlease choose different "
                "Language.");
      return;
    }
    _player->setObjectName("player_play2");
    QString text = ui->src2->toPlainText();
    if (playSelected)
      text = selectedText;

    QStringList src2Parts;
    QMediaPlaylist *src2playlist = new QMediaPlaylist(_player);
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
    _player->setPlaylist(src2playlist);
    _player->play();
    ui->play2->setIcon(QIcon(":/icons/loader-2-fill.png"));
  } else {
    _player->blockSignals(true);
    _player->stop();
    ui->play1->setIcon(QIcon(":/icons/volume-up-line.png"));
    ui->play2->setIcon(QIcon(":/icons/volume-up-line.png"));
    _player->blockSignals(false);
  }

  if (player1Playing) {
    on_play2_clicked();
  }
}

void MainWindow::on_play1_clicked() {
  bool player2Playing = _player->objectName().split("_").last() == "play2" &&
                        _player->state() == QMediaPlayer::PlayingState;

  if (_player->state() != QMediaPlayer::PlayingState) {
    if (_supportedTts.contains(getSourceLang(), Qt::CaseInsensitive) == false) {
      showError("Selected language '" + getSourceLang() +
                "' is not supported by TTS Engine.\nPlease choose different "
                "Language.");
      return;
    }
    _player->setObjectName("player_play1");
    QString text = ui->src1->toPlainText();
    if (playSelected)
      text = selectedText;

    QStringList src1Parts;
    QMediaPlaylist *src1playlist = new QMediaPlaylist(_player);
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
    _player->setPlaylist(src1playlist);
    _player->play();
    ui->play1->setIcon(QIcon(":/icons/loader-2-fill.png"));
  } else {
    _player->blockSignals(true);
    _player->stop();
    ui->play1->setIcon(QIcon(":/icons/volume-up-line.png"));
    ui->play2->setIcon(QIcon(":/icons/volume-up-line.png"));
    _player->blockSignals(false);
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
    clipboard->setText(text);
  }
}

void MainWindow::on_share_clicked() {
  _share->setTranslation(ui->src2->toPlainText(), translationId);
  if (_share->isVisible() == false) {
    _share->showNormal();
  }
}

void MainWindow::on_settings_clicked() {
  if (settingsWidget->isVisible() == false) {
    settingsWidget->adjustSize();
    QScreen *pScreen = QGuiApplication::screenAt(
        this->mapToGlobal({settingsWidget->width() / 2, 0}));
    QRect availableScreenSize = pScreen->availableGeometry();
    settingsWidget->move(availableScreenSize.center() -
                         settingsWidget->rect().center());
    settingsWidget->showNormal();
  }
}

void MainWindow::on_history_clicked() {
  if (historyWidget->isVisible() == false) {
    historyWidget->loadHistory();
    QScreen *pScreen = QGuiApplication::screenAt(
        this->mapToGlobal({historyWidget->width() / 2, 0}));
    QRect availableScreenSize = pScreen->availableGeometry();
    historyWidget->move(availableScreenSize.center() -
                        historyWidget->rect().center());
    historyWidget->showNormal();
  }
}

void MainWindow::on_lineByline_clicked() {
  if (translationId.isEmpty()) {
    showError("Translate something first.");
    return;
  }
  if (_lineByLine->isVisible() == false) {
    if (translationId != _lineByLine->getTId()) {
      _lineByLine->clearData();
      if (currentTranslationData.isEmpty() == false) {
        _lineByLine->setData(currentTranslationData,
                             getLangName(getSourceLang()),
                             getLangName(getTransLang()), translationId);
      } else {
        // load translation from cache;
        QFile jsonFile(utils::returnPath("cache") + "/" + translationId +
                       ".glate");
        jsonFile.open(QFile::ReadOnly);
        currentTranslationData.clear();
        processTranslation(jsonFile.readAll());
        _lineByLine->setData(currentTranslationData,
                             getLangName(getSourceLang()),
                             getLangName(getTransLang()), translationId);
      }
    }
    QScreen *pScreen = QGuiApplication::screenAt(
        this->mapToGlobal({_lineByLine->width() / 2, 0}));
    QRect availableScreenSize = pScreen->availableGeometry();
    _lineByLine->move(availableScreenSize.center() -
                      _lineByLine->rect().center());
    _lineByLine->showNormal();
  }
}

// convert lang code to lang name.
QString MainWindow::getLangName(QString langCode) {
  if (langCode == "auto") {
    return "Auto Detected";
  } else {
    QStringList langNameList = _langName;
    langNameList.removeAt(0); // remove auto detect
    QString langName = langNameList.at(_langCode.lastIndexOf(langCode) - 1);
    return langName;
  }
}

// not tested not being used yet
QString MainWindow::getLangCode(QString langName) {
  if (langName.contains("auto", Qt::CaseInsensitive)) {
    return "auto";
  } else {
    QStringList langCodeList = _langCode;
    QString langCode = langCodeList.at(_langName.lastIndexOf(langName) - 1);
    return langCode;
  }
}

void MainWindow::on_help_clicked() {
  if (slider == nullptr) {
    slider = new Slider(this);
    slider->setWindowTitle(QApplication::applicationName() + " | Introduction");
    slider->setWindowFlag(Qt::Dialog);
    slider->setWindowModality(Qt::ApplicationModal);
    slider->setMinimumSize(680, 420);
  }
  if (slider->isVisible() == false) {
    slider->showNormal();
    slider->start();
  }
}
