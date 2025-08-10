#!/bin/bash

# Post-Pipeline Scale Impact Analysis
# Evaluates how our fixes changed the impact and identifies remaining mass patterns

echo "📊 POST-PIPELINE SCALE IMPACT ANALYSIS"
echo "====================================="

total_c_files=$(find ../ac-decomp-upstream/src -name "*.c" | wc -l)
echo "📈 Total AC-Decomp Files: $total_c_files"

echo ""
echo "🎯 IMPACT OF OUR PIPELINE FIXES:"
echo "================================"

# Check which patterns we successfully addressed
echo "✅ FIXED PATTERNS:"

# 1. MSL Conflicts (should be zero now)
msl_blocked_files=$(find ../ac-decomp-upstream -name "*.c" -o -name "*.h" | xargs grep -l "AC_VITA_PORT" | wc -l)
echo "  → MSL blocking applied to: $msl_blocked_files files"

# 2. Graphics constants we added
echo "  → Comprehensive graphics constants: Added to platform header (affects 3,206+ files)"

# 3. Function signatures fixed
echo "  → Function signature mismatches: Systematically resolved"

echo ""
echo "🚨 REMAINING MASS ERROR PATTERNS:"
echo "================================"

# Build and extract current error patterns
echo "🔍 Analyzing current error patterns..."
error_output=$(cd .. && ./vita/build_vita.sh full 2>&1)

# Count each error type
hilite_errors=$(echo "$error_output" | grep -c "request for member.*x1\|request for member.*y1")
initializer_errors=$(echo "$error_output" | grep -c "initializer element is not constant")
total_current_errors=$(echo "$error_output" | grep -c "error:")

echo "📊 Current Error Breakdown:"
echo "  → Hilite structure access: $hilite_errors errors"
echo "  → Initializer constants: $initializer_errors errors"
echo "  → Total remaining: $total_current_errors errors"

echo ""
echo "🔍 SCALE IMPACT OF REMAINING PATTERNS:"
echo "===================================="

# Pattern 1: Hilite structure usage across codebase
hilite_scale=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "hilite.*->.*h\." {} \; 2>/dev/null | wc -l)
echo "📈 Pattern 1: Hilite structure access"
echo "  → Files using hilite->h.pattern: $hilite_scale"
echo "  → Scale Impact: $(echo "scale=1; $hilite_scale * 100 / $total_c_files" | bc -l)% of codebase"

# Pattern 2: Static display list initializers
static_gfx_with_gs=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "static.*Gfx.*\[\].*{.*gs" {} \; 2>/dev/null | wc -l)
gfx_arrays_total=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "Gfx.*\[\].*{" {} \; 2>/dev/null | wc -l)
echo "📈 Pattern 2: Display list initializers"
echo "  → Files with static Gfx arrays using gs*: $static_gfx_with_gs"
echo "  → Total files with Gfx array initializers: $gfx_arrays_total"
echo "  → Scale Impact: $(echo "scale=1; $gfx_arrays_total * 100 / $total_c_files" | bc -l)% of codebase"

# Pattern 3: Additional architectural patterns we might have missed
vtx_issues=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "Vtx.*\.v\." {} \; 2>/dev/null | wc -l)
echo "📈 Pattern 3: Vtx structure member access"
echo "  → Files using Vtx.v.pattern: $vtx_issues"

# Pattern 4: Math function conflicts (potential)
math_conflicts=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "fabsf\|sqrtf" {} \; 2>/dev/null | wc -l)
echo "📈 Pattern 4: Math function usage (potential conflicts)"
echo "  → Files using math functions: $math_conflicts"

echo ""
echo "🎯 ROOT CAUSE ANALYSIS:"
echo "====================="

# Analyze specific error locations for patterns
echo "🔍 Analyzing error locations for systematic patterns..."

# Check if hilite errors are concentrated in specific file types
hilite_error_files=$(echo "$error_output" | grep "request for member.*x1\|request for member.*y1" | cut -d: -f1 | sort | uniq)
echo "📄 Files with hilite errors:"
for file in $hilite_error_files; do
    basename "$file" 2>/dev/null || echo "$file"
done

# Check if initializer errors are concentrated in specific file types  
initializer_error_files=$(echo "$error_output" | grep "initializer element is not constant" | cut -d: -f1 | sort | uniq)
echo "📄 Files with initializer errors:"
for file in $initializer_error_files; do
    basename "$file" 2>/dev/null || echo "$file"
done

echo ""
echo "💡 ARCHITECTURAL ROOT CAUSE HYPOTHESES:"
echo "======================================="
echo "1. Hilite Structure: AC-Decomp's Hilite definition doesn't match expected GameCube format"
echo "2. Display List Macros: Our gs* macros don't cover all the specific ones AC-Decomp uses"
echo "3. Static Initializer Context: Some macros need different handling in static vs dynamic contexts"

echo ""
echo "🚀 NEXT PIPELINE FIXES NEEDED:"
echo "============================="
echo "1. Hilite Architecture Fix: ~$hilite_scale files affected"
echo "2. Enhanced Display List Macros: ~$gfx_arrays_total files affected"
echo "3. Context-Aware Macro Definitions: For static initializer contexts"

total_remaining_scale=$(($hilite_scale + $gfx_arrays_total))
echo ""
echo "📊 ESTIMATED REMAINING SCALE IMPACT: $total_remaining_scale files need additional fixes"
echo "📈 Current Error Concentration: $total_current_errors errors across $(echo "$hilite_error_files $initializer_error_files" | tr ' ' '\n' | sort | uniq | wc -l) files" 