#include "simrun/ir/json_ir_parser.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "simrun/behavior/opcodes.h"
#include "simrun/core/enums.h"

namespace simrun {
namespace {

struct JsonValue {
    enum class Type {
        kNull,
        kBool,
        kInteger,
        kString,
        kArray,
        kObject,
    };

    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue>;

    Type type = Type::kNull;
    bool bool_value = false;
    std::int64_t integer_value = 0;
    std::string string_value{};
    Array array_value{};
    Object object_value{};
};

class JsonParser {
public:
    explicit JsonParser(std::string_view input) : input_(input) {}

    JsonValue Parse() {
        SkipWhitespace();
        JsonValue value = ParseValue();
        SkipWhitespace();
        if (!IsAtEnd()) {
            throw std::invalid_argument("unexpected trailing characters in JSON");
        }
        return value;
    }

private:
    [[nodiscard]] bool IsAtEnd() const noexcept {
        return pos_ >= input_.size();
    }

    [[nodiscard]] char CurrentChar() const {
        if (IsAtEnd()) {
            throw std::invalid_argument("unexpected end of JSON input");
        }
        return input_[pos_];
    }

    void SkipWhitespace() {
        while (!IsAtEnd()) {
            const unsigned char ch = static_cast<unsigned char>(input_[pos_]);
            if (!std::isspace(ch)) {
                break;
            }
            pos_ += 1;
        }
    }

    [[nodiscard]] bool Consume(char expected) {
        if (!IsAtEnd() && input_[pos_] == expected) {
            pos_ += 1;
            return true;
        }
        return false;
    }

    void Expect(char expected, const char* message) {
        if (!Consume(expected)) {
            throw std::invalid_argument(message);
        }
    }

    JsonValue ParseValue() {
        SkipWhitespace();
        if (IsAtEnd()) {
            throw std::invalid_argument("unexpected end while parsing value");
        }

        const char ch = CurrentChar();
        if (ch == '{') {
            return ParseObject();
        }
        if (ch == '[') {
            return ParseArray();
        }
        if (ch == '"') {
            JsonValue value{};
            value.type = JsonValue::Type::kString;
            value.string_value = ParseString();
            return value;
        }
        if (ch == '-' || (ch >= '0' && ch <= '9')) {
            JsonValue value{};
            value.type = JsonValue::Type::kInteger;
            value.integer_value = ParseInteger();
            return value;
        }
        if (StartsWith("true")) {
            pos_ += 4;
            JsonValue value{};
            value.type = JsonValue::Type::kBool;
            value.bool_value = true;
            return value;
        }
        if (StartsWith("false")) {
            pos_ += 5;
            JsonValue value{};
            value.type = JsonValue::Type::kBool;
            value.bool_value = false;
            return value;
        }
        if (StartsWith("null")) {
            pos_ += 4;
            JsonValue value{};
            value.type = JsonValue::Type::kNull;
            return value;
        }

        throw std::invalid_argument("invalid JSON token");
    }

    JsonValue ParseObject() {
        Expect('{', "expected '{' for JSON object");
        JsonValue value{};
        value.type = JsonValue::Type::kObject;

        SkipWhitespace();
        if (Consume('}')) {
            return value;
        }

        while (true) {
            SkipWhitespace();
            if (CurrentChar() != '"') {
                throw std::invalid_argument("expected string key in JSON object");
            }
            const std::string key = ParseString();

            SkipWhitespace();
            Expect(':', "expected ':' after JSON object key");
            SkipWhitespace();
            JsonValue field_value = ParseValue();
            value.object_value.insert_or_assign(key, std::move(field_value));

            SkipWhitespace();
            if (Consume('}')) {
                break;
            }
            Expect(',', "expected ',' between JSON object fields");
        }

        return value;
    }

    JsonValue ParseArray() {
        Expect('[', "expected '[' for JSON array");
        JsonValue value{};
        value.type = JsonValue::Type::kArray;

        SkipWhitespace();
        if (Consume(']')) {
            return value;
        }

        while (true) {
            SkipWhitespace();
            value.array_value.push_back(ParseValue());
            SkipWhitespace();
            if (Consume(']')) {
                break;
            }
            Expect(',', "expected ',' between JSON array elements");
        }

        return value;
    }

    std::string ParseString() {
        Expect('"', "expected '\"' to start JSON string");
        std::string output{};

        while (true) {
            if (IsAtEnd()) {
                throw std::invalid_argument("unterminated JSON string");
            }
            const char ch = input_[pos_++];
            if (ch == '"') {
                break;
            }
            if (ch == '\\') {
                if (IsAtEnd()) {
                    throw std::invalid_argument("unterminated JSON escape");
                }
                const char escaped = input_[pos_++];
                switch (escaped) {
                    case '"':
                        output.push_back('"');
                        break;
                    case '\\':
                        output.push_back('\\');
                        break;
                    case '/':
                        output.push_back('/');
                        break;
                    case 'b':
                        output.push_back('\b');
                        break;
                    case 'f':
                        output.push_back('\f');
                        break;
                    case 'n':
                        output.push_back('\n');
                        break;
                    case 'r':
                        output.push_back('\r');
                        break;
                    case 't':
                        output.push_back('\t');
                        break;
                    case 'u':
                        ParseUnicodeEscape(&output);
                        break;
                    default:
                        throw std::invalid_argument("unsupported JSON escape sequence");
                }
                continue;
            }
            output.push_back(ch);
        }

        return output;
    }

    std::int64_t ParseInteger() {
        const std::size_t start = pos_;
        if (Consume('-')) {
            // sign consumed
        }
        if (IsAtEnd()) {
            throw std::invalid_argument("invalid JSON number");
        }
        if (CurrentChar() == '0') {
            pos_ += 1;
        } else {
            if (!std::isdigit(static_cast<unsigned char>(CurrentChar()))) {
                throw std::invalid_argument("invalid JSON number");
            }
            while (!IsAtEnd() && std::isdigit(static_cast<unsigned char>(CurrentChar()))) {
                pos_ += 1;
            }
        }

        if (!IsAtEnd() && (CurrentChar() == '.' || CurrentChar() == 'e' || CurrentChar() == 'E')) {
            throw std::invalid_argument("floating-point numbers are not supported in IR JSON");
        }

        const std::string token(input_.substr(start, pos_ - start));
        std::size_t parsed = 0;
        const long long parsed_value = std::stoll(token, &parsed, 10);
        if (parsed != token.size()) {
            throw std::invalid_argument("invalid integer in IR JSON");
        }
        if (parsed_value < std::numeric_limits<std::int64_t>::min() ||
            parsed_value > std::numeric_limits<std::int64_t>::max()) {
            throw std::invalid_argument("integer out of int64 range in IR JSON");
        }
        return static_cast<std::int64_t>(parsed_value);
    }

    [[nodiscard]] bool StartsWith(const std::string_view token) const noexcept {
        if (input_.size() - pos_ < token.size()) {
            return false;
        }
        return input_.substr(pos_, token.size()) == token;
    }

    void ParseUnicodeEscape(std::string* output) {
        if (pos_ + 4 > input_.size()) {
            throw std::invalid_argument("invalid \\u escape in JSON string");
        }
        std::uint32_t codepoint = 0;
        for (std::size_t i = 0; i < 4; ++i) {
            const char hex = input_[pos_++];
            codepoint <<= 4;
            if (hex >= '0' && hex <= '9') {
                codepoint |= static_cast<std::uint32_t>(hex - '0');
            } else if (hex >= 'a' && hex <= 'f') {
                codepoint |= static_cast<std::uint32_t>(10 + (hex - 'a'));
            } else if (hex >= 'A' && hex <= 'F') {
                codepoint |= static_cast<std::uint32_t>(10 + (hex - 'A'));
            } else {
                throw std::invalid_argument("invalid hex digit in \\u JSON escape");
            }
        }

        if (codepoint <= 0x7F) {
            output->push_back(static_cast<char>(codepoint));
            return;
        }
        // IR JSON is ASCII-oriented; preserve non-ASCII codepoints as '?'
        output->push_back('?');
    }

    std::string_view input_{};
    std::size_t pos_ = 0;
};

const JsonValue& RequireField(const JsonValue::Object& object, const std::string_view key) {
    const auto it = object.find(std::string(key));
    if (it == object.end()) {
        throw std::invalid_argument("missing required field in IR JSON: " + std::string(key));
    }
    return it->second;
}

const JsonValue* OptionalField(const JsonValue::Object& object, const std::string_view key) {
    const auto it = object.find(std::string(key));
    if (it == object.end()) {
        return nullptr;
    }
    return &it->second;
}

const JsonValue::Object& AsObject(const JsonValue& value, const std::string_view context) {
    if (value.type != JsonValue::Type::kObject) {
        throw std::invalid_argument("expected JSON object for " + std::string(context));
    }
    return value.object_value;
}

const JsonValue::Array& AsArray(const JsonValue& value, const std::string_view context) {
    if (value.type != JsonValue::Type::kArray) {
        throw std::invalid_argument("expected JSON array for " + std::string(context));
    }
    return value.array_value;
}

std::int64_t AsInt64(const JsonValue& value, const std::string_view context) {
    if (value.type != JsonValue::Type::kInteger) {
        throw std::invalid_argument("expected integer JSON value for " + std::string(context));
    }
    return value.integer_value;
}

std::string AsString(const JsonValue& value, const std::string_view context) {
    if (value.type != JsonValue::Type::kString) {
        throw std::invalid_argument("expected string JSON value for " + std::string(context));
    }
    return value.string_value;
}

QueueDiscipline ParseQueueDiscipline(const JsonValue& value) {
    if (value.type == JsonValue::Type::kInteger) {
        if (value.integer_value == 0) {
            return QueueDiscipline::kFifo;
        }
    }
    const std::string token = AsString(value, "queue discipline");
    if (token == "Fifo" || token == "kFifo") {
        return QueueDiscipline::kFifo;
    }
    throw std::invalid_argument("unsupported queue discipline in IR JSON: " + token);
}

EventPhase ParseEventPhase(const JsonValue& value) {
    if (value.type == JsonValue::Type::kInteger) {
        const std::int64_t raw = value.integer_value;
        if (raw >= 0 && raw <= 4) {
            return static_cast<EventPhase>(raw);
        }
    }
    const std::string token = AsString(value, "event phase");
    if (token == "RequestArrival" || token == "kRequestArrival") {
        return EventPhase::kRequestArrival;
    }
    if (token == "ServiceCompletion" || token == "kServiceCompletion") {
        return EventPhase::kServiceCompletion;
    }
    if (token == "TimerExpiration" || token == "kTimerExpiration") {
        return EventPhase::kTimerExpiration;
    }
    if (token == "ResponseArrival" || token == "kResponseArrival") {
        return EventPhase::kResponseArrival;
    }
    if (token == "WorkloadGeneration" || token == "kWorkloadGeneration") {
        return EventPhase::kWorkloadGeneration;
    }
    throw std::invalid_argument("unsupported event phase in IR JSON: " + token);
}

OpCode ParseOpCode(const JsonValue& value) {
    if (value.type == JsonValue::Type::kInteger) {
        const std::int64_t raw = value.integer_value;
        if (raw >= 0 && raw <= static_cast<std::int64_t>(OpCode::kPreemptCurrent)) {
            return static_cast<OpCode>(raw);
        }
    }
    const std::string token = AsString(value, "opcode");
    if (token == "Halt" || token == "kHalt") {
        return OpCode::kHalt;
    }
    if (token == "SetFlag" || token == "kSetFlag") {
        return OpCode::kSetFlag;
    }
    if (token == "IncrementHop" || token == "kIncrementHop") {
        return OpCode::kIncrementHop;
    }
    if (token == "WritePayloadWord" || token == "kWritePayloadWord") {
        return OpCode::kWritePayloadWord;
    }
    if (token == "AddComponentStateWord" || token == "kAddComponentStateWord") {
        return OpCode::kAddComponentStateWord;
    }
    if (token == "ScheduleTimer" || token == "kScheduleTimer") {
        return OpCode::kScheduleTimer;
    }
    if (token == "SpawnRequest" || token == "kSpawnRequest") {
        return OpCode::kSpawnRequest;
    }
    if (token == "InvalidateToken" || token == "kInvalidateToken") {
        return OpCode::kInvalidateToken;
    }
    if (token == "RouteNextLink" || token == "kRouteNextLink") {
        return OpCode::kRouteNextLink;
    }
    if (token == "Log" || token == "kLog") {
        return OpCode::kLog;
    }
    if (token == "PreemptCurrent" || token == "kPreemptCurrent") {
        return OpCode::kPreemptCurrent;
    }
    throw std::invalid_argument("unsupported opcode in IR JSON: " + token);
}

std::int32_t ToInt32(std::int64_t value, const std::string& field_name) {
    if (value < std::numeric_limits<std::int32_t>::min() || value > std::numeric_limits<std::int32_t>::max()) {
        throw std::invalid_argument("integer out of int32 range for field: " + field_name);
    }
    return static_cast<std::int32_t>(value);
}

std::int16_t ToInt16(std::int64_t value, const std::string& field_name) {
    if (value < std::numeric_limits<std::int16_t>::min() || value > std::numeric_limits<std::int16_t>::max()) {
        throw std::invalid_argument("integer out of int16 range for field: " + field_name);
    }
    return static_cast<std::int16_t>(value);
}

GlobalParameters ParseGlobalParameters(const JsonValue::Object& root) {
    const JsonValue::Object& source = AsObject(RequireField(root, "global_parameters"), "global_parameters");
    return GlobalParameters{
        .time_unit = AsInt64(RequireField(source, "time_unit"), "global_parameters.time_unit"),
        .horizon = AsInt64(RequireField(source, "horizon"), "global_parameters.horizon"),
        .seed = static_cast<std::uint64_t>(AsInt64(RequireField(source, "seed"), "global_parameters.seed")),
        .max_requests = ToInt32(AsInt64(RequireField(source, "max_requests"), "global_parameters.max_requests"), "global_parameters.max_requests"),
        .payload_words = ToInt32(AsInt64(RequireField(source, "payload_words"), "global_parameters.payload_words"), "global_parameters.payload_words"),
        .max_tokens = ToInt32(AsInt64(RequireField(source, "max_tokens"), "global_parameters.max_tokens"), "global_parameters.max_tokens"),
    };
}

template <typename T, typename ParseFn>
std::vector<T> ParseVectorField(const JsonValue::Object& root, const std::string& field_name, ParseFn parse_fn) {
    const JsonValue* field = OptionalField(root, field_name);
    if (field == nullptr) {
        return {};
    }
    const JsonValue::Array& array = AsArray(*field, field_name);
    std::vector<T> result{};
    result.reserve(array.size());
    for (std::size_t i = 0; i < array.size(); ++i) {
        result.push_back(parse_fn(array[i], i));
    }
    return result;
}

}  // namespace

SimulationIR ParseSimulationIRJson(const std::string_view json_text) {
    const JsonValue root_value = JsonParser(json_text).Parse();
    const JsonValue::Object& root = AsObject(root_value, "root");

    SimulationIR ir{};
    ir.global_parameters = ParseGlobalParameters(root);

    ir.components = ParseVectorField<ComponentSpec>(root, "components", [](const JsonValue& node, std::size_t) {
        const JsonValue::Object& source = AsObject(node, "components[]");
        return ComponentSpec{
            .id = ToInt32(AsInt64(RequireField(source, "id"), "components[].id"), "components[].id"),
            .capacity = ToInt32(AsInt64(RequireField(source, "capacity"), "components[].capacity"), "components[].capacity"),
            .state_words = ToInt32(AsInt64(RequireField(source, "state_words"), "components[].state_words"), "components[].state_words"),
            .waiting_queue_capacity = ToInt32(AsInt64(RequireField(source, "waiting_queue_capacity"), "components[].waiting_queue_capacity"), "components[].waiting_queue_capacity"),
            .service_time = AsInt64(RequireField(source, "service_time"), "components[].service_time"),
            .discipline = ParseQueueDiscipline(RequireField(source, "discipline")),
        };
    });

    ir.links = ParseVectorField<LinkSpec>(root, "links", [](const JsonValue& node, std::size_t) {
        const JsonValue::Object& source = AsObject(node, "links[]");
        return LinkSpec{
            .id = ToInt32(AsInt64(RequireField(source, "id"), "links[].id"), "links[].id"),
            .src_component_id = ToInt32(AsInt64(RequireField(source, "src_component_id"), "links[].src_component_id"), "links[].src_component_id"),
            .dst_component_id = ToInt32(AsInt64(RequireField(source, "dst_component_id"), "links[].dst_component_id"), "links[].dst_component_id"),
            .propagation_delay = AsInt64(RequireField(source, "propagation_delay"), "links[].propagation_delay"),
            .bandwidth = ToInt32(AsInt64(RequireField(source, "bandwidth"), "links[].bandwidth"), "links[].bandwidth"),
        };
    });

    ir.request_types = ParseVectorField<RequestTypeSpec>(root, "request_types", [](const JsonValue& node, std::size_t) {
        const JsonValue::Object& source = AsObject(node, "request_types[]");
        return RequestTypeSpec{
            .id = ToInt32(AsInt64(RequireField(source, "id"), "request_types[].id"), "request_types[].id"),
            .flag_count = ToInt16(AsInt64(RequireField(source, "flag_count"), "request_types[].flag_count"), "request_types[].flag_count"),
            .max_hops = ToInt16(AsInt64(RequireField(source, "max_hops"), "request_types[].max_hops"), "request_types[].max_hops"),
        };
    });

    ir.routing_entries = ParseVectorField<RoutingEntry>(root, "routing_entries", [](const JsonValue& node, std::size_t) {
        const JsonValue::Object& source = AsObject(node, "routing_entries[]");
        return RoutingEntry{
            .request_type = ToInt32(AsInt64(RequireField(source, "request_type"), "routing_entries[].request_type"), "routing_entries[].request_type"),
            .flag = ToInt16(AsInt64(RequireField(source, "flag"), "routing_entries[].flag"), "routing_entries[].flag"),
            .hop = ToInt16(AsInt64(RequireField(source, "hop"), "routing_entries[].hop"), "routing_entries[].hop"),
            .link_id = ToInt32(AsInt64(RequireField(source, "link_id"), "routing_entries[].link_id"), "routing_entries[].link_id"),
        };
    });

    ir.behavior_bindings = ParseVectorField<BehaviorBinding>(root, "behavior_bindings", [](const JsonValue& node, std::size_t) {
        const JsonValue::Object& source = AsObject(node, "behavior_bindings[]");
        return BehaviorBinding{
            .component_id = ToInt32(AsInt64(RequireField(source, "component_id"), "behavior_bindings[].component_id"), "behavior_bindings[].component_id"),
            .request_type = ToInt32(AsInt64(RequireField(source, "request_type"), "behavior_bindings[].request_type"), "behavior_bindings[].request_type"),
            .flag = ToInt16(AsInt64(RequireField(source, "flag"), "behavior_bindings[].flag"), "behavior_bindings[].flag"),
            .phase = ParseEventPhase(RequireField(source, "phase")),
            .instruction_pointer = ToInt32(AsInt64(RequireField(source, "instruction_pointer"), "behavior_bindings[].instruction_pointer"), "behavior_bindings[].instruction_pointer"),
        };
    });

    ir.opcode_stream = ParseVectorField<Instruction>(root, "opcode_stream", [](const JsonValue& node, std::size_t) {
        const JsonValue::Object& source = AsObject(node, "opcode_stream[]");
        Instruction instruction{};
        instruction.opcode = ParseOpCode(RequireField(source, "opcode"));
        const JsonValue* args_value = OptionalField(source, "args");
        if (args_value != nullptr) {
            const JsonValue::Array& args = AsArray(*args_value, "opcode_stream[].args");
            const std::size_t arg_count = args.size() > instruction.args.size() ? instruction.args.size() : args.size();
            for (std::size_t i = 0; i < arg_count; ++i) {
                instruction.args[i] = AsInt64(args[i], "opcode_stream[].args[]");
            }
        }
        return instruction;
    });

    ir.workloads = ParseVectorField<WorkloadDefinition>(root, "workloads", [](const JsonValue& node, std::size_t) {
        const JsonValue::Object& source = AsObject(node, "workloads[]");
        return WorkloadDefinition{
            .request_type = ToInt32(AsInt64(RequireField(source, "request_type"), "workloads[].request_type"), "workloads[].request_type"),
            .initial_flag = ToInt16(AsInt64(RequireField(source, "initial_flag"), "workloads[].initial_flag"), "workloads[].initial_flag"),
            .entry_component_id = ToInt32(AsInt64(RequireField(source, "entry_component_id"), "workloads[].entry_component_id"), "workloads[].entry_component_id"),
            .start_time = AsInt64(RequireField(source, "start_time"), "workloads[].start_time"),
            .interval = AsInt64(RequireField(source, "interval"), "workloads[].interval"),
            .count = ToInt32(AsInt64(RequireField(source, "count"), "workloads[].count"), "workloads[].count"),
        };
    });

    ir.bootstrap_events = ParseVectorField<BootstrapEventSpec>(root, "bootstrap_events", [](const JsonValue& node, std::size_t) {
        const JsonValue::Object& source = AsObject(node, "bootstrap_events[]");
        return BootstrapEventSpec{
            .time = AsInt64(RequireField(source, "time"), "bootstrap_events[].time"),
            .request_type = ToInt32(AsInt64(RequireField(source, "request_type"), "bootstrap_events[].request_type"), "bootstrap_events[].request_type"),
            .flag = ToInt16(AsInt64(RequireField(source, "flag"), "bootstrap_events[].flag"), "bootstrap_events[].flag"),
            .component_id = ToInt32(AsInt64(RequireField(source, "component_id"), "bootstrap_events[].component_id"), "bootstrap_events[].component_id"),
            .phase = ParseEventPhase(RequireField(source, "phase")),
        };
    });

    return ir;
}

SimulationIR LoadSimulationIRFromJsonFile(const std::string& path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in.is_open()) {
        throw std::invalid_argument("failed to open IR JSON file: " + path);
    }
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        text.erase(0, 3);
    }
    return ParseSimulationIRJson(text);
}

}  // namespace simrun
