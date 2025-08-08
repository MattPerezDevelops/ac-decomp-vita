#!/usr/bin/env python3
"""
⚡ Quick Essential Fixes
Adds just the essential missing functions we know we need from build errors
"""

# Essential missing functions from recent build errors
ESSENTIAL_FUNCTIONS = [
    # Display processor functions we know are missing
    ('gSPLoadUcode', 'void gSPLoadUcode(Gfx* gfx, u64* ucode, u64* ucode_data);'),
    ('gDPLoadTLUT', 'void gDPLoadTLUT(Gfx* gfx, u32 tile, u32 count, void* tlut);'),
    ('gDPSetOtherMode', 'void gDPSetOtherMode(Gfx* gfx, u32 mode0, u32 mode1);'),
    ('gDPSetCombineLERP', 'void gDPSetCombineLERP(Gfx* gfx, u32 a0, u32 b0, u32 c0, u32 d0, u32 a1, u32 b1, u32 c1, u32 d1, u32 a2, u32 b2, u32 c2, u32 d2, u32 a3, u32 b3, u32 c3, u32 d3);'),
    ('gDPSetBlendColor', 'void gDPSetBlendColor(Gfx* gfx, u32 r, u32 g, u32 b, u32 a);'),
    ('gDPSetPrimDepth', 'void gDPSetPrimDepth(Gfx* gfx, u16 z, u16 dz);'),
    
    # Audio functions we know are needed
    ('sAdo_OngenTrgStart', 'void sAdo_OngenTrgStart(u16 id, const xyz_t* pos);'),
]

def add_essential_functions():
    """Add essential functions to existing platform wrapper"""
    
    header_path = '/Users/matt/RiderProjects/ac-decomp/vita/platform/ac_vita_platform.h'
    impl_path = '/Users/matt/RiderProjects/ac-decomp/vita/platform/ac_vita_platform.c'
    
    # Check current content to avoid duplicates
    with open(header_path, 'r') as f:
        header_content = f.read()
    with open(impl_path, 'r') as f:
        impl_content = f.read()
    
    # Add missing declarations
    new_declarations = []
    new_implementations = []
    
    for func_name, declaration in ESSENTIAL_FUNCTIONS:
        if func_name not in header_content:
            new_declarations.append(declaration)
            
            # Generate implementation
            if 'void' in declaration:
                # Extract parameters for void suppression
                params_match = re.search(r'\(([^)]*)\)', declaration)
                if params_match:
                    params = params_match.group(1)
                    if params.strip() and params.strip() != 'void':
                        param_names = []
                        for param in params.split(','):
                            param = param.strip()
                            if param:
                                # Extract parameter name (last word)
                                words = param.split()
                                if words:
                                    param_name = words[-1].replace('*', '').replace('[', '').replace(']', '')
                                    param_names.append(f"(void){param_name};")
                        
                        impl = f"{declaration.replace(';', '')} {{\n    {' '.join(param_names)}\n    /* TODO: Implement {func_name} */\n}}"
                    else:
                        impl = f"{declaration.replace(';', '')} {{\n    /* TODO: Implement {func_name} */\n}}"
                else:
                    impl = f"{declaration.replace(';', '')} {{\n    /* TODO: Implement {func_name} */\n}}"
            else:
                impl = f"{declaration.replace(';', '')} {{\n    /* TODO: Implement {func_name} */\n    return 0;\n}}"
            
            new_implementations.append(impl)
    
    if new_declarations:
        # Add to header
        with open(header_path, 'a') as f:
            f.write('\n// Essential missing functions\n')
            f.write('\n'.join(new_declarations) + '\n')
        
        # Add to implementation
        with open(impl_path, 'a') as f:
            f.write('\n// Essential missing function implementations\n')
            f.write('\n\n'.join(new_implementations) + '\n')
        
        print(f"✅ Added {len(new_declarations)} essential functions")
    else:
        print("✅ All essential functions already present")

if __name__ == "__main__":
    import re
    print("⚡ QUICK ESSENTIAL FIXES")
    add_essential_functions()
    print("🔄 Ready for build test!") 