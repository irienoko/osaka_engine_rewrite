local vector = require("scripts/vector")
entity = {}

function entity.new(position, velocity, model)
    local self = { }
    self.position = position
    self.velocity = velocity
    self.model = model

    return self
end


return entity