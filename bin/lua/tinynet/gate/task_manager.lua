local class = require("tinynet/core/class")
local TaskManager = class("TaskManager")

function TaskManager:constructor()
    self.queues = {}
end

function TaskManager:push_task(queue_id, task)
    local queue = self.queues[queue_id]
    if not queue then
        local TaskQueue = require("tinynet/gate/task_queue")
        queue = TaskQueue.new(queue_id)
        self.queues[queue_id] = queue
    end
    queue:push(task)
end

function TaskManager:close_queue(queue_id)
    local queue = self.queues[queue_id]
    if not queue then
        return
    end
    queue:close()
    self.queues[queue_id] = nil
end

return TaskManager.new()
