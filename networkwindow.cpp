#include "header.h"

std::string runCommand(const char *cmd) {
  char buffer[128];
  std::string result = "";
  FILE *pipe = popen(cmd, "r");
  if (!pipe) {
    return "Command execution failed.";
  }
  while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
    result += buffer;
  }
  pclose(pipe);
  return result;
}

void getNetworkStatistics(unsigned long long &rxBytes,
                          unsigned long long &txBytes) {
  std::string ifconfigOutput = runCommand(
      "ifconfig wlp2s0"); // Replace with the actual network interface name
  // Parse ifconfigOutput and extract RX and TX data
  // Example: Parse the output and extract relevant statistics
  sscanf(ifconfigOutput.c_str(), "RX bytes:%llu", &rxBytes);
  sscanf(ifconfigOutput.c_str(), "TX bytes:%llu", &txBytes);
}

void networkWindow(const char *id, ImVec2 size, ImVec2 position) {
  ImGui::Begin(id);
  ImGui::SetWindowSize(id, size);
  ImGui::SetWindowPos(id, position);

  ImGui::Text("Network Information:");

  // Get network interfaces
  struct ifaddrs *ifaddr, *ifa;
  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    return;
  }

  ImGui::BeginTabBar("NetworkTabs");

  // Iterate over network interfaces
  for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == nullptr) {
      continue;
    }

    // Display network interface name
    ImGui::Text("%s", ifa->ifa_name);

    // Display RX and TX data in tables
    if (ImGui::BeginTabItem(ifa->ifa_name)) {
      ImGui::Columns(8, "NetworkData", true);
      ImGui::Separator();
      ImGui::Text("Bytes");
      ImGui::NextColumn();
      ImGui::Text("Packets");
      ImGui::NextColumn();
      ImGui::Text("Errs");
      ImGui::NextColumn();
      ImGui::Text("Drop");
      ImGui::NextColumn();
      ImGui::Text("Fifo");
      ImGui::NextColumn();
      ImGui::Text("Frame");
      ImGui::NextColumn();
      ImGui::Text("Compressed");
      ImGui::NextColumn();
      ImGui::Text("Multicast");
      ImGui::NextColumn();
      ImGui::Separator();

      // Display RX data here (replace with actual data)
      ImGui::Text("RX Bytes");
      ImGui::NextColumn();
      ImGui::Text("RX Packets");
      ImGui::NextColumn();
      ImGui::Text("RX Errs");
      ImGui::NextColumn();
      ImGui::Text("RX Drop");
      ImGui::NextColumn();
      ImGui::Text("RX Fifo");
      ImGui::NextColumn();
      ImGui::Text("RX Frame");
      ImGui::NextColumn();
      ImGui::Text("RX Compressed");
      ImGui::NextColumn();
      ImGui::Text("RX Multicast");
      ImGui::NextColumn();

      // Display TX data here (replace with actual data)
      ImGui::Text("TX Bytes");
      ImGui::NextColumn();
      ImGui::Text("TX Packets");
      ImGui::NextColumn();
      ImGui::Text("TX Errs");
      ImGui::NextColumn();
      ImGui::Text("TX Drop");
      ImGui::NextColumn();
      ImGui::Text("TX Fifo");
      ImGui::NextColumn();
      ImGui::Text("TX Colls");
      ImGui::NextColumn();
      ImGui::Text("TX Carrier");
      ImGui::NextColumn();
      ImGui::Text("TX Compressed");
      ImGui::NextColumn();

      ImGui::Columns(1);
      ImGui::Separator();

      // Visual display of network usage in GB, KB, or MB

      unsigned long long rxBytes = 0, txBytes = 0;
      getNetworkStatistics(rxBytes, txBytes);

      // Convert bytes to GB, KB, or MB
      std::string rxUsage = formatBytes(rxBytes);
      std::string txUsage = formatBytes(txBytes);

      // Display RX and TX visual usage
      ImGui::ProgressBar(0.3f, ImVec2(-1, 0), rxUsage.c_str());
      ImGui::ProgressBar(0.1f, ImVec2(-1, 0), txUsage.c_str());

      ImGui::EndTabItem();
    }
  }

  ImGui::EndTabBar();

  freeifaddrs(ifaddr); // Free memory allocated by getifaddrs
  ImGui::End();
}

// Helper function to format bytes to GB, KB, or MB
std::string formatBytes(unsigned long long bytes) {
  const char *suffix[] = {"B", "KB", "MB", "GB"};
  char length = sizeof(suffix) / sizeof(suffix[0]);

  int i = 0;
  double dblBytes = static_cast<double>(bytes);

  if (bytes > 1024) {
    for (i = 0; (bytes / 1024) > 0 && i < length - 1; i++, bytes /= 1024) {
      dblBytes = bytes / 1024.0;
    }
  }

  char formattedBytes[32];
  snprintf(formattedBytes, sizeof(formattedBytes), "%.2f %s", dblBytes,
           suffix[i]);
  return std::string(formattedBytes);
}
