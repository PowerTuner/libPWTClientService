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
#include "ClientService.h"
#include "ServiceWorker.h"
#include "pwtShared/Utils.h"

namespace PWTCS {
    ClientService::~ClientService() {
        serviceThread->quit();
        serviceThread->wait();
        delete serviceThread;
    }

    ClientService::ClientService() {
        service = new ServiceWorker();
        serviceThread = new QThread();

        service->moveToThread(serviceThread);

        QObject::connect(serviceThread, &QThread::started, service, &ServiceWorker::init);
        QObject::connect(serviceThread, &QThread::finished, service, &QObject::deleteLater);
        QObject::connect(service, &ServiceWorker::logMessageSent, this, &ClientService::onLogMessageSent);
        QObject::connect(service, &ServiceWorker::serviceConnected, this, &ClientService::onServiceConnected);
        QObject::connect(service, &ServiceWorker::serviceError, this, &ClientService::onServiceError);
        QObject::connect(service, &ServiceWorker::serviceDisconnected, this, &ClientService::onServiceDisconnected);
        QObject::connect(service, &ServiceWorker::commandFailed, this, &ClientService::onCommandFailed);
        QObject::connect(service, &ServiceWorker::deviceInfoPacketReceived, this, &ClientService::onDeviceInfoPacketReceived);
        QObject::connect(service, &ServiceWorker::daemonPacketReceived, this, &ClientService::onDaemonPacketReceived);
        QObject::connect(service, &ServiceWorker::currentSettingsApplied, this, &ClientService::onCurrentSettingsApplied);
        QObject::connect(service, &ServiceWorker::profileListReceived, this, &ClientService::onProfileListReceived);
        QObject::connect(service, &ServiceWorker::profileWritten, this, &ClientService::onProfileWritten);
        QObject::connect(service, &ServiceWorker::profileDeleted, this, &ClientService::onProfileDeleted);
        QObject::connect(service, &ServiceWorker::profileApplied, this, &ClientService::onProfileApplied);
        QObject::connect(service, &ServiceWorker::daemonSettingsReceived, this, &ClientService::onDaemonSettingsReceived);
        QObject::connect(service, &ServiceWorker::daemonSettingsApplied, this, &ClientService::onDaemonSettingsApplied);
        QObject::connect(service, &ServiceWorker::batteryStatusChanged, this, &ClientService::onBatteryStatusChanged);
        QObject::connect(service, &ServiceWorker::wakeFromSleepEvent, this, &ClientService::onWakeFromSleepEvent);
        QObject::connect(service, &ServiceWorker::applyTimerTick, this, &ClientService::onApplyTimerTick);
        QObject::connect(service, &ServiceWorker::profilesExported, this, &ClientService::onProfilesExported);
        QObject::connect(service, &ServiceWorker::profilesImported, this, &ClientService::onProfilesImported);
        QObject::connect(this, &ClientService::workerDisconnectFromDaemon, service, &ServiceWorker::disconnectFromDaemon);
        QObject::connect(this, &ClientService::workerConnectToDaemon, service, &ServiceWorker::connectToDaemon);
        QObject::connect(this, &ClientService::workerSendGetDeviceInfoPacketRequest, service, &ServiceWorker::sendGetDeviceInfoPacketRequest);
        QObject::connect(this, &ClientService::workerSendGetDaemonPacketRequest, service, &ServiceWorker::sendGetDaemonPacketRequest);
        QObject::connect(this, &ClientService::workerSendApplySettingsRequest, service, &ServiceWorker::sendApplySettingsRequest);
        QObject::connect(this, &ClientService::workerSendGetDaemonSettingsRequest, service, &ServiceWorker::sendGetDaemonSettingsRequest);
        QObject::connect(this, &ClientService::workerSendApplyDaemonSettingsRequest, service, &ServiceWorker::sendApplyDaemonSettingsRequest);
        QObject::connect(this, &ClientService::workerSendGetProfileListRequest, service, &ServiceWorker::sendGetProfileListRequest);
        QObject::connect(this, &ClientService::workerSendDeleteProfileRequest, service, &ServiceWorker::sendDeleteProfileRequest);
        QObject::connect(this, &ClientService::workerSendWriteProfileRequest, service, &ServiceWorker::sendWriteProfileRequest);
        QObject::connect(this, &ClientService::workerSendLoadProfileRequest, service, &ServiceWorker::sendLoadProfileRequest);
        QObject::connect(this, &ClientService::workerSendApplyProfileRequest, service, &ServiceWorker::sendApplyProfileRequest);
        QObject::connect(this, &ClientService::workerSendExportProfilesRequest, service, &ServiceWorker::sendExportProfilesRequest);
        QObject::connect(this, &ClientService::workerSendImportProfilesRequest, service, &ServiceWorker::sendImportProfilesRequest);

        serviceThread->start();
    }

    void ClientService::connectToDaemon(const QString &adr, const quint16 port) {
        emit workerConnectToDaemon(adr, port);
    }

    void ClientService::onServiceConnected(const QString &adr, const quint16 port) {
        saddr = adr;
        sport = port;
        connected = true;

        emit serviceConnected();
    }

    void ClientService::onServiceDisconnected() {
        saddr = "";
        sport = -1;
        connected = false;
        emit serviceDisconnected();
    }

    void ClientService::onServiceError() {
        connected = false;
        emit serviceError();
    }
}
