#ifndef FLOWN_CORE_LAUNCH_RESOURCE_LAUNCHER_H
#define FLOWN_CORE_LAUNCH_RESOURCE_LAUNCHER_H

#include "core/model/resource.h"

#include <functional>

namespace flown::launch {

class ResourceLauncher {
public:
    enum class Result {
        OpenRequestAccepted,
        InvalidLocation,
        MissingDirectory,
        UnsupportedType,
        OpenFailed,
    };

    using OpenUrl = std::function<bool(const QUrl &)>;

    explicit ResourceLauncher(OpenUrl openUrl = {});
    Result launch(const model::Resource &resource) const;

private:
    OpenUrl openUrl_;
};

} // namespace flown::launch

#endif // FLOWN_CORE_LAUNCH_RESOURCE_LAUNCHER_H
