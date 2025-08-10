#!/bin/bash

# Scale Impact Analysis for 2000+ AC-Decomp Files
# Evaluates how our identified error patterns might affect the entire codebase

echo "🔍 Evaluating Scale Impact Across 2000+ AC-Decomp Files..."
echo "============================================================"

total_c_files=$(find ../ac-decomp-upstream/src -name "*.c" | wc -l)
total_h_files=$(find ../ac-decomp-upstream/include -name "*.h" | wc -l)
echo "📊 Total Analysis Scope: $total_c_files C files + $total_h_files header files"

echo ""
echo "📈 Pattern 1: Display List Initializer Constants (HIGH IMPACT)"
echo "  → Searching for static display list arrays with gs* macros..."
gs_initializer_files=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "gs[A-Z].*{" {} \; 2>/dev/null | wc -l)
static_gfx_arrays=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "static.*Gfx.*\[\].*=" {} \; 2>/dev/null | wc -l)
echo "    📊 Files with gs* in initializers: $gs_initializer_files"
echo "    📊 Files with static Gfx arrays: $static_gfx_arrays" 
echo "    🎯 SCALE ESTIMATE: $(($gs_initializer_files + $static_gfx_arrays)) files potentially affected"

echo ""
echo "📈 Pattern 2: Vtx Structure Member Access (HIGH IMPACT)"
echo "  → Searching for vertex structure usage patterns..."
vtx_member_files=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "\.v\." {} \; 2>/dev/null | wc -l)
vtx_usage_files=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "Vtx" {} \; 2>/dev/null | wc -l)
echo "    📊 Files using .v. member access: $vtx_member_files"
echo "    📊 Files using Vtx type: $vtx_usage_files"
echo "    🎯 SCALE ESTIMATE: $vtx_member_files files directly affected"

echo ""
echo "📈 Pattern 3: Missing Graphics Constants (MEDIUM IMPACT)"  
echo "  → Searching for graphics constant usage..."
shade_usage=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "SHADE" {} \; 2>/dev/null | wc -l)
segment_usage=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "G_MWO_SEGMENT" {} \; 2>/dev/null | wc -l)
graphics_constants=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "G_[A-Z]" {} \; 2>/dev/null | wc -l)
echo "    📊 Files using SHADE constants: $shade_usage"
echo "    📊 Files using G_MWO_SEGMENT: $segment_usage"
echo "    📊 Files using G_* constants: $graphics_constants"
echo "    🎯 SCALE ESTIMATE: $graphics_constants files potentially affected"

echo ""
echo "📈 Pattern 4: Hilite Structure Access (LOW IMPACT)"
echo "  → Searching for hilite/lighting usage..."
hilite_usage=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "hilite" {} \; 2>/dev/null | wc -l)
lookat_usage=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "LookAt" {} \; 2>/dev/null | wc -l)
echo "    📊 Files using hilite: $hilite_usage"
echo "    📊 Files using LookAt: $lookat_usage"
echo "    🎯 SCALE ESTIMATE: $hilite_usage files potentially affected"

echo ""
echo "🔍 Additional Pipeline Pattern Detection..."
echo "  → Math function usage (potential math.h conflicts)..."
math_funcs=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "sqrtf\|fabsf\|sinf\|cosf\|tanf" {} \; 2>/dev/null | wc -l)
echo "    📊 Files using math functions: $math_funcs"

echo "  → Array assignment patterns (memcpy candidates)..."
array_assignments=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l " = \*[a-zA-Z]" {} \; 2>/dev/null | wc -l)
echo "    📊 Files with potential array assignments: $array_assignments"

echo "  → Function pointer patterns..."
func_ptrs=$(find ../ac-decomp-upstream/src -name "*.c" -exec grep -l "\*[a-zA-Z_][a-zA-Z0-9_]*)" {} \; 2>/dev/null | wc -l)
echo "    📊 Files with function pointers: $func_ptrs"

echo ""
echo "🎯 TOTAL SCALE IMPACT SUMMARY"
echo "============================================================"
total_estimated_impact=$(($gs_initializer_files + $static_gfx_arrays + $vtx_member_files + $graphics_constants + $hilite_usage))
percentage=$(echo "scale=1; $total_estimated_impact * 100 / $total_c_files" | bc -l)

echo "📊 Files likely to need pipeline fixes: $total_estimated_impact out of $total_c_files"
echo "📈 Estimated impact percentage: ${percentage}% of AC-Decomp codebase"
echo ""
echo "🚨 HIGH PRIORITY PIPELINE FIXES NEEDED:"
echo "  1. Display list initializer macros: ~$(($gs_initializer_files + $static_gfx_arrays)) files"
echo "  2. Vtx structure definition: ~$vtx_member_files files"  
echo "  3. Graphics constants: ~$graphics_constants files"
echo "  4. Math function conflicts: ~$math_funcs files"
echo "  5. Array assignments: ~$array_assignments files"
echo ""
echo "💡 Our pipeline methodology can systematically address these at scale!" 