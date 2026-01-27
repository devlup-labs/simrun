#include "compiler_driver.h"
#include "validator.h"
#include "profile_repository.h"
#include "profile_resolver.h"
#include "ir_serializer.h"
#include <stdexcept>

string compileIR(IR& ir) {

    // 🔴 VALIDATION STAGE
    string err = validateIR(ir);
    if (!err.empty()) {
        throw runtime_error(err);   // propagate to server
    }

    // 🟢 PROFILE RESOLUTION
    ProfileRepository repo(
        "/home/ishita-tyagi/Desktop/Compiler-Simrun/profiles"
    );
    ProfileResolver resolver(repo);
    resolver.resolve(ir);

    return writeIRToJsonFile(ir, "/tmp/final_ir.json");
}
