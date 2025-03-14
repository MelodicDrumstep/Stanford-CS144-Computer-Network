#pragma once

template <typename Func>
class TaskExecutionScopeGuard {
public:
  TaskExecutionScopeGuard(Func && func) : task_(std::forward<Func>(func)) {}

  ~TaskExecutionScopeGuard() {
    task_();
  }

private:
  Func task_;
};