#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDesktopServices>
#include <QHotkey>
#include <QProcess>
#include <QSettings>
#include <QWidget>

namespace Ui {
class Settings;
}

class Settings : public QWidget {
  Q_OBJECT

public:
  explicit Settings(QWidget *parent = nullptr, QHotkey *hotKey = nullptr);
  ~Settings();
signals:
  void translationRequest(QString selected);
  void themeToggled();
public slots:
  QKeySequence keySequenceEditKeySequence();
  bool quickResultCheckBoxChecked();
private slots:
  void get_selected_word_fromX11();
  void set_x11_selection();
  void show_requested_text();
  void setShortcut(const QKeySequence &sequence);

  void readSettings();
  void on_github_clicked();

  void on_rate_clicked();

  void on_donate_clicked();

  void on_dark_toggled(bool checked);

  void on_light_toggled(bool checked);

private:
  Ui::Settings *ui;
  QSettings settings;
  QHotkey *nativeHotkey = nullptr;
  QString x11_selected;
};

#endif // SETTINGS_H
