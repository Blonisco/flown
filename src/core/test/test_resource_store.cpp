#include "core/storage/resource_store.h"

#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

using flown::model::Context;
using flown::model::Resource;
using flown::storage::ResourceStore;

class ResourceStoreTest : public QObject {
    Q_OBJECT
private slots:
    void resourceRoundTripAndUpdate();
    void contextAndAssociations();
    void cascadeAndForeignKeys();
    void persistsAcrossInstances();
    void invalidInputAndMissingRecords();
    void defaultEmptyFields();
};

void ResourceStoreTest::resourceRoundTripAndUpdate()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ResourceStore store(dir.filePath("catalog.sqlite"));
    QVERIFY2(store.open(), qPrintable(store.lastError()));
    const QUuid id = QUuid::createUuid();
    const QUrl file = QUrl::fromLocalFile(dir.filePath("a file.txt"));
    QVERIFY2(store.saveResource({id, Resource::Type::File, file, "Original"}), qPrintable(store.lastError()));
    auto saved = store.resource(id);
    QVERIFY(saved.has_value());
    QCOMPARE(saved->id, id);
    QCOMPARE(saved->type, Resource::Type::File);
    QCOMPARE(saved->location, file);
    QCOMPARE(saved->displayName, QString("Original"));

    const QUrl web("https://example.com/a%20b?q=hello%20world");
    QVERIFY2(store.saveResource({id, Resource::Type::Url, web, "Updated"}), qPrintable(store.lastError()));
    saved = store.resource(id);
    QVERIFY(saved.has_value());
    QCOMPARE(saved->type, Resource::Type::Url);
    QCOMPARE(saved->location.toString(QUrl::FullyEncoded), web.toString(QUrl::FullyEncoded));
    QCOMPARE(saved->displayName, QString("Updated"));

    const QUuid directoryId = QUuid::createUuid();
    QVERIFY2(store.saveResource({directoryId, Resource::Type::Directory,
                                 QUrl::fromLocalFile(dir.path()), "Directory"}), qPrintable(store.lastError()));
    QCOMPARE(store.resource(directoryId)->type, Resource::Type::Directory);
}

void ResourceStoreTest::contextAndAssociations()
{
    QTemporaryDir dir;
    ResourceStore store(dir.filePath("catalog.sqlite"));
    QVERIFY2(store.open(), qPrintable(store.lastError()));
    const QUuid contextId = QUuid::createUuid();
    const QUuid first = QUuid::createUuid();
    const QUuid second = QUuid::createUuid();
    Context context(contextId, "Before", "Description");
    context.addResource(first);
    QVERIFY2(store.saveContext(context), qPrintable(store.lastError()));
    QVERIFY(store.context(contextId)->resourceIds().isEmpty()); // saveContext does not sync links.
    QVERIFY2(store.saveResource({first, Resource::Type::File, QUrl::fromLocalFile("/tmp/first"), "First"}), qPrintable(store.lastError()));
    QVERIFY2(store.saveResource({second, Resource::Type::Url, QUrl("https://example.com/"), "Second"}), qPrintable(store.lastError()));
    QVERIFY2(store.addResourceToContext(contextId, first), qPrintable(store.lastError()));
    QVERIFY2(store.addResourceToContext(contextId, first), qPrintable(store.lastError()));
    QVERIFY2(store.addResourceToContext(contextId, second), qPrintable(store.lastError()));
    auto loaded = store.context(contextId);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->resourceIds(), (QVector<QUuid>{first, second}));
    QCOMPARE(store.resourcesInContext(contextId).size(), 2);
    QCOMPARE(store.resourcesInContext(contextId).at(0).id, first);
    context.setName("After");
    context.setDescription("Changed");
    QVERIFY2(store.saveContext(context), qPrintable(store.lastError()));
    loaded = store.context(contextId);
    QCOMPARE(loaded->name(), QString("After"));
    QCOMPARE(loaded->description(), QString("Changed"));
    QCOMPARE(loaded->resourceIds().size(), 2);
    QVERIFY2(store.removeResourceFromContext(contextId, first), qPrintable(store.lastError()));
    QVERIFY2(store.removeResourceFromContext(contextId, first), qPrintable(store.lastError()));
    QCOMPARE(store.context(contextId)->resourceIds(), (QVector<QUuid>{second}));
}

void ResourceStoreTest::cascadeAndForeignKeys()
{
    QTemporaryDir dir;
    ResourceStore store(dir.filePath("catalog.sqlite"));
    QVERIFY2(store.open(), qPrintable(store.lastError()));
    const QUuid c1 = QUuid::createUuid(), c2 = QUuid::createUuid(), r1 = QUuid::createUuid(), r2 = QUuid::createUuid();
    QVERIFY(store.saveContext(Context(c1, "One", "Description")));
    QVERIFY(store.saveContext(Context(c2, "Two", "Description")));
    QVERIFY(store.saveResource({r1, Resource::Type::File, QUrl::fromLocalFile(dir.filePath("content.txt")), "File"}));
    QVERIFY(store.saveResource({r2, Resource::Type::Url, QUrl("https://example.com/"), "Web"}));
    QVERIFY(store.addResourceToContext(c1, r1));
    QVERIFY(store.addResourceToContext(c2, r1));
    QVERIFY(store.addResourceToContext(c2, r2));
    QVERIFY(!store.addResourceToContext(c1, QUuid::createUuid()));
    QVERIFY(!store.lastError().isEmpty());
    QVERIFY(store.deleteResource(r1));
    QVERIFY(!store.resource(r1).has_value());
    QVERIFY(store.context(c1)->resourceIds().isEmpty());
    QCOMPARE(store.context(c2)->resourceIds(), (QVector<QUuid>{r2}));
    QVERIFY(store.deleteContext(c2));
    QVERIFY(!store.context(c2).has_value());
    QVERIFY(store.resource(r2).has_value());
    QVERIFY(store.resourcesInContext(c2).isEmpty());
}

void ResourceStoreTest::persistsAcrossInstances()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("catalog.sqlite");
    const QUuid contextId = QUuid::createUuid(), resourceId = QUuid::createUuid();
    {
        ResourceStore first(path);
        QVERIFY2(first.open(), qPrintable(first.lastError()));
        QVERIFY(first.saveContext(Context(contextId, "Persistent", "Description")));
        QVERIFY(first.saveResource({resourceId, Resource::Type::Url, QUrl("https://example.com/"), "Web"}));
        QVERIFY(first.addResourceToContext(contextId, resourceId));
        ResourceStore second(path);
        QVERIFY2(second.open(), qPrintable(second.lastError()));
        QCOMPARE(second.context(contextId)->resourceIds(), (QVector<QUuid>{resourceId}));
    }
    ResourceStore reopened(path);
    QVERIFY2(reopened.open(), qPrintable(reopened.lastError()));
    QCOMPARE(reopened.resourcesInContext(contextId).size(), 1);
}

void ResourceStoreTest::invalidInputAndMissingRecords()
{
    QTemporaryDir dir;
    ResourceStore store(dir.filePath("catalog.sqlite"));
    QVERIFY2(store.open(), qPrintable(store.lastError()));
    QVERIFY(!store.saveResource({QUuid(), Resource::Type::File, QUrl::fromLocalFile("/tmp/x"), ""}));
    QVERIFY(!store.lastError().isEmpty());
    QVERIFY(!store.saveResource({QUuid::createUuid(), Resource::Type::File, QUrl(), ""}));
    QVERIFY(!store.saveContext(Context(QUuid(), "Invalid")));
    QVERIFY(!store.addResourceToContext(QUuid(), QUuid::createUuid()));
    QVERIFY(!store.resource(QUuid::createUuid()).has_value());
    QVERIFY(!store.context(QUuid::createUuid()).has_value());
    QVERIFY(store.resourcesInContext(QUuid::createUuid()).isEmpty());
}

void ResourceStoreTest::defaultEmptyFields()
{
    QTemporaryDir dir;
    ResourceStore store(dir.filePath("catalog.sqlite"));
    QVERIFY2(store.open(), qPrintable(store.lastError()));
    const QUuid id = QUuid::createUuid();
    QVERIFY2(store.saveContext(Context(id, "Default description")), qPrintable(store.lastError()));
    QCOMPARE(store.context(id)->description(), QStringLiteral(""));
    QVERIFY2(store.saveContext(Context(id, "Updated")), qPrintable(store.lastError()));
    QCOMPARE(store.context(id)->description(), QStringLiteral(""));
}

QTEST_GUILESS_MAIN(ResourceStoreTest)
#include "test_resource_store.moc"
