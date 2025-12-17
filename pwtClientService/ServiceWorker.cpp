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
#include "ServiceWorker.h"
#include "pwtShared/Utils.h"

namespace PWTCS {
    ServiceWorker::~ServiceWorker() {
        abortSocket();

        for (const ClientServiceCmdTimer *tm: reqTimerPool)
            delete tm;

        delete sock;
    }

    void ServiceWorker::init() {
        qRegisterMetaType<PWTS::DeviceInfoPacket>();
        qRegisterMetaType<PWTS::ClientPacket>();
        qRegisterMetaType<PWTS::DaemonPacket>();

        sock = new QTcpSocket();

        sockStreamIn.setDevice(sock);

        QObject::connect(sock, &QTcpSocket::connected, this, &ServiceWorker::onConnected);
        QObject::connect(sock, &QTcpSocket::disconnected, this, &ServiceWorker::onDisconnected);
        QObject::connect(sock, &QTcpSocket::readyRead, this, &ServiceWorker::onReadyRead);
        QObject::connect(sock, &QTcpSocket::errorOccurred, this, &ServiceWorker::onErrorOccurred);
    }

    void ServiceWorker::abortSocket() const {
        const QSignalBlocker sblock {sock};

        stopAllTimers();

        if (sock->state() != QAbstractSocket::UnconnectedState)
            sock->abort();

        sock->close();
    }

    void ServiceWorker::connectToDaemon(const QString &adr, const quint16 port) {
        abortSocket();

        saddr = adr;
        sport = port;
        sock->connectToHost(QHostAddress(adr), port);
    }

    void ServiceWorker::disconnectFromDaemon() {
        abortSocket();

        if (sock->isOpen())
            emit logMessageSent(QStringLiteral("Failed to close daemon socket"));
    }

    bool ServiceWorker::disconnect() const {
        abortSocket();
        return !sock->isOpen();
    }

    void ServiceWorker::startRequestTimer(const PWTS::DCMD cmd) {
        for (ClientServiceCmdTimer *tm: reqTimerPool) {
            if (tm->isActive())
                continue;

            tm->reset(saddr, cmd);
            return;
        }

        reqTimerPool.append(new ClientServiceCmdTimer(saddr, cmd, this));
        QObject::connect(reqTimerPool.last(), &ClientServiceCmdTimer::commandTimeout, this, &ServiceWorker::onCommandTimeout);
        reqTimerPool.last()->start();
    }

    void ServiceWorker::stopAllTimers() const {
        for (ClientServiceCmdTimer *tm: reqTimerPool)
            tm->stop();
    }

    void ServiceWorker::stopTimerForCMD(const PWTS::DCMD cmd) const {
        for (ClientServiceCmdTimer *tm: reqTimerPool) {
            if (tm->getAddr() != saddr || tm->getCMD() != cmd)
                continue;

            tm->stop();
            break;
        }
    }

    bool ServiceWorker::hasValidMessageArgs(const QList<QVariant> &args) const {
        if (args.isEmpty())
            return false;

        switch (static_cast<PWTS::DCMD>(args[0].toInt())) {
            case PWTS::DCMD::DAEMON_CMD_FAIL: {
                if (args.size() < 1)
                    return false;
            }
                break;
            case PWTS::DCMD::PRINT_ERROR:
            case PWTS::DCMD::GET_DAEMON_SETTS:
            case PWTS::DCMD::APPLY_CLIENT_SETTINGS:
            case PWTS::DCMD::DELETE_PROFILE:
            case PWTS::DCMD::WRITE_PROFILE:
            case PWTS::DCMD::GET_PROFILE_LIST:
            case PWTS::DCMD::EXPORT_PROFILES:
            case PWTS::DCMD::IMPORT_PROFILES:
            case PWTS::DCMD::APPLY_TIMER:
            case PWTS::DCMD::APPLY_DAEMON_SETT:
            case PWTS::DCMD::SYS_WAKE_FROM_SLEEP: {
                if (args.size() < 2)
                    return false;
            }
                break;
            case PWTS::DCMD::APPLY_PROFILE:
            case PWTS::DCMD::LOAD_PROFILE:
            case PWTS::DCMD::BATTERY_STATUS_CHANGED: {
                if (args.size() < 3)
                    return false;
            }
                break;
            default:
                break;
        }

        return true;
    }

    void ServiceWorker::parseCMD(const QList<QVariant> &args) {
        if (!hasValidMessageArgs(args)) {
            emit logMessageSent(setErrorMsg(QStringLiteral("parseCMD: args is invalid")));
            emit serviceError();
            return;
        }

        const PWTS::DCMD cmd = static_cast<PWTS::DCMD>(args[0].toInt());

        switch (cmd) {
            case PWTS::DCMD::PRINT_ERROR: {
                emit logMessageSent(setErrorMsg(PWTS::getErrorStr(static_cast<PWTS::DError>(args[1].toInt()))));
                emit commandFailed();
            }
                break;
            case PWTS::DCMD::DAEMON_CMD_FAIL:
                stopTimerForCMD(static_cast<PWTS::DCMD>(args[1].toInt()));
                break;
            case PWTS::DCMD::GET_DEVICE_INFO_PACKET: {
                stopTimerForCMD(cmd);

                if (!args[1].canConvert<PWTS::DeviceInfoPacket>()) {
                    emit logMessageSent(setErrorMsg(QStringLiteral("Unable to unpack device info packet")));
                    emit commandFailed();
                    break;
                }

                const PWTS::DeviceInfoPacket packet = args[1].value<PWTS::DeviceInfoPacket>();

                if (packet.error != PWTS::PacketError::NoError) {
                    emit logMessageSent(PWTS::getPacketErrorStr(packet.error));
                    emit commandFailed();
                    break;
                }

                emit deviceInfoPacketReceived(packet);
            }
                break;
            case PWTS::DCMD::GET_DAEMON_PACKET: {
                stopTimerForCMD(cmd);

                if (!args[1].canConvert<PWTS::DaemonPacket>()) {
                    emit logMessageSent(setErrorMsg(QStringLiteral("Unable to unpack daemon packet")));
                    emit commandFailed();
                    break;
                }

                const PWTS::DaemonPacket packet = args[1].value<PWTS::DaemonPacket>();

                if (packet.error != PWTS::PacketError::NoError) {
                    emit logMessageSent(PWTS::getPacketErrorStr(packet.error));
                    emit commandFailed();
                    break;
                }

                emit daemonPacketReceived(packet);
            }
                break;
            case PWTS::DCMD::GET_DAEMON_SETTS: {
                stopTimerForCMD(cmd);

                const QByteArray data = args[1].toByteArray();

                if (data.isEmpty()) {
                    emit logMessageSent(setErrorMsg(QStringLiteral("Unable to get daemon settings")));
                    emit commandFailed();

                } else {
                    emit daemonSettingsReceived(data);
                }
            }
                break;
            case PWTS::DCMD::APPLY_CLIENT_SETTINGS: {
                stopTimerForCMD(cmd);

                QSet<PWTS::DError> errors;

                if (!PWTS::unpackData<QSet<PWTS::DError>>(args[1].toByteArray(), errors)) {
                    emit logMessageSent(setErrorMsg(QStringLiteral("Unable to get apply settings result")));
                    emit commandFailed();

                } else {
                    emit currentSettingsApplied(errors);
                }
            }
                break;
            case PWTS::DCMD::DELETE_PROFILE: {
                stopTimerForCMD(cmd);
                emit profileDeleted(args[1].toBool());
            }
                break;
            case PWTS::DCMD::WRITE_PROFILE: {
                stopTimerForCMD(cmd);
                emit profileWritten(args[1].toBool());
            }
                break;
            case PWTS::DCMD::GET_PROFILE_LIST:
                emit profileListReceived(args[1].toStringList());
                break;
            case PWTS::DCMD::APPLY_PROFILE: {
                stopTimerForCMD(cmd);

                QSet<PWTS::DError> errors;

                if (!PWTS::unpackData<QSet<PWTS::DError>>(args[1].toByteArray(), errors)) {
                    emit logMessageSent(setErrorMsg(QStringLiteral("Unable to get apply profile result")));
                    emit commandFailed();

                } else {
                    emit profileApplied(errors, args[2].toString());
                }
            }
                break;
            case PWTS::DCMD::LOAD_PROFILE: {
                stopTimerForCMD(cmd);

                if (!args[1].canConvert<PWTS::DaemonPacket>()) {
                    emit logMessageSent(setErrorMsg(QStringLiteral("Unable to unpack daemon packet")));
                    emit commandFailed();
                    break;
                }

                emit logMessageSent(QString("Loaded profile: %1").arg(args[2].toString()));
                emit daemonPacketReceived(args[1].value<PWTS::DaemonPacket>());
            }
                break;
            case PWTS::DCMD::EXPORT_PROFILES: {
                stopTimerForCMD(cmd);

                QHash<QString, QByteArray> exported;

                if (!PWTS::unpackData<QHash<QString, QByteArray>>(args[1].toByteArray(), exported)) {
                    emit logMessageSent(setErrorMsg(QStringLiteral("Failed to get exported profiles data")));
                    emit commandFailed();

                } else {
                    emit profilesExported(exported);
                }
            }
                break;
            case PWTS::DCMD::IMPORT_PROFILES: {
                stopTimerForCMD(cmd);
                emit profilesImported(args[1].toBool());
            }
                break;
            case PWTS::DCMD::APPLY_DAEMON_SETT: {
                stopTimerForCMD(cmd);
                emit daemonSettingsApplied(args[1].toBool());
            }
                break;
            case PWTS::DCMD::BATTERY_STATUS_CHANGED: {
                QSet<PWTS::DError> errors;

                if (!PWTS::unpackData<QSet<PWTS::DError>>(args[1].toByteArray(), errors)) {
                    emit logMessageSent(setErrorMsg(QStringLiteral("Unable to get battery status change event result")));
                    emit commandFailed();

                } else {
                    emit batteryStatusChanged(errors, args[2].toString());
                }
            }
                break;
            case PWTS::DCMD::SYS_WAKE_FROM_SLEEP: {
                QSet<PWTS::DError> errors;

                if (!PWTS::unpackData<QSet<PWTS::DError>>(args[1].toByteArray(), errors)) {
                    emit logMessageSent(setErrorMsg(QStringLiteral("Unable to get wake from sleep event result")));
                    emit commandFailed();

                } else {
                    emit wakeFromSleepEvent(errors);
                }
            }
                break;
            case PWTS::DCMD::APPLY_TIMER: {
                QSet<PWTS::DError> errors;

                if (!PWTS::unpackData<QSet<PWTS::DError>>(args[1].toByteArray(), errors)) {
                    emit logMessageSent(setErrorMsg(QStringLiteral("Unable to get apply timer result")));
                    emit commandFailed();

                } else {
                    emit applyTimerTick(errors);
                }
            }
                break;
            default: {
                stopTimerForCMD(cmd);
                emit logMessageSent(setErrorMsg(QString("unknown cmd %1").arg(static_cast<int>(cmd))));
                emit commandFailed();
            }
                break;
        }
    }

    void ServiceWorker::sendCMD(const PWTS::DCMD cmd, const QList<QVariant> &args) {
        QByteArray data;

        if (!PWTS::packData<QList<QVariant>>(args, data)) {
            emit logMessageSent(setErrorMsg(QString("Failed to send cmd %1").arg(static_cast<int>(cmd))));
            emit commandFailed();
            return;
        }

        sock->write(data);
        sock->flush();
        startRequestTimer(cmd);
    }

    void ServiceWorker::sendGetDeviceInfoPacketRequest() {
        constexpr PWTS::DCMD cmd = PWTS::DCMD::GET_DEVICE_INFO_PACKET;
        const QList<QVariant> args {static_cast<int>(cmd)};

        sendCMD(cmd, args);
    }

    void ServiceWorker::sendGetDaemonPacketRequest() {
        constexpr PWTS::DCMD cmd = PWTS::DCMD::GET_DAEMON_PACKET;
        const QList<QVariant> args {static_cast<int>(cmd)};

        sendCMD(cmd, args);
    }

    void ServiceWorker::sendApplySettingsRequest(const PWTS::ClientPacket &packet) {
        constexpr PWTS::DCMD cmd = PWTS::DCMD::APPLY_CLIENT_SETTINGS;
        const QList<QVariant> args {static_cast<int>(cmd), QVariant::fromValue<PWTS::ClientPacket>(packet)};

        sendCMD(cmd, args);
    }

    void ServiceWorker::sendGetDaemonSettingsRequest() {
        constexpr PWTS::DCMD cmd = PWTS::DCMD::GET_DAEMON_SETTS;
        const QList<QVariant> args {static_cast<int>(cmd)};

        sendCMD(cmd, args);
    }

    void ServiceWorker::sendGetProfileListRequest() {
        constexpr PWTS::DCMD cmd = PWTS::DCMD::GET_PROFILE_LIST;
        const QList<QVariant> args {static_cast<int>(cmd)};

        sendCMD(cmd, args);
    }

    void ServiceWorker::sendDeleteProfileRequest(const QString &name) {
        constexpr PWTS::DCMD cmd = PWTS::DCMD::DELETE_PROFILE;
        const QList<QVariant> args {static_cast<int>(cmd), name};

        sendCMD(cmd, args);
    }

    void ServiceWorker::sendWriteProfileRequest(const QString &name, const PWTS::ClientPacket &packet) {
        constexpr PWTS::DCMD cmd = PWTS::DCMD::WRITE_PROFILE;
        const QList<QVariant> args {static_cast<int>(cmd), name, QVariant::fromValue<PWTS::ClientPacket>(packet)};

        sendCMD(cmd, args);
    }

    void ServiceWorker::sendLoadProfileRequest(const QString &name) {
        constexpr PWTS::DCMD cmd = PWTS::DCMD::LOAD_PROFILE;
        const QList<QVariant> args {static_cast<int>(cmd), name};

        sendCMD(cmd, args);
    }

    void ServiceWorker::sendApplyProfileRequest(const QString &name) {
        constexpr PWTS::DCMD cmd = PWTS::DCMD::APPLY_PROFILE;
        const QList<QVariant> args {static_cast<int>(cmd), name};

        sendCMD(cmd, args);
    }

    void ServiceWorker::sendExportProfilesRequest(const QString &name) {
        constexpr PWTS::DCMD cmd = PWTS::DCMD::EXPORT_PROFILES;
        const QList<QVariant> args {static_cast<int>(cmd), name};

        sendCMD(cmd, args);
    }

    void ServiceWorker::sendImportProfilesRequest(const QHash<QString, QByteArray> &profiles) {
        constexpr PWTS::DCMD cmd = PWTS::DCMD::IMPORT_PROFILES;
        QByteArray profilesData;
        QList<QVariant> args;

        if (!PWTS::packData<QHash<QString, QByteArray>>(profiles, profilesData)) {
            emit logMessageSent(setErrorMsg("Import profiles: failed to pack profiles data for send"));
            emit commandFailed();
            return;
        }

        args.append(static_cast<int>(cmd));
        args.append(profilesData);

        sendCMD(cmd, args);
    }

    void ServiceWorker::sendApplyDaemonSettingsRequest(const QByteArray &data) {
        constexpr PWTS::DCMD cmd = PWTS::DCMD::APPLY_DAEMON_SETT;
        const QList<QVariant> args {static_cast<int>(cmd), data};

        sendCMD(cmd, args);
    }

    void ServiceWorker::onConnected() {
        emit serviceConnected(saddr, sport);
    }

    void ServiceWorker::onDisconnected() {
        if (!disconnect())
            emit logMessageSent(setErrorMsg(QStringLiteral("failed to gracefully disconnect daemon!")));

        emit serviceDisconnected();
    }

    void ServiceWorker::onReadyRead() {
        QList<QVariant> args;

        while (true) {
            sockStreamIn.startTransaction();
            sockStreamIn >> args;

            if (!sockStreamIn.commitTransaction())
                break;

            if (args.isEmpty()) {
                emit logMessageSent(setErrorMsg(QStringLiteral("Failed to get data from daemon")));
                emit commandFailed();
                break;
            }

            parseCMD(args);
        }
    }

    void ServiceWorker::onErrorOccurred(const QAbstractSocket::SocketError error) {
        switch (error) {
            case QAbstractSocket::RemoteHostClosedError:
                emit logMessageSent(setErrorMsg(QStringLiteral("Remote host connection closed")));
                break;
            case QAbstractSocket::HostNotFoundError:
                emit logMessageSent(setErrorMsg(QStringLiteral("Host not found")));
                break;
            case QAbstractSocket::ConnectionRefusedError:
                emit logMessageSent(setErrorMsg(QStringLiteral("Connection refused, make sure server is running at given address and port")));
                break;
            case QAbstractSocket::SocketResourceError:
                emit logMessageSent(QStringLiteral("No more avilable sockets in you system, please retry later or manually close some of them"));
                break;
            default:
                emit logMessageSent(setErrorMsg(sock->errorString()));
                break;
        }

        if (!disconnect())
            emit logMessageSent(setErrorMsg(QStringLiteral("Failed to close connection on error")));

        emit serviceError();
    }

    void ServiceWorker::onCommandTimeout(const QString &sockAddr, const PWTS::DCMD cmd) {
        emit logMessageSent(QString("[%1]: request timeout for command: %2").arg(sockAddr).arg(static_cast<int>(cmd)));
        emit commandFailed();
    }
}
