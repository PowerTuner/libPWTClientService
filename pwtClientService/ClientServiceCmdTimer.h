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

#include <QTimer>

#include "pwtShared/Include/DaemonCMD.h"

namespace PWTCS {
    class ClientServiceCmdTimer final: public QTimer {
        Q_OBJECT

    private:
        static constexpr int reqTimeoutMs = 120 * 1000;
        PWTS::DCMD dcmd;
        QString addr;

    public:
        ClientServiceCmdTimer(const QString &adr, const PWTS::DCMD cmd, QObject *parent = nullptr): QTimer(parent) {
            this->addr = adr;
            this->dcmd = cmd;

            setInterval(reqTimeoutMs);
            setSingleShot(true);

            QObject::connect(this, &QTimer::timeout, this, &ClientServiceCmdTimer::onTimeout);
        }

        [[nodiscard]] QString getAddr() const { return addr; }
        [[nodiscard]] PWTS::DCMD getCMD() const { return dcmd; }

        void reset(const QString &adr, const PWTS::DCMD cmd) {
            this->addr = adr;
            this->dcmd = cmd;

            start();
        }

    private slots:
        void onTimeout() {
            emit commandTimeout(addr, dcmd);
        };

    signals:
        void commandTimeout(const QString &adr, PWTS::DCMD cmd);
    };
}
