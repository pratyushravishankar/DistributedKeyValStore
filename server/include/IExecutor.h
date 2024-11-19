#pragma once 

#include <functional>
#include <chrono>


struct TaskMeta {
    std::string serverName; // TODO  modify to ServerMeta
    std::string taskName;
};

std::ostream& operator<<(std::ostream& os, const TaskMeta& tm) {
    return os << tm.serverName << ": " <<  tm.taskName;
}



using Task = std::pair<TaskMeta, std::function<void()>>;

class IExecutor {
public:
    virtual void schedule(Task task, std::chrono::milliseconds delay) = 0;
    virtual void execute(Task task) = 0;
    virtual std::chrono::milliseconds currentTime() const =0;
    virtual ~IExecutor() = default;
};
