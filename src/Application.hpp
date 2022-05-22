#pragma once

#include <QApplication>
#include <memory>

#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "singletons/NativeMessaging.hpp"

namespace chatterino {

class TwitchIrcServer;
class PubSub;

class CommandController;
class AccountController;
class NotificationController;
class HighlightController;
class HotkeyController;

class Theme;
class WindowManager;
class Logging;
class Paths;
class AccountManager;
class Emotes;
class Settings;
class Fonts;
class Toasts;
class ChatterinoBadges;
class FfzBadges;

class BaseApplication
{
public:
    BaseApplication();
    virtual ~BaseApplication() = default;

    static BaseApplication *instance;

    virtual AccountController *const getAccounts() = 0;
};

class Application : public BaseApplication
{
    std::vector<std::unique_ptr<Singleton>> singletons_;
    int argc_;
    char **argv_;

public:
    static Application *instance;

    Application(Settings &settings, Paths &paths);

    void initialize(Settings &settings, Paths &paths);
    void load();
    void save();

    int run(QApplication &qtApp);

    friend void test();

    Theme *const themes{};
    Fonts *const fonts{};
    Emotes *const emotes{};
    AccountController *const accounts{};
    AccountController *const getAccounts() override
    {
        return this->accounts;
    };
    HotkeyController *const hotkeys{};
    WindowManager *const windows{};
    Toasts *const toasts{};

    CommandController *const commands{};
    NotificationController *const notifications{};
    HighlightController *const highlights{};
    TwitchIrcServer *const twitch{};
    ChatterinoBadges *const chatterinoBadges{};
    FfzBadges *const ffzBadges{};

    /*[[deprecated]]*/ Logging *const logging{};

private:
    void addSingleton(Singleton *singleton);
    void initPubSub();
    void initNm(Paths &paths);

    template <typename T,
              typename = std::enable_if_t<std::is_base_of<Singleton, T>::value>>
    T &emplace()
    {
        auto t = new T;
        this->singletons_.push_back(std::unique_ptr<T>(t));
        return *t;
    }

    NativeMessagingServer nmServer{};
};

Application *getApp();
BaseApplication *getBapp();

}  // namespace chatterino
