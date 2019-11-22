if not Brush then Brush = {} end

function Brush:Init(world)
	self.world = world

	self.brushSize = 10.0
	self.brushColor = Vector4(1, 1, 1, 1)	
end

function Brush:SetBrushSize (size)
	self.brushSize = size
end

function Brush:SetBrushOpacity (opacity)
	self.brushColor.w = opacity
end

function Brush:SetBrushColor (color)
	self.brushColor.x = color.x
	self.brushColor.y = color.y
	self.brushColor.z = color.z
end

function Brush:DrawBrushFromViewport(pos, ray)
	local denom = WorkingPlane.planeNormal:Dot (ray)
	if math.abs(denom)> 0.00001 then
		local rayToPlane = WorkingPlane.planeOrigin - pos
		local t = rayToPlane:Dot (WorkingPlane.planeNormal) / denom

		local brushPos = pos + ray * Vector3(t, t, t)
		
		self:DrawBrushSpherical (brushPos, self.brushSize)
	end
end

function Brush:DrawBrushSpherical (position, radius)
	local maxVolumeRes = Vector3(self.world.maxVolumeRes, self.world.maxVolumeRes, self.world.maxVolumeRes)

	local volumePosition = (position + self.world.volumeScale * Vector3(0.5, 0.5, 0.5)) * maxVolumeRes
	local volumeRadius = radius * 0.5

	self.world.volumeCore:DrawBrush (volumePosition, volumeRadius, self.brushColor)
end