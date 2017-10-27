loader = {
    registered = {},
    nextToCheck = nil,
}
  
function loader.print()
    for k,v in pairs(loader.registered) do print(k) end
end

function loader.require(script, onreload)
    if loader.registered[script] ~= nil then
        print("Script already loaded: "..script)
        return 
    end

    print("Script loading: "..script)
    require(script)
    
    local file, _ = string.gsub(script, '%.', '/')
    local check_date = FileIO.GetFileDateModifRaw(script_env.path .. file .. script_env.src_ext)
    if check_date == 0 then
        check_date = FileIO.GetFileDateModifRaw(script_env.guipath .. file .. script_env.src_ext)
    end

    loader.registered[script] = {
        date = check_date,
        func = onreload
    }
    loader.nextToCheck = nil
end
   
function loader:check_modif()    
    local script, params = next(loader.registered, loader.nextToCheck)
    if script == nil then 
        loader.nextToCheck = nil
        return 
    else
        loader.nextToCheck = script
    end

    local file, _ = string.gsub(script, '%.', '/')

    local check_date = FileIO.GetFileDateModifRaw(script_env.path .. file .. script_env.src_ext)
    if check_date == 0 then
        check_date = FileIO.GetFileDateModifRaw(script_env.guipath .. file .. script_env.src_ext)
    end

    if check_date ~= params.date then
        print("Script reloading: "..script)
        package.loaded[script] = nil

        require(script)
        loader.registered[script].date = check_date

        if loader.registered[script].func ~= nil then
            loader.registered[script].func()
        end
    end
end

function loader.functionSerialize(func)
    return string.dump(func)
end

function loader.functionDeserialize(code)
    local func, errorMsg = load(code, "")
    if func ~= nil then
        return func
    else
        error(errorMsg)
        return nil
    end
end