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

signals:
  void showWindow();
  void hideWindow();

private:
  QSystemTrayIcon *trayIcon = nullptr;
  QMenu *trayMenu = nullptr;
  QAction *showAction = nullptr;
  QAction *hideAction = nullptr;
  QAction *quitAction = nullptr;

private slots:
  void showMainWindow();

  void hideMainWindow();

  void quitApplication();
};

#endif // SYSTEMTRAYMANAGER_H
