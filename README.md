[<img src="https://mcuxpresso.nxp.com/static/icon/nxp-logo-color.svg" width="100"/>](https://www.nxp.com)
# LLM Edge Studio

[![License](https://img.shields.io/badge/License-Proprietary-red?logo=addthis&logoColor=white)](./LICENSE.txt)
[![Platforms](https://img.shields.io/badge/Platforms-FRDM_i.MX_8M_Plus_|_FRDM_i.MX_95_|_FRDM_i.MX_95_Pro-blue?logo=arm&logoColor=white)](https://www.nxp.com/products/processors-and-microcontrollers/arm-processors/i-mx-applications-processors:IMX_HOME)
[![Language](https://img.shields.io/badge/C++-00599C?logo=cplusplus)](https://isocpp.org/)
[![AI/ML](https://img.shields.io/badge/AI/ML-GenAI-orange?logo=openai&logoColor=white)](https://www.nxp.com/docs/en/user-guide/UG10166.pdf)
[![Python](https://img.shields.io/badge/eIQ_AAF_Connector-2.1.0-green.svg?logo=python&logoColor=white)]()
[![BSP](https://img.shields.io/badge/BSP_>=-LF6.18.20--2.0.0-purple.svg?logo=linux&logoColor=white)](https://www.nxp.com/design/design-center/software/embedded-software/i-mx-software/embedded-linux-for-i-mx-applications-processors:IMXLINUX)

Experience real-time AI assistance at the edge with **Ara240**, powered by NXP’s **LLM Edge Studio**. This solution enables interactive, text-based conversations driven by advanced generative AI. By running *Large Language Models* (LLMs) locally on the NXP **Ara240** *Discrete Neural Processing Unit* (DNPU), it delivers seamless, high-performance conversational interaction without relying on the cloud.
Designed to showcase the performance, scalability, and responsiveness of the **Ara240** architecture, this solution demonstrates the power of localized intelligence by executing LLM workloads directly at the edge, eliminating cloud dependency and drastically reducing latency. The result is secure, real-time conversational AI engineered for next-generation smart kiosks, industrial control interfaces, and intelligent edge systems.

## Table of Contents
1. [Software](#step1)
2. [Hardware](#step2)
3. [Setup](#step3)
4. [Results](#step4)
5. [FAQs](#step5) 
6. [Support](#step6)
7. [Release Notes](#step7)

## 1. Software<a name="step1"></a>

Introducing **LLM Edge Studio**: the NXP launcher designed for interaction with compatible *Large Language Models* (LLMs) accelerated by the **Ara240** DNPU at the edge. This application, compatible with FRDM i.MX 95 and FRDM i.MX 8M Plus, enables rapid interaction with locally running LLMs. It uses the eIQ AAF Connector to communicate with the **Ara240** Runtime SDK and provides a user-friendly GUI for model selection and prompt input.

### 1.1 Software Architecture

![block-diagram](./images/llm-edge-studio-diagram.svg)

LLM Edge Studio provides a simple interface for interacting with locally accelerated Large Language Models. The user prompts are sent to the eIQ AAF Connector via HTTP requests.
From there, the requests are handled by the Ara240 Runtime SDK —comprising client libraries, Optimum‑Ara and the Proxy- before reaching the Ara240 hardware over PCIe, where the user's prompt is processed by a pre-loaded LLM.
The LLM output is streamed back to the user, along the same path in reverse until it is finally displayed on the GUI; this architecture enables real-time response delivery as the output is generated, ensuring a seamless experience.

### 1.2 Currently supported LLMs

| **Model**           | **Params (billion)** | **Time To First Token**<br><sub>(s) | **Avg. Token rate**<br><sub>(Tokens/second) | **Model Card**                                                                         | **License**                                                                           |
| ------------------- | -------------------- | ------------ | ------- | -------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------- |
| Qwen2.5-coder-1.5B  | 1.54                 | 0.26-9.51         | 14.92    | [nxp/Qwen2.5-coder-1.5B-Ara240](https://huggingface.co/nxp/Qwen2.5-Coder-1.5B-Ara240)  | [Apache-2.0](https://huggingface.co/nxp/Qwen2.5-Coder-1.5B-Ara240/blob/main/LICENSE)  |
| Qwen2.5-7B-Instruct | 7.61                 | 1.85-16.73         | 5.99     | [nxp/Qwen2.5-7B-Instruct-Ara240](https://huggingface.co/nxp/Qwen2.5-7B-Instruct-Ara240) | [Apache-2.0](https://huggingface.co/nxp/Qwen2.5-7B-Instruct-Ara240/blob/main/LICENSE) |

> **Note:**\
> **TTFT:** - Time to first token (TTFT). Reported as a range: the lower bound corresponds to prompts up to 128 tokens, and the upper bound reflects prompts at maximum context length.\
> **Avg. Token Rate:** Average token rate over the context length.
 
## 2. Hardware<a name="step2"></a>

### 2.1 Supported Platforms

| Platform                                                                                                    | Supported |
| ----------------------------------------------------------------------------------------------------------- | :-------: |
| [FRDM i.MX 8M Plus](https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-IMX8MPLUS) |     ✅     |
| [FRDM i.MX 95](https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-IMX95)          |     ✅     |
| [FRDM i.MX 95 PRO](https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-IMX95-PRO)  |     ✅     |


### 2.2 Required hardware 

- Supported FRDM i.MX platform
- microSD card (recommended size >=64 GB)
- [Ara240 16GB M.2 Module](https://www.nxp.com/design/design-center/development-boards-and-designs/ARA2-M2-16G-GT)
- USB-C power supply (5V/3A recommended)
- 1920x1080 HDMI display monitor
- HDMI cable
- USB mouse and keyboard
- Two USB-C to USB-A debug cables
- USB hub

## 3. Setup<a name="step3"></a>
Since **LLM Edge Studio** is already part of i.MX Linux BSP, you can simply download a pre-built BSP image from NXP's Official website and flash it to your FRDM i.MX board to install it.
The detailed steps for testing this application are provided below.

### 3.1 Installation

1. Download the Linux i.MX image (version 6.18.20_2.0.0 or greater) corresponding to your FRDM i.MX board from [Embedded Linux for i.MX](https://www.nxp.com/design/design-center/software/embedded-software/i-mx-software/embedded-linux-for-i-mx-applications-processors:IMXLINUX). 
2. Download the [Universal Update Utility](https://github.com/nxp-imx/mfgtools) (release version 1.5.243 or latest).
3. Configure the Boot Switch on the FRDM board to *serial downloader* mode (make sure the board is turned off), please refer to the Quick Start Guide corresponding to yout FRDM i.MX board for this step:
	- [FRDM i.MX 8M Plus Quick Start Guide](https://www.nxp.com/docs/en/quick-reference-guide/FRDMIMX8MPLUSQSG.pdf)
	- [FRDM i.MX 95 Quick Start Guide](https://www.nxp.com/docs/en/quick-reference-guide/FRDMIMX95QSG.pdf)
	- [FRDM i.MX 95 Pro Quick Start Guide](https://www.nxp.com/docs/en/user-guide/FRDMIMX95PROQSG.pdf)
4. Insert the SD card in the FRDM i.MX board.
5. Connect your FRDM i.MX board to the power supply and connect its USB-C and DEBUG ports to your host machine.
6. Open a terminal in your host machine and run the Universal Update Utility using the following command along with the `.bin-flash` file and the `.wic` image corresponding to your FRDM i.MX board:
	```bash
	.\uuu.exe -b sd_all <imx-boot-*.bin-flash> <imx-image-full-*.wic>
	```
7. Power on the FRDM i.MX board and wait for the flashing process to complete.

### 3.2 Running LLM Edge Studio

This application is already available in NXP's [GoPoint for i.MX Applications Processors](https://www.nxp.com/design/design-center/software/i-mx-developer-resources/gopoint-for-i-mx-applications-processors:GOPOINT) launcher, to run it follow the next steps:
1. Connect the HDMI display, USB keyboard and mouse to your FRDM i.MX board (a USB hub might be needed). 
2. Configure the Boot Switch on the FRDM board to *USDHC2 4-bit SD3.0* mode (make sure the board is turned off).
3. Connect the **Ara240 16GB M.2 Module** to the FRDM i.MX board.
4. Power on the FRDM i.MX board and wait for the boot process to complete.
5. Connect the FRDM i.MX board to the internet, for detailed information about Wi-Fi or Ethernet connectivity please refer to [i.MX Linux User's Guide](https://www.nxp.com/docs/en/user-guide/UG10163.pdf) chapter 11.
6. Once the boot process is completed click the **GoPoint** icon that appears in the upper left corner of the screen.
7. Navigate inside **GoPoint** interface and select **LLM Edge Studio**, then click the **Launch Demo** button.
8. Download the available models inside the **LLM Edge Studio** Interface.
8. Once a model download is completed, you can load it by clicking the *Load* button.
9. Now the application is ready and you can begin a conversation with the LLM by writting your queries in the text area and pressing the *submit* button.


## 4. Results<a name="step4"></a>
The figure below shows **LLM Edge Studio's** Interface with an answer provided to a user question.
<center>
<br><img src="./images/llm-edge-studio-query.png" width=720>
<br><i>Figure 1. LLM Edge Studio main view.</i>
</center>

### 4.1 Model Configuration

LLM generation parameters can be adjusted at any time without reloading the model. Click the **gear icon (⚙)** in the model selector bar to open the **Model Configuration** popup.

<center>
<br><img src="./images/model_gear_conf.png" width=720>
<br><i>Figure 2. Gear button used to open the Model Configuration popup.</i>
</center>

<center>
<br><img src="./images/model_conf.png" width=720>
<br><i>Figure 3. Model Configuration popup with stepper controls.</i>
</center>

 <div align="center">
    <br></br>
   <figure>
     <img src="./images/llm-preview.webp" altn>
   </figure>
 </div>

 <div align="center">
   <em> Preview 1. Demonstration of LLM Edge Studio answering a question using Qwen2.5 7B Instruct model. </em>
   <br></br>
</div>

### 4.2 Parameters

| Parameter | Range | Step | Description |
|---|---|---|---|
| **Max Tokens** | 1 – 4096 | 1 | Maximum number of tokens the model will generate per response. |
| **Temperature** | 0.0 – 1.0 | 0.1 | Controls output randomness. Lower values produce more focused and deterministic responses; higher values increase creativity and diversity. |
| **Top K** | 0 – 100 | 1 | Limits sampling to the K most probable next tokens. Set to `0` to disable. |
| **Top P** | 0.0 – 1.0 | 0.1 | Nucleus sampling threshold. Only tokens whose cumulative probability reaches this value are considered. |

### 4.3 Controls

- **Text field** — you can type a value directly or use `[<]` / `[>]` stepper buttons to increment or decrement the value.
- **Reset** — restores all fields to the default values for the currently selected model. Defaults are read from `server_config.json` at startup and overlaid with per-model values from `generation_config.json` when a model is selected. When a model is ejected, the parameters return to the global `server_config.json` baseline.
- **Apply** — commits the current values to the backend. The button is disabled until all four fields contain valid values. Changes will take effect on the next prompt submission.


## 5. FAQs<a name="step5"></a>

### 5.1 Limitations

A single instance of the program **cannot run multiple models simultaneously across different endpoints**. Model selection is limited to one per instance, regardless of the number of endpoints available.

### 5.2 UI not rendered correctly in higher resolution monitors

The application has been designed for 1920x1080 (FHD) resolution and has not yet been updated to support higher resolutions. Since it lacks dynamic resizing, certain UI elements may appear misaligned. As a workaround you can force Weston to use FHD by modifying the `/etc/xdg/weston.ini` script and restart the Weston service:

```plain
[output]
#name=HDMI-A-1
mode=1920x1080@60
#transform=rotate-90
```

### 5.3 Board Reboots Running Inference

If your board reboots when submitting a prompt for inference, the issue may be related to insufficient power supply Ensure that the power source you're using is the correct one.

> **WARN:** Using a lower-rated power supply may cause system instability or unexpected reboots during high-load operations such as inference.

### 5.4 Roadmap

Support for running multiple models on separate endpoints within a single instance is under consideration for future releases. This enhancement would enable more flexible and scalable deployments.

## 6. Support<a name="step6"></a>

#### Project Metadata

<!----- Boards ----->
[![Board badge](https://img.shields.io/badge/Board-FRDM&ndash;IMX95&ndash;PRO-blue)]()
[![Board badge](https://img.shields.io/badge/Board-FRDM&ndash;IMX95-blue)]()
[![Board badge](https://img.shields.io/badge/Board-FRDM&ndash;IMX8MPLUS-blue)]()

<!----- Categories ----->
[![Category badge](https://img.shields.io/badge/Category-AI/ML-yellowgreen)](https://mcuxpresso.nxp.com/appcodehub?category=aiml)
[![Category badge](https://img.shields.io/badge/Category-USER%20INTERFACE-yellowgreen)](https://mcuxpresso.nxp.com/appcodehub?category=ui)

<!----- Peripherals ----->
[![Peripheral badge](https://img.shields.io/badge/Peripheral-DISPLAY-yellow)](https://mcuxpresso.nxp.com/appcodehub?peripheral=display)
[![Peripheral badge](https://img.shields.io/badge/Peripheral-HDMI-yellow)](https://mcuxpresso.nxp.com/appcodehub?peripheral=hdmi)

<!----- Toolchains ----->
[![Toolchain badge](https://img.shields.io/badge/Toolchain-GCC-orange)](https://mcuxpresso.nxp.com/appcodehub?toolchain=gcc)

Questions regarding the content/correctness of this example can be entered as Issues within this GitHub repository.

>**Note**: For more general technical questions regarding NXP Microcontrollers and the difference in expected functionality, enter your questions on the [NXP Community Forum](https://community.nxp.com/)

[![Follow us on Youtube](https://img.shields.io/badge/Youtube-Follow%20us%20on%20Youtube-red.svg)](https://www.youtube.com/NXP_Semiconductors)
[![Follow us on LinkedIn](https://img.shields.io/badge/LinkedIn-Follow%20us%20on%20LinkedIn-blue.svg)](https://www.linkedin.com/company/nxp-semiconductors)
[![Follow us on Facebook](https://img.shields.io/badge/Facebook-Follow%20us%20on%20Facebook-blue.svg)](https://www.facebook.com/nxpsemi/)
[![Follow us on Twitter](https://img.shields.io/badge/X-Follow%20us%20on%20X-black.svg)](https://x.com/NXP)

## 7. Release Notes<a name="step7"></a>
| Version | Description / Update                           | Date                        |
|:-------:|------------------------------------------------|----------------------------:|
| 1.0     | Initial release on Application Code Hub        | September 29<sup>th</sup> 2026 |

## Licensing

This repository is licensed under the [LA_OPT_Online Code Hosting NXP_Software_License](./LICENSE.txt) license.
