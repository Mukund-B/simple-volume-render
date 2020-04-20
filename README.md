# simple-volume-render

This is an implementation of a simple single-pass ray tracer in C++ and OpenGL. File path and transfer function settings are edited through _Config.txt_. The program currently supports **pvm** and **raw** file forrmats.

The program can render the loaded dataset using:
* Maximum Intensity Rendering
* Iso-surface Rendering
* Emission-Absorption Compositing Equation (with alpha blending)

## TO DO
1. Implement better trackball support.
2. Extend file format support.
3. Implement Compute Shader based gradient calculation for IsoSurface rendering.
4. Implement a GUI for easier uability.
