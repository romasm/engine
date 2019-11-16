if not VolumeWorld then VolumeWorld = class () end

function VolumeWorld:init(path)
	if path == nil then
		self.coreWorld = GetWorldMgr():CreateWorld()
		self.path = ""
		self.unsave = true
			
		self.volumeResolutionX = 512
		self.volumeResolutionY = 512
		self.volumeResolutionZ = 512

		self.maxVolumeRes = math.max(math.max(self.volumeResolutionX, self.volumeResolutionY), self.volumeResolutionZ)		
		self.volumeScale = Vector3(self.volumeResolutionX / self.maxVolumeRes, self.volumeResolutionY / self.maxVolumeRes, self.volumeResolutionZ / self.maxVolumeRes)
		self.planeFade = 20.0

		self.volumeCore = VolumePainter()
		if not self.volumeCore:Init(self.volumeResolutionX, self.volumeResolutionY, self.volumeResolutionZ) then
			return false
		end

		self:InitVolumeWorld()
		self:CreateVolumeRenderer()
		
		-- test
		--self.volumeCore:ImportTexture (PATH.SYS_TEXTURES .. "test_volume" .. EXT.TEXTURE)
	else
		self.coreWorld = GetWorldMgr():OpenWorld(path)
		self.path = path
		self.unsave = false

		if self.coreWorld == nil then
			Gui.DialogOk (MainWindow.mainWinRoot.entity, "Scene file is not found or file version is outdated")
			return false
		end
	end
	return true
end

function VolumeWorld:Close()
	if self.coreWorld == nil then return end
	
	self.volumeCube = nil
	self.environment = nil
	self.sceneRenderer = nil

	if self.volumeMaterial then
		Resource.DropMaterial(self.volumeMaterial:GetName())
		self.volumeMaterial = nil
	end
	
	GetWorldMgr():CloseWorld(self.coreWorld)
	self.coreWorld = nil
end

function VolumeWorld:Tick(dt)
	if not self.planeEnt then return end

	self.planeOrigin = self.coreWorld.transform:GetPosition_W(self.planeEnt)
	self.planeNormal = self.coreWorld.transform:GetUpward_W(self.planeEnt)

	--transform to local
	self.planeOriginLocal = self.planeOrigin / self.volumeScale + Vector3(0.5, 0.5, 0.5)

	self.volumeMaterial:SetVector3(self.planeOriginLocal, "cutPlaneOriginL", SHADERS.PS)
	self.volumeMaterial:SetVector3(self.planeNormal, "cutPlaneNormal", SHADERS.PS)
end

function VolumeWorld:SaveAs(path)
	if self.coreWorld == nil then return false end

	local newPath = path
	if newPath == nil then
		newPath = self.path
		if self.unsave == false then return true end
	end
	
	if self.path:len() == 0 then return false end

	self.coreWorld:Save(newPath)
	self.path = newPath
	self.unsave = false
	return true
end

function VolumeWorld:AttachViewport(cameraEntity, width, height)
	if self.sceneRenderer then return nil end

	local initVolumeRenderConfig = RenderInitConfig()
	--initVolumeRenderConfig.

	self.sceneRenderer = self.coreWorld:CreateScene(cameraEntity, width, height, initVolumeRenderConfig)
	return self.sceneRenderer:GetSRV()
end

function VolumeWorld:ResizeViewport(width, height)
	if self.sceneRenderer == nil then return nil end

	self.sceneRenderer:Resize(width, height)
	return self.sceneRenderer:GetSRV()
end

function VolumeWorld:CreateVolumeRenderer()
	self.volumeCube = self.coreWorld:CreateEntity()
	if not self.volumeCube:IsNull() then
		self.coreWorld:SetEntityType(self.volumeCube, EDITOR_VARS.TYPE)
		self.coreWorld.transform:AddComponent(self.volumeCube)
		self.coreWorld.visibility:AddComponent(self.volumeCube)

		if self.coreWorld.staticMesh:AddComponent(self.volumeCube) then
			self.coreWorld.staticMesh:SetMesh(self.volumeCube, PATH.SYS_MESHES .. "invert_cube" .. EXT.MESH)
			
			local materialName = PATH.SYS_MATS .. "volume_renderer" .. EXT.MATERIAL
			self.volumeMaterial = Resource.GetMaterial(materialName)
			self.coreWorld.staticMesh:SetMaterial(self.volumeCube, 0, materialName)
			
			self.coreWorld.transform:SetScale_L(self.volumeCube, self.volumeScale)

			self.volumeMaterial:SetVector3(self.volumeScale, "volumeScale", SHADERS.PS)
			local volumeScaleInv = Vector3(1.0, 1.0, 1.0) / self.volumeScale
			self.volumeMaterial:SetVector3(volumeScaleInv, "volumeScaleInv", SHADERS.PS)
			self.volumeMaterial:SetFloat(self.planeFade, "cutPlaneFade", SHADERS.PS)

			self.volumeMaterial:SetShaderResource(self.volumeCore:GetSRV(), "textureVolume", SHADERS.PS)
		
		end
	end
end

function VolumeWorld:InitVolumeWorld()
--if not self.volumeMaterial then return end
--self.volumeMaterial:SetTextureName (PATH.SYS_TEXTURES .. "test_volume" .. EXT.TEXTURE, "textureVolume", SHADERS.PS)

	self.environment = self.coreWorld:CreateEntity()
	if not self.environment:IsNull() then
		self.coreWorld:SetEntityType(self.environment, EDITOR_VARS.TYPE)
		self.coreWorld.transform:AddComponent(self.environment)

		if self.coreWorld.staticMesh:AddComponent(self.environment) then
			self.coreWorld.staticMesh:SetMesh(self.environment, PATH.SYS_MESHES .. "sky_shpere" .. EXT.MESH)

			local materialName = PATH.ENVS .. "default" .. EXT.MATERIAL

			self.coreWorld.staticMesh:SetMaterial(self.environment, 0, materialName)
			self.coreWorld.transform:SetScale_L3F(self.environment, 5000.0, 5000.0, 5000.0)
		end


	end
	
	self.planeEnt = self.coreWorld:CreateEntity()
	if not self.planeEnt:IsNull() then
		self.coreWorld:SetEntityType(self.planeEnt, "Node")
		self.coreWorld.transform:AddComponent(self.planeEnt)
		self.coreWorld.visibility:AddComponent(self.planeEnt)

		if self.coreWorld.staticMesh:AddComponent(self.planeEnt) then
			self.coreWorld.staticMesh:SetMesh(self.planeEnt, PATH.EDITOR_MESHES .. "unit_plane" .. EXT.MESH)
			self.coreWorld.staticMesh:SetMaterial(self.planeEnt, 0, PATH.EDITOR_MESHES .. "unit_plane" .. EXT.MATERIAL)

			self.coreWorld.transform:SetScale_L3F(self.planeEnt, 10.0, 10.0, 10.0)
			self.coreWorld.transform:SetRotationPYR_L3F(self.planeEnt, 0.0, 0.0, math.pi * 0.5)
		end
	end
	
	self.planeOrigin = Vector3(0.0, 0.0, 0.0)
	self.planeNormal = Vector3(1.0, 0.0, 0.0)

end

function VolumeWorld:Test(pos, ray)
	local plane = Vector4.CreatePlane (self.planeOrigin, self.planeNormal)
	--local brushPos = Vector4.PlaneLineCollide (plane, pos, ray)
	
	local denom = self.planeNormal:Dot (ray)
	if math.abs(denom)> 0.00001 then
		local rayToPlane = self.planeOrigin - pos
		local t = rayToPlane:Dot (self.planeNormal) / denom

		local brushPos = pos + ray * Vector3(t, t, t)

		print (brushPos.x)
		print (brushPos.y)
		print (brushPos.z)

		self:DrawBrush (brushPos, 0.05)
	end
end

function VolumeWorld:DrawBrush (position, radius)
	print("Draw brush")

	local volumePosition = (position + self.volumeScale * Vector3(0.5, 0.5, 0.5)) * Vector3(self.maxVolumeRes, self.maxVolumeRes, self.maxVolumeRes)
	local volumeRadius = radius * self.maxVolumeRes

	self.volumeCore:DrawBrush (volumePosition, volumeRadius)
end
