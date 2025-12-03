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

#include <QTcpSocket>

#include "pwtShared/Include/Packets/DeviceInfoPacket.h"
#include "pwtShared/Include/Packets/ClientPacket.h"
#include "pwtShared/Include/Packets/DaemonPacket.h"
#include "pwtShared/Include/DaemonCMD.h"
#include "ClientServiceCmdTimer.h"

namespace PWTCS {
    class ServiceWorker final: public QObject {
        Q_OBJECT

    private:
        QTcpSocket *sock = nullptr;
        QList<ClientServiceCmdTimer *> reqTimerPool;
        QDataStream sockStreamIn;
        QString saddr;
        quint16 sport;

        [[nodiscard]] QString setErrorMsg(const QString &msg) const { return QString("[%1]: %2").arg(saddr, msg); }

        void abortSocket() const;
        [[nodiscard]] bool disconnect() const;
        [[nodiscard]] bool hasValidMessageArgs(const QList<QVariant> &args) const;
        void parseCMD(const QList<QVariant> &args);
        void sendCMD(PWTS::DCMD cmd, const QList<QVariant> &args);
        void startRequestTimer(PWTS::DCMD cmd);
        void stopAllTimers() const;
        void stopTimerForCMD(PWTS::DCMD cmd) const;

    public:
        ~ServiceWorker() override;

    private slots:
        void onConnected();
        void onDisconnected();
        void onReadyRead();
        void onErrorOccurred(QAbstractSocket::SocketError error);
        void onCommandTimeout(const QString &sockAddr, PWTS::DCMD cmd);

    public slots:
        void init();
        void disconnectFromDaemon();
        void connectToDaemon(const QString &adr, quint16 port);
        void sendGetDeviceInfoPacketRequest();
        void sendGetDaemonPacketRequest();
        void sendApplySettingsRequest(const PWTS::ClientPacket &packet);
        void sendGetDaemonSettingsRequest();
        void sendGetProfileListRequest();
        void sendDeleteProfileRequest(const QString &name);
        void sendWriteProfileRequest(const QString &name, const PWTS::ClientPacket &packet);
        void sendLoadProfileRequest(const QString &name);
        void sendApplyProfileRequest(const QString &name);
        void sendExportProfilesRequest(const QString &name);
        void sendImportProfilesRequest(const QHash<QString, QByteArray> &profiles);
        void sendApplyDaemonSettingsRequest(const QByteArray &data);

    signals:
        void logMessageSent(const QString &msg);
        void serviceError();
        void serviceConnected(const QString &adr, quint16 port);
        void serviceDisconnected();
        void commandFailed();
        void deviceInfoPacketReceived(const PWTS::DeviceInfoPacket &packet);
        void daemonPacketReceived(const PWTS::DaemonPacket &packet);
        void currentSettingsApplied(const QSet<PWTS::DError> &errors);
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
