#include "gldapi.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <nlohmann/json.hpp>
#include "timestamp.h"

// Helper function to get filename without path and without extension
std::string get_base_filename(const std::string& filepath) {
    // Get filename without path
    size_t pos = filepath.find_last_of("/\\");
    std::string filename = (pos == std::string::npos) ? filepath : filepath.substr(pos + 1);
    
    // Remove extension
    size_t dot = filename.find_last_of('.');
    if (dot != std::string::npos) {
        filename = filename.substr(0, dot);
    }
    return filename;
}

int main(int argc, char* argv[]) {
    std::string fileName = argv[1];
    GridLabD gld;
    
    // Parse flags
    bool checkpoint_mode = false;
    bool restore_mode = false;
    int num_steps = 2;
    std::vector<std::string> extra_gld_args;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--checkpoint") {
            checkpoint_mode = true;
        } else if (arg == "--restore") {
            restore_mode = true;
        } else if (arg == "--steps" && i + 1 < argc) {
            num_steps = std::stoi(argv[++i]);
        } else {
            extra_gld_args.push_back(arg);
        }
    }

    // Load GLM/JSON file
    if(restore_mode) {
        fileName = get_base_filename(fileName) + "_checkpoint.json";
    }

    // Build argv: slot 0 = nullptr (program name), slot 1 = fileName, then extra args
    std::vector<const char*> gld_argv;
    gld_argv.push_back(nullptr);
    gld_argv.push_back(fileName.c_str());
    for (const auto &a : extra_gld_args)
        gld_argv.push_back(a.c_str());
    int test_argc = static_cast<int>(gld_argv.size());

    try{
        gld.load_glm(test_argc, const_cast<char**>(gld_argv.data()));
    } catch (const std::exception& e) {
        std::cerr << "Error loading GLM: " << e.what() << std::endl;
        return 1;
    }

    if (checkpoint_mode) {
        // Run N steps and save checkpoint
        double sim_time;
        for (int i = 0; i < num_steps; i++) {
            gld.step(sim_time);
        }
        printf("Completed %d steps. Simulation time: %.2f\n", num_steps, sim_time);
        
        // Get checkpoint and save it
        nlohmann::json checkpoint = gld.get_checkpoint_json();
        printf("Checkpoint saved.\n");
    }
    else if (restore_mode) {
        printf("Checkpoint loaded.\n");
        std::cout << gld.gld_model.dump(4) << std::endl; // Pretty print with 4-space indent
        gld.run();
        nlohmann::json checkpoint = gld.get_checkpoint_json();
        printf("Checkpoint saved.\n");
    }
    else {
        // Default: just run to completion
        gld.run();
        nlohmann::json checkpoint = gld.get_checkpoint_json();
    }
    
    // Exit
    gld.exit_gld(fileName.c_str());
    
    return 0;
}
