# Fluid Nightmare

Rendering of particle-base fluids with real-time ray tracing.

## Project Description

TBD.

## Environment

- Windows 10
- Visual Studio 2019 (make sure it is updated s.t. it includes the latest C++ compiler)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows), but beware: _Gears-Vk_ (the used framework) has currently some issues with the latest SDK. Therefore, SDK version **1.2.162.1** is recommended.

## Setup Instructions and Submodules

1. Clone the repository with all submodules recursively:       
`git clone https://github.com/cg-tuwien/FluidNightmare.git . --recursive` (to check out into `.`)
2. Open `fluid-nightmare.sln` with Visual Studio 2019, set `fluid-nightmare` as the startup project, build and run

You might want to have all submodules checked-out to their `master` branch. You can do so using:      
`git submodule foreach --recursive git checkout master`.       
There are two submodules: One under `gears_vk/` (referencing https://github.com/cg-tuwien/Gears-Vk) and another under `gears_vk/auto_vk/` (referencing https://github.com/cg-tuwien/Auto-Vk).    

To update the submodules on a daily basis, use the following command:  
`git submodule update --recursive`

To contribute to either of the submodules, please do so via pull requests and follow the ["Contributing Guidelines" from Gears-Vk](https://github.com/cg-tuwien/Gears-Vk/blob/master/CONTRIBUTING.md). Every time you check something in, make sure that the correct submodule-commits (may also reference forks) are referenced so that one can always get a compiling and working version by cloning as described in step 1!

## Documentation 

TBD.

## License 

TBD.
