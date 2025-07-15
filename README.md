# Deskmon

Deskmon is a productivity monitoring application that tracks user activity and provides insights through a system tray interface.

## Prerequisites

- **Qt 6.x**: Ensure you have the Qt development environment installed. You can download it from [the official Qt website](https://www.qt.io/download).
- **CMake**: Make sure CMake is installed. You can install it via Homebrew with the command:
  ```bash
  brew install cmake
  ```

## Building the Application

1. **Clone the repository** (if you haven't already):
   ```bash
   git clone https://github.com/yourusername/Deskmon.git
   cd Deskmon
   ```

2. **Create a build directory**:
   ```bash
   mkdir build
   cd build
   ```

3. **Run CMake to configure the project**:
   ```bash
   cmake ..
   ```

4. **Build the application**:
   ```bash
   make
   ```

## Running the Application

After a successful build, you can run the application with the following command:

```bash
cd build
./Deskmon.app/Contents/MacOS/Deskmon --show
```

This will launch the Deskmon application and display the main window immediately.

## Notes

- If you encounter any issues during the build process, ensure that all dependencies are correctly installed and that you are using the correct version of Qt.
- For further assistance, refer to the documentation or open an issue in the repository.