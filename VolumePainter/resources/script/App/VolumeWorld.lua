if not VolumeWorld then VolumeWorld = {} end

function VolumeWorld:LoadWorld(path)
	if self.initialized then VolumeWorld:Close () end

	self.path = path
	self.unsave = false

	--Gui.DialogOk (MainWindow.mainWinRoot.entity, "Scene file is not found or file version is outdated")
end

function VolumeWorld:CreateWorld()
	if self.initialized then VolumeWorld:Close() end
		
	self.path = ""
	self.unsave = true

	self.volumeResolutionX = 512
	self.volumeResolutionY = 512
	self.volumeResolutionZ = 512

	VolumeWorld:Init ()
end

function VolumeWorld:Init()
	self.initialized = true

	self.coreWorld = GetWorldMgr ():CreateWorld ()

	self.maxVolumeRes = math.max (math.max (self.volumeResolutionX, self.volumeResolutionY), self.volumeResolutionZ)
	self.volumeScale = Vector3 (self.volumeResolutionX / self.maxVolumeRes, self.volumeResolutionY / self.maxVolumeRes, self.volumeResolutionZ / self.maxVolumeRes)

	self.volumeCore = VolumePainter ()
	if not self.volumeCore:Init (self.volumeResolutionX, self.volumeResolutionY, self.volumeResolutionZ) then
		return false
	end

	self:CreateEnvironment ()
	self:CreateVolumeRenderer ()

	WorkingPlane:Init ()
	Brush:Init ()
	
	-- TODO: serialization
	self.visualizationType = 1
	self:SetDensityScale (70.0)
	self:SetAbsorptionColor (Vector3 (0.5, 0.5, 0.5))
	self:SetAsymmetry (0.2)
	self:SetAbsorptionScale (30.0)
	self:SetSolidThreshold (0.5)
	self:SetLightPitchYaw (30, 0)
	self:SetLightColor (Vector3 (1.0, 1.0, 1.0))
	self:SetLightIntensity (8.0)
	self:SetShadowStepsCount (20)
	self:SetStepsCount (60)

	self:SeEnvColor (Vector3 (0.02, 0.02, 0.02))
	self:SetEnvOpacity(0.5)

	Viewport:SetWorld (self)

	return true
end

function VolumeWorld:Close()
	if self.coreWorld == nil then return end
	
	self.volumeCube = nil
	self.environment = nil
	self.sceneRenderer = nil

	Resource.DropMaterial (self.materialVolume:GetName())
	self.materialVolume = nil
	
	Resource.DropMaterial (self.materialEnv:GetName())
	self.materialEnv = nil

	WorkingPlane:Close ()
	Brush:Close ()

	Viewport:ClearWorld ()

	GetWorldMgr():CloseWorld(self.coreWorld)
	self.coreWorld = nil
	
	self.initialized = false
end

function VolumeWorld:Tick(dt)
	WorkingPlane:Tick(dt)
end

function VolumeWorld:ImportTexture(filepath)
	self.volumeCore:ImportTexture(filepath)
end

function VolumeWorld:ExportTexture(filepath)
	self.volumeCore:ExportTexture (filepath, 0, 0)
end

function VolumeWorld:SaveAs(path)
	if not self.initialized then return false end

	local newPath = path
	if newPath == nil then
		newPath = self.path
		if self.unsave == false then return true end
	else
		MainWindow:SetCaption (newPath)
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

function VolumeWorld:CreateVolumeRenderer ()

	local materialVolumeName = PATH.SYS_MATS .. "volume_shaded" .. EXT.MATERIAL
	self.materialVolume = Resource.GetMaterial (materialVolumeName)
		
	local volumeScaleInv = Vector3 (1.0, 1.0, 1.0) / self.volumeScale
	self.materialVolume:SetVector3 (self.volumeScale, "volumeScale", SHADERS.PS)
	self.materialVolume:SetVector3 (volumeScaleInv, "volumeScaleInv", SHADERS.PS)
	self.materialVolume:SetShaderResource (self.volumeCore:GetSRV (), "textureVolume", SHADERS.PS)

	self.volumeCube = self.coreWorld:CreateEntity()
	if not self.volumeCube:IsNull() then
		self.coreWorld:SetEntityType(self.volumeCube, EDITOR_VARS.TYPE)
		self.coreWorld.transform:AddComponent(self.volumeCube)
		self.coreWorld.visibility:AddComponent(self.volumeCube)

		if self.coreWorld.staticMesh:AddComponent(self.volumeCube) then
			self.coreWorld.staticMesh:SetMesh(self.volumeCube, PATH.SYS_MESHES .. "invert_cube" .. EXT.MESH)
			self.coreWorld.staticMesh:SetMaterial (self.volumeCube, 0, materialVolumeName)
			self.coreWorld.transform:SetScale_L(self.volumeCube, self.volumeScale)		
		end
	end



	self.volumeFrame = self.coreWorld:CreateEntity ()
	if not self.volumeFrame:IsNull () then
		self.coreWorld:SetEntityType (self.volumeFrame, EDITOR_VARS.TYPE)
		self.coreWorld.transform:AddComponent (self.volumeFrame)
		self.coreWorld.visibility:AddComponent (self.volumeFrame)

		if self.coreWorld.staticMesh:AddComponent (self.volumeFrame) then
			self.coreWorld.staticMesh:SetMesh (self.volumeFrame, PATH.SYS_MESHES .. "invert_cube" .. EXT.MESH)
			self.coreWorld.staticMesh:SetMaterial (self.volumeFrame, 0, PATH.SYS_MATS .. "volume_frame" .. EXT.MATERIAL)
			self.coreWorld.transform:SetScale_L (self.volumeFrame, self.volumeScale)
		end
	end

end

function VolumeWorld:CreateEnvironment ()
	local materialEnvName = PATH.ENVS .. "default" .. EXT.MATERIAL
	self.materialEnv = Resource.GetMaterial (materialEnvName)

	self.environment = self.coreWorld:CreateEntity()
	if not self.environment:IsNull() then
		self.coreWorld:SetEntityType(self.environment, EDITOR_VARS.TYPE)
		self.coreWorld.transform:AddComponent(self.environment)

		if self.coreWorld.staticMesh:AddComponent(self.environment) then
			self.coreWorld.staticMesh:SetMesh(self.environment, PATH.SYS_MESHES .. "sky_shpere" .. EXT.MESH)
			self.coreWorld.staticMesh:SetMaterial (self.environment, 0, materialEnvName)
			self.coreWorld.transform:SetScale_L3F(self.environment, 5000.0, 5000.0, 5000.0)
		end
	end
end

function VolumeWorld:SetVisualizationType (type)
	if type == 1 then
		self.materialVolume:SetShader ("../resources/shaders/volume/volume_cube")
	elseif type == 2 then
		self.materialVolume:SetShader ("../resources/shaders/volume/volume_cube_color")
	elseif type == 3 then
		self.materialVolume:SetShader ("../resources/shaders/volume/volume_cube_solid")
	end

	self.visualizationType = type
end

function VolumeWorld:SetDensityScale (scale)
	self.densityScale = scale	
	self.materialVolume:SetFloat (self.densityScale, "densityScale", SHADERS.PS)
end

function VolumeWorld:SetAbsorptionColor (color)
	self.absorptionColor = Vector3 (color.x, color.y, color.z)
	self.materialVolume:SetVector3 (self.absorptionColor, "absorptionColor", SHADERS.PS)
end

function VolumeWorld:SetAbsorptionScale (scale)
	self.absorptionScale = scale
	self.materialVolume:SetFloat (self.absorptionScale, "absorptionScale", SHADERS.PS)
end

function VolumeWorld:SetAsymmetry (asymmetry)
	self.asymmetry = asymmetry
	self.materialVolume:SetFloat (self.asymmetry, "asymmetry", SHADERS.PS)
end

function VolumeWorld:SetSolidThreshold (threshold)
	self.solidThreshold = threshold
	self.materialVolume:SetFloat (self.solidThreshold, "solidThreshold", SHADERS.PS)
end

function VolumeWorld:SetStepsCount (count)
	self.stepsCount = count
	self.materialVolume:SetFloat (self.stepsCount, "stepsCount", SHADERS.PS)
end

function VolumeWorld:SetShadowStepsCount (count)
	self.shadowStepsCount = count
	self.materialVolume:SetFloat (self.shadowStepsCount, "shadowStepsCount", SHADERS.PS)
end

function VolumeWorld:SetLightColor (color)
	self.lightColor = Vector3 (color.x, color.y, color.z)
	self.materialVolume:SetVector3 (self.lightColor, "lightColor", SHADERS.PS)
end

function VolumeWorld:SetLightIntensity (intensity)
	self.lightIntensity = intensity
	self.materialVolume:SetFloat (self.lightIntensity, "lightIntensity", SHADERS.PS)
end

function VolumeWorld:SetLightPitchYaw (pitch, yaw)
	self.lightPitch = pitch
	self.lightYaw = yaw

	local rotMatrix = Matrix.CreateRotationYPR (CMath.DegToRad(self.lightYaw), 0, CMath.DegToRad(self.lightPitch))
	self.lightDir = Vector3.Transform (Vector3.UnitX, rotMatrix)
	self.lightDir:Normalize()

	self.materialVolume:SetVector3 (self.lightDir, "lightDirection", SHADERS.PS)
end

function VolumeWorld:SetEnvTexture (texture)
	self.materialEnv:SetTextureName (texture, "skyTexture", SHADERS.PS)
end

function VolumeWorld:SeEnvColor (color)
	self.envColor = Vector3 (color.x, color.y, color.z)
	self.materialEnv:SetVector3 (self.envColor, "skyColor", SHADERS.PS)
end

function VolumeWorld:SetEnvOpacity(opacity)
	self.envMapOpacity = opacity
	self.materialEnv:SetFloat (self.envMapOpacity, "skyTexOpacity", SHADERS.PS)
end