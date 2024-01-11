#include "header.h"
#include "imgui/lib/imgui.h"
#include <iomanip>

float fps = 30.0f;
float yscale = 100.0f;
const int MaxDataPoints = 100;
int dataOffset = 0;
float cpuUsageHistory[MaxDataPoints];
std::vector<float> temperatureData;
std::vector<float> fanSpeedData;

float GetCpuUsage() {
  FILE *file = fopen("/proc/stat", "r");
  if (file) {
    char buf[128];
    fgets(buf, sizeof(buf), file); // Read the first line of /proc/stat
    fclose(file);

    unsigned long user, nice, system, idle, iowait, irq, softirq;
    sscanf(buf, "cpu %lu %lu %lu %lu %lu %lu %lu", &user, &nice, &system, &idle,
           &iowait, &irq, &softirq);

    unsigned long total = user + nice + system + idle + iowait + irq + softirq;
    unsigned long idleTicks = idle;
    return 100.0f * (1.0f - (float)(idleTicks) / (float)(total));
  }
  return 0.0f;
}

int systemWindow(const char *id, ImVec2 size, ImVec2 position) {

  ImGui::Begin(id);
  ImGui::SetWindowSize(id, size);
  ImGui::SetWindowPos(id, position);
  float usagePetcentage;
  ImGui::Text("Operating System: %s", getOsName());
  char *username = getlogin();
  ImGui::Text("Logged in user: %s", username);
  char hostname[HOST_NAME_MAX];
  if (gethostname(hostname, sizeof(hostname)) == 0) {
    ImGui::Text("host Name: %s", hostname);
  }
  // Display CPU Info
  ImGui::Text("CPU Info: %s", CPUinfo().c_str());
  if (sensors_init(nullptr) < 0) {
    ImGui::Text("Error initializing libsensors");
  } else {
    // Display Fan Speeds and Thermals
    int chip_nr = 0;
    const sensors_chip_name *chip;

    while ((chip = sensors_get_detected_chips(nullptr, &chip_nr)) != nullptr) {
      const sensors_feature *feature;

      int feature_nr = 0;
      while ((feature = sensors_get_features(chip, &feature_nr)) != nullptr) {
        if (feature->type == SENSORS_FEATURE_FAN) {
          double speed;
          if (sensors_get_value(chip, feature->number, &speed) == 0) {
            fanSpeedData.push_back(
                static_cast<float>(speed)); // Store fan speed data
          }
        }
        if (feature->type == SENSORS_FEATURE_TEMP) {
          double temp;
          if (sensors_get_value(chip, feature->number, &temp) == 0) {
            temperatureData.push_back(
                static_cast<float>(temp)); // Store temperature data
          }
        }
      }
    }
    sensors_cleanup();
    // sleep(60);
  }

  const char *command = "ps aux --no-header | wc -l";
  FILE *fp = popen(command, "r");
  if (fp != nullptr) {
    char output[16];
    if (fgets(output, sizeof(output), fp) != nullptr) {
      int processCount = std::atoi(output);
      ImGui::Text("Total Running Processes: %d", processCount);
    }
    pclose(fp);
  }

  FILE *statFile = fopen("/proc/stat", "r");
  if (statFile != nullptr) {

    char line[256];
    while (fgets(line, sizeof(line), statFile)) {
      if (strncmp(line, "cpu", 3) == 0) {
        int core;
        unsigned long long user, nice, system, idle, iowait, irq, softirq,
            steal, guest, guest_nice;
        if (sscanf(line,
                   "cpu%d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                   &core, &user, &nice, &system, &idle, &iowait, &irq, &softirq,
                   &steal, &guest, &guest_nice) == 11) {
          unsigned long long totalCpuTime =
              user + nice + system + idle + iowait + irq + softirq + steal;
          unsigned long long idleTime = idle + iowait;

          usagePetcentage =
              (totalCpuTime - idleTime) / (double)totalCpuTime * 100;
        }
      }
    }
    fclose(statFile);

    if (ImGui::BeginTabBar("TabBar")) {
      if (ImGui::BeginTabItem("CPU")) {
        const int bufferSize = 100;
        float cpuUsageBuffer[bufferSize] = {};
        int plotOffset = 0;
        float cpuUsage = GetCpuUsage();
        auto startTime = std::chrono::high_resolution_clock::now();
        // Update the CPU usage buffer
        cpuUsageBuffer[plotOffset] = cpuUsage;
        plotOffset = (plotOffset + 1) % bufferSize;
        ImGui::Text("CPU Usage: %3.2lf%%", *cpuUsageBuffer);
        ImGui::Text("CPU Usage: %3.2lf%%", usagePetcentage);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << usagePetcentage;
        std::string usagePercentageString = ss.str();

        // // Update the circular buffer with the new CPU usage value
        cpuUsageHistory[dataOffset] = usagePetcentage;
        dataOffset = (dataOffset + 1) % MaxDataPoints;
        static bool animationPaused = false;

        if (ImPlot::BeginPlot("CPU Usage", ImVec2(600, yscale))) {
          // Plot the CPU usage data
          if (!animationPaused) {
            ImPlot::PlotLine("Usage", cpuUsageHistory, MaxDataPoints, 100.0f,
                             0.0f, 0, dataOffset);
          }
          // End the plot
          ImPlot::EndPlot();
        }

        // Stop/Resume animation button
        if (ImGui::Button(animationPaused ? "Resume" : "Pause")) {
          animationPaused = !animationPaused;
        }

        // Plot the fan speed data

        // FPS slider
        ImGui::SliderFloat("FPS", &fps, 1.0f, 60.0f);

        // Y Scale slider
        ImGui::SliderFloat("Y Scale", &yscale, 10.0f, 200.0f);

        ImGui::EndTabItem();
        auto endTime = std::chrono::high_resolution_clock::now();
        float frameTime =
            std::chrono::duration<float>(endTime - startTime).count();
        float targetFrameTime = 1.0f / fps;

        if (!animationPaused) {
          float sleepTime = std::max(targetFrameTime - frameTime, 0.0f);
          std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
        }
      }

      if (ImGui::BeginTabItem("TEMP")) {
        if (!temperatureData.empty()) {

          if (ImPlot::BeginPlot("CPU Temperature")) {
            // Plot the CPU usage data
            ImPlot::PlotLine("CPU Temperature", &temperatureData[0],
                             temperatureData.size(), 10000.0f, 0.0f);
            // End the plot
            ImPlot::EndPlot();
          }
        }
        static bool animationPaused = false;
        if (ImGui::Button(animationPaused ? "Resume" : "Pause")) {
          animationPaused = !animationPaused;
        }
        ImGui::SliderFloat("FPS", &fps, 1.0f, 60.0f);

        // Y Scale slider
        ImGui::SliderFloat("Y Scale", &yscale, 10.0f, 200.0f);

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("FAN")) {
        if (!fanSpeedData.empty()) {
          if (ImPlot::BeginPlot("FAN speed")) {
            // Plot the CPU usage data
            ImPlot::PlotLine("Fan Speed (RPM)", &fanSpeedData[0],
                             fanSpeedData.size());
            // End the plot
            ImPlot::EndPlot();
          }
        }
        static bool animationPaused = false;
        if (ImGui::Button(animationPaused ? "Resume" : "Pause")) {
          animationPaused = !animationPaused;
        }
        ImGui::SliderFloat("FPS", &fps, 1.0f, 60.0f);

        // Y Scale slider
        ImGui::SliderFloat("Y Scale", &yscale, 10.0f, 200.0f);

        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }
  } else {
    ImGui::Text("Error: Cannot read CPU usage data");
  }

  ImGui::End();
  return 0;
}

// Function to get CPU usage percentage
