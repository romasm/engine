if not Brush then Brush = {} end

function Brush:Init()
	self.brushSize = 30.0
	self.brushHardness = 0.0
	self.brushColor = Vector4(1, 1, 1, 1)	

	self.minCorner = Vector3(VolumeWorld.volumeResolutionX, VolumeWorld.volumeResolutionY, VolumeWorld.volumeResolutionZ)
	self.maxCorner = Vector3(0, 0, 0)
end

function Brush:Close()

end

function Brush:SetToolsVisibility (visible)

end

function Brush:SetBrushSize (size)
	self.brushSize = size
end

function Brush:SetBrushHardness (hardness)
	self.brushHardness = hardness
end

function Brush:SetBrushOpacity (opacity)
	self.brushColor.w = opacity
end

function Brush:SetBrushColor (color)
	self.brushColor.x = color.x
	self.brushColor.y = color.y
	self.brushColor.z = color.z
end

function Brush:StopDraw()
	VolumeWorld.volumeCore:PushDifference(self.minCorner, self.maxCorner)

	self.prevBrushPos = nil
	self.minCorner = Vector3(VolumeWorld.volumeResolutionX, VolumeWorld.volumeResolutionY, VolumeWorld.volumeResolutionZ)
	self.maxCorner = Vector3(0, 0, 0)
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
	local maxVolumeRes = Vector3(VolumeWorld.maxVolumeRes, VolumeWorld.maxVolumeRes, VolumeWorld.maxVolumeRes)

	local volumePosition = (position + VolumeWorld.volumeScale * Vector3(0.5, 0.5, 0.5)) * maxVolumeRes
	local volumeRadius = radius * 0.5
		
	local volumeRadiusVector = Vector3(volumeRadius, volumeRadius, volumeRadius)
	self.minCorner = Vector3.Min(self.minCorner, volumePosition - volumeRadiusVector)
	self.maxCorner = Vector3.Max(self.maxCorner, volumePosition + volumeRadiusVector)

	local prevVolumePosition = volumePosition
	if self.prevBrushPos then
		prevVolumePosition = self.prevBrushPos
	end

	VolumeWorld.volumeCore:DrawBrush (prevVolumePosition, volumePosition, volumeRadius, self.brushColor, self.brushHardness)
	
	self.prevBrushPos = volumePosition
end