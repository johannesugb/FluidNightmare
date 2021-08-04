# Fluid Nightmare

Rendering of particle-based fluids with real-time ray tracing.      
This project is implemented using the [Gears-Vk](https://github.com/cg-tuwien/Gears-Vk) framework.

## Project Description

TBD.

## Environment

- Windows 10
- Visual Studio 2019 (make sure it is updated s.t. it includes the latest C++ compiler)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows) 

_Gears-Vk_ (the used framework) is compatible with the latest version of the Vulkan SDK, which is currently **1.2.182.0**, and should run without validation errors (watch out for red error messages displayed in Debug builds).

Make sure to read about the two frameworks
- [`Auto-Vk/README.md`](https://github.com/cg-tuwien/Auto-Vk/blob/master/README.md)
- [`Gears-Vk/README.md`](https://github.com/cg-tuwien/Gears-Vk/blob/master/README.md)
- [_Gears-Vk_'s Visual Studio Project Management](https://github.com/cg-tuwien/Gears-Vk/tree/master/visual_studio#project-management-with-visual-studio)
- [_Gears-Vk_'s Post Build Helper](https://github.com/cg-tuwien/Gears-Vk/tree/master/visual_studio#post-build-helper), Hint: Also have a look at its [recommended settings](https://github.com/cg-tuwien/Gears-Vk/tree/master/visual_studio#post-build-helper-settings)

## Setup Instructions and Submodules

1. Clone the repository with all submodules recursively:       
`git clone https://github.com/cg-tuwien/FluidNightmare.git . --recursive` (to check out into `.`)
2. Open `fluid-nightmare.sln` with Visual Studio 2019, set `fluid-nightmare` as the startup project, build and run

There are two submodules: One under `gears_vk/` (referencing https://github.com/cg-tuwien/Gears-Vk) and another under `gears_vk/auto_vk/` (referencing https://github.com/cg-tuwien/Auto-Vk).    
If they weren't properly initialized during the `clone`, get them as follows:      
`git submodule update --init --recursive`

To update the submodules on a daily basis, use the following command:  
`git submodule update --recursive`

To contribute to either of the submodules, please do so via pull requests and follow the ["Contributing Guidelines" from Gears-Vk](https://github.com/cg-tuwien/Gears-Vk/blob/master/CONTRIBUTING.md). Every time you check something in, make sure that the correct submodule-commits (may also reference forks) are referenced so that one can always get a compiling and working version by cloning as described in step 1!

## Visual Studio

If you are experiencing setup problems (e.g. after installing a new Vulkan SDK version), the project can be put into a clean state with the following steps:
- Clean the solution `Build -> Clean Solution` then rebuild `Build -> Rebuild Solution`.
- If that doesn't help, close Visual Studio, and delete the folders `.vs`, `bin`, and `temp`. Then open the solution and build.

## Documentation 

TBD.

## License 

TBD.
