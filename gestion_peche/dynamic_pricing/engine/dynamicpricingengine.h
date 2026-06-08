#ifndef DYNAMICPRICINGENGINE_H
#define DYNAMICPRICINGENGINE_H

#include "../models/dynamicpricingmodels.h"

class DynamicPricingEngine
{
public:
    DynamicPricingEngine();

    DynamicPricingRecommendation calculate(const DynamicPricingContext &context,
                                           bool manualOverrideEnabled,
                                           double manualOverridePrice) const;
};

#endif // DYNAMICPRICINGENGINE_H
