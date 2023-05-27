#include "systemtraymanager.h"

SystemTrayManager::SystemTrayManager(QObject *parent) : QObject{parent} {
  if (QSystemTrayIcon::isSystemTrayAvailable()) {
    trayIcon =
        new QSystemTrayIcon(QIcon(":/icons/app/icon-64.png"), this->parent());

    trayMenu = new QMenu(0);
    showAction = new QAction("Show", this);
    hideAction = new QAction("Hide", this);
    quitAction = new QAction("Quit", this);

    showAction->setDisabled(true);

    trayMenu->addAction(showAction);
    trayMenu->addAction(hideAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);

    trayIcon->setContextMenu(trayMenu);

    connect(showAction, &QAction::triggered, this,
            &SystemTrayManager::showMainWindow);
    connect(hideAction, &QAction::triggered, this,
            &SystemTrayManager::hideMainWindow);
    connect(quitAction, &QAction::triggered, this,
            &SystemTrayManager::quitApplication);

    trayIcon->show();
  } else {
    QMessageBox::information(nullptr, "Information",
                             "Unable to find System tray.");
  }
}

SystemTrayManager::~SystemTrayManager() {
  if (trayIcon)
    delete trayIcon;
}

void SystemTrayManager::showMainWindow() {
  emit showWindow();
  showAction->setDisabled(true);
  hideAction->setDisabled(false);
}

void SystemTrayManager::hideMainWindow() {
  emit hideWindow();
  hideAction->setDisabled(true);
  showAction->setDisabled(false);
}

void SystemTrayManager::quitApplication() { qApp->quit(); }
