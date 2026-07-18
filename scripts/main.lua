local vector = require("scripts/vector")
local entity = require("scripts/entity")


--entities api implementation
-- dog = entity {pos, vel, model} 
--dog.model = obj.Load(path)
--dog.pos = {0}
--dog.vel = {0}
--SpawnEntity(dog)  --> C function adds provided entity to table for rendering

function onLoad()
    dog = entity.new({0, 0, 0}, 2, 3)
    print(dog.position[1])
end

function onRun()
    print("running")
end

function onClose()
    print("closed")
end

onLoad()