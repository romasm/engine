function Gui.TB_asset()
return GuiMenu({
    styles = {
        GuiStyles.menu_topbar,
    },
    
    id = "tb_asset_menu",
    
    height = 52, 

    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_import_mesh",
        top = 1,
        text = { str = "Import mesh", },
    }),

    ----
    GuiButton({
        styles = {GuiStyles.menu_button_default,},
        id = "tb_import_tex",
        top = 26,
        text = { str = "Import texture", },
    }),
})
end