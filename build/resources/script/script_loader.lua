loader = {
    registered = {},
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
end
   
function loader:check_modif()
    for script, params in pairs(loader.registered) do
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
end
