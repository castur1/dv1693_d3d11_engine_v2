# DV1693 Project assignment submission
Hello! This is my submission for the D3D11 project assignment. I've implemented the full list of technique requirements.

## Controls
- Forwards, backwards, and side-to-side with **WASD**
- Up with **Space**, down with **Left Shift**
- Look around with the **Mouse** (or the **Arrow keys**)
- Hold **X** to move/rotate faster
- Uncapture the mouse with **Esc**; recapture by clicking the window.
- Toggle fullscreen with **F4**
- Toggle the debug overlay with **F1**

The following keybinds also exist but can be accessed through the editor GUI:
- **1**/**2**/**3** to switch to their respective scene
- **C** to cycle the deferred rendering debug mode
- **F** to cycle the current skybox

*(Note that the window must be in focus for your inputs to be registered.)*

The editor interface should hopefully be self-explanatory.

## Techniques
- **Deferred rendering**: Easiest to see through *renderer.deferredMode = 5* under Settings
- **Shadow Mapping**: Should be self-demonstrating. *castsShadows* can be toggled for each light source component under Inspector. The player's capsule model also casts shadows.
- **Particle system**: There's fire particles coming out of the dragon's mouth in scenes *demo_0* and *demo_2*.
- **Dynamic cube mapping**: There's a reflective sphere in each scene with a moving entity nearby. The player's capsule model can be seen in the reflections.
- **Tessellation**: *renderer.wireframe* can be toggled under Settings. The brick sphere in scenes *demo_0* and *demo_1* is tessellated with displacement mapping, as is the ground in *demo_2*.
- **Hierarchical Frustum Culling**: Easiest to see through *octree.showWireframe* and *renderer.freezeCamera* under Settings.
- **Normal mapping**: Probably easiest seen on the Stanford Dragon model while looking at the specular highlights or when *renderer.deferredMode = 2*.

## Credits
- The engine was created by me, Casper Turesson
- Nature 3D assets by https://quaternius.itch.io/ (CC0 license)
- Skybox textures by https://bumbadida.itch.io/ (public domain)
- Dear ImGui (https://github.com/ocornut/imgui) for the editor
- stb_image (https://github.com/nothings/stb) for image loading
- tinyobjloader (https://github.com/tinyobjloader/tinyobjloader) for model loading