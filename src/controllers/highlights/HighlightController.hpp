#pragma once

#include "common/Singleton.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

class HighlightController final : public Singleton
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;
};

}  // namespace chatterino
