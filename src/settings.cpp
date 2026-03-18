#include "settings.h"

#include <QDesktopServices>

#include "ui_settings.h"

#include <QGuiApplication>

Settings::Settings(QWidget *parent, QHotkey *hotKey)
    : QWidget(parent), ui(new Ui::Settings) {
  ui->setupUi(this);
  this->nativeHotkey = hotKey;
  const QString platform = QGuiApplication::platformName().toLower();
  m_hotkeySupported = !platform.contains("wayland");

  readSettings();

  connect(ui->voiceGenderGlobal, &QComboBox::currentIndexChanged, this,
          [=](int index) { settings.setValue("voiceGender", index); });

  if (m_hotkeySupported && this->nativeHotkey != nullptr) {
    connect(this->nativeHotkey, &QHotkey::activated, this,
            &Settings::get_selected_word_fromX11);

    connect(ui->quickResultCheckBox, &QCheckBox::toggled, this->nativeHotkey,
            &QHotkey::setRegistered);

    connect(ui->keySequenceEdit, &QKeySequenceEdit::keySequenceChanged, this,
            &Settings::setShortcut);
  }

  if (!m_hotkeySupported) {
    const QString unsupportedTooltip =
        tr("Global shortcuts are unavailable on native Wayland sessions.");
    ui->quickResultCheckBox->setChecked(false);
    ui->quickResultCheckBox->setEnabled(false);
    ui->quickResultCheckBox->setText(tr("Quick Translate (unavailable)"));
    ui->quickResultCheckBox->setToolTip(unsupportedTooltip);
    ui->keySequenceEdit->setEnabled(false);
    ui->keySequenceEdit->setToolTip(unsupportedTooltip);
    settings.setValue("quicktrans", false);
    if (this->nativeHotkey != nullptr)
      this->nativeHotkey->setRegistered(false);
  }

  ui->dark->setChecked(settings.value("theme", "dark").toString() == "dark");
  ui->light->setChecked(settings.value("theme").toString() == "light");

  const QString hotkeyNote =
      m_hotkeySupported
          ? QString()
          : tr("<p align=\"center\"><span><b>Note:</b> Global shortcuts are "
               "not available on native Wayland sessions.</span></p>");

  QString aboutTxt =
      R"(<p align="center"> <span style="font-weight: bold;">Glate</span></p>
         <p align="center"><span>Designed and Developed</span></p>
         <p align="center"><span>by </span><span style="font-weight:bold;">Keshav Bhatt</span>
         <span> &lt;keshavnrj@gmail.com&gt;</span></p>
         <p align="center"><span>Website: https://ktechpit.com</span></p>
         <p align="center"><span>Runtime: %1</span></p>
         <p align="center"><span>Version: %2</span></p>
         %3)";

  ui->aboutTextBrowser->setText(
      aboutTxt.arg(qVersion(), qApp->applicationVersion(), hotkeyNote));
}

Settings::~Settings() { delete ui; }

void Settings::setShortcut(const QKeySequence &sequence) {
  if (!m_hotkeySupported || this->nativeHotkey == nullptr)
    return;
  this->nativeHotkey->setShortcut(sequence,
                                  ui->quickResultCheckBox->isChecked());
  // save quick trans shortcut instanty
  settings.setValue("quicktrans_shortcut", sequence);
}

void Settings::readSettings() {

  // quick trans settings
  if (settings.value("quicktrans").isValid()) {
    ui->quickResultCheckBox->setChecked(settings.value("quicktrans").toBool());
  } else if (settings.value("quicktrans").isValid()) {
    ui->quickResultCheckBox->setChecked(true);
  } else {
    ui->quickResultCheckBox->setChecked(false);
  }
  // quick trans key sequence
  if (settings.value("quicktrans_shortcut").isValid()) {
    auto k =
        QKeySequence(settings.value("quicktrans_shortcut").toString());
    ui->keySequenceEdit->setKeySequence(k);
    if (m_hotkeySupported && this->nativeHotkey != nullptr) {
      this->nativeHotkey->setShortcut(k.toString(),
                                      settings.value("quicktrans").toBool());
    }
  } else {
    // default value if value not founds
    QKeySequence k = QKeySequence::fromString(tr("Ctrl+Shift+Space"));
    ui->keySequenceEdit->setKeySequence(k);
    if (m_hotkeySupported && this->nativeHotkey != nullptr) {
      this->nativeHotkey->setShortcut(k.toString(), true);
      ui->quickResultCheckBox->setChecked(true);
    }
  }

  // global TTS voice preference
  ui->voiceGenderGlobal->setCurrentIndex(
      settings.value("voiceGender", 0).toInt());
}

void Settings::get_selected_word_fromX11() {
  auto xsel = new QProcess(this);
  xsel->setObjectName("xclip");
  xsel->start("xclip", QStringList() << "-o"
                                     << "-sel");
  connect(xsel, SIGNAL(finished(int)), this, SLOT(set_x11_selection()));
}

void Settings::set_x11_selection() {
  auto xselection = this->findChild<QObject *>("xclip");
  if (xselection) {
    x11_selected = ((QProcess *)(xselection))->readAllStandardOutput();
    if (!x11_selected.trimmed().isEmpty())
      show_requested_text();

    xselection->deleteLater();
  }
}

void Settings::show_requested_text() { emit translationRequest(x11_selected); }

bool Settings::quickResultCheckBoxChecked() {
  return ui->quickResultCheckBox->isChecked();
}

QKeySequence Settings::keySequenceEditKeySequence() {
  return ui->keySequenceEdit->keySequence();
}

void Settings::on_github_clicked() {
  QDesktopServices::openUrl(QUrl("https://github.com/keshavbhatt/glate"));
}

void Settings::on_rate_clicked() {
  QDesktopServices::openUrl(QUrl("snap://glate"));
}

void Settings::on_donate_clicked() {
  QDesktopServices::openUrl(QUrl("https://paypal.me/keshavnrj/5"));
}

void Settings::on_dark_toggled(bool checked) {
  settings.setValue("theme", checked ? "dark" : "light");
  emit themeToggled();
}

void Settings::on_light_toggled(bool checked) {
  settings.setValue("theme", checked ? "light" : "dark");
  emit themeToggled();
}
