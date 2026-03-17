# Real-Time CPU PBR Path Tracer
> 基于 C++17 多线程与微面元物理模型的简易实时路径追踪渲染器 

[Image of a high quality PBR sphere render demonstration]
<img width="1596" height="926" alt="image" src="https://github.com/user-attachments/assets/b4da4833-fff8-4734-bcc3-b00404feaf3e" />
[Image of a luminous sphere render demonstration]
<img width="1602" height="939" alt="image" src="https://github.com/user-attachments/assets/06b9f886-5d4e-4ad3-aa8f-c08ee48da7bc" />

## 📖 Introduction (项目简介)
本项目为基于 C++ 构建的 **CPU 软件光线追踪器**。重点验证了基于物理的渲染（PBR）核心光照数学模型与蒙特卡洛积分算法。

通过结合C++17并行算法与Frame Accumulation技术，渲染器在纯 CPU 环境下达到交互式的实时渲染帧率。

## ✨ Core Features

### 1. 基于物理的渲染 (Physically Based Rendering)
- **微面元 BRDF 模型:** 实现了完整的 Cook-Torrance BRDF。
  - **F (菲涅尔方程):** 使用 Fresnel-Schlick 近似计算基础反射率（F0），区分金属与非金属工作流。
  - **D (法线分布):** 基于 GGX (Trowbridge-Reitz) 的法线分布函数。
  - **G (几何遮蔽):** 使用 Smith-Schlick 联合几何遮蔽函数处理微表面的自遮挡效应。
- **材质系统:** 支持实时调节 Albedo（基础色）、Roughness（粗糙度）、Metallic（金属度）以及 Emission（自发光）。

### 2. 蒙特卡洛积分与采样优化 (Sampling Optimization)
- **重要性采样 (Importance Sampling):** 摒弃低效的均匀半球采样，利用 GGX 分布生成采样方向，极大加速了高光分支的渲染方程收敛。
- **随机波瓣选择 (Lobe Selection):** 借鉴俄罗斯轮盘赌思想，根据金属度与菲涅尔项动态计算高光概率。利用随机数分离高光与漫反射分支，避免光线弹射数量指数级爆炸，并通过Weight Compensation保证宏观上的**能量守恒**。

### 3. 多线程与交互式架构 (Multithreading & UI)
- **像素级并发:** 利用 C++17 `std::execution::par` 策略实现全画面像素的多线程并行计算。
- **渐进式抗锯齿:** 基于时间序列的帧累积（Frame Accumulation）算法，有效消除蒙特卡洛积分带来的高频噪点。支持动态修改场景时的无缝缓存重置。
- **实时调试面板:** 结合 ImGui 提供所见即所得的参数调试界面，支持动态切换材质属性、相机位置与光追模式（纯发光材质 / PBR 路径追踪）。

## 🛠️ Dependencies (依赖项)
- **C++17** (必须支持并行算法库)
- **[Walnut](https://github.com/TheCherno/Walnut):** 提供底层应用窗口与 Vulkan 纹理渲染后处理
- **[GLM](https://github.com/g-truc/glm):** 3D 向量与矩阵数学库
- **[ImGui](https://github.com/ocornut/imgui):** 实时 UI 界面

## 🚀 How to Build (构建指南)
1. Clone the repository including submodules:
   ```bash
   git clone --recursive [https://github.com/YourUsername/YourRepoName.git](https://github.com/YourUsername/YourRepoName.git)

📝 TODOs
[ ] 引入 BVH (Bounding Volume Hierarchy) 加速结构

[ ] 解析 .obj / .gltf 复杂多边形网格 (Triangle Mesh) 求交

[ ] 添加天空盒 (HDRI Environment Map) 基于图像的光照 (IBL)
