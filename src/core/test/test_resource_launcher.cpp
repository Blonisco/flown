#include "core/launch/resource_launcher.h"

#include <QTemporaryDir>
#include <QtTest>

using flown::launch::ResourceLauncher;
using flown::model::Resource;

class ResourceLauncherTest : public QObject {
    Q_OBJECT
private slots:
    void opensWebUrl();
    void opensDirectory();
    void rejectsInvalidResources();
    void reportsOpenFailure();
};

void ResourceLauncherTest::opensWebUrl()
{
    QUrl opened;
    ResourceLauncher launcher([&](const QUrl &url) { opened = url; return true; });
    const QUrl web(QStringLiteral("https://example.com/a%20b?q=hello%20world"));
    QCOMPARE(launcher.launch({QUuid::createUuid(), Resource::Type::Url, web, {}}),
             ResourceLauncher::Result::Opened);
    QCOMPARE(opened, web);
}

void ResourceLauncherTest::opensDirectory()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QUrl opened;
    ResourceLauncher launcher([&](const QUrl &url) { opened = url; return true; });
    const QUrl folder = QUrl::fromLocalFile(dir.path());
    QCOMPARE(launcher.launch({QUuid::createUuid(), Resource::Type::Directory, folder, {}}),
             ResourceLauncher::Result::Opened);
    QCOMPARE(opened, folder);
}

void ResourceLauncherTest::rejectsInvalidResources()
{
    int calls = 0;
    ResourceLauncher launcher([&](const QUrl &) { ++calls; return true; });
    QTemporaryDir dir;
    QCOMPARE(launcher.launch({{}, Resource::Type::Url, QUrl("ftp://example.com"), {}}),
             ResourceLauncher::Result::InvalidLocation);
    QCOMPARE(launcher.launch({{}, Resource::Type::Url, QUrl("https:///missing-host"), {}}),
             ResourceLauncher::Result::InvalidLocation);
    QCOMPARE(launcher.launch({{}, Resource::Type::Directory, QUrl("https://example.com"), {}}),
             ResourceLauncher::Result::InvalidLocation);
    QCOMPARE(launcher.launch({{}, Resource::Type::Directory,
                              QUrl::fromLocalFile(dir.filePath("missing")), {}}),
             ResourceLauncher::Result::MissingDirectory);
    QCOMPARE(launcher.launch({{}, Resource::Type::File,
                              QUrl::fromLocalFile(dir.path()), {}}),
             ResourceLauncher::Result::UnsupportedType);
    QCOMPARE(calls, 0);
}

void ResourceLauncherTest::reportsOpenFailure()
{
    ResourceLauncher launcher([](const QUrl &) { return false; });
    QCOMPARE(launcher.launch({{}, Resource::Type::Url, QUrl("https://example.com"), {}}),
             ResourceLauncher::Result::OpenFailed);
}

QTEST_GUILESS_MAIN(ResourceLauncherTest)
#include "test_resource_launcher.moc"
