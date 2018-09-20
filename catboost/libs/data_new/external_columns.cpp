#include "external_columns.h"

#include <catboost/libs/helpers/exception.h>
#include <catboost/libs/quantization/utils.h>


namespace NCB {

    NCB::TMaybeOwningArrayHolder<ui8> TExternalFloatValuesHolder::ExtractValues(
        NPar::TLocalExecutor* localExecutor
    ) const {
        TVector<ui8> result;
        result.yresize(GetSize());

        const auto& borders = FeaturesManager->GetBorders(FeatureManagerFeatureId);
        const auto nanMode = FeaturesManager->GetNanMode(FeatureManagerFeatureId);

        TConstMaybeOwningArraySubset<float, ui32>(&SrcData, SubsetIndexing).ParallelForEach(
            [&] (ui32 idx, float srcValue) { result[idx] = Binarize<ui8>(nanMode, borders, srcValue); },
            localExecutor,
            BINARIZATION_BLOCK_SIZE
        );

        return NCB::TMaybeOwningArrayHolder<ui8>::CreateOwning(std::move(result));
    }


    NCB::TMaybeOwningArrayHolder<ui32> TExternalCatValuesHolder::ExtractValues(
        NPar::TLocalExecutor* localExecutor
    ) const {
        TVector<ui32> result;
        result.yresize(GetSize());

        const auto& perfectHash = FeaturesManager->GetCategoricalFeaturesPerfectHash(
            FeatureManagerFeatureId
        );

        TConstMaybeOwningArraySubset<ui32, ui32>(&SrcData, SubsetIndexing).ParallelForEach(
            [&] (ui32 idx, ui32 srcValue) {
                auto it = perfectHash.find(srcValue); // find is guaranteed to be thread-safe

                // TODO(akhropov): replace by assert for performance?
                CB_ENSURE(it != perfectHash.end(),
                          "Error: hash for feature #" << FeatureManagerFeatureId << " was not found "
                          << srcValue);

                result[idx] = it->second;
            },
            localExecutor,
            BINARIZATION_BLOCK_SIZE
        );

        return NCB::TMaybeOwningArrayHolder<ui32>::CreateOwning(std::move(result));
    }

}
