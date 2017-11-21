#ifndef TASKPROTOCOLTASK_H
#define TASKPROTOCOLTASK_H

#include <QObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

class TaskProtocolTask: public QObject
{
    Q_OBJECT

public:
    TaskProtocolTask();
    virtual ~TaskProtocolTask() {}

    bool completed() const;
    void setCompleted(bool completed);
    QSet<uint64_t> visibleBodies();
    QSet<uint64_t> selectedBodies();

    virtual void beforeNext();
    virtual void beforePrev();

    bool loadJson(QJsonObject json);
    QJsonObject toJson();

    void addTag(QString tag);
    void removeTag(QString tag);
    bool hasTag(QString tag);
    QStringList getTags();
    void clearTags();

    virtual QString tasktype() = 0;
    virtual QString actionString() = 0;
    virtual QString targetString() = 0;    
    virtual QWidget * getTaskWidget();

protected:
    static const QString KEY_COMPLETED;
    static const QString KEY_TAGS;
    static const QString KEY_VISIBLE;
    static const QString KEY_SELECTED;

    bool m_completed;
    QSet<uint64_t> m_visibleBodies;
    QSet<uint64_t> m_selectedBodies;
    QSet<QString> m_tags;

    QString objectToString(QJsonObject json);

private:
    bool loadStandard(QJsonObject json);
    virtual bool loadSpecific(QJsonObject json) = 0;
    virtual QJsonObject addToJson(QJsonObject json) = 0;
    virtual void onCompleted();
};

#endif // TASKPROTOCOLTASK_H
