#pragma once

#include "common/Singleton.hpp"
#include "common/UniqueAccess.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"

#include <QColor>
#include <QUrl>
#include <boost/optional.hpp>

#include <functional>
#include <memory>
#include <utility>

namespace chatterino {

struct HighlightResult {
    /**
     * @brief alert designates whether this highlight should trigger the taskbar to flash
     **/
    boost::optional<bool> alert;

    boost::optional<bool> playSound;

    // customSoundUrl may only be set if playSound has been set
    boost::optional<QUrl> customSoundUrl;

    boost::optional<std::shared_ptr<QColor>> color;

    bool operator==(const HighlightResult &other) const
    {
        if (this->alert != other.alert)
        {
            return false;
        }
        if (this->playSound != other.playSound)
        {
            return false;
        }
        if (this->customSoundUrl != other.customSoundUrl)
        {
            return false;
        }

        auto lhsColorSet = this->color.has_value();
        auto rhsColorSet = other.color.has_value();
        if (lhsColorSet != rhsColorSet)
        {
            return false;
        }

        if (lhsColorSet && rhsColorSet)
        {
            auto lhsColorPtr = *this->color;
            auto rhsColorPtr = *other.color;
            if (lhsColorPtr && rhsColorPtr)
            {
                if (*lhsColorPtr != *rhsColorPtr)
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool operator!=(const HighlightResult &other) const
    {
        return !(*this == other);
    }

    bool empty() const
    {
        return !this->alert && !this->playSound && !this->color;
    }

    bool full() const
    {
        return this->alert && this->playSound && this->customSoundUrl &&
               this->color;
    }
};

struct HighlightCheck {
    using Checker = std::function<boost::optional<HighlightResult>(
        const MessageParseArgs &args, const std::vector<Badge> &badges,
        const QString &senderName, const QString &originalMessage)>;
    Checker cb;
};

class HighlightController final : public Singleton
{
public:
    void initialize(Settings &settings, Paths &paths) override;

    /**
     * @brief Checks the given message parameters if it matches our internal checks, and returns a result
     **/
    [[nodiscard]] std::pair<bool, HighlightResult> check(
        const MessageParseArgs &args, const std::vector<Badge> &badges,
        const QString &senderName, const QString &originalMessage) const;

private:
    void rebuildChecks();

    void rebuildSubscriptionHighlights(std::vector<HighlightCheck> &checks);
    void rebuildWhisperHighlights(std::vector<HighlightCheck> &checks);
    void rebuildMessageHighlights(std::vector<HighlightCheck> &checks);
    void rebuildUserHighlights(std::vector<HighlightCheck> &checks);
    void rebuildBadgeHighlights(std::vector<HighlightCheck> &checks);

    pajlada::SettingListener rebuildListener_;
    UniqueAccess<std::vector<HighlightCheck>> checks_;

    pajlada::Signals::SignalHolder signalHolder_;
};

}  // namespace chatterino
