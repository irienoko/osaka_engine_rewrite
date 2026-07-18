local sqrt = math.sqrt
local max = math.max
local min = math.min
local acos = math.acos
local cos = math.cos
local sin = math.sin

vector = {}
-- Create a new vector
function vector.new(...)
	return {...}
end

-- Add two vectors
function vector.add(v1, v2)
	assert(#v1 == #v2, "Vectors must be of the same length.")
	local result = {}
	for i, value in ipairs(v1) do
		result[i] = value + v2[i]
	end
	return result
end

-- Subtract two vectors
function vector.subtract(v1, v2)
	assert(#v1 == #v2, "Vectors must be of the same length.")
	local result = {}
	for i = 1, #v1 do
		result[i] = v1[i] - v2[i]
	end
	return result
end

-- Dot product of two vectors
function vector.dot(v1, v2)
	assert(#v1 == #v2, "Vectors must be of the same length.")
	local result = 0
	for i = 1, #v1 do
		result = result + v1[i]*v2[i]
	end
	return result
end

-- Magnitude of a vector
function vector.magnitude(v)
	local result = 0
	for i = 1, #v do
		result = result + v[i]*v[i]  -- Use multiplication instead of ^2 for efficiency
	end
	return sqrt(result)
end

-- Scalar multiplication
function vector.scalarMultiply(v, scalar)
	local result = {}
	for i = 1, #v do
		result[i] = v[i] * scalar
	end
	return result
end

-- Vector normalization
function vector.normalize(v)
	local mag = vector.magnitude(v)
	if mag == 0 then return v end
	return vector.scalarMultiply(v, 1/mag)
end

-- Cross product
function vector.cross(v1, v2)
	assert(#v1 == 3 and #v2 == 3, "Cross product is only defined for 3D vectors.")
	local result = {
		v1[2]*v2[3] - v1[3]*v2[2],
		v1[3]*v2[1] - v1[1]*v2[3],
		v1[1]*v2[2] - v1[2]*v2[1]
	}
	return result
end

-- Distance between two vectors
function vector.distance(v1, v2)
	assert(#v1 == #v2, "Vectors must be of the same length.")
	return vector.magnitude(vector.subtract(v1, v2))
end

-- Angle between two vectors
function vector.angle(v1, v2)
	assert(#v1 == #v2, "Vectors must be of the same length.")
	return acos(vector.dot(v1, v2) / (vector.magnitude(v1) * vector.magnitude(v2)))
end

-- Projection of v1 onto v2
function vector.projection(v1, v2)
	assert(#v1 == #v2, "Vectors must be of the same length.")
	local v2_normalized = vector.normalize(v2)
	local projection_length = vector.dot(v1, v2_normalized)
	return vector.scalarMultiply(v2_normalized, projection_length)
end

-- Vector rotation
function vector.rotate(v, angle)
	assert(#v == 2, "Rotation is only defined for 2D vectors.")
	local x = v[1]*cos(angle) - v[2]*sin(angle)
	local y = v[1]*sin(angle) + v[2]*cos(angle)
	return {x, y}
end

return vector