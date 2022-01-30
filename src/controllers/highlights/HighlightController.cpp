#include "controllers/highlights/HighlightController.hpp"

#include "debug/Benchmark.hpp"

namespace chatterino {

void HighlightController::initialize(Settings &settings, Paths & /*paths*/)
{
    this->rebuildListener_.addSetting(settings.enableWhisperHighlight);
    this->rebuildListener_.addSetting(settings.enableWhisperHighlightSound);
    this->rebuildListener_.addSetting(settings.enableWhisperHighlightTaskbar);
    this->rebuildListener_.addSetting(settings.whisperHighlightSoundUrl);
    this->rebuildListener_.addSetting(settings.whisperHighlightColor);
    this->rebuildListener_.addSetting(settings.enableSelfHighlight);
    this->rebuildListener_.addSetting(settings.enableSubHighlight);
    this->rebuildListener_.addSetting(settings.enableSubHighlightSound);
    this->rebuildListener_.addSetting(settings.enableSubHighlightTaskbar);

    this->rebuildListener_.setCB([this] {
        this->rebuildChecks();
    });

    this->signalHolder_.managedConnect(
        getCSettings().highlightedBadges.delayedItemsChanged, [this] {
            this->rebuildChecks();
        });

    this->signalHolder_.managedConnect(
        getCSettings().highlightedUsers.delayedItemsChanged, [this] {
            this->rebuildChecks();
        });

    this->signalHolder_.managedConnect(
        getCSettings().highlightedMessages.delayedItemsChanged, [this] {
            this->rebuildChecks();
        });

    this->rebuildChecks();

    // TODO: Rebuild on user changed
}

void HighlightController::rebuildChecks()
{
    BenchmarkGuard benchmarkGuard("rebuildChecks");

    // Access checks for modification
    auto checks = this->checks_.access();
    checks->clear();

    // CURRENT ORDER:
    // Subscription -> Whisper -> User -> Message -> Badge

    this->rebuildSubscriptionHighlights(*checks);
    qDebug() << "Rebuilt subscription highlights: " << checks->size();

    this->rebuildWhisperHighlights(*checks);
    qDebug() << "Rebuilt whisper highlights: " << checks->size();

    this->rebuildUserHighlights(*checks);
    qDebug() << "Rebuilt user highlights: " << checks->size();

    this->rebuildMessageHighlights(*checks);
    qDebug() << "Rebuilt message highlights: " << checks->size();

    this->rebuildBadgeHighlights(*checks);
    qDebug() << "Rebuilt badge highlights: " << checks->size();
}

void HighlightController::rebuildSubscriptionHighlights(
    std::vector<HighlightCheck> &checks)
{
    auto *settings = getSettings();

    if (settings->enableSubHighlight)
    {
        auto highlightSound = settings->enableSubHighlightSound.getValue();
        auto highlightTaskbar = settings->enableSubHighlightTaskbar.getValue();
        auto highlightSoundUrl = settings->whisperHighlightSoundUrl.getValue();

        // The custom sub highlight color is handled in ColorProvider

        checks.emplace_back(HighlightCheck{
            [=](const auto &args, const auto &badges, const auto &senderName,
                const auto &originalMessage)
                -> boost::optional<HighlightResult> {
                (void)badges;           // unused
                (void)senderName;       // unused
                (void)originalMessage;  // unused
                if (!args.isSubscriptionMessage)
                {
                    return boost::none;
                }

                HighlightResult result;

                if (highlightTaskbar)
                {
                    result.alert = true;
                }

                if (highlightSound)
                {
                    result.playSound = true;
                }

                if (!highlightSoundUrl.isEmpty())
                {
                    result.customSoundUrl = QUrl(highlightSoundUrl);
                }

                result.color =
                    ColorProvider::instance().color(ColorType::Subscription);

                return result;
            }});
    }
}

void HighlightController::rebuildWhisperHighlights(
    std::vector<HighlightCheck> &checks)
{
    auto settings = getSettings();

    if (settings->enableWhisperHighlight)
    {
        auto highlightSound = settings->enableWhisperHighlightSound.getValue();
        auto highlightTaskbar =
            settings->enableWhisperHighlightTaskbar.getValue();
        auto highlightSoundUrl = settings->whisperHighlightSoundUrl.getValue();

        // The custom whisper highlight color is handled in ColorProvider

        checks.emplace_back(HighlightCheck{
            [=](const auto &args, const auto &badges, const auto &senderName,
                const auto &originalMessage)
                -> boost::optional<HighlightResult> {
                (void)badges;           // unused
                (void)senderName;       // unused
                (void)originalMessage;  // unused
                if (!args.isReceivedWhisper)
                {
                    return boost::none;
                }

                HighlightResult result;

                if (highlightTaskbar)
                {
                    result.alert = true;
                }

                if (highlightSound)
                {
                    result.playSound = true;
                }

                if (!highlightSoundUrl.isEmpty())
                {
                    result.customSoundUrl = QUrl(highlightSoundUrl);
                }

                result.color =
                    ColorProvider::instance().color(ColorType::Whisper);

                return result;
            }});
    }
}

void HighlightController::rebuildMessageHighlights(
    std::vector<HighlightCheck> &checks)
{
    auto currentUser = getBapp()->getAccounts()->twitch.getCurrent();
    QString currentUsername = currentUser->getUserName();
    auto settings = getSettings();

    if (settings->enableSelfHighlight && !currentUsername.isEmpty())
    {
        HighlightPhrase highlight(
            currentUsername, settings->showSelfHighlightInMentions,
            settings->enableSelfHighlightTaskbar,
            settings->enableSelfHighlightSound, false, false,
            settings->selfHighlightSoundUrl.getValue(),
            ColorProvider::instance().color(ColorType::SelfHighlight));

        checks.emplace_back(HighlightCheck{
            [highlight](const auto &args, const auto &badges,
                        const auto &senderName, const auto &originalMessage)
                -> boost::optional<HighlightResult> {
                (void)args;             // unused
                (void)badges;           // unused
                (void)originalMessage;  // unused

                if (!highlight.isMatch(senderName))
                {
                    return boost::none;
                }
                HighlightResult result;

                result.color = highlight.getColor();

                if (highlight.hasAlert())
                {
                    result.alert = true;
                }

                if (highlight.hasSound())
                {
                    result.playSound = true;

                    if (highlight.hasCustomSound())
                    {
                        result.customSoundUrl = highlight.getSoundUrl();
                    }
                }

                return result;
            }});
        // ADD SELF HIGHLIGHT
    }

    auto messageHighlights = getSettings()->highlightedMessages.readOnly();
    for (const auto &highlight : *messageHighlights)
    {
        checks.emplace_back(HighlightCheck{
            [highlight](const auto &args, const auto &badges,
                        const auto &senderName, const auto &originalMessage)
                -> boost::optional<HighlightResult> {
                (void)args;        // unused
                (void)badges;      // unused
                (void)senderName;  // unused

                if (!highlight.isMatch(originalMessage))
                {
                    return boost::none;
                }
                HighlightResult result;

                result.color = highlight.getColor();

                if (highlight.hasAlert())
                {
                    result.alert = true;
                }

                if (highlight.hasSound())
                {
                    result.playSound = true;

                    if (highlight.hasCustomSound())
                    {
                        result.customSoundUrl = highlight.getSoundUrl();
                    }
                }

                return result;
            }});
    }
}

void HighlightController::rebuildUserHighlights(
    std::vector<HighlightCheck> &checks)
{
    auto settings = getSettings();

    auto userHighlights = getCSettings().highlightedUsers.readOnly();

    for (const auto &highlight : *userHighlights)
    {
        checks.emplace_back(HighlightCheck{
            [highlight](const auto &args, const auto &badges,
                        const auto &senderName, const auto &originalMessage)
                -> boost::optional<HighlightResult> {
                (void)args;             // unused
                (void)badges;           // unused
                (void)originalMessage;  // unused

                if (!highlight.isMatch(senderName))
                {
                    return boost::none;
                }
                HighlightResult result;

                result.color = highlight.getColor();

                if (highlight.hasAlert())
                {
                    result.alert = true;
                }

                if (highlight.hasSound())
                {
                    result.playSound = true;

                    if (highlight.hasCustomSound())
                    {
                        result.customSoundUrl = highlight.getSoundUrl();
                    }
                }

                return result;
            }});
    }
}

void HighlightController::rebuildBadgeHighlights(
    std::vector<HighlightCheck> &checks)
{
    auto settings = getSettings();

    auto badgeHighlights = getCSettings().highlightedBadges.readOnly();

    for (const auto &highlight : *badgeHighlights)
    {
        checks.emplace_back(HighlightCheck{
            [highlight](const auto &args, const auto &badges,
                        const auto &senderName, const auto &originalMessage)
                -> boost::optional<HighlightResult> {
                (void)args;             // unused
                (void)senderName;       // unused
                (void)originalMessage;  // unused
                for (const Badge &badge : badges)
                {
                    if (highlight.isMatch(badge))
                    {
                        HighlightResult result;

                        result.color = highlight.getColor();

                        if (highlight.hasAlert())
                        {
                            result.alert = true;
                        }

                        if (highlight.hasSound())
                        {
                            result.playSound = true;

                            if (highlight.hasCustomSound())
                            {
                                result.customSoundUrl = highlight.getSoundUrl();
                            }
                        }

                        return result;
                    }
                }

                return boost::none;
            }});
    }
}

std::pair<bool, HighlightResult> HighlightController::check(
    const MessageParseArgs &args, const std::vector<Badge> &badges,
    const QString &senderName, const QString &originalMessage) const
{
    BenchmarkGuard("HighlightController::check");

    HighlightResult result;

    // Access for checking
    const auto checks = this->checks_.accessConst();

    bool highlighted = false;

    for (const auto &check : *checks)
    {
        if (auto checkResult =
                check.cb(args, badges, senderName, originalMessage);
            checkResult)
        {
            highlighted = true;

            if (checkResult->alert)
            {
                if (!result.alert)
                {
                    result.alert = checkResult->alert;
                }
            }

            if (checkResult->playSound)
            {
                if (!result.playSound)
                {
                    result.playSound = checkResult->playSound;
                }
            }

            if (checkResult->customSoundUrl)
            {
                if (!result.customSoundUrl)
                {
                    result.customSoundUrl = checkResult->customSoundUrl;
                }
            }

            if (checkResult->color)
            {
                if (!result.color)
                {
                    result.color = checkResult->color;
                }
            }

            if (result.full())
            {
                break;
            }
        }
    }

    return {highlighted, result};
}

}  // namespace chatterino
