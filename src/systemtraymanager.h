#ifndef SYSTEMTRAYMANAGER_H
#define SYSTEMTRAYMANAGER_H

#include <QApplication>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>

class SystemTrayManager : public QObject {
  Q_OBJECT
public:
  explicit SystemTrayManager(QObject *parent = nullptr);
  ~SystemTrayManager();

  void updateMenu(bool windowVisible);
  bool isTrayAvailable() const;

signals:
  void showWindow();
  void hideWindow();
  void quitRequested();

private:
  QSystemTrayIcon *trayIcon = nullptr;
  QMenu *m_trayMenu = nullptr;
  QAction *m_showAction = nullptr;
  QAction *m_hideAction = nullptr;
  QAction *m_quitAction = nullptr;
  bool m_windowVisible = true;

private slots:
  void showMainWindow();
  void hideMainWindow();
  void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
  void quitApplication();
};

#endif // SYSTEMTRAYMANAGER_H
