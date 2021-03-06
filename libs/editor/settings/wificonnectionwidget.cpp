/*
    Copyright (c) 2013 Lukas Tinkl <ltinkl@redhat.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "debug.h"
#include "wificonnectionwidget.h"
#include "ui_wificonnectionwidget.h"

#include <NetworkManagerQt/Utils>
#include <KLocalizedString>

#include "uiutils.h"

WifiConnectionWidget::WifiConnectionWidget(const NetworkManager::Setting::Ptr &setting, QWidget* parent, Qt::WindowFlags f):
    SettingWidget(setting, parent, f),
    m_ui(new Ui::WifiConnectionWidget)
{
    qsrand(QTime::currentTime().msec());

    m_ui->setupUi(this);

    connect(m_ui->btnRandomMacAddr, &QPushButton::clicked, this, &WifiConnectionWidget::generateRandomClonedMac);
    connect(m_ui->SSIDCombo, &SsidComboBox::ssidChanged, this, static_cast<void (WifiConnectionWidget::*)()>(&WifiConnectionWidget::ssidChanged));
    connect(m_ui->modeComboBox, static_cast<void (KComboBox::*)(int)>(&KComboBox::currentIndexChanged), this, &WifiConnectionWidget::modeChanged);
    connect(m_ui->band, static_cast<void (KComboBox::*)(int)>(&KComboBox::currentIndexChanged), this, &WifiConnectionWidget::bandChanged);

    // Validation
    connect(m_ui->macAddress, &HwAddrComboBox::hwAddressChanged, this, &WifiConnectionWidget::slotWidgetChanged);
    connect(m_ui->BSSIDCombo, &BssidComboBox::bssidChanged, this, &WifiConnectionWidget::slotWidgetChanged);

    KAcceleratorManager::manage(this);

    if (setting) {
        loadConfig(setting);
    }
}

WifiConnectionWidget::~WifiConnectionWidget()
{
    delete m_ui;
}

void WifiConnectionWidget::loadConfig(const NetworkManager::Setting::Ptr &setting)
{
    NetworkManager::WirelessSetting::Ptr wifiSetting = setting.staticCast<NetworkManager::WirelessSetting>();

    m_ui->SSIDCombo->init(QString::fromUtf8(wifiSetting->ssid()));

    if (wifiSetting->mode() != NetworkManager::WirelessSetting::Infrastructure) {
        m_ui->modeComboBox->setCurrentIndex(wifiSetting->mode());
    }
    modeChanged(wifiSetting->mode());

    m_ui->BSSIDCombo->init(NetworkManager::macAddressAsString(wifiSetting->bssid()), QString::fromUtf8(wifiSetting->ssid()));

    m_ui->band->setCurrentIndex(wifiSetting->band());
    if (wifiSetting->band() != NetworkManager::WirelessSetting::Automatic) {
        m_ui->channel->setCurrentIndex(m_ui->channel->findData(wifiSetting->channel()));
    }

    m_ui->macAddress->init(NetworkManager::Device::Wifi, NetworkManager::macAddressAsString(wifiSetting->macAddress()));

    if (!wifiSetting->clonedMacAddress().isEmpty()) {
        m_ui->clonedMacAddress->setText(NetworkManager::macAddressAsString(wifiSetting->clonedMacAddress()));
    }

    if (wifiSetting->mtu()) {
        m_ui->mtu->setValue(wifiSetting->mtu());
    }

    if (wifiSetting->hidden()) {
        m_ui->hiddenNetwork->setChecked(true);
    }
}

QVariantMap WifiConnectionWidget::setting() const
{
    NetworkManager::WirelessSetting wifiSetting;

    wifiSetting.setSsid(m_ui->SSIDCombo->ssid().toUtf8());

    wifiSetting.setMode(static_cast<NetworkManager::WirelessSetting::NetworkMode>(m_ui->modeComboBox->currentIndex()));

    wifiSetting.setBssid(NetworkManager::macAddressFromString(m_ui->BSSIDCombo->bssid()));

    if (wifiSetting.mode() != NetworkManager::WirelessSetting::Infrastructure && m_ui->band->currentIndex() != 0) {
        wifiSetting.setBand((NetworkManager::WirelessSetting::FrequencyBand)m_ui->band->currentIndex());
        wifiSetting.setChannel(m_ui->channel->itemData(m_ui->channel->currentIndex()).toUInt());
    }

    wifiSetting.setMacAddress(NetworkManager::macAddressFromString(m_ui->macAddress->hwAddress()));

    if (!m_ui->clonedMacAddress->text().isEmpty() && m_ui->clonedMacAddress->text() != ":::::") {
        wifiSetting.setClonedMacAddress(NetworkManager::macAddressFromString(m_ui->clonedMacAddress->text()));
    }

    if (m_ui->mtu->value()) {
        wifiSetting.setMtu(m_ui->mtu->value());
    }

    wifiSetting.setHidden(m_ui->hiddenNetwork->isChecked());

    return wifiSetting.toMap();
}

void WifiConnectionWidget::generateRandomClonedMac()
{
    QByteArray mac;
    mac.resize(6);
    for (int i = 0; i < 6; i++) {
        int random = qrand() % 255;
        mac[i] = random;
    }

    // Disable the multicast bit and enable the locally administered bit.
    mac[0] = mac[0] & ~0x1;
    mac[0] = mac[0] |  0x2;

    m_ui->clonedMacAddress->setText(NetworkManager::macAddressAsString(mac));
}

void WifiConnectionWidget::ssidChanged()
{
    m_ui->BSSIDCombo->init(m_ui->BSSIDCombo->bssid(), m_ui->SSIDCombo->ssid());
    slotWidgetChanged();

    // Emit that SSID has changed so we can pre-configure wireless security
    Q_EMIT ssidChanged(m_ui->SSIDCombo->ssid());
}

void WifiConnectionWidget::modeChanged(int mode)
{
    if (mode == NetworkManager::WirelessSetting::Infrastructure) {
        m_ui->BSSIDLabel->setVisible(true);
        m_ui->BSSIDCombo->setVisible(true);
        m_ui->bandLabel->setVisible(false);
        m_ui->band->setVisible(false);
        m_ui->channelLabel->setVisible(false);
        m_ui->channel->setVisible(false);
    } else {
        m_ui->BSSIDLabel->setVisible(false);
        m_ui->BSSIDCombo->setVisible(false);
        m_ui->bandLabel->setVisible(true);
        m_ui->band->setVisible(true);
        m_ui->channelLabel->setVisible(true);
        m_ui->channel->setVisible(true);
    }
}

void WifiConnectionWidget::bandChanged(int band)
{
    m_ui->channel->clear();

    if (band == NetworkManager::WirelessSetting::Automatic) {
        m_ui->channel->setEnabled(false);
    } else {
        fillChannels((NetworkManager::WirelessSetting::FrequencyBand)band);
        m_ui->channel->setEnabled(true);
    }
}

void WifiConnectionWidget::fillChannels(NetworkManager::WirelessSetting::FrequencyBand band)
{
    QList<QPair<int, int> > channels;

    if (band == NetworkManager::WirelessSetting::A) {
        channels = NetworkManager::getAFreqs();
    } else if (band == NetworkManager::WirelessSetting::Bg) {
        channels = NetworkManager::getBFreqs();
    } else {
        qCWarning(PLASMA_NM) << Q_FUNC_INFO << "Unhandled band number" << band;
        return;
    }

    QListIterator<QPair<int,int> > i(channels);
    while (i.hasNext()) {
        QPair<int,int> channel = i.next();
        m_ui->channel->addItem(i18n("%1 (%2 MHz)", channel.first, channel.second), channel.first);
    }
}

bool WifiConnectionWidget::isValid() const
{
    return !m_ui->SSIDCombo->currentText().isEmpty() && m_ui->macAddress->isValid() && m_ui->BSSIDCombo->isValid();
}
