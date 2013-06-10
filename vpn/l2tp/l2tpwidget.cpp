/*
    Copyright 2013 Jan Grulich <jgrulich@redhat.com>

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

#include "l2tpwidget.h"
#include "l2tpadvancedwidget.h"
#include "ui_l2tp.h"
#include "nm-l2tp-service.h"

#include <NetworkManagerQt/Setting>

#include <QDBusMetaType>
#include <QDebug>

L2tpWidget::L2tpWidget(const NetworkManager::VpnSetting::Ptr &setting, QWidget* parent, Qt::WindowFlags f):
    SettingWidget(setting, parent, f),
    m_ui(new Ui::L2tpWidget),
    m_setting(setting)
{
    qDBusRegisterMetaType<NMStringMap>();

    m_ui->setupUi(this);

    connect(m_ui->cboUserPasswordType, SIGNAL(currentIndexChanged(int)), SLOT(userPasswordTypeChanged(int)));
    connect(m_ui->cbShowPassword, SIGNAL(toggled(bool)), SLOT(showPassword(bool)));
    connect(m_ui->btnIPSecSettings, SIGNAL(clicked(bool)), SLOT(showAdvanced()));

    connect(m_ui->gateway, SIGNAL(textChanged(QString)), SLOT(slotWidgetChanged()));

    if (m_setting)
        loadConfig(setting);
}

L2tpWidget::~L2tpWidget()
{
    delete m_ui;
}

void L2tpWidget::loadConfig(const NetworkManager::Setting::Ptr &setting)
{
    Q_UNUSED(setting);

    const NMStringMap data = m_setting->data();
    const NMStringMap secrets = m_setting->secrets();

    const QString gateway = data.value(NM_L2TP_KEY_GATEWAY);
    if (!gateway.isEmpty()) {
        m_ui->gateway->setText(gateway);
    }

    const QString user = data.value(NM_L2TP_KEY_USER);
    if (!user.isEmpty()) {
        m_ui->username->setText(user);
    }

    const QString userPassword = secrets.value(NM_L2TP_KEY_PASSWORD);
    if (!userPassword.isEmpty()) {
        m_ui->password->setText(userPassword);
    }

    const NetworkManager::Setting::SecretFlags userPassType = static_cast<NetworkManager::Setting::SecretFlags>(data.value(NM_L2TP_KEY_PASSWORD"-flags").toInt());
    if (userPassType.testFlag(NetworkManager::Setting::NotSaved)) {
        m_ui->cboUserPasswordType->setCurrentIndex(1); // always ask
    } else if (userPassType.testFlag(NetworkManager::Setting::NotRequired)) {
        m_ui->cboUserPasswordType->setCurrentIndex(2); // not required
    } else {
        m_ui->cboUserPasswordType->setCurrentIndex(0); // saved
    }

    const QString domain = data.value(NM_L2TP_KEY_DOMAIN);
    if (!domain.isEmpty()) {
        m_ui->domain->setText(domain);
    }
}

QVariantMap L2tpWidget::setting(bool agentOwned) const
{
    NetworkManager::VpnSetting setting;
    setting.setServiceType(QLatin1String(NM_DBUS_SERVICE_L2TP));
    NMStringMap data;
    if (!m_tmpSetting.isNull()) {
        data = m_tmpSetting->data();
    }
    NMStringMap secrets;

    if (!m_ui->gateway->text().isEmpty()) {
        data.insert(NM_L2TP_KEY_GATEWAY, m_ui->gateway->text());
    }

    if (!m_ui->username->text().isEmpty()) {
        data.insert(NM_L2TP_KEY_USER, m_ui->username->text());
    }

    if (m_ui->password->isEnabled() && !m_ui->password->text().isEmpty()) {
        secrets.insert(NM_L2TP_KEY_PASSWORD, m_ui->password->text());
    }

    const int userPasswordTypeIndex =  m_ui->cboUserPasswordType->currentIndex();
    if (userPasswordTypeIndex == 1) { // always ask
        data.insert(NM_L2TP_KEY_PASSWORD"-flags", QString::number(NetworkManager::Setting::NotSaved));
    } else if (userPasswordTypeIndex == 2) { // not required
        data.insert(NM_L2TP_KEY_PASSWORD"-flags", QString::number(NetworkManager::Setting::NotRequired));
    } else { // none
        if (agentOwned) {
            data.insert(NM_L2TP_KEY_PASSWORD"-flags", QString::number(NetworkManager::Setting::AgentOwned));
        } else {
            data.insert(NM_L2TP_KEY_PASSWORD"-flags", QString::number(NetworkManager::Setting::None));
        }
    }

    if (!m_ui->domain->text().isEmpty())
        data.insert(NM_L2TP_KEY_DOMAIN, m_ui->domain->text());

    setting.setData(data);
    setting.setSecrets(secrets);
    return setting.toMap();
}


void L2tpWidget::userPasswordTypeChanged(int index)
{
    if (index == 0 || index == 1) {
        m_ui->password->setEnabled(false);
    }
    else {
        m_ui->password->setEnabled(true);
    }
}

void L2tpWidget::showPassword(bool show)
{
    m_ui->password->setPasswordMode(!show);
}

void L2tpWidget::showAdvanced()
{
    QPointer<L2tpAdvancedWidget> adv;
    if (m_tmpSetting.isNull()) {
        adv = new L2tpAdvancedWidget(m_setting, this);
    } else {
        adv = new L2tpAdvancedWidget(m_tmpSetting, this);
    }
    if (adv->exec() == QDialog::Accepted) {
        NMStringMap advData = adv->setting();
        if (!advData.isEmpty()) {
            if (m_tmpSetting.isNull()) {
                m_tmpSetting = NetworkManager::VpnSetting::Ptr(new NetworkManager::VpnSetting);
            }
            m_tmpSetting->setData(advData);
        }
    }

    if (adv) {
        adv->deleteLater();
    }
}

bool L2tpWidget::isValid() const
{
    return !m_ui->gateway->text().isEmpty();
}
