# Core 架构

## 职责与目录

`core/model` 定义领域模型：`Resource` 表示文件、目录或网页的引用，`Context` 表示命名情景及其包含的资源 ID。资源的实际内容不存入数据库。

`core/storage` 负责持久化。`ResourceStore` 使用 Qt SQL 的 SQLite 驱动，创建时接收数据库路径；调用 `open()` 打开连接并初始化表结构。每个 `ResourceStore` 实例使用独立的 Qt SQL 连接。

`core/test` 留作 Core 测试目录。

## 数据模型

资源和情景是多对多关系：一个情景可包含多个资源，同一资源也可被多个情景引用。

| 表 | 字段 | 作用 |
| --- | --- | --- |
| `resources` | `id`、`type`、`location`、`display_name` | 保存资源的 UUID、类型、URL 和显示名 |
| `contexts` | `id`、`name`、`description` | 保存情景的 UUID、名称和描述 |
| `context_resources` | `context_id`、`resource_id` | 保存情景与资源的关联，两个字段组成主键 |

`type` 取值对应 `Resource::Type`：`0` 为文件，`1` 为目录，`2` 为 URL。`location` 是完整编码的 URL 字符串；本地文件和目录使用 `file://` URL。关联表有外键约束，删除资源或情景时会级联删除关联记录。删除资源记录不会删除它指向的文件或网页。

## 存储接口

`saveResource()` 和 `saveContext()` 按 UUID 插入或更新各自的记录。`saveContext()` 不同步 `Context::resourceIds()`；使用 `addResourceToContext()` 和 `removeResourceFromContext()` 单独修改关联。关联的复合主键防止重复；重复添加同一关联视为成功。

`resource()`、`context()` 和 `resourcesInContext()` 负责读取。`context()` 会读取关联，并填充返回对象的 `resourceIds()`。写入或查询失败时，可通过 `lastError()` 获取 SQLite 错误；查询接口使用空结果表示未找到，调用方需要结合错误信息判断查询失败。
