#include "header.h"

void fetchProcessData(std::vector<ProcessInfo> &processList,
                      const std::string &filterText) {

  DIR *procDir = opendir("/proc"); // Open the /proc directory

  if (!procDir) {
    std::cerr << "Failed to open /proc directory." << std::endl;
    return;
  }

  struct dirent *entry;
  while ((entry = readdir(procDir)) != nullptr) {
    if (entry->d_type == DT_DIR) {
      int pid;
      if (sscanf(entry->d_name, "%d", &pid) != 1) {
        continue; // Skip entries that are not process directories
      }
      std::ifstream processDir("/proc/" + std::to_string(pid));
      if (!processDir.is_open()) {
        std::cout << "Skipped process with PID: " << pid << std::endl;
        continue;
      }

      std::ifstream statusFile("/proc/" + std::to_string(pid) + "/status");
      std::string line;
      std::string name;
      std::string state;

      while (std::getline(statusFile, line)) {
        if (line.find("Name:") == 0) {
          name = line.substr(6);
        } else if (line.find("State:") == 0) {
          state = line.substr(7);
        }
      }

      std::ifstream statFile("/proc/" + std::to_string(pid) + "/stat");
      std::string statLine;

      if (std::getline(statFile, statLine)) {
        std::istringstream statStream(statLine);
        std::string token;

        for (int i = 1; i <= 13; ++i) {
          statStream >> token;
        }

        unsigned long long utime, stime;
        statStream >> utime >> stime;

        unsigned long long totalTime = utime + stime;
        float cpuUsage = static_cast<float>(totalTime) / sysconf(_SC_CLK_TCK);

        std::ifstream statmFile("/proc/" + std::to_string(pid) + "/statm");
        std::string statmLine;
        unsigned long long memoryUsage = 0;

        if (std::getline(statmFile, statmLine)) {
          std::istringstream statmStream(statmLine);
          statmStream >> memoryUsage;
        }

        float totalMemory = sysconf(_SC_PAGESIZE) *
                            static_cast<float>(memoryUsage) / (1024 * 1024);

        if (!filterText.empty() && name.find(filterText) == std::string::npos) {
          continue;
        }

        processList.emplace_back(pid, name, state, cpuUsage, totalMemory,
                                 false);
      }
    }
  }
  closedir(procDir);
}

void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position,
                           std::vector<ProcessInfo> &processList,
                           std::string &filterText) {

  FILE *meminfo = fopen("/proc/meminfo", "r");
  if (meminfo == NULL) {
    return;
  }

  char line[256];
  unsigned long long total_memory = 0;
  unsigned long long available_memory = 0;

  while (fgets(line, sizeof(line), meminfo)) {
    if (strstr(line, "MemTotal:") == line) {
      sscanf(line, "MemTotal: %llu", &total_memory);
    } else if (strstr(line, "MemAvailable:") == line) {
      sscanf(line, "MemAvailable: %llu", &available_memory);
    }
  }

  fclose(meminfo);

  float ramUsage =
      100.0f * (1.0f - (float)available_memory / (float)total_memory);

  std::string command = "df -P / | tail -1 | awk '{print $5}'";
  FILE *df_output = popen(command.c_str(), "r");
  float diskUsage = 0.0f;

  if (df_output != NULL) {
    fscanf(df_output, "%f", &diskUsage);
    pclose(df_output);
  }

  ImGui::Begin(id);
  ImGui::SetWindowSize(id, size);
  ImGui::SetWindowPos(id, position);

  ImGui::Text("RAM Usage: %.2f%%", ramUsage);
  ImGui::ProgressBar(ramUsage / 100.0f, ImVec2(-1, 0), "##RAMBar");
  ImGui::Separator();

  ImGui::Text("Disk Usage: %.2f%%", diskUsage);
  ImGui::ProgressBar(diskUsage / 100.0f, ImVec2(-1, 0), "##DiskBar");
  ImGui::Separator();

  float swapUsage = getSwapUsage();
  ImGui::Text("SWAP Usage:%.2f%%", swapUsage);
  ImGui::ProgressBar(swapUsage / 100.0f, ImVec2(-1, 0), "##SWAPBar");

  if (ImGui::BeginTabBar("ProcessTabs")) {
    if (ImGui::BeginTabItem("Processes")) {
      char filterBuffer[256];
      strcpy(filterBuffer, filterText.c_str());

      ImGui::InputText("Filter", filterBuffer, sizeof(filterBuffer));

      filterText = filterBuffer;
      ImGui::Columns(6, "ProcessTable", true);
      ImGui::Separator();
      ImGui::Text("PID");
      ImGui::NextColumn();
      ImGui::Text("Name");
      ImGui::NextColumn();
      ImGui::Text("State");
      ImGui::NextColumn();
      ImGui::Text("CPU Usage");
      ImGui::NextColumn();
      ImGui::Text("Memory Usage");
      ImGui::NextColumn();
      ImGui::Text("Select");
      ImGui::NextColumn();
      ImGui::Separator();

      for (int i = 0; i < processList.size(); ++i) {
        if (!filterText.empty() &&
            processList[i].name.find(filterText) == std::string::npos)
          continue;

        ImGui::Text("%d", processList[i].pid);
        ImGui::NextColumn();
        ImGui::Text("%s", processList[i].name.c_str());
        ImGui::NextColumn();
        ImGui::Text("%s", processList[i].state.c_str());
        ImGui::NextColumn();
        ImGui::Text("%.2f%%", processList[i].cpuUsage);
        ImGui::NextColumn();
        ImGui::Text("%.2f%%", processList[i].memoryUsage);
        ImGui::NextColumn();

        ImGui::Checkbox(("##Select" + std::to_string(i)).c_str(),
                        &processList[i].selected);
        ImGui::NextColumn();
      }

      ImGui::Columns(1);
      ImGui::Separator();

      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }

  ImGui::End();
}

float getSwapUsage() {
  FILE *fp = fopen("/proc/meminfo", "r");
  if (fp == NULL) {
    return -1.0f;
  }

  char line[256];
  float totalSwap = 0.0f;
  float freeSwap = 0.0f;

  while (fgets(line, sizeof(line), fp)) {
    if (strncmp(line, "SwapTotal:", 10) == 0) {
      sscanf(line, "SwapTotal: %f kB", &totalSwap);
    } else if (strncmp(line, "SwapFree:", 9) == 0) {
      sscanf(line, "SwapFree: %f kB", &freeSwap);
    }
  }

  fclose(fp);

  if (totalSwap == 0.0f) {
    return -1.0f;
  }

  float swapUsage = 100.0f * ((totalSwap - freeSwap) / totalSwap);
  return swapUsage;
}
