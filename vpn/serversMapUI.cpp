#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <map>
#include <cstdlib>
#include <csignal>
#include <thread>
#include <chrono>
#include <fstream>

namespace fs = std::filesystem;

// Simple logger
void log(const std::string& msg) {
    std::ofstream logfile("vpn_client.log", std::ios_base::app);
    logfile << "[" << std::time(nullptr) << "] " << msg << std::endl;
}

// Global process PID
pid_t child_pid = -1;

// Signal handler for Ctrl+C
void signal_handler(int signum) {
    if (child_pid > 0) {
        std::cout << "\nDisconnecting..." << std::endl;
        kill(child_pid, SIGTERM);
        log("Disconnected VPN (terminated child process)");
    }
    std::exit(0);
}

int main() {
    std::string config_dir = "./vpn_configs";
    if (!fs::exists(config_dir) || !fs::is_directory(config_dir)) {
        std::cerr << "Config directory " << config_dir << " does not exist. Please create and add config files." << std::endl;
        return 1;
    }

    std::cout << "=== Nexus VPN Client (C++) ===" << std::endl;

    signal(SIGINT, signal_handler);

    while (true) {
        std::cout << "\nSelect protocol:\n1. OpenVPN\n2. WireGuard\n3. Exit\n> ";
        int proto_choice;
        std::cin >> proto_choice;
        std::cin.ignore();
        std::string protocol;
        if (proto_choice == 1) protocol = "OpenVPN"; 
        else if (proto_choice == 2) protocol = "WireGuard";
        else break;

        // Map country name to config path
        std::map<std::string, fs::path> country_configs;
        for (const auto& entry : fs::directory_iterator(config_dir)) {
            if (protocol == "OpenVPN" && entry.path().extension() == ".ovpn") {
                std::string country = entry.path().stem().string();
                country_configs[country] = entry.path();
            }
            if (protocol == "WireGuard" && entry.path().extension() == ".conf") {
                std::string country = entry.path().stem().string();
                country_configs[country] = entry.path();
            }
        }

        if (country_configs.empty()) {
            std::cout << "No " << protocol << " config files found in " << config_dir << std::endl;
            continue;
        }

        // List available countries
        std::vector<std::string> countries;
        std::cout << "Available countries:" << std::endl;
        int idx = 1;
        for (const auto& pair : country_configs) {
            std::cout << idx << ". " << pair.first << std::endl;
            countries.push_back(pair.first);
            ++idx;
        }

        std::cout << "Select country [1-" << countries.size() << "]: ";
        size_t country_idx;
        std::cin >> country_idx;
        std::cin.ignore();
        if (country_idx < 1 || country_idx > countries.size()) {
            std::cout << "Invalid selection." << std::endl;
            continue;
        }
        std::string picked_country = countries[country_idx - 1];
        std::string cfg_path = country_configs[picked_country].string();

        std::cout << "Connecting to server in " << picked_country << "..." << std::endl;
        log("Connecting to " + picked_country);

        if (protocol == "OpenVPN") {
            child_pid = fork();
            if (child_pid == 0) {
                execlp("sudo", "sudo", "openvpn", "--config", cfg_path.c_str(), (char*) nullptr);
                std::cerr << "Failed to exec openvpn!" << std::endl;
                std::exit(1);
            }
        } else if (protocol == "WireGuard") {
            child_pid = fork();
            if (child_pid == 0) {
                execlp("sudo", "sudo", "wg-quick", "up", cfg_path.c_str(), (char*) nullptr);
                std::cerr << "Failed to exec wg-quick!" << std::endl;
                std::exit(1);
            }
        }

        std::cout << "Connection established! Press Ctrl+C to disconnect." << std::endl;
        log(protocol + " connection established.");

        // Wait for Ctrl+C
        while (true) std::this_thread::sleep_for(std::chrono::seconds(1));

        ever exits.
    }
    return 0;
}
