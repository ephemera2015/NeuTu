#ifndef PROTOCOLTASKCONFIG_H
#define PROTOCOLTASKCONFIG_H

#include <QString>

#include "neutube_def.h"

class ProtocolTaskConfig
{
public:
  ProtocolTaskConfig();

  QString getTaskType() const;
  neutube::EToDoAction getDefaultTodo() const;

  void setTaskType(const QString &type);
  void setDefaultTodo(neutube::EToDoAction action);

private:
  QString m_taskType;
  neutube::EToDoAction m_defaultTodo = neutube::TO_DO;
};

#endif // PROTOCOLTASKCONFIG_H
