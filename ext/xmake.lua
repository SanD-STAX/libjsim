local function extension(name, configure)
    target("jsim-" .. name)
        set_kind("static")
        add_files(name .. "/src/*.cpp")
        add_headerfiles(name .. "/include/(jsim/*.hpp)")
        add_includedirs(name .. "/include", { public = true })
        add_deps("jsim")
        if configure then configure() end
    target_end()
end

extension("str")
extension("num")
extension("reflect")
extension("indy")
extension("math")
extension("random")
extension("stacktrace")
extension("jni")

if has_config("with_jbc") then
    extension("jbc", function()
        add_packages("libjbc", { public = true })
    end)
end

if has_config("with_unicorn") then
    extension("jni-uc", function()
        add_deps("jsim-jni")
        local unicorn_dir = get_config("unicorn_dir") or ""
        if unicorn_dir == "" then
            add_packages("unicorn", { public = true })
        else
            add_defines("UNICORN_SHARED", { public = true })
            add_includedirs(path.join(unicorn_dir, "include"), { public = true })
            add_linkdirs(path.join(unicorn_dir, "lib"), { public = true })
            add_links("unicorn", { public = true })
            add_runenvs("PATH", path.join(unicorn_dir, "lib"), path.join(unicorn_dir, "bin"))
        end
    end)
end
