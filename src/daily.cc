// SPDX-License-Identifier: MIT
#include "daily.h"
#include "storage.h"
#include <NickelHook.h>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QSettings>
#include <QTextStream>

static const char *ROOT = "/mnt/onboard/.adds/daily-sync/";
static DailySync *controller = nullptr;
static QObject *(*getSync)();
static QObject *(*getWireless)();
static QObject *(*getWorkflow)();
static QObject *(*getPower)();
static QObject *(*getPlug)();
static bool (*isSyncing)(QObject *);
static bool (*isSleeping)(QObject *);
static bool (*usbVisible)(QObject *);
static bool (*isConnected)(QObject *);
static bool (*wirelessUp)(QObject *);
static void (*wirelessOff)(QObject *);
static bool (*airplane)(QObject *);
static void (*setAirplane)(QObject *, bool);
static void (*connectSilently)(QObject *);
static void (*startSync)(QObject *);
static void (*cancelSync)(QObject *);
static void (*originalLastSync)(void *, QDateTime const &);
static void (*originalFailure)(QObject *);
static void (*originalManualEvent)(QString const &, const char *, bool);

static QString path(const char *name) { return QString::fromLatin1(ROOT) + name; }
static QDateTime fullSyncStamp() {
    // Read only: never set or sync account preferences. Nickel may persist its
    // full-sync timestamp without passing through our intercepted PLT entry.
    QSettings settings("/mnt/onboard/.kobo/Kobo/Kobo eReader.conf", QSettings::IniFormat);
    return settings.value("ApplicationPreferences/LastSyncTime").toDateTime();
}
static QString readText(const QString &name) {
    QFile f(name);
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()).trimmed() : QString();
}
static bool writeText(const QString &name, const QString &value) {
    QSaveFile f(name);
    if (!f.open(QIODevice::WriteOnly)) return false;
    QByteArray data = (value + "\n").toUtf8();
    if (f.write(data) != data.size()) { f.cancelWriting(); return false; }
    return f.commit();
}

DailySync::DailySync(QObject *parent) : QObject(parent) {
    wakeTimer.setSingleShot(true);
    workTimer.setInterval(1000); // Runs only during an attempt; never wakes a sleeping device.
    connect(&wakeTimer, SIGNAL(timeout()), this, SLOT(evaluate()));
    connect(&workTimer, SIGNAL(timeout()), this, SLOT(drive()));
    connect(&watcher, SIGNAL(directoryChanged(QString)), this, SLOT(filesChanged()));
}
DailySync::~DailySync() { if (controller == this) controller = nullptr; }

bool DailySync::managed() const {
    if (QFileInfo(path("paused")).exists()) return false;
    QSettings cfg(path("config.ini"), QSettings::IniFormat);
    return cfg.value("DailySync/Enabled", true).toBool();
}
int DailySync::option(const char *key, int fallback, int minimum, int maximum) const {
    QSettings cfg(path("config.ini"), QSettings::IniFormat);
    bool ok = false;
    int v = cfg.value(QString("DailySync/") + key, fallback).toInt(&ok);
    return ok ? qBound(minimum, v, maximum) : fallback;
}
bool DailySync::usable() const {
    return ready && awake && !usb && !isSleeping(power) && !usbVisible(plug);
}
void DailySync::status(const QString &message) {
    QString entry = QDateTime::currentDateTime().toString(Qt::ISODate) + " " + message;
    nh_log("%s", qPrintable(message));
    if (!plug || usb || usbVisible(plug)) return;
    writeText(path("status.log"), entry);
    QFile f(path("daily-sync.log"));
    if (f.size() > 65536) {
        QFile::remove(path("daily-sync.log.1"));
        f.rename(path("daily-sync.log.1"));
        f.setFileName(path("daily-sync.log"));
    }
    if (f.open(QIODevice::WriteOnly | QIODevice::Append)) f.write((entry + "\n").toUtf8());
}

namespace {
struct LegacyStorage {
    bool exists(const char *name) const { return QFileInfo(path(name)).exists(); }
    QString read(const char *name) const { return readText(path(name)); }
    qint64 timestamp(const QString &value) const {
        const QDateTime date = QDateTime::fromString(value, Qt::ISODate);
        return date.isValid() ? date.toMSecsSinceEpoch() : -1;
    }
    bool write(const char *name, const QString &value) const { return writeText(path(name), value); }
    void remove(const char *name) const { QFile::remove(path(name)); }
    void move(const char *from, const char *to) const { QFile::rename(path(from), path(to)); }
};
}
void DailySync::migrateLegacyFiles() {
    LegacyStorage storage;
    // Also migrate while paused, but never touch the exported filesystem.
    migrateLegacyState(storage, ready && !usb && !usbVisible(plug));
}

void DailySync::bootstrap() {
    syncManager = getSync(); wireless = getWireless(); workflow = getWorkflow();
    power = getPower(); plug = getPlug();
    if (!syncManager || !wireless || !workflow || !power || !plug) {
        if (++bootstrapAttempts < 6) QTimer::singleShot(5000, this, SLOT(bootstrap()));
        else status("INACTIVE: native managers unavailable");
        return;
    }
    bool ok = true;
    ok &= bool(connect(power, SIGNAL(resumedFromSleep()), this, SLOT(woke())));
    ok &= bool(connect(power, SIGNAL(aboutToSleep()), this, SLOT(sleeping())));
    ok &= bool(connect(syncManager, SIGNAL(finished()), this, SLOT(finished())));
    ok &= bool(connect(syncManager, SIGNAL(syncStarted(SyncState::SyncType)), this, SLOT(started())));
    ok &= bool(connect(syncManager, SIGNAL(downloadStarted(Volume)), this, SLOT(downloadStarted())));
    ok &= bool(connect(syncManager, SIGNAL(cancelStarted()), this, SLOT(cancelled())));
    ok &= bool(connect(wireless, SIGNAL(wifiEnabled(bool)), this, SLOT(wifiChanged(bool))));
    ok &= bool(connect(wireless, SIGNAL(networkFailedToConnect()), this, SLOT(connectingFailed())));
    ok &= bool(connect(plug, SIGNAL(aboutToConnect()), this, SLOT(usbConnected())));
    ok &= bool(connect(plug, SIGNAL(aboutToUnplug()), this, SLOT(usbDisconnected())));
    ok &= bool(connect(plug, SIGNAL(doneProcessing()), this, SLOT(usbDisconnected())));
    if (!ok) {
        disconnect(power, nullptr, this, nullptr); disconnect(syncManager, nullptr, this, nullptr);
        disconnect(wireless, nullptr, this, nullptr); disconnect(plug, nullptr, this, nullptr);
        status("INACTIVE: native signal mismatch; no Wi-Fi changes made");
        return;
    }
    ready = true;
    wasManaged = managed();
    usb = usbVisible(plug);
    awake = !isSleeping(power);
    if (!usb) watcher.addPath(QString::fromLatin1(ROOT));
    migrateLegacyFiles();
    status("READY v0.1.6: daily sync; non-book state files; manual Wi-Fi and book downloads allowed until sleep");
    if (awake) woke();
}
void DailySync::woke() {
    awake = true;
    nh_log("WAKE: phase=%d", int(policy.phase));
    if (!policy.active()) wakeTimer.start(option("WakeDelaySeconds", 5, 2, 30) * 1000);
}
void DailySync::sleeping() {
    awake = false;
    wakeTimer.stop();
    wifiPolicy.reset();
    // Sleep ends the user network session. An active attempt still gets bounded
    // cancellation/cleanup before shutdown, including a paused explicit request.
    if (policy.active()) wifiPolicy.owned = true;
    if (policy.active()) beginCancel("sleep requested");
    else if (managed()) disableWifi();
}
void DailySync::usbConnected() {
    nh_log("USB_EXPORT: stopping automation timers, phase=%d; no onboard writes until unplug", int(policy.phase));
    usb = true;
    wakeTimer.stop(); workTimer.stop();
    if (!watcher.directories().isEmpty()) watcher.removePaths(watcher.directories());
    // Nickel owns USB entry/cancellation. Never write onboard state during USB export.
    policy.reset(); forced = false;
    wifiPolicy.reset();
}
void DailySync::usbDisconnected() {
    usb = false;
    migrateLegacyFiles();
    if (awake) woke();
}
void DailySync::filesChanged() {
    if (!ready || usb || usbVisible(plug)) return;
    bool enabled = managed();
    if (enabled != wasManaged) {
        wasManaged = enabled;
        status(enabled ? "RESUMED: automation enabled" : "PAUSED: manual Wi-Fi allowed");
        if (!enabled) wifiPolicy.allowManual();
        if (enabled && awake) woke();
    }
    if (QFileInfo(path("sync-now")).exists()) {
        QFile::remove(path("sync-now"));
        if (!policy.active()) { forced = true; wakeTimer.start(100); }
    }
}
void DailySync::evaluate() {
    if (!usable()) return;
    migrateLegacyFiles();
    if (!watcher.directories().contains(QString::fromLatin1(ROOT))) watcher.addPath(QString::fromLatin1(ROOT));
    if (policy.active()) return;
    // Requests copied over USB cannot generate an inotify event while the disk is exported.
    if (QFileInfo(path("sync-now")).exists()) {
        QFile::remove(path("sync-now"));
        forced = true;
    }
    // An explicit menu request remains usable while automatic scheduling is paused.
    if (!managed() && !forced) return;
    QDate today = QDate::currentDate();
    if (today.year() < 2024) { status("SKIPPED: device clock is not set"); return; }
    QString last = readText(path("last-success.dat")).left(10);
    // A failed rename must not discard today's quota and trigger an extra sync.
    if (last.isEmpty()) last = readText(path("last-success.txt")).left(10);
    if (!SyncPolicy::due(today.toString(Qt::ISODate).toStdString(), last.toStdString(), forced)) {
        status("ALREADY_DONE: automatic sync skipped; manual Wi-Fi unchanged");
        return;
    }
    bool manual = forced;
    forced = false;
    policy.begin(manual ? SyncPolicy::MenuRequest : SyncPolicy::DailyWake);
    wifiPolicy.begin(false, !airplane(workflow) || wirelessUp(wireless), !manual);
    deadline = QDateTime::currentDateTime().addSecs(option("ConnectTimeoutSeconds", 60, 15, 180));
    nextProgress = QDateTime::currentDateTime().addSecs(10);
    status(manual ? "CONNECTING: explicit manual full sync requested"
                  : "CONNECTING: daily full sync requested");
    workTimer.start();
    if (airplane(workflow)) setAirplane(workflow, false);
    connectSilently(workflow);
}
void DailySync::drive() {
    if (!policy.active() || usb) { workTimer.stop(); return; }
    QDateTime now = QDateTime::currentDateTime();
    if (now >= nextProgress) {
        nextProgress = now.addSecs(10);
        status(QString("WAITING: phase=%1 native_syncing=%2 connected=%3 deadline=%4")
               .arg(int(policy.phase)).arg(isSyncing(syncManager)).arg(isConnected(wireless))
               .arg(deadline.toString(Qt::ISODate)));
    }
    if (policy.phase == SyncPolicy::Draining) {
        checkFullSyncStamp();
        if (!isSyncing(syncManager) && now >= drainUntil) settle();
        else if (now >= deadline) beginCancel("cleanup deadline exceeded");
        return;
    }
    if (policy.phase == SyncPolicy::Cancelling) {
        if (!isSyncing(syncManager) || now >= deadline) {
            policy.completed = false;
            settle();
        }
        return;
    }
    if (now >= deadline) {
        beginCancel(policy.phase == SyncPolicy::Requested ?
                    (policy.trigger == SyncPolicy::NativeRequest ? "manual sync did not start before connection deadline"
                                                               : "native sync did not emit syncStarted within 20 seconds")
                                                        : "connection or sync timeout");
        return;
    }
    if (policy.phase == SyncPolicy::Connecting && isConnected(wireless) && !isSyncing(syncManager)) {
        // Always request our own full user sync. An incidental book sync on connection is insufficient.
        policy.launched();
        baselineFullSync = fullSyncStamp();
        requestedAt = now.toUTC();
        deadline = now.addSecs(20);
        nextProgress = now.addSecs(10);
        status("REQUESTING: invoking N3SyncManager::silentSync (same full-sync dispatch as native menu)");
        startSync(syncManager);
        status(QString("START_RETURNED: phase=%1 native_syncing=%2; waiting for native signals")
               .arg(int(policy.phase)).arg(isSyncing(syncManager)));
    }
}
void DailySync::manualRequested() {
    // This is called by Nickel's explicit manual-sync event, before its connection
    // workflow. User requests keep their network session after sync completion.
    if (!usable() || (!managed() && !policy.active())) return;
    wifiPolicy.allowManual();
    if (policy.active()) {
        status("MANUAL_REQUEST: existing attempt keeps Wi-Fi available until sleep");
        return;
    }
    if (!managed() || !policy.begin(SyncPolicy::NativeRequest)) return;
    wifiPolicy.begin(true, true);
    baselineFullSync = fullSyncStamp();
    requestedAt = QDateTime::currentDateTimeUtc();
    forced = false;
    wakeTimer.stop();
    deadline = QDateTime::currentDateTime().addSecs(option("ConnectTimeoutSeconds", 60, 15, 180));
    nextProgress = QDateTime::currentDateTime().addSecs(10);
    workTimer.start();
    status("MANUAL_REQUEST: native Kobo sync allowed; Wi-Fi remains available until sleep");
    // Do not call startSync here: Nickel will run the original request itself.
}
void DailySync::downloadStarted() {
    if (!usable() || (!managed() && !policy.active())) return;
    // Sync completion is not download completion. Relinquish network ownership
    // before content transfer, even when it started inside our automatic attempt.
    wifiPolicy.allowManual();
    status("DOWNLOAD_STARTED: Wi-Fi left to Nickel until sleep; no content details logged");
}
void DailySync::started() {
    if (usb || policy.phase != SyncPolicy::Requested) return;
    policy.started();
    deadline = QDateTime::currentDateTime().addSecs(option("MaxSyncSeconds", 300, 60, 1800));
    nextProgress = QDateTime::currentDateTime().addSecs(10);
    status("STARTED: native syncStarted signal received; exchange is running");
}
void DailySync::observeFullSuccess() {
    if (!usb && policy.phase == SyncPolicy::Running) {
        policy.success();
        status("SUCCESS_SIGNAL: native full-sync timestamp updated");
    }
}
void DailySync::observeFailure() {
    if (!usb && (policy.phase == SyncPolicy::Requested || policy.phase == SyncPolicy::Running)) {
        policy.failure();
        status("FAILURE_SIGNAL: native sync failed");
    }
}
void DailySync::checkFullSyncStamp() {
    if (usb || !policy.nativeStarted || policy.fullSuccess ||
        (policy.phase != SyncPolicy::Running && policy.phase != SyncPolicy::Draining)) return;
    QDateTime stamp = fullSyncStamp();
    // An old timestamp, a finished signal alone, or an incidental book sync is
    // insufficient. Require a newly advanced native full-sync timestamp from
    // this attempt, as well as the previously observed native start.
    if (!requestedAt.isValid() || !stamp.isValid() ||
        !SyncPolicy::freshFullStamp(baselineFullSync.isValid() ? baselineFullSync.toMSecsSinceEpoch() : 0,
                                   stamp.toMSecsSinceEpoch(), requestedAt.toMSecsSinceEpoch())) return;
    policy.success();
    status("SUCCESS_TIMESTAMP: native full-sync time advanced during this attempt");
}
void DailySync::finished() {
    if (usb || (policy.phase != SyncPolicy::Requested && policy.phase != SyncPolicy::Running)) return;
    checkFullSyncStamp();
    policy.finish();
    drainUntil = QDateTime::currentDateTime().addSecs(3);
    // Let Nickel finish its native cleanup before disabling the radio.
    status(policy.completed ? "CONFIRMED: full sync succeeded; waiting for cleanup"
                            : "UNCONFIRMED: no full-success timestamp; retry on next wake");
}
void DailySync::cancelled() {
    if (policy.phase == SyncPolicy::Requested || policy.phase == SyncPolicy::Running) beginCancel("sync cancelled by Nickel or user");
}
void DailySync::connectingFailed() {
    if (policy.phase == SyncPolicy::Connecting) beginCancel("known Wi-Fi network unavailable");
}
void DailySync::beginCancel(const QString &reason) {
    if (usb || !policy.active() || policy.phase == SyncPolicy::Cancelling) return;
    if (!wifiPolicy.cancelNativeOnTimeout(awake)) {
        // A tracking deadline must not cancel an ongoing user download or sync.
        // Stop monitoring without declaring success or taking their network down.
        policy.completed = false;
        status("TRACKING_STOPPED: " + reason + "; manual Wi-Fi unchanged");
        settle();
        return;
    }
    policy.cancel();
    deadline = QDateTime::currentDateTime().addSecs(15);
    status("RETRY_NEXT_WAKE: " + reason);
    if (isSyncing(syncManager)) cancelSync(syncManager);
}
void DailySync::writeSuccess() {
    QString stamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    if (!writeText(path("last-success.dat"), stamp)) status("ERROR: unable to persist success date");
}
void DailySync::settle() {
    bool success = policy.completed;
    bool closeWifi = wifiPolicy.closeAfterAttempt();
    workTimer.stop();
    // Set Idle first so a synchronous wifiEnabled callback cannot restart this attempt.
    policy.reset();
    wifiPolicy.finished();
    if (closeWifi) disableWifi();
    if (success) writeSuccess();
    if (closeWifi)
        status(success ? "DONE: full sync confirmed; Wi-Fi disabled"
                       : "FAILED: Wi-Fi disabled; retry at next wake if no success today");
    else
        status(success ? "DONE: full sync confirmed; manual Wi-Fi unchanged until sleep"
                       : "UNCONFIRMED: tracking ended; manual Wi-Fi unchanged; retry at next wake if no success today");
}
void DailySync::disableWifi() {
    if (!ready || usb || usbVisible(plug)) return;
    if (!airplane(workflow)) setAirplane(workflow, true);
    else if (wirelessUp(wireless)) wirelessOff(wireless);
}
void DailySync::wifiChanged(bool enabled) {
    if (!enabled && !policy.active()) { wifiPolicy.reset(); return; }
    if (!enabled || !usable() || !managed() || policy.active()) return;
    // An idle radio enable may be a book tap, browser or manual Wi-Fi switch.
    // Do not guess which one it was and cancel Nickel's work.
    wifiPolicy.allowExisting();
    status("WIFI_AVAILABLE: connection outside managed attempt left to Nickel until sleep");
}

extern "C" __attribute__((visibility("default"))) void daily_last_sync(void *self, QDateTime const &date) {
    originalLastSync(self, date);
    if (controller) QMetaObject::invokeMethod(controller, "observeFullSuccess", Qt::AutoConnection);
}
extern "C" __attribute__((visibility("default"))) void daily_failure(QObject *self) {
    if (controller && self == getSync()) QMetaObject::invokeMethod(controller, "observeFailure", Qt::AutoConnection);
    originalFailure(self);
}
extern "C" __attribute__((visibility("default"))) void daily_manual_event(QString const &view, const char *action, bool value) {
    if (controller) QMetaObject::invokeMethod(controller, "manualRequested", Qt::AutoConnection);
    originalManualEvent(view, action, value);
}
static int init() {
    QStringList version = readText("/mnt/onboard/.kobo/version").split(',');
    if (version.isEmpty() ||
        !(version.last().endsWith("377") || version.last().endsWith("380"))) {
        nh_log("INACTIVE: only Forma 377/380 is allowed");
        return 0;
    }
    if (!QCoreApplication::instance()) return 1;
    controller = new DailySync(QCoreApplication::instance());
    QTimer::singleShot(15000, controller, SLOT(bootstrap()));
    return 0;
}

static struct nh_info info = {
    .name = "DailySync",
    .desc = "One full Kobo sync at first daily wake, then Wi-Fi off",
    .uninstall_flag = "/mnt/onboard/.adds/daily-sync/uninstall",
    .uninstall_xflag = "/mnt/onboard/.adds/daily-sync/installed",
    .failsafe_delay = 5,
};
static struct nh_hook hooks[] = {
    { .sym = "_ZN19ApplicationSettings15setLastSyncTimeERK9QDateTime", .sym_new = "daily_last_sync", .lib = "libnickel.so.1.0.0", .out = nh_symoutptr(originalLastSync) },
    { .sym = "_ZN15SyncManagerBase4failEv", .sym_new = "daily_failure", .lib = "libnickel.so.1.0.0", .out = nh_symoutptr(originalFailure) },
    { .sym = "_ZN15SyncManagerBase19fireManualSyncEventERK7QStringPKcb", .sym_new = "daily_manual_event", .lib = "libnickel.so.1.0.0", .out = nh_symoutptr(originalManualEvent) },
    {0},
};
static struct nh_dlsym symbols[] = {
    { .name = "_ZN13N3SyncManager14sharedInstanceEv", .out = nh_symoutptr(getSync) },
    { .name = "_ZN15WirelessManager14sharedInstanceEv", .out = nh_symoutptr(getWireless) },
    { .name = "_ZN23WirelessWorkflowManager14sharedInstanceEv", .out = nh_symoutptr(getWorkflow) },
    { .name = "_ZN22N3PowerWorkflowManager14sharedInstanceEv", .out = nh_symoutptr(getPower) },
    { .name = "_ZN19PlugWorkflowManager14sharedInstanceEv", .out = nh_symoutptr(getPlug) },
    { .name = "_ZNK15SyncManagerBase9isSyncingEv", .out = nh_symoutptr(isSyncing) },
    { .name = "_ZNK22N3PowerWorkflowManager8sleepingEv", .out = nh_symoutptr(isSleeping) },
    { .name = "_ZN19PlugWorkflowManager20pluggedInViewVisibleEv", .out = nh_symoutptr(usbVisible) },
    { .name = "_ZN15WirelessManager9connectedEv", .out = nh_symoutptr(isConnected) },
    { .name = "_ZNK15WirelessManager2upEv", .out = nh_symoutptr(wirelessUp) },
    { .name = "_ZN15WirelessManager3offEv", .out = nh_symoutptr(wirelessOff) },
    { .name = "_ZN23WirelessWorkflowManager14isAirplaneModeEv", .out = nh_symoutptr(airplane) },
    { .name = "_ZN23WirelessWorkflowManager15setAirplaneModeEb", .out = nh_symoutptr(setAirplane) },
    { .name = "_ZN23WirelessWorkflowManager23connectWirelessSilentlyEv", .out = nh_symoutptr(connectSilently) },
    { .name = "_ZN13N3SyncManager10silentSyncEv", .out = nh_symoutptr(startSync) },
    { .name = "_ZN13N3SyncManager6cancelEv", .out = nh_symoutptr(cancelSync) },
    {0},
};
static bool uninstall() {
    return nh_delete_file("/mnt/onboard/.adds/nm/daily-sync");
}
NickelHook(.init = init, .info = &info, .hook = hooks, .dlsym = symbols, .uninstall = uninstall);
