#ifndef SYSTEMTRAYMANAGER_H
#define SYSTEMTRAYMANAGER_H

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QObject>
#include <QSystemTrayIcon>

class SystemTrayManager : public QObject {
  Q_OBJECT
public:
  explicit SystemTrayManager(QObject *parent = nullptr);

  ~SystemTrayManager();

  void updateMenu(bool windowVisible);

signals:
  void showWindow();
  void hideWindow();

private:
  QSystemTrayIcon *trayIcon = nullptr;
  QMenu *m_trayMenu = nullptr;
  QAction *m_showAction = nullptr;
  QAction *m_hideAction = nullptr;
  QAction *m_quitAction = nullptr;

private slots:
  void showMainWindow();

  void hideMainWindow();

  void quitApplication();
};

#endif // SYSTEMTRAYMANAGER_H
