// SPDX-License-Identifier: MIT
#pragma once
#include <QObject>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QDateTime>
#include "policy.h"

class DailySync : public QObject {
    Q_OBJECT
public:
    explicit DailySync(QObject *parent = nullptr);
    ~DailySync();
public slots:
    void observeFullSuccess();
    void observeFailure();
    void manualRequested();
    void downloadStarted();
    void bootstrap();
    void woke();
    void sleeping();
    void evaluate();
    void drive();
    void started();
    void finished();
    void cancelled();
    void wifiChanged(bool enabled);
    void connectingFailed();
    void usbConnected();
    void usbDisconnected();
    void filesChanged();
private:
    bool managed() const;
    bool usable() const;
    int option(const char *key, int fallback, int minimum, int maximum) const;
    void status(const QString &message);
    void disableWifi();
    void beginCancel(const QString &reason);
    void settle();
    void writeSuccess();
    SyncPolicy policy;
    WifiPolicy wifiPolicy;
    QObject *syncManager = nullptr;
    QObject *wireless = nullptr;
    QObject *workflow = nullptr;
    QObject *power = nullptr;
    QObject *plug = nullptr;
    QTimer wakeTimer, workTimer;
    QFileSystemWatcher watcher;
    QDateTime deadline, drainUntil;
    QDateTime nextProgress;
    bool awake = true;
    bool usb = false;
    bool forced = false;
    bool ready = false;
    bool wasManaged = true;
    int bootstrapAttempts = 0;
};
