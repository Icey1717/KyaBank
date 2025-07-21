# 🏦 KyaBank

**KyaBank** is a command-line utility for working with `.BNK`, `.G2D`, and `.G3D` asset archives from the PlayStation 2 game **Kya: Dark Lineage**.

It supports listing and extracting `.BNK` archive contents, as well as converting game textures and models into modern formats like `.png` and `.gltf`.

---

## 🔧 Features

- 📦 **List** and **extract** contents of `.BNK` archive files  
- 🎨 **Convert** `.G2D` texture files to `.png`  
- 🧱 **Convert** `.G3D` mesh files to `.gltf` or `.glb`

---

## 📥 Installation

Build from source using your preferred C++ toolchain (requires C++17 or later and a suitable build system like CMake).

---

## 🚀 Usage

```bash
KyaBank list <path_to_bnk_file>
KyaBank extract <path_to_bnk_file> -o <output_directory> [--flatten]

KyaBank texconvert <path_to_g2d_file_or_folder> -o <output_directory>
KyaBank meshconvert <path_to_g3d_file_or_folder> -o <output_directory>
```

### 🔹 Commands

#### `list`
List all files contained in a `.BNK` archive.

```bash
KyaBank list example.bnk
```

#### `extract`
Extract contents of a `.BNK` archive.

```bash
KyaBank extract example.bnk -o ./output
```

Use `--flatten` to ignore folder hierarchy when extracting.

#### `texconvert`
Convert `.G2D` texture files to `.png`.

```bash
KyaBank texconvert path/to/file.g2d -o ./textures
KyaBank texconvert path/to/folder_with_g2ds -o ./textures
```

#### `meshconvert`
Convert `.G3D` mesh files to `.gltf`/`.glb`.

```bash
KyaBank meshconvert path/to/file.g3d -o ./models
KyaBank meshconvert path/to/folder_with_g3ds -o ./models
```

---

## 🧪 Debug Mode

You can use the `KYABANK_ARGS` environment variable to simulate command-line input when launching the program without arguments:

```bash
set KYABANK_ARGS="extract test.bnk -o output"
KyaBank
```

This is useful for debugging or integration with IDEs that don't pass arguments easily.

---

## 📂 Example Archive Contents

```text
> KyaBank list LEVEL01.BNK
Listing Files contained in G:\repos\KYA_TEMP\CDEURO\LEVEL\LEVEL_2\LEVEL.BNK:
D:\PROJECTS\B-WITCH\RESOURCE\BUILD\LEVEL_2\install.bin
D:\PROJECTS\B-WITCH\RESOURCE\BUILD\OBJECT\CHARACTER\Companion\Map\CMP_TIPS.G2D
D:\PROJECTS\B-WITCH\RESOURCE\BUILD\OBJECT\CHARACTER\Companion\Map\CMP_HALO.G2D
D:\PROJECTS\B-WITCH\RESOURCE\BUILD\PARTICLE\sparks.g2d
```

---

## ✅ Status

Actively maintained. Contributions welcome!

---

## 📄 License

MIT License – see [LICENSE](./LICENSE) for details.
