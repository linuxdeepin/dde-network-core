// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xdgactivation.h"

#include <DSGApplication>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QtWaylandClient/QWaylandClientExtension>

#include "qwayland-xdg-activation-v1.h"

#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandinputdevice_p.h>
#include <private/qwaylandwindow_p.h>

Q_LOGGING_CATEGORY(trayXdgActivation, "dde.network.xdgactivation")

namespace dde {
namespace network {

// ---------------------------------------------------------------------------
// XdgActivationTokenV1
// ---------------------------------------------------------------------------

class XdgActivationTokenV1 : public QObject, public QtWayland::xdg_activation_token_v1
{
    Q_OBJECT
public:
    ~XdgActivationTokenV1() override
    {
        destroy();
    }

Q_SIGNALS:
    void done(const QString &token);

protected:
    void xdg_activation_token_v1_done(const QString &token) override
    {
        Q_EMIT done(token);
    }
};

// ---------------------------------------------------------------------------
// XdgActivationV1 (singleton extension)
// ---------------------------------------------------------------------------

namespace {

class XdgActivationV1 : public QWaylandClientExtensionTemplate<XdgActivationV1>,
                         public QtWayland::xdg_activation_v1
{
public:
    XdgActivationV1()
        : QWaylandClientExtensionTemplate<XdgActivationV1>(1)
    {
        qCDebug(trayXdgActivation) << "Initializing xdg_activation_v1 extension, version:";
        initialize();
        qCDebug(trayXdgActivation) << "xdg_activation_v1 initialized, isActive:" << isActive();
    }

    ~XdgActivationV1() override
    {
        if (isInitialized())
            destroy();
    }

    XdgActivationTokenV1 *createTokenProvider(QWindow *window, const QString &appId)
    {
        qCDebug(trayXdgActivation) << "Creating token provider, window:" << window << "appId:" << appId;
        auto *provider = new XdgActivationTokenV1;
        provider->init(get_activation_token());

        if (window) {
            if (auto *waylandWindow = dynamic_cast<QtWaylandClient::QWaylandWindow *>(window->handle())) {
                if (auto *surface = waylandWindow->wlSurface()) {
                    qCDebug(trayXdgActivation) << "Setting surface for token request";
                    provider->set_surface(surface);
                } else {
                    qCWarning(trayXdgActivation) << "WaylandWindow has no wlSurface";
                }
                if (auto *inputDevice = waylandWindow->display()->lastInputDevice()) {
                    qCDebug(trayXdgActivation) << "Setting serial:" << inputDevice->serial() << "seat:" << inputDevice->wl_seat();
                    provider->set_serial(inputDevice->serial(), inputDevice->wl_seat());
                } else {
                    qCWarning(trayXdgActivation) << "No lastInputDevice available";
                }
            } else {
                qCWarning(trayXdgActivation) << "Window handle is not a QWaylandWindow";
            }
        } else {
            qCWarning(trayXdgActivation) << "No window provided for token request";
        }

        if (!appId.isEmpty())
            provider->set_app_id(appId);

        provider->commit();
        qCDebug(trayXdgActivation) << "Token provider committed";
        return provider;
    }
};

XdgActivationV1 *activationV1()
{
    static QPointer<XdgActivationV1> activation;
    if (activation)
        return activation;

    activation = new XdgActivationV1;
    activation->setParent(qApp);
    return activation;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// XdgActivation
// ---------------------------------------------------------------------------

XdgActivation::XdgActivation(QObject *parent)
    : QObject(parent)
{
}

XdgActivation::~XdgActivation() = default;

bool XdgActivation::isActive() const
{
    auto *activation = activationV1();
    const bool active = activation && activation->isActive();
    qCDebug(trayXdgActivation) << "isActive:" << active;
    return active;
}

void XdgActivation::requestToken(QWindow *window, const QString &appId)
{
    qCDebug(trayXdgActivation) << "requestToken called, window:" << window << "appId:" << appId;

    if (m_provider) {
        qCWarning(trayXdgActivation) << "XDG activation token request already started, ignoring";
        return;
    }

    if (!isActive()) {
        qCDebug(trayXdgActivation) << "xdg_activation_v1 is not active; token request skipped";
        Q_EMIT tokenReady({});
        return;
    }

    const QString effectiveAppId = appId.isEmpty() ? QString::fromUtf8(DTK_CORE_NAMESPACE::DSGApplication::id()) : appId;
    qCDebug(trayXdgActivation) << "effectiveAppId:" << effectiveAppId;

    if (effectiveAppId.isEmpty())
        qCWarning(trayXdgActivation) << "XDG activation request has empty app id";

    auto effectiveWindow = window ? window : QGuiApplication::focusWindow();
    qCDebug(trayXdgActivation) << "effectiveWindow:" << effectiveWindow;
    auto *provider = activationV1()->createTokenProvider(effectiveWindow, effectiveAppId);
    provider->setParent(this);
    m_provider = provider;

    connect(provider, &XdgActivationTokenV1::done, this, [this, provider, effectiveAppId](const QString &token) {
        m_provider = nullptr;
        if (token.isEmpty())
            qCWarning(trayXdgActivation) << "XDG activation token missing for app:" << effectiveAppId;
        else
            qCDebug(trayXdgActivation) << "XDG activation token received for app:" << effectiveAppId << "token length:" << token.length();
        provider->deleteLater();
        Q_EMIT tokenReady(token);
    }, Qt::SingleShotConnection);
}

} // namespace network
} // namespace dde

#include "xdgactivation.moc"
