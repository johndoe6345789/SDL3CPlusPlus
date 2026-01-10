#pragma once

#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services {

struct WorkflowParameterValue {
    enum class Type {
        String,
        Number,
        Bool,
        StringList,
        NumberList
    };

    Type type = Type::String;
    std::string stringValue;
    double numberValue = 0.0;
    bool boolValue = false;
    std::vector<std::string> stringList;
    std::vector<double> numberList;

    static WorkflowParameterValue FromString(std::string value) {
        WorkflowParameterValue param;
        param.type = Type::String;
        param.stringValue = std::move(value);
        return param;
    }

    static WorkflowParameterValue FromNumber(double value) {
        WorkflowParameterValue param;
        param.type = Type::Number;
        param.numberValue = value;
        return param;
    }

    static WorkflowParameterValue FromBool(bool value) {
        WorkflowParameterValue param;
        param.type = Type::Bool;
        param.boolValue = value;
        return param;
    }

    static WorkflowParameterValue FromStringList(std::vector<std::string> value) {
        WorkflowParameterValue param;
        param.type = Type::StringList;
        param.stringList = std::move(value);
        return param;
    }

    static WorkflowParameterValue FromNumberList(std::vector<double> value) {
        WorkflowParameterValue param;
        param.type = Type::NumberList;
        param.numberList = std::move(value);
        return param;
    }
};

}  // namespace sdl3cpp::services
