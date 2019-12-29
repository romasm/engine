if not WorkingPlane then WorkingPlane = {} end

function WorkingPlane:Init ()
	self.planeEnt = VolumeWorld.coreWorld:CreateEntity ()
	if not self.planeEnt:IsNull () then
		VolumeWorld.coreWorld:SetEntityType (self.planeEnt, "Node")
		VolumeWorld.coreWorld.transform:AddComponent (self.planeEnt)
		VolumeWorld.coreWorld.visibility:AddComponent (self.planeEnt)

		if VolumeWorld.coreWorld.staticMesh:AddComponent (self.planeEnt) then
			VolumeWorld.coreWorld.staticMesh:SetMesh (self.planeEnt, PATH.EDITOR_MESHES .. "unit_plane" .. EXT.MESH)
			VolumeWorld.coreWorld.staticMesh:SetMaterial (self.planeEnt, 0, PATH.EDITOR_MESHES .. "unit_plane" .. EXT.MATERIAL)

			VolumeWorld.coreWorld.transform:SetScale_L3F (self.planeEnt, 2.5, 2.5, 2.5)
			VolumeWorld.coreWorld.transform:SetRotationPYR_L3F (self.planeEnt, 0.0, 0.0, math.pi * 0.5)
		end
	end
	
	local planeMat = VolumeWorld.coreWorld.staticMesh:GetMaterialObject (self.planeEnt, 0)
	planeMat:SetVector3 (VolumeWorld.volumeScale * Vector3 (0.5, 0.5, 0.5), "boxExtents", SHADERS.PS)
	
	self.planeOrigin = Vector3 (0.0, 0.0, 0.0)
	self.planeNormal = Vector3 (1.0, 0.0, 0.0)

	self.planeFadeEnable = 0
	self:SetPlaneFade (50.0)
	self:SetPlaneVisible (true)

	self.positioningType = 0
	self.attachDistance = 1.0
end

function WorkingPlane:Close ()
	self.planeEnt = nil
end

function WorkingPlane:SetPositioning (type)
	if self.positioningType == type then return end

	self.positioningType = type

	if self.positioningType == 0 then
		local tranasform = VolumeWorld.coreWorld.transform:GetTransform_W (self.planeEnt)
		VolumeWorld.coreWorld.transform:Detach (self.planeEnt)
		VolumeWorld.coreWorld.transform:SetTransform_L (self.planeEnt, tranasform)

		TransformControls:Activate ()
		TransformControls:UpdateTransform ( { Tools.transformEntity })
	else
		VolumeWorld.coreWorld.transform:Attach (self.planeEnt, EditorCamera.cameraEntity)
		VolumeWorld.coreWorld.transform:SetScale_L3F (self.planeEnt, 2.5, 2.5, 2.5)
		VolumeWorld.coreWorld.transform:SetRotationPYR_L3F (self.planeEnt, math.pi * 0.5, 0.0, 0.0)
		VolumeWorld.coreWorld.transform:SetPosition_L3F (self.planeEnt, 0, 0, self.attachDistance)

		TransformControls:Deactivate ()
	end
end

function WorkingPlane:SetAttachDist (dist)
	self.attachDistance = dist
	VolumeWorld.coreWorld.transform:SetPosition_L3F (self.planeEnt, 0, 0, self.attachDistance)
end

function WorkingPlane:SetToolsVisibility (visible)
	self.toolsVisibility = visible
	VolumeWorld.coreWorld:SetEntityEditorVisible (self.planeEnt, self.toolsVisibility and self.planeVisible)
end

function WorkingPlane:SetPlaneVisible (visible)
	self.planeVisible = visible
	VolumeWorld.coreWorld:SetEntityEditorVisible (self.planeEnt, self.toolsVisibility and self.planeVisible)
end

function WorkingPlane:SetPlaneFade (fade)
	self.planeFade = fade

	VolumeWorld.materialVolume:SetFloat (self.planeFade * self.planeFadeEnable, "cutPlaneFade", SHADERS.PS)
end

function WorkingPlane:SetPlaneFadeEnable (enable)
	if enable then
		self.planeFadeEnable = 1
	else
		self.planeFadeEnable = 0
	end

	VolumeWorld.materialVolume:SetFloat (self.planeFade * self.planeFadeEnable, "cutPlaneFade", SHADERS.PS)
end

function WorkingPlane:Tick (dt)
	if not self.planeEnt then return end

	self.planeOrigin = VolumeWorld.coreWorld.transform:GetPosition_W (self.planeEnt)
	self.planeNormal = VolumeWorld.coreWorld.transform:GetUpward_W (self.planeEnt)

	--transform to local
	self.planeOriginLocal = self.planeOrigin / VolumeWorld.volumeScale + Vector3 (0.5, 0.5, 0.5)

	VolumeWorld.materialVolume:SetVector3 (self.planeOriginLocal, "cutPlaneOriginL", SHADERS.PS)
	VolumeWorld.materialVolume:SetVector3 (self.planeNormal, "cutPlaneNormal", SHADERS.PS)
end