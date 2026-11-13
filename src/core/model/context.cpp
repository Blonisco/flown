#include "context.h"

#include <utility>

namespace flown::model {

Context::Context(QUuid id, QString name, QString description)
    : id_(std::move(id))
    , name_(std::move(name))
    , description_(std::move(description))
{
}

const QUuid &Context::id() const { return id_; }
const QString &Context::name() const { return name_; }
void Context::setName(QString name) { name_ = std::move(name); }
const QString &Context::description() const { return description_; }
void Context::setDescription(QString description) { description_ = std::move(description); }

const QVector<QUuid> &Context::resourceIds() const { return resourceIds_; }

bool Context::addResource(const QUuid &resourceId)
{
    if (resourceId.isNull() || resourceIds_.contains(resourceId)) {
        return false;
    }
    resourceIds_.append(resourceId);
    return true;
}

bool Context::removeResource(const QUuid &resourceId)
{
    return resourceIds_.removeAll(resourceId) != 0;
}

} // namespace flown::model
