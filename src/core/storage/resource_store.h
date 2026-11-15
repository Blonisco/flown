#ifndef FLOWN_CORE_STORAGE_RESOURCE_STORE_H
#define FLOWN_CORE_STORAGE_RESOURCE_STORE_H

#include "core/model/context.h"
#include "core/model/resource.h"

#include <QSqlDatabase>
#include <QString>
#include <QVector>
#include <optional>

namespace flown::storage {

// SQLite-backed catalog. Each instance owns a separate Qt SQL connection.
class ResourceStore {
public:
    explicit ResourceStore(const QString &databasePath);
    ~ResourceStore();
    ResourceStore(const ResourceStore &) = delete;
    ResourceStore &operator=(const ResourceStore &) = delete;

    bool open();
    bool isOpen() const;
    QString lastError() const;

    bool saveResource(const model::Resource &resource);
    bool saveContext(const model::Context &context);
    bool deleteResource(const QUuid &id);
    bool deleteContext(const QUuid &id);
    bool addResourceToContext(const QUuid &contextId, const QUuid &resourceId);
    bool removeResourceFromContext(const QUuid &contextId, const QUuid &resourceId);

    std::optional<model::Resource> resource(const QUuid &id);
    std::optional<model::Context> context(const QUuid &id);
    QVector<model::Resource> resourcesInContext(const QUuid &contextId);

private:
    bool execute(const QString &sql);
    bool fail(const QString &message);
    QSqlDatabase db_;
    QString connectionName_;
    QString error_;
    bool initialized_ = false;
};

} // namespace flown::storage
#endif
