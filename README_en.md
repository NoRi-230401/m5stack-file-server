# Project Documentation: m5stack-file-server

## 1. Overview

`m5stack-file-server` is a project that transforms an M5Stack device (ESP32-based) into a **Web File Server** operating on a WiFi network. It allows easy management of files on both the SD card and the internal SPIFFS via a web browser.

**Main Purpose:**

*   Provide a means to access and manage files on the M5Stack from a PC or smartphone.
*   Facilitate program updates, configuration file editing, log file checking, etc.

**Supported Devices:**

*   ESP32-based M5Stack devices with an SD card slot, such as M5Stack Core, Core2, CoreS3.
    *   (An SD card slot is not required if only using SPIFFS).

## 2. Key Features

*   **File System Access:**
    *   SD Card (if `SD_USE = true`)
    *   SPIFFS (Internal flash memory file system, if `SPIFFS_USE = true`)
*   **Web Interface (Responsive Design):**
    *   **File/Directory Listing:**
        *   SD Card: Displays the contents (files and subdirectories) of the current directory.
        *   SPIFFS: Displays the list of files in the root directory.
        *   Shows filename and size (blank for directories).
        *   Layout adapts for PC/smartphone viewing.
    *   **File Operations:**
        *   Upload: Transfer files from PC/smartphone to M5Stack.
        *   Download: Save files from M5Stack to PC/smartphone.
        *   Stream: Display files directly viewable in the browser (images, text, etc.).
        *   Delete: Select files/directories for deletion (with confirmation).
        *   Rename: Select files/directories to change their names.
        *   View Text: Display the content of text-based files (e.g., `.txt`, `.log`, `.html`, `.js`, `.css`, `.json`, `.c`, `.h`, `.cpp`) in the browser.
    *   **SD Card Directory Operations:**
        *   Change Directory (Chdir): Navigate into subdirectories.
        *   Make Directory (Mkdir): Create new directories.
        *   Remove Directory (Rmdir): Delete empty directories (with confirmation).
        *   Go to Parent Directory (Up).
        *   Go to Root Directory (Top).
*   **Networking Features:**
    *   **WiFi Connection:** Connects to a WiFi access point using the specified SSID and password (STA mode).
    *   **mDNS:** Allows access via a hostname like `http://(your_server_name).local/` (e.g., `http://stackchan.local/`) instead of an IP address.
    *   **Configuration File:** Network settings (SSID, password, server name) can be overridden by placing a `wifi.txt` file in the root of the SD card or SPIFFS, instead of using the default values in the source code.
*   **System Functions:**
    *   **Status Page (`/system`):**
        *   File System Info (Total/Used/Free space for SD/SPIFFS).
        *   Memory Info (Total/Free/Min Free/Max Alloc block for SRAM/PSRAM).
        *   NVS (Non-Volatile Storage) Info.
        *   CPU Info (Model, Revision, Cores, Frequency, etc.).
        *   Network Info (IP Address, Server Name, SSID, MAC Address, etc.).
        *   Time Info (RTC Time, NTP Synced Time).
        *   File Transfer Stats (Size and speed of last upload/download).
    *   **NTP Time Synchronization:** Retrieves time from an NTP server at startup and adjusts the M5Stack's RTC (Real-Time Clock) (if `RTC_ADJUST_REQ = true`).
    *   **Web API:**
        *   `/shutdown?reboot=on`: Reboot the device (with confirmation dialog).
        *   `/shutdown`: Shut down (power off) the device (with confirmation dialog).
        *   `/test?ok=true`: For testing API functionality.
*   **Other Features:**
    *   **SD Card Updater:** (Optional, if `ENABLE_SD_UPDATER` is defined) Firmware update functionality from the SD card.
    *   **Display Output:** (Optional, if `DISP_ON = true`) Shows IP address and simple log messages on the M5Stack screen.
    *   **Favicon:** Displays `favicon.ico` if present in the root of the SD card or SPIFFS.
    *   **Home Image:** Displays `homeImg.gif` on the homepage if present in the root of the SD card or SPIFFS.

## 3. File Structure and Roles

The project consists of the following files:

*   **`main.cpp`**:
    *   **Role:** Application **entry point** (`setup()`, `loop()`).
    *   **Details:**
        *   Defines global configuration constants (SD/SPIFFS usage, display on/off, RTC adjustment, etc.).
        *   Calls `setupServer()` to orchestrate the initialization of file systems, network, and web server.
        *   In the main loop (`loop()`), handles requests from the Web API (`requestManage()`) and performs RTC time adjustment (`adjustRTC()`).
*   **`fileServer.cpp`**:
    *   **Role:** Basic web server setup and common functionalities.
    *   **Details:**
        *   Initializes and starts the `AsyncWebServer` object (`server`) (`fileServerStart()`).
        *   Generates common HTML header (`HTML_Header()`), footer (`HTML_Footer()`), and CSS styles (`HTML_Style()`) with responsive design.
        *   Defines handlers for the homepage (`/`) and system information page (`/system`).
        *   Provides file system initialization function (`FS_start()`).
        *   Returns appropriate Content-Type based on file extension (`getContentType()`).
        *   Handles undefined URL access via the `notFound()` handler, delegating to specific file system handlers.
*   **`util.cpp`**:
    *   **Role:** Provides various **utility functions**.
    *   **Details:**
        *   Network related: WiFi connection (`wifiStart()`), mDNS start (`mdnsStart()`).
        *   Time related: Get time from NTP and set RTC (`adjustRTC()`), format time strings (`getTmRTC()`, `getTmNTP()`, `strTmInfo`).
        *   Settings loading: Read network settings from `wifi.txt` (`getSetting()`).
        *   Data conversion: Convert file sizes to KB/MB/GB etc. (`ConvBytesUnits()`), URL encode/decode (`urlEncode()`, `urlDecode()`).
        *   Logging: Common output to Serial and M5Stack display (`prt()`).
        *   Debugging: Get/print heap memory information (`getHeapInf()`, `prtHeapInf`).
*   **`SD_handler.cpp`**:
    *   **Role:** Handles **SD card** related web interface and file/directory operations.
    *   **Details:**
        *   File/directory listing (`SD_Dir`, `SD_Directory`).
        *   Web UI generation and processing handlers for file operations (upload, download, stream, delete, rename, view text) (`SD_handleFileUpload`, `SD_Handle_File_Delete`, `SD_Handle_File_Rename`, `SD_View_Text`, etc.).
        *   Web UI generation and processing handlers for directory operations (change, create, remove) (`SDdir_*` functions: `SDdir_Handle_chdir`, `SDdir_Handle_mkdir`, `SDdir_Handle_rmdir`, etc.).
        *   Confirmation page generation for delete operations (`SD_Generate_Confirm_Page`, `SDdir_Generate_Confirm_Page`).
        *   SD card related `notFound` processing (`SD_notFound`, `SDdir_notFound`).
*   **`SPIFFS_handler.cpp`**:
    *   **Role:** Handles **SPIFFS** related web interface and file operations.
    *   **Details:**
        *   File listing (`SPIFFS_Dir`, `SPIFFS_Directory`).
        *   Web UI generation and processing handlers for file operations (upload, download, stream, delete, rename, view text) (`SPIFFS_handleFileUpload`, `SPIFFS_Handle_File_Delete`, `SPIFFS_Handle_File_Rename`, `SPIFFS_View_Text`, etc.).
        *   Confirmation page generation for delete operations (`SPIFFS_Generate_Confirm_Page`).
        *   SPIFFS related `notFound` processing (`SPIFFS_notFound`).
        *   (No directory operations as SPIFFS doesn't natively support directories in the same way).
*   **`webApi.cpp`**:
    *   **Role:** Provides **Web API** endpoints for device control.
    *   **Details:**
        *   Sets up API routes like `/shutdown`, `/test` (`webApiSetup()`).
        *   Handles API requests (`handle_shutdown`, `handle_test`).
        *   Manages flags (`requestManage()`, `sendReq()`) for actual device control (reboot `REBOOT()`, power off `POWER_OFF()`, stop `STOP()`) executed in the `loop()` function.
        *   Generates simple HTML pages for API responses.
*   **`fileServer.h`** (Assumed Header File):
    *   **Role:** Shared definitions and declarations for the entire project.
    *   **Details:**
        *   Includes necessary libraries (`M5Unified.h`, `WiFi.h`, `ESPAsyncWebServer.h`, `SPIFFS.h`, `SD.h`, etc.).
        *   Defines constants (`FS_SD`, `FS_SPIFFS`, `UNIT_AUTO`, etc.).
        *   Declares global variables (`extern server`, `extern webpage`, `extern SSID`, `extern IP_ADDR`, `extern SD_ENABLE`, `extern SdPath`, etc.).
        *   Defines the `fileinfo` struct.
        *   Provides function prototypes for functions defined in the `.cpp` files.

## 4. Setup and Usage

1.  **Build and Flash:**
    *   Open the project in a development environment like PlatformIO, set the target device, build, and flash it to your M5Stack.
2.  **Network Configuration:**
    *   **Method 1 (Recommended):** Create a file named `wifi.txt` in the root directory of the SD card or SPIFFS with the following format:
        ```
        YOUR_WIFI_SSID
        YOUR_WIFI_SSID_PASSWORD
        your_server_name
        ```
        (Ensure each line ends with a newline. `your_server_name` is used for mDNS.)
    *   **Method 2:** Directly edit the values of `YOUR_SSID`, `YOUR_SSID_PASS`, and `YOUR_SERVER_NAME` in `main.cpp`.
3.  **Access:**
    *   Power on the M5Stack. It will connect to WiFi, and the IP address and server name will be displayed on the Serial Monitor (and the screen if enabled).
    *   Open a web browser on a PC or smartphone connected to the same network and navigate to the displayed IP address (`http://<IP_address>/`) or the mDNS name (`http://<server_name>.local/`).
4.  **Operation:**
    *   Use the menus and buttons on the displayed web interface to perform file operations and view system information.

## 5. Customization

*   **Enable/Disable Features:** Modify the constants `SD_USE`, `SPIFFS_USE`, `DISP_ON`, `RTC_ADJUST_REQ`, etc., in `main.cpp`.
*   **Network Settings:** Change the `wifi.txt` file or the default values in `main.cpp` as described above.
*   **Appearance:**
    *   Place a `favicon.ico` file in the root of the SD card or SPIFFS to change the browser tab icon.
    *   Place a `homeImg.gif` file in the root of the SD card or SPIFFS to display an image on the homepage.
    *   Edit the CSS within the `HTML_Style()` function in `fileServer.cpp` to modify the web interface design.

