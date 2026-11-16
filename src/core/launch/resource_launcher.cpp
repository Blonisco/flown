#include "resource_launcher.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>

namespace flown::launch {

ResourceLauncher::ResourceLauncher(OpenUrl openUrl)
    : openUrl_(openUrl ? std::move(openUrl) : OpenUrl(QDesktopServices::openUrl))
{
}

ResourceLauncher::Result ResourceLauncher::launch(const model::Resource &resource) const
{
    const QUrl &url = resource.location;
    if (!url.isValid() || url.isEmpty()) return Result::InvalidLocation;

    switch (resource.type) {
    case model::Resource::Type::Url:
        if ((url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))
            || url.host().isEmpty()) return Result::InvalidLocation;
        break;
    case model::Resource::Type::Directory:
        if (!url.isLocalFile()) return Result::InvalidLocation;
        if (!QFileInfo(url.toLocalFile()).isDir()) return Result::MissingDirectory;
        break;
    case model::Resource::Type::File:
        return Result::UnsupportedType;
    default:
        return Result::UnsupportedType;
    }

    return openUrl_(url) ? Result::Opened : Result::OpenFailed;
}

} // namespace flown::launch
