if not Importer then Importer = {} end

function Importer:Init()
    self.filterMesh = dlgFilter()
	self.filterMesh:Add("FBX", "*.fbx")
	self.filterMesh:Add("OBJ", "*.obj")

    self.filterTexture = dlgFilter()
	self.filterTexture:Add("JPEG", "*.jpg")
	self.filterTexture:Add("PNG", "*.png")
	self.filterTexture:Add("TARGA", "*.tga")
end

function Importer:OpenMeshes(win)
    local res = dlgOpenFilesMultiple(win:GetHWND(), "Import meshes", self.filterMesh)
    local size = res:Size()
    if size == 0 then return true end

    for i = 0, size - 1 do
        local file = res:Get(i)

        local point = file:find("%.", -4)
        if point ~= nil then point = point - 1 end
        local resource = file:sub(1, point)

        Resource.ImportResource({
            filePath = file,
            resourceName = resource,
            importMesh = true,
            isSkinnedMesh = false,
        })

    end
end

function Importer:ImportLoadMesh(sourceFile, callback, data)
    local point = sourceFile:find("%.", -4)
    if point ~= nil then point = point - 1 end
    local resource = sourceFile:sub(1, point)

    Resource.ImportResourceCallback({
        filePath = sourceFile,
        resourceName = resource,
        importMesh = true,
        isSkinnedMesh = false,
    }, callback, data)
end