#include "systemtraymanager.h"
#include <QDebug>

SystemTrayManager::SystemTrayManager(QObject *parent) : QObject{parent} {
  if (QSystemTrayIcon::isSystemTrayAvailable()) {
    trayIcon =
        new QSystemTrayIcon(QIcon(":/icons/app/icon-64.png"), this->parent());

    m_trayMenu = new QMenu(0);
    m_showAction =
        new QAction(tr("Show %1").arg(qApp->applicationName()), this);
    m_hideAction =
        new QAction(tr("Hide %1").arg(qApp->applicationName()), this);
    m_quitAction = new QAction(tr("Quit"), this);

    m_trayMenu->addAction(m_showAction);
    m_trayMenu->addAction(m_hideAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_quitAction);

    trayIcon->setContextMenu(m_trayMenu);

    connect(m_showAction, &QAction::triggered, this,
            &SystemTrayManager::showMainWindow);
    connect(m_hideAction, &QAction::triggered, this,
            &SystemTrayManager::hideMainWindow);
    connect(m_quitAction, &QAction::triggered, this,
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

void SystemTrayManager::updateMenu(bool windowVisible) {
  if (QSystemTrayIcon::isSystemTrayAvailable()) {
    qWarning() << windowVisible;
    m_showAction->setEnabled(!windowVisible);
    m_hideAction->setEnabled(windowVisible);
  }
}

void SystemTrayManager::showMainWindow() { emit showWindow(); }

void SystemTrayManager::hideMainWindow() { emit hideWindow(); }

void SystemTrayManager::quitApplication() { qApp->quit(); }
