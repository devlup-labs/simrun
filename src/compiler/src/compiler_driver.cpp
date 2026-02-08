#include "compiler_driver.h"
#include "validator.h"
#include "profile_repository.h"
#include "profile_resolver.h"
#include "ir_serializer.h"
#include <stdexcept>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;
string compileIR(IR& ir) {

    // VALIDATION STAGE
    string err = validateIR(ir);
    if (!err.empty()) {
        throw runtime_error(err);   // propagate to server
    }

    // PROFILE RESOLUTION
    std::cout << "Current working directory: " << fs::current_path().string() << std::endl;
    
    string profilesPath = "./profiles";
    if (!fs::exists(profilesPath)) {
        std::cerr << "Profiles directory not found at: " << fs::absolute(profilesPath).string() << std::endl;
        throw runtime_error("Profiles directory not found at: " + fs::absolute(profilesPath).string());
    }
    
    std::cout << "Profiles directory found at: " << fs::absolute(profilesPath).string() << std::endl;
    
    ProfileRepository repo(profilesPath);
    ProfileResolver resolver(repo);
    resolver.resolve(ir);

    return writeIRToJsonFile(ir, "./final_ir.json");
}
