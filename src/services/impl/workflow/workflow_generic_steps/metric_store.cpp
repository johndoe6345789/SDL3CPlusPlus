#include "services/interfaces/workflow/workflow_generic_steps/metric_store.hpp"

namespace sdl3cpp::services::impl {

double AggregateDebugMetric(const DebugMetricData& data,
                            DebugMetricAggregation type) {
    if (data.values.empty()) {
        return 0.0;
    }

    switch (type) {
        case DebugMetricAggregation::MIN:
            return data.minValue;
        case DebugMetricAggregation::MAX:
            return data.maxValue;
        case DebugMetricAggregation::SUM:
            return data.sumValue;
        case DebugMetricAggregation::COUNT:
            return static_cast<double>(data.recordCount);
        case DebugMetricAggregation::AVG:
            if (data.recordCount == 0) return 0.0;
            return data.sumValue / static_cast<double>(data.recordCount);
    }
    return 0.0;
}

void RecordDebugMetric(DebugMetricData& data, double value) {
    data.values.push_back(value);
    data.recordCount++;
    data.sumValue += value;

    if (data.recordCount == 1) {
        data.minValue = value;
        data.maxValue = value;
    } else {
        if (value < data.minValue) data.minValue = value;
        if (value > data.maxValue) data.maxValue = value;
    }
}

std::map<std::string, DebugMetricData>& DebugMetricsStore() {
    static std::map<std::string, DebugMetricData> store;
    return store;
}

}  // namespace sdl3cpp::services::impl
