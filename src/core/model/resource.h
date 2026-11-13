#ifndef FLOWN_CORE_MODEL_RESOURCE_H
#define FLOWN_CORE_MODEL_RESOURCE_H

#include <QString>
#include <QUuid>
#include <QUrl>

namespace flown::model {

// A resource is a reference to something used in a context, not its contents.
struct Resource {
    enum class Type {
        File,
        Directory,
        Url,
    };

    QUuid id;
    Type type;
    QUrl location;      // file:// URL for local files/directories; URL for web resources.
    QString displayName; // Optional; the UI may fall back to location.
};

} // namespace flown::model

#endif // FLOWN_CORE_MODEL_RESOURCE_H
