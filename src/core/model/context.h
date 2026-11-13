#ifndef FLOWN_CORE_MODEL_CONTEXT_H
#define FLOWN_CORE_MODEL_CONTEXT_H

#include <QString>
#include <QUuid>
#include <QVector>

namespace flown::model {

// A named scenario containing references to resources held elsewhere.
class Context {
public:
    Context(QUuid id, QString name, QString description = {});

    const QUuid &id() const;
    const QString &name() const;
    void setName(QString name);
    const QString &description() const;
    void setDescription(QString description);

    const QVector<QUuid> &resourceIds() const;
    bool addResource(const QUuid &resourceId);
    bool removeResource(const QUuid &resourceId);

private:
    QUuid id_;
    QString name_;
    QString description_;
    QVector<QUuid> resourceIds_;
};

} // namespace flown::model

#endif // FLOWN_CORE_MODEL_CONTEXT_H
