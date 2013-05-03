/***************************************************************************
 *   Copyright (C) 2012 by Daniel Nicoletti                                *
 *   dantti12@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; see the file COPYING. If not, write to       *
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,  *
 *   Boston, MA 02110-1301, USA.                                           *
 ***************************************************************************/

#ifndef DEVICE_MODEL_H
#define DEVICE_MODEL_H

#include <QStandardItemModel>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusMessage>

#include <NetworkManagerQt/manager.h>

typedef QMap<QString, QString>  StringStringMap;
typedef QList<QDBusObjectPath> ObjectPathList;

class DeviceConnectionModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum DeviceRoles {
        RoleIsDevice = Qt::UserRole + 1,
        RoleIsConnection,
        RoleIsConnectionParent,
        RoleIsConnectionCategory,
        RoleIsConnectionCategoryActiveCount,
        RoleDeviceUNI,
        RoleConectionPath,
        RoleState,
        RoleConnectionActive,
        RoleSort
    };
    explicit DeviceConnectionModel(QObject *parent = 0);
    void init();

    Qt::ItemFlags flags(const QModelIndex &index) const;

signals:
    void parentAdded(const QModelIndex &index);

private slots:
    void initConnections();
    void removeConnections();

    void deviceAdded(const QString &uni);
    void deviceChanged();
    void deviceRemoved(const QString &uni);
    void addDevice(const NetworkManager::Device::Ptr &device);
    void changeDevice(QStandardItem *stdItem, const NetworkManager::Device::Ptr &device);

    void connectionAdded(const QString &path);
    void connectionChanged();
    void connectionRemoved(const QString &path);
    void addConnection(const NetworkManager::Settings::Connection::Ptr &connection);
    void changeConnection(QStandardItem *stdItem, const NetworkManager::Settings::Connection::Ptr &connection);

private:
    QStandardItem *findDeviceItem(const QString &uni);
    QStandardItem *findConnectionItem(const QString &path);
    QStandardItem *findOrCreateConnectionType(NetworkManager::Settings::ConnectionSettings::ConnectionType type);
};

#endif // DEVICE_MODEL_H
