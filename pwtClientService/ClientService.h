/*
 * This file is part of PWTClientService.
 * Copyright (C) 2025 kylon
 *
 * PWTClientService is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PWTClientService is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <QThread>

#include "serviceExport.h"
#include "pwtShared/Include/Packets/DeviceInfoPacket.h"
#include "pwtShared/Include/Packets/ClientPacket.h"
#include "pwtShared/Include/Packets/DaemonPacket.h"
#include "../version.h"

namespace PWTCS {
    class ServiceWorker;

    class PWTCSERVICE_EXPORT ClientService final: public QObject {
        Q_OBJECT

    private:
        PWTS::CPUVendor cpuVendor = PWTS::CPUVendor::Unknown;
        PWTS::OSType osType = PWTS::OSType::Unknown;
        bool connected = false;
        quint16 sport = -1;
        QString saddr;
        QThread *serviceThread;
        ServiceWorker *service;

    public:
        ClientService();
        ~ClientService() override;

        [[nodiscard]] static constexpr int getMajorVersion() { return SERVICE_VER_MAJOR; }
        [[nodiscard]] static constexpr int getMinorVersion() { return SERVICE_VER_MINOR; }
        [[nodiscard]] bool isConnected() const { return connected; }
        [[nodiscard]] QString getDaemonAddress() const { return saddr; }
        [[nodiscard]] quint16 getDaemonPort() const { return sport; }
        void disconnectFromDaemon() { emit workerDisconnectFromDaemon(); }
        void sendGetDeviceInfoPacketRequest() { emit workerSendGetDeviceInfoPacketRequest(); }
        void sendGetDaemonPacketRequest() { emit workerSendGetDaemonPacketRequest(); }
        void sendApplySettingsRequest(const PWTS::ClientPacket &packet) { emit workerSendApplySettingsRequest(packet); }
        void sendGetDaemonSettingsRequest() { emit workerSendGetDaemonSettingsRequest(); }
        void sendGetProfileListRequest() { emit workerSendGetProfileListRequest(); }
        void sendDeleteProfileRequest(const QString &name) { emit workerSendDeleteProfileRequest(name); }
        void sendWriteProfileRequest(const QString &name, const PWTS::ClientPacket &packet) { emit workerSendWriteProfileRequest(name, packet); }
        void sendLoadProfileRequest(const QString &name) { emit workerSendLoadProfileRequest(name); }
        void sendApplyProfileRequest(const QString &name) { emit workerSendApplyProfileRequest(name); }
        void sendExportProfilesRequest(const QString &name) { emit workerSendExportProfilesRequest(name); }
        void sendImportProfilesRequest(const QHash<QString, QByteArray> &profiles) { emit workerSendImportProfilesRequest(profiles); }
        void sendApplyDaemonSettingsRequest(const QByteArray &data) { emit workerSendApplyDaemonSettingsRequest(data); }

        void connectToDaemon(const QString &adr, quint16 port);

    private slots:
        void onLogMessageSent(const QString &msg) { emit logMessageSent(msg); }
        void onCommandFailed() { emit commandFailed(); }
        void onDeviceInfoPacketReceived(const PWTS::DeviceInfoPacket &packet) { emit deviceInfoPacketReceived(packet); }
        void onDaemonPacketReceived(const PWTS::DaemonPacket &packet) { emit daemonPacketReceived(packet); }
        void onCurrentSettingsApplied(const QSet<PWTS::DError> &errors) { emit settingsApplied(errors); }
        void onDaemonSettingsApplied(const bool success) { emit daemonSettingsApplied(success); }
        void onBatteryStatusChanged(const QSet<PWTS::DError> &errors, const QString &name) { emit batteryStatusChanged(errors, name); }
        void onWakeFromSleepEvent(const QSet<PWTS::DError> &errors) { emit wakeFromSleepEvent(errors); }
        void onApplyTimerTick(const QSet<PWTS::DError> &errors) { emit applyTimerTick(errors); }
        void onDaemonSettingsReceived(const QByteArray &data) { emit daemonSettingsReceived(data); }
        void onProfileApplied(const QSet<PWTS::DError> &errors, const QString &name) { emit profileApplied(errors, name); }
        void onProfileListReceived(const QList<QString> &list) { emit profileListReceived(list); }
        void onProfileDeleted(const bool result) { emit profileDeleted(result); }
        void onProfileWritten(const bool result) { emit profileWritten(result); }
        void onProfilesExported(const QHash<QString, QByteArray> &exported) { emit profilesExported(exported); }
        void onProfilesImported(const bool result) { emit profilesImported(result); }

        void onServiceConnected(const QString &adr, quint16 port);
        void onServiceDisconnected();
        void onServiceError();

    signals:
        void workerDisconnectFromDaemon();
        void workerConnectToDaemon(const QString &adr, quint16 port);
        void workerSendGetDeviceInfoPacketRequest();
        void workerSendGetDaemonPacketRequest();
        void workerSendApplySettingsRequest(const PWTS::ClientPacket &packet);
        void workerSendGetDaemonSettingsRequest();
        void workerSendApplyDaemonSettingsRequest(const QByteArray &data);
        void workerSendGetProfileListRequest();
        void workerSendDeleteProfileRequest(const QString &name);
        void workerSendWriteProfileRequest(const QString &name, const PWTS::ClientPacket &packet);
        void workerSendLoadProfileRequest(const QString &name);
        void workerSendApplyProfileRequest(const QString &name);
        void workerSendExportProfilesRequest(const QString &name);
        void workerSendImportProfilesRequest(const QHash<QString, QByteArray> &profiles);
        void logMessageSent(const QString &msg);
        void serviceError();
        void serviceConnected();
        void serviceDisconnected();
        void commandFailed();
        void deviceInfoPacketReceived(const PWTS::DeviceInfoPacket &packet);
        void daemonPacketReceived(const PWTS::DaemonPacket &packet);
        void settingsApplied(const QSet<PWTS::DError> &errors);
        void daemonSettingsApplied(bool success);
        void batteryStatusChanged(const QSet<PWTS::DError> &errors, const QString &name);
        void wakeFromSleepEvent(const QSet<PWTS::DError> &errors);
        void applyTimerTick(const QSet<PWTS::DError> &errors);
        void daemonSettingsReceived(const QByteArray &data);
        void profileApplied(const QSet<PWTS::DError> &errors, const QString &name);
        void profileListReceived(const QList<QString> &list);
        void profileDeleted(bool result);
        void profileWritten(bool result);
        void profilesExported(const QHash<QString, QByteArray> &exported);
        void profilesImported(bool result);
    };
}
