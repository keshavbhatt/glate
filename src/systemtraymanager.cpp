#include "systemtraymanager.h"

SystemTrayManager::SystemTrayManager(QObject *parent) : QObject{parent} {
  if (QSystemTrayIcon::isSystemTrayAvailable()) {
    trayIcon =
        new QSystemTrayIcon(QIcon(":/icons/app/icon-64.png"), this->parent());
    trayIcon->setToolTip(qApp->applicationName() + " " +
                         qApp->applicationVersion());

    m_trayMenu = new QMenu(nullptr);
    m_showAction =
        new QAction(tr("Restore %1").arg(qApp->applicationName()), this);
    m_hideAction =
        new QAction(tr("Minimize to Tray").arg(qApp->applicationName()), this);
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
    connect(trayIcon, &QSystemTrayIcon::activated, this,
            &SystemTrayManager::onTrayIconActivated);

    trayIcon->show();
  }
}

SystemTrayManager::~SystemTrayManager() {
  if (trayIcon)
    delete trayIcon;
}

bool SystemTrayManager::isTrayAvailable() const {
  return QSystemTrayIcon::isSystemTrayAvailable() && trayIcon != nullptr;
}

void SystemTrayManager::updateMenu(bool windowVisible) {
  m_windowVisible = windowVisible;
  if (isTrayAvailable()) {
    m_showAction->setEnabled(!windowVisible);
    m_hideAction->setEnabled(windowVisible);
  }
}

void SystemTrayManager::onTrayIconActivated(
    QSystemTrayIcon::ActivationReason reason) {
  if (reason == QSystemTrayIcon::Trigger ||
      reason == QSystemTrayIcon::DoubleClick) {
    if (m_windowVisible) {
      hideMainWindow();
    } else {
      showMainWindow();
    }
  }
}

void SystemTrayManager::showMainWindow() { emit showWindow(); }

void SystemTrayManager::hideMainWindow() { emit hideWindow(); }

void SystemTrayManager::quitApplication() { qApp->quit(); }
