#include "compiler_driver.h"
#include "ir_parser.h"
#include "validator.h"
#include "profile_repository.h"
#include "profile_resolver.h"
#include "ir_serializer.h"

string compileIR(const string& rawIR, bool& ok) {

    IR ir = parseIR(rawIR);

    string err = validateIR(ir);
    if (!err.empty()) {
        ok = false;
        return err;
    }

    ProfileRepository repo("../profiles");
    ProfileResolver resolver(repo);

    resolver.resolve(ir);

    ok = true;
    return serializeIR(ir);
}
