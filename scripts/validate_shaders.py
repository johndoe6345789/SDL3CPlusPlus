#!/usr/bin/env python3
"""
Mega-strict shader pipeline validator
Catches issues before they reach the GPU driver
"""

import re
import sys
from dataclasses import dataclass
from typing import List, Dict, Set, Optional, Tuple

@dataclass
class ShaderAttribute:
    location: int
    type: str
    name: str
    
@dataclass
class VertexLayoutAttribute:
    location: int
    type: str
    name: str
    size: int  # bytes
    
@dataclass
class ValidationError:
    severity: str  # 'ERROR', 'WARNING', 'INFO'
    message: str
    shader_name: Optional[str] = None
    line_number: Optional[int] = None

class ShaderValidator:
    def __init__(self):
        self.errors: List[ValidationError] = []
        
    def error(self, msg: str, shader: str = None, line: int = None):
        self.errors.append(ValidationError('ERROR', msg, shader, line))
        
    def warning(self, msg: str, shader: str = None, line: int = None):
        self.errors.append(ValidationError('WARNING', msg, shader, line))
        
    def info(self, msg: str, shader: str = None, line: int = None):
        self.errors.append(ValidationError('INFO', msg, shader, line))
    
    def extract_shader_inputs(self, glsl_source: str, shader_name: str) -> List[ShaderAttribute]:
        """Extract vertex shader input attributes"""
        inputs = []
        
        # Match: layout (location = N) in type name;
        pattern = r'layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s+in\s+(\w+)\s+(\w+)\s*;'
        
        for line_num, line in enumerate(glsl_source.split('\n'), 1):
            match = re.search(pattern, line)
            if match:
                location = int(match.group(1))
                attr_type = match.group(2)
                name = match.group(3)
                inputs.append(ShaderAttribute(location, attr_type, name))
                self.info(f"Found input: location={location}, type={attr_type}, name={name}", 
                         shader_name, line_num)
        
        return inputs
    
    def extract_shader_outputs(self, glsl_source: str, shader_name: str) -> List[ShaderAttribute]:
        """Extract shader output attributes"""
        outputs = []
        
        # Match: layout (location = N) out type name;
        pattern = r'layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s+out\s+(\w+)\s+(\w+)\s*;'
        
        for line_num, line in enumerate(glsl_source.split('\n'), 1):
            match = re.search(pattern, line)
            if match:
                location = int(match.group(1))
                attr_type = match.group(2)
                name = match.group(3)
                outputs.append(ShaderAttribute(location, attr_type, name))
        
        return outputs
    
    def validate_vertex_layout_match(self, shader_inputs: List[ShaderAttribute], 
                                     vertex_layout: List[VertexLayoutAttribute],
                                     shader_name: str):
        """Validate that shader inputs match the vertex layout"""
        
        # Check that all shader inputs have corresponding vertex layout entries
        shader_locs = {attr.location: attr for attr in shader_inputs}
        layout_locs = {attr.location: attr for attr in vertex_layout}
        
        # Check for missing attributes in layout
        for loc, shader_attr in shader_locs.items():
            if loc not in layout_locs:
                self.error(f"Shader '{shader_name}' expects input at location {loc} "
                          f"({shader_attr.name}: {shader_attr.type}) but vertex layout doesn't provide it")
        
        # Check for type mismatches
        for loc in shader_locs.keys() & layout_locs.keys():
            shader_attr = shader_locs[loc]
            layout_attr = layout_locs[loc]
            
            # Type compatibility check
            shader_type = shader_attr.type
            layout_type = layout_attr.type
            
            if not self.types_compatible(shader_type, layout_type):
                self.error(f"Type mismatch at location {loc}: shader expects {shader_type} "
                          f"but vertex layout provides {layout_type}")
        
        # Check for location gaps
        if shader_locs:
            max_loc = max(shader_locs.keys())
            for i in range(max_loc):
                if i not in shader_locs and i in layout_locs:
                    self.warning(f"Vertex layout provides unused attribute at location {i}")
    
    def types_compatible(self, shader_type: str, layout_type: str) -> bool:
        """Check if shader type and layout type are compatible"""
        
        # Mapping of GLSL types to expected vertex layout types
        type_map = {
            'vec2': 'float2',
            'vec3': 'float3',
            'vec4': 'float4',
            'float': 'float',
            'int': 'int',
            'ivec2': 'int2',
            'ivec3': 'int3',
            'ivec4': 'int4',
        }
        
        expected_layout_type = type_map.get(shader_type)
        return expected_layout_type == layout_type or layout_type == shader_type
    
    def validate_vertex_buffer_stride(self, vertex_layout: List[VertexLayoutAttribute],
                                      actual_vertex_size: int):
        """Validate that the vertex layout stride matches the actual vertex struct size"""
        
        # Calculate expected stride from layout
        expected_stride = sum(attr.size for attr in vertex_layout)
        
        if expected_stride != actual_vertex_size:
            self.error(f"Vertex layout stride mismatch: layout expects {expected_stride} bytes "
                      f"but actual vertex struct is {actual_vertex_size} bytes")
            
            self.info(f"Layout breakdown:")
            for attr in vertex_layout:
                self.info(f"  {attr.name}: {attr.size} bytes")
    
    def validate_location_continuity(self, shader_inputs: List[ShaderAttribute], shader_name: str):
        """Warn about non-continuous location assignments"""
        
        if not shader_inputs:
            return
        
        locations = sorted([attr.location for attr in shader_inputs])
        expected = list(range(len(locations)))
        
        if locations != expected:
            self.warning(f"Shader '{shader_name}' has non-continuous location assignments: {locations} "
                        f"(expected: {expected}). This may cause issues with some drivers.",
                        shader_name)
    
    def validate_spir_v_compatibility(self, glsl_source: str, shader_name: str):
        """Check for common SPIR-V compilation issues"""
        
        lines = glsl_source.split('\n')
        
        for line_num, line in enumerate(lines, 1):
            # Check for inputs/outputs without location qualifiers (required for SPIR-V)
            if re.search(r'\bin\s+\w+\s+\w+\s*;', line) and 'layout' not in line:
                if '#version' not in line and '//' not in line:
                    self.error(f"SPIR-V requires 'layout(location=N)' for all inputs/outputs",
                              shader_name, line_num)
            
            if re.search(r'\bout\s+\w+\s+\w+\s*;', line) and 'layout' not in line:
                if '#version' not in line and '//' not in line and 'gl_' not in line:
                    self.error(f"SPIR-V requires 'layout(location=N)' for all inputs/outputs",
                              shader_name, line_num)
    
    def validate_interface_matching(self, vertex_outputs: List[ShaderAttribute],
                                   fragment_inputs: List[ShaderAttribute],
                                   vs_name: str, fs_name: str):
        """Validate that vertex shader outputs match fragment shader inputs"""
        
        vs_locs = {attr.location: attr for attr in vertex_outputs}
        fs_locs = {attr.location: attr for attr in fragment_inputs}
        
        # Fragment shader inputs should match vertex shader outputs
        for loc, fs_attr in fs_locs.items():
            if loc not in vs_locs:
                self.error(f"Fragment shader '{fs_name}' expects input at location {loc} "
                          f"({fs_attr.name}) but vertex shader '{vs_name}' doesn't output it")
            else:
                vs_attr = vs_locs[loc]
                if vs_attr.type != fs_attr.type:
                    self.error(f"Type mismatch at location {loc}: VS outputs {vs_attr.type} "
                              f"but FS expects {fs_attr.type}")
    
    def validate_attribute_name_conventions(self, shader_inputs: List[ShaderAttribute], 
                                          shader_name: str):
        """Validate attribute naming conventions"""
        
        # Expected prefixes for different shader stages
        valid_prefixes = ['i_', 'a_', 'v_']
        
        for attr in shader_inputs:
            if not any(attr.name.startswith(prefix) for prefix in valid_prefixes):
                self.warning(f"Attribute '{attr.name}' doesn't follow naming convention "
                           f"(expected prefix: {valid_prefixes})", shader_name)
    
    def print_report(self):
        """Print validation report"""
        
        errors = [e for e in self.errors if e.severity == 'ERROR']
        warnings = [e for e in self.errors if e.severity == 'WARNING']
        infos = [e for e in self.errors if e.severity == 'INFO']
        
        print("\n" + "="*80)
        print("SHADER PIPELINE VALIDATION REPORT")
        print("="*80)
        
        if errors:
            print(f"\n🔴 ERRORS ({len(errors)}):")
            for err in errors:
                loc = f" [{err.shader_name}:{err.line_number}]" if err.shader_name else ""
                print(f"  ✗ {err.message}{loc}")
        
        if warnings:
            print(f"\n⚠️  WARNINGS ({len(warnings)}):")
            for warn in warnings:
                loc = f" [{warn.shader_name}:{warn.line_number}]" if warn.shader_name else ""
                print(f"  ⚠ {warn.message}{loc}")
        
        if infos and '--verbose' in sys.argv:
            print(f"\nℹ️  INFO ({len(infos)}):")
            for info in infos:
                loc = f" [{info.shader_name}:{info.line_number}]" if info.shader_name else ""
                print(f"  ℹ {info.message}{loc}")
        
        print("\n" + "="*80)
        print(f"Summary: {len(errors)} errors, {len(warnings)} warnings, {len(infos)} info")
        print("="*80 + "\n")
        
        return len(errors) == 0

def main():
    validator = ShaderValidator()
    
    # Example: Define expected vertex layout based on bgfx_graphics_backend.cpp
    # This should match: Position, Normal, Tangent, TexCoord0, Color0
    vertex_layout = [
        VertexLayoutAttribute(0, 'float3', 'Position', 12),
        VertexLayoutAttribute(1, 'float3', 'Normal', 12),
        VertexLayoutAttribute(2, 'float3', 'Tangent', 12),
        VertexLayoutAttribute(3, 'float2', 'TexCoord0', 8),
        VertexLayoutAttribute(4, 'float3', 'Color0', 12),
    ]
    
    # Expected vertex struct size: 3+3+3+2+3 = 14 floats = 56 bytes
    vertex_struct_size = 56
    
    # Example vertex shader (this would come from reading actual shader files)
    example_vs = """
#version 450
layout (location = 0) in vec3 i_position;
layout (location = 1) in vec3 i_normal;
layout (location = 2) in vec3 i_tangent;
layout (location = 3) in vec2 i_texcoord_0;

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_texcoord;

void main() {
    gl_Position = vec4(i_position, 1.0);
    v_normal = i_normal;
    v_texcoord = i_texcoord_0;
}
"""
    
    example_fs = """
#version 450
layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_texcoord;

layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(v_normal * 0.5 + 0.5, 1.0);
}
"""
    
    # Validation pipeline
    print("Running shader pipeline validation...")
    
    # 1. Extract attributes
    vs_inputs = validator.extract_shader_inputs(example_vs, "example_vs")
    vs_outputs = validator.extract_shader_outputs(example_vs, "example_vs")
    fs_inputs = validator.extract_shader_inputs(example_fs, "example_fs")
    
    # 2. Validate vertex layout matching
    validator.validate_vertex_layout_match(vs_inputs, vertex_layout, "example_vs")
    
    # 3. Validate stride
    validator.validate_vertex_buffer_stride(vertex_layout, vertex_struct_size)
    
    # 4. Validate location continuity
    validator.validate_location_continuity(vs_inputs, "example_vs")
    
    # 5. Validate SPIR-V compatibility
    validator.validate_spir_v_compatibility(example_vs, "example_vs")
    validator.validate_spir_v_compatibility(example_fs, "example_fs")
    
    # 6. Validate interface matching
    validator.validate_interface_matching(vs_outputs, fs_inputs, "example_vs", "example_fs")
    
    # 7. Validate naming conventions
    validator.validate_attribute_name_conventions(vs_inputs, "example_vs")
    
    # Print report
    success = validator.print_report()
    
    return 0 if success else 1

if __name__ == '__main__':
    sys.exit(main())
