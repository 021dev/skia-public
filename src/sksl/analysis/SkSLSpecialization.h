/*
 * Copyright 2024 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKSL_SPECIALIZATION
#define SKSL_SPECIALIZATION

#include "include/private/base/SkTArray.h"
#include "src/core/SkChecksum.h"
#include "src/core/SkTHash.h"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace SkSL {

class FunctionCall;
class FunctionDeclaration;
class Variable;
struct Program;

namespace Analysis {

// The current index of the specialization function being walked through, used to
// track what the proper specialization function call should be if walking through a
// specialized function call stack.
using SpecializationIndex = int;
static constexpr SpecializationIndex kUnspecialized = -1;

// Global uniforms used by a specialization, maps <function parameter, global uniform>
using SpecializedParameters = skia_private::THashMap<const Variable*, const Variable*>;
// The set of specializated implementations needed for a given function.
using Specializations = skia_private::TArray<SpecializedParameters>;
// The full set of all specializations required by the program.
using SpecializationMap = skia_private::THashMap<const FunctionDeclaration*, Specializations>;

// This can be used as a key into a map of specialized function declarations. Most backends which
// implement function specialization will have a need for this.
struct SpecializedFunctionKey {
    struct Hash {
        size_t operator()(const SpecializedFunctionKey& entry) {
            return SkGoodHash()(entry.fDeclaration) ^
                   SkGoodHash()(entry.fSpecializationIndex);
        }
    };

    bool operator==(const SpecializedFunctionKey& other) const {
        return fDeclaration == other.fDeclaration &&
               fSpecializationIndex == other.fSpecializationIndex;
    }

    const FunctionDeclaration* fDeclaration = nullptr;
    SpecializationIndex fSpecializationIndex = Analysis::kUnspecialized;
};

// This is used as a key into the SpecializedCallMap.
struct SpecializedCallKey {
    struct Hash {
        size_t operator()(const SpecializedCallKey& entry) {
            return SkGoodHash()(entry.fStableID) ^
                   SkGoodHash()(entry.fParentSpecializationIndex);
        }
    };

    bool operator==(const SpecializedCallKey& other) const {
        return fStableID == other.fStableID &&
               fParentSpecializationIndex == other.fParentSpecializationIndex;
    }

    const uint32_t fStableID = 0;
    SpecializationIndex fParentSpecializationIndex = Analysis::kUnspecialized;
};

// The mapping of function calls and their inherited specialization to their corresponding
// specialization index in `Specializations`
using SpecializedCallMap = skia_private::THashMap<SpecializedCallKey,
                                                  SpecializationIndex,
                                                  SpecializedCallKey::Hash>;
struct SpecializationInfo {
    SpecializationMap fSpecializationMap;
    SpecializedCallMap fSpecializedCallMap;
};

// A function that returns true if the parameter variable fits the criteria
// to create a specialization.
using ParameterMatchesFn = std::function<bool(const Variable&)>;

// Finds functions that contain parameters that should be specialized on and writes the
// specialization info to the provided `SpecializationInfo`.
void FindFunctionsToSpecialize(const Program& program,
                               SpecializationInfo* info,
                               const ParameterMatchesFn& specializationFn);

// Given a function call and the active specialization index, looks up the specialization index for
// the call target. In other words: in the specialization map, we first look up the call target's
// declaration, which yields a Specialization array. We would find the correct mappings in the array
// at the SpecializationIndex returned by this function.
SpecializationIndex FindSpecializationIndexForCall(const FunctionCall& call,
                                                   const SpecializationInfo& info,
                                                   SpecializationIndex activeSpecializationIndex);

}  // namespace Analysis
}  // namespace SkSL

#endif
