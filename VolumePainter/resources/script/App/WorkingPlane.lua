if not WorkingPlane then WorkingPlane = {} end

function WorkingPlane:Init (world)
	self.world = world

	self.planeEnt = self.world.coreWorld:CreateEntity ()
	if not self.planeEnt:IsNull () then
		self.world.coreWorld:SetEntityType (self.planeEnt, "Node")
		self.world.coreWorld.transform:AddComponent (self.planeEnt)
		self.world.coreWorld.visibility:AddComponent (self.planeEnt)

		if self.world.coreWorld.staticMesh:AddComponent (self.planeEnt) then
			self.world.coreWorld.staticMesh:SetMesh (self.planeEnt, PATH.EDITOR_MESHES .. "unit_plane" .. EXT.MESH)
			self.world.coreWorld.staticMesh:SetMaterial (self.planeEnt, 0, PATH.EDITOR_MESHES .. "unit_plane" .. EXT.MATERIAL)

			self.world.coreWorld.transform:SetScale_L3F (self.planeEnt, 5.0, 5.0, 5.0)
			self.world.coreWorld.transform:SetRotationPYR_L3F (self.planeEnt, 0.0, 0.0, math.pi * 0.5)
		end
	end

	self.planeOrigin = Vector3 (0.0, 0.0, 0.0)
	self.planeNormal = Vector3 (1.0, 0.0, 0.0)

	self.planeFadeEnable = 0
	self:SetPlaneFade (50.0)
	self:SetPlaneVisible (true)
end

function WorkingPlane:SetPlaneVisible (visible)
	self.planeVisible = visible
	self.world.coreWorld:SetEntityEditorVisible (self.planeEnt, self.planeVisible)
end

function WorkingPlane:SetPlaneFade (fade)
	self.planeFade = fade

	self.world.volumeMaterial:SetFloat (self.planeFade * self.planeFadeEnable, "cutPlaneFade", SHADERS.PS)
end

function WorkingPlane:SetPlaneFadeEnable (enable)
	if enable then
		self.planeFadeEnable = 1
	else
		self.planeFadeEnable = 0
	end

	self.world.volumeMaterial:SetFloat (self.planeFade * self.planeFadeEnable, "cutPlaneFade", SHADERS.PS)
end

function WorkingPlane:Tick (dt)
	if not self.planeEnt then return end

	self.planeOrigin = self.world.coreWorld.transform:GetPosition_W (self.planeEnt)
	self.planeNormal = self.world.coreWorld.transform:GetUpward_W (self.planeEnt)

	--transform to local
	self.planeOriginLocal = self.planeOrigin / self.world.volumeScale + Vector3 (0.5, 0.5, 0.5)

	self.world.volumeMaterial:SetVector3 (self.planeOriginLocal, "cutPlaneOriginL", SHADERS.PS)
	self.world.volumeMaterial:SetVector3 (self.planeNormal, "cutPlaneNormal", SHADERS.PS)
end