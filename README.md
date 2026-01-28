
# Volumetric Cloud Study (DX11 Port)

This project analyzes high-quality volumetric cloud rendering techniques from ShaderToy and **ports and optimizes them for the DirectX 11 (C++) environment**.
Through the process of converting GLSL to HLSL, I gained a deep understanding of Raymarching principles and the volumetric rendering pipeline.

> **Original Source:** [ShaderToy: Integrated Cloud Shader](https://www.shadertoy.com/view/3sffzj) by (Original Author Name)

## 🎯 Project Goal

* **Porting**: Completely migrate WebGL (GLSL) based shader code to the DirectX 11 (HLSL) environment.
* **Understanding**: Analyze density modeling based on Signed Distance Fields (SDF) and Noise.
* **Engineering**: Design a C++ application-level resource management structure (Texture, Sampler, Buffer), going beyond standalone shader execution.

## 🛠 Tech Stack

* **Language**: C++ (ISO C++17), HLSL (Shader Model 5.0)
* **API**: DirectX 11
* **Libraries**: DirectXTK (WICTextureLoader), ImGui
* **Environment**: Windows 10/11, Visual Studio 2022

---

## 💡 Key Implementations

### 1. GLSL to HLSL Conversion

Rewrote web-based shader code to fit HLSL syntax and integrated it into the DX11 pipeline.

* **Math Mapping**: Resolved syntax differences and mapped types (e.g., `vec3` → `float3`, `mix` → `lerp`, `fract` → `frac`).
* **Resource Binding**: Converted ShaderToy's channel system into DX11's `Texture2D` and `ShaderResourceView` structures for proper binding.

### 2. Texture & Sampler Management

Separated logic handled in a single shader according to resource characteristics to ensure quality.

* **Sampler Separation**:
* **Cloud Noise**: Applied `Linear Sampler` for smooth interpolation.
* **Blue Noise (Dithering)**: Created and applied a separate `Point Sampler` to ensure data accuracy and prevent noise pattern blurring.


* **Texture Baking**: Separated 3D noise generation logic into a specific Compute Shader (or rendering pass) to understand the structure of 3D Texture Atlas creation.

### 3. C++ System Architecture

Built an extensible application structure beyond simple rendering.

* **ResourceManager**: Implemented a texture management system utilizing `std::unordered_map` to prevent duplicate loading.
* **Input System**: Implemented camera input control logic that respects the window focus state to prevent misoperation.

### 4. Interactive Parameter Analysis (GUI)

Integrated **ImGui** to adjust rendering parameters in real-time for an academic approach.

* **Real-time Tuning**: Dynamically modify physical coefficients (Scattering, Extinction, Density Multiplier) without recompiling.
* **Visual Verification**: Observe how numerical changes in the Beer-Lambert law or Phase Function directly impact the visual output.

---

## 📝 Study Notes

Through this project, I verified the following graphics theories via code implementation:

* **Raymarching Loop**: Understanding spatial traversal using `ro + rd * t` and analyzing quality variations based on `Step Size`.
* **Lighting Physics**: Practical application of the `Beer-Lambert Law` (light attenuation) and `Henyey-Greenstein` phase function (light scattering).
* **Optimization**: Analyzed techniques for packing 3D noise into a 2D Atlas to reduce texture sampling costs.
* *(More detailed study notes on specific shader logic and parameter behaviors will be added here.)*

---