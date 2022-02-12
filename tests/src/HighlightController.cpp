#include "controllers/highlights/HighlightController.hpp"
#include "Application.hpp"
#include "BaseSettings.hpp"
#include "messages/MessageBuilder.hpp"       // for MessageParseArgs
#include "providers/twitch/TwitchBadge.hpp"  // for Badge

#include <gtest/gtest.h>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QString>

using namespace chatterino;

class MockApplication : BaseApplication
{
    AccountController *const getAccounts() override
    {
        return &this->accounts;
    }

    AccountController accounts;
    // TODO: Figure this out
};

TEST(HighlightController, A)
{
    QString DEFAULT_SETTINGS = R"!(
{
    "highlighting": {
        "selfHighlight": {
            "enableSound": true
        },
        "blacklist": [
            {
                "pattern": "zenix",
                "regex": false
            }
        ],
        "users": [
            {
                "pattern": "pajlada",
                "showInMentions": false,
                "alert": false,
                "sound": false,
                "regex": false,
                "case": false,
                "soundUrl": "",
                "color": "#7fffffff"
            },
            {
                "pattern": "gempir",
                "showInMentions": true,
                "alert": true,
                "sound": false,
                "regex": false,
                "case": false,
                "soundUrl": "",
                "color": "#7ff19900"
            }
        ],
        "alwaysPlaySound": true,
        "highlights": [
            {
                "pattern": "!testmanxd",
                "showInMentions": true,
                "alert": true,
                "sound": true,
                "regex": false,
                "case": false,
                "soundUrl": "",
                "color": "#7f7f3f49"
            }
        ],
        "badges": [
            {
                "name": "broadcaster",
                "displayName": "Broadcaster",
                "alert": false,
                "sound": false,
                "soundUrl": "",
                "color": "#7f427f00"
            },
            {
                "name": "subscriber",
                "displayName": "Subscriber",
                "alert": false,
                "sound": false,
                "soundUrl": "",
                "color": "#7f7f3f49"
            },
            {
                "name": "founder",
                "displayName": "Founder",
                "alert": false,
                "sound": false,
                "soundUrl": "",
                "color": "#7fe8b7eb"
            }
        ],
        "subHighlightColor": "#64ffd641"
    }
})!";

    {
        // Write default settings to the mock settings json file
        QDir().mkpath("/tmp/c2-tests");
        QFile settingsFile("/tmp/c2-tests/settings.json");
        assert(settingsFile.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&settingsFile);
        out << DEFAULT_SETTINGS;
    }

    MockApplication mockApplication;
    Settings settings("/tmp/c2-tests");
    Paths paths;

    HighlightController controller;
    controller.initialize(settings, paths);

    struct TestCase {
        // TODO: create one of these from a raw irc message? hmm xD
        struct {
            MessageParseArgs args;
            std::vector<Badge> badges;
            QString senderName;
            QString originalMessage;
        } input;

        struct {
            bool state;
            HighlightResult result;
        } expected;
    };

    std::vector<TestCase> tests{
        {
            {
                // input
                MessageParseArgs{},  // no special args
                {},                  // no badges
                "pajlada",           // sender name
                "hello!",            // original message
            },
            {
                // expected
                true,  // state
                {
                    false,                                  // alert
                    false,                                  // playsound
                    boost::none,                            // custom sound url
                    std::make_shared<QColor>("#7fffffff"),  // color
                    false,
                },
            },
        },
        {
            {
                // input
                MessageParseArgs{},  // no special args
                {},                  // no badges
                "pajlada2",          // sender name
                "hello!",            // original message
            },
            {
                // expected
                false,                           // state
                HighlightResult::emptyResult(),  // result
            },
        },
        // TODO: Test with founder badge (simple)
        // TODO: Test matching multiple things (e.g. founder badge AND send by pajlada)
    };

    for (const auto &[input, expected] : tests)
    {
        auto [isMatch, matchResult] = controller.check(
            input.args, input.badges, input.senderName, input.originalMessage);

        EXPECT_EQ(isMatch, expected.state);
        EXPECT_EQ(matchResult, expected.result);
    }
}
