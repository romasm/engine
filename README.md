Engine project
=============

![repoimg](https://github.com/romasm/engine/blob/master/repo_img.png?raw=true)

Run
--------
Just run **engine\build\launch_dev.bat** 

Compile
--------
To compile the project you need Visual Studio and CMake.

First compile the thirdparty projects.
- Go to **engine\thirdparty\Assimp** and run **create_project.bat**, open generated solution **Assimp.sln** and compile assimp project for both Debug and Release configs.
- Go to **engine\thirdparty\BulletPhysics** and run **create_project.bat**, open generated solution **engine\thirdparty\BulletPhysics\build3\vs2015\Bullet3Solution.sln** and compile it for both Debug and Release configs.

Run **create_project.bat** in root folder. It will generate **Engine.sln**. Remove HVisulas.h & .cpp, VoxelRender.h & .cpp from the Core project as it contains unfinishes features. Build the **Core** project.

License
--------

Attribution 4.0 International (CC BY 4.0)
[https://creativecommons.org/licenses/by-nc/4.0/](https://creativecommons.org/licenses/by/4.0/)
