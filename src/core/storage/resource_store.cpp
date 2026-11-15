#include "resource_store.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace flown::storage {
namespace {
QString key(const QUuid &id) { return id.toString(QUuid::WithoutBraces); }

std::optional<model::Resource> readResource(const QSqlQuery &query)
{
    const auto id = QUuid(query.value(0).toString());
    const int type = query.value(1).toInt();
    const QUrl location(query.value(2).toString());
    if (id.isNull() || type < 0 || type > 2 || !location.isValid()) return std::nullopt;
    return model::Resource{id, static_cast<model::Resource::Type>(type), location,
                           query.value(3).toString()};
}
}

ResourceStore::ResourceStore(const QString &databasePath)
    : connectionName_(key(QUuid::createUuid()))
{
    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    db_.setDatabaseName(databasePath);
}

ResourceStore::~ResourceStore()
{
    db_.close();
    db_ = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName_);
}

QString ResourceStore::lastError() const { return error_; }
bool ResourceStore::fail(const QString &message) { error_ = message; return false; }

bool ResourceStore::execute(const QString &sql)
{
    QSqlQuery query(db_);
    return query.exec(sql) || fail(query.lastError().text());
}

bool ResourceStore::open()
{
    if (!db_.open()) return fail(db_.lastError().text());
    if (!execute(QStringLiteral("PRAGMA foreign_keys = ON"))) return false;
    if (!execute(QStringLiteral("CREATE TABLE IF NOT EXISTS resources ("
        "id TEXT PRIMARY KEY, type INTEGER NOT NULL CHECK (type BETWEEN 0 AND 2), "
        "location TEXT NOT NULL, display_name TEXT NOT NULL DEFAULT '')"))) return false;
    if (!execute(QStringLiteral("CREATE TABLE IF NOT EXISTS contexts ("
        "id TEXT PRIMARY KEY, name TEXT NOT NULL, description TEXT NOT NULL DEFAULT '')"))) return false;
    if (!execute(QStringLiteral("CREATE TABLE IF NOT EXISTS context_resources ("
        "context_id TEXT NOT NULL REFERENCES contexts(id) ON DELETE CASCADE, "
        "resource_id TEXT NOT NULL REFERENCES resources(id) ON DELETE CASCADE, "
        "PRIMARY KEY (context_id, resource_id))"))) return false;
    return execute(QStringLiteral("CREATE INDEX IF NOT EXISTS context_resources_resource_id "
                                  "ON context_resources(resource_id)"));
}

bool ResourceStore::saveResource(const model::Resource &resource)
{
    if (resource.id.isNull() || !resource.location.isValid() || resource.location.isEmpty())
        return fail(QStringLiteral("Invalid resource id or location"));
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("INSERT INTO resources (id,type,location,display_name) VALUES (?,?,?,?) "
        "ON CONFLICT(id) DO UPDATE SET type=excluded.type, location=excluded.location, "
        "display_name=excluded.display_name"));
    query.addBindValue(key(resource.id));
    query.addBindValue(static_cast<int>(resource.type));
    query.addBindValue(resource.location.toString(QUrl::FullyEncoded));
    query.addBindValue(resource.displayName);
    return query.exec() || fail(query.lastError().text());
}

bool ResourceStore::saveContext(const model::Context &context)
{
    if (context.id().isNull()) return fail(QStringLiteral("Invalid context id"));
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("INSERT INTO contexts (id,name,description) VALUES (?,?,?) "
        "ON CONFLICT(id) DO UPDATE SET name=excluded.name, description=excluded.description"));
    query.addBindValue(key(context.id()));
    query.addBindValue(context.name());
    query.addBindValue(context.description().isNull() ? QStringLiteral("") : context.description());
    return query.exec() || fail(query.lastError().text());
}

bool ResourceStore::deleteResource(const QUuid &id)
{
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("DELETE FROM resources WHERE id=?"));
    query.addBindValue(key(id));
    return query.exec() || fail(query.lastError().text());
}

bool ResourceStore::deleteContext(const QUuid &id)
{
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("DELETE FROM contexts WHERE id=?"));
    query.addBindValue(key(id));
    return query.exec() || fail(query.lastError().text());
}

bool ResourceStore::addResourceToContext(const QUuid &contextId, const QUuid &resourceId)
{
    if (contextId.isNull() || resourceId.isNull()) return fail(QStringLiteral("Invalid id"));
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("INSERT OR IGNORE INTO context_resources VALUES (?,?)"));
    query.addBindValue(key(contextId));
    query.addBindValue(key(resourceId));
    return query.exec() || fail(query.lastError().text());
}

bool ResourceStore::removeResourceFromContext(const QUuid &contextId, const QUuid &resourceId)
{
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("DELETE FROM context_resources WHERE context_id=? AND resource_id=?"));
    query.addBindValue(key(contextId));
    query.addBindValue(key(resourceId));
    return query.exec() || fail(query.lastError().text());
}

std::optional<model::Resource> ResourceStore::resource(const QUuid &id)
{
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("SELECT id,type,location,display_name FROM resources WHERE id=?"));
    query.addBindValue(key(id));
    if (!query.exec()) { fail(query.lastError().text()); return std::nullopt; }
    return query.next() ? readResource(query) : std::nullopt;
}

std::optional<model::Context> ResourceStore::context(const QUuid &id)
{
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("SELECT id,name,description FROM contexts WHERE id=?"));
    query.addBindValue(key(id));
    if (!query.exec()) { fail(query.lastError().text()); return std::nullopt; }
    if (!query.next()) return std::nullopt;
    model::Context result(QUuid(query.value(0).toString()), query.value(1).toString(),
                          query.value(2).toString());
    QSqlQuery links(db_);
    links.prepare(QStringLiteral("SELECT resource_id FROM context_resources WHERE context_id=? ORDER BY rowid"));
    links.addBindValue(key(id));
    if (!links.exec()) { fail(links.lastError().text()); return std::nullopt; }
    while (links.next()) result.addResource(QUuid(links.value(0).toString()));
    return result;
}

QVector<model::Resource> ResourceStore::resourcesInContext(const QUuid &contextId)
{
    QVector<model::Resource> result;
    QSqlQuery query(db_);
    query.prepare(QStringLiteral("SELECT r.id,r.type,r.location,r.display_name FROM resources r "
        "JOIN context_resources cr ON cr.resource_id=r.id WHERE cr.context_id=? ORDER BY cr.rowid"));
    query.addBindValue(key(contextId));
    if (!query.exec()) { fail(query.lastError().text()); return result; }
    while (query.next()) {
        if (auto resource = readResource(query)) result.append(*resource);
    }
    return result;
}

} // namespace flown::storage
