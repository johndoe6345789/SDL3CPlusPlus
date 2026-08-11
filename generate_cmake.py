#!/usr/bin/env python3
"""
CMakeLists.txt Generator from JSON Configuration

This script generates CMakeLists.txt from a JSON configuration file using Jinja2.
Benefits:
  • Removes 1500+ lines of repetitive CMake code
  • Makes dependencies and targets declarative (JSON)
  • Easy to add new targets without CMake expertise
  • Modular: include files for dependencies, targets, tests
  • Version controllable: JSON changes are easier to review than CMake

Usage:
  python3 generate_cmake.py --config cmake_config.json --template CMakeLists.txt.jinja2 --output CMakeLists.txt

Example JSON structure:
  {
    "project": { "name": "MyProject", "cmake_minimum_version": "3.24" },
    "options": [
      { "name": "OPTION_NAME", "type": "BOOL", "default": "ON" }
    ],
    "targets": [
      { "name": "my_app", "type": "executable", "sources": [...] }
    ],
    "test_targets": [
      { "name": "test_something", "sources": [...] }
    ]
  }
"""

import json
import argparse
import sys
from pathlib import Path
from glob import glob
from os import path

try:
    from jinja2 import Environment, FileSystemLoader, select_autoescape
except ImportError:
    print("ERROR: jinja2 is required. Install with: pip install jinja2")
    sys.exit(1)


def load_config(config_path: str) -> dict:
    """Load configuration from JSON file."""
    with open(config_path, 'r') as f:
        return json.load(f)


def _posix_path(p: str) -> str:
    """Normalize path separators to forward slashes for CMake compatibility."""
    return p.replace('\\', '/')


def expand_test_glob_patterns(config: dict) -> dict:
    """Expand glob patterns in test_targets list itself.

    Supports:
    - "pattern": "tests/unit/**/*.cpp" - generates test from each cpp file
    - Can be mixed with explicit test definitions
    """
    exclusions = set(config.get('source_exclusions', []))
    expanded_tests = []

    for test_def in config.get('test_targets', []):
        if isinstance(test_def, dict) and 'pattern' in test_def:
            # This is a glob pattern definition - expand it
            pattern = test_def['pattern']
            matches = sorted(_posix_path(m) for m in glob(pattern))
            matches = [m for m in matches if m not in exclusions]

            template = test_def.copy()
            del template['pattern']  # Remove pattern key

            for match in matches:
                # Create a test target from each match
                test_name = Path(match).stem  # e.g., test_audio_play_step
                test = template.copy()
                test['name'] = test_name
                test['sources'] = [match] + template.get('sources', [])[1:]  # Replace first source with match
                expanded_tests.append(test)
        else:
            # Regular test definition
            expanded_tests.append(test_def)

    config['test_targets'] = expanded_tests
    return config


def expand_globs(config: dict) -> dict:
    """Expand glob patterns in source lists and apply exclusions."""
    # First expand test_targets from glob patterns
    config = expand_test_glob_patterns(config)

    exclusions = set(config.get('source_exclusions', []))

    for target in config.get('targets', []):
        if 'sources' in target:
            expanded = []
            for src in target['sources']:
                if '*' in src:
                    # Glob pattern - expand it
                    matches = sorted(_posix_path(m) for m in glob(src))
                    # Filter out excluded files
                    matches = [m for m in matches if m not in exclusions]
                    expanded.extend(matches)
                elif src not in exclusions:  # Check literal sources too
                    expanded.append(_posix_path(src))
            target['sources'] = expanded

    for test in config.get('test_targets', []):
        if 'sources' in test:
            expanded = []
            for src in test['sources']:
                if '*' in src:
                    matches = sorted(_posix_path(m) for m in glob(src))
                    # Filter out excluded files
                    matches = [m for m in matches if m not in exclusions]
                    expanded.extend(matches)
                elif src not in exclusions:
                    expanded.append(_posix_path(src))
            test['sources'] = expanded

        # Provide smart defaults for test targets from config
        if 'include_directories' not in test:
            test['include_directories'] = config.get('test_defaults', {}).get('include_directories', ['src'])
        if 'link_libraries' not in test or not test['link_libraries']:
            # Use configured defaults or standard test libraries
            test['link_libraries'] = config.get('test_defaults', {}).get('link_libraries', [
                'GTest::gtest_main',
                'GTest::gmock',
                'glm::glm',
                'Bullet::Bullet',
                'EnTT::EnTT'
            ])

    return config


def generate_cmake(config: dict, template_path: str, output_path: str) -> None:
    """Generate CMakeLists.txt from config and template."""
    # Expand glob patterns in source lists
    config = expand_globs(config)

    # Setup Jinja2 environment
    template_dir = Path(template_path).parent
    template_name = Path(template_path).name

    env = Environment(
        loader=FileSystemLoader(template_dir),
        autoescape=select_autoescape(),
        trim_blocks=False,
        lstrip_blocks=False
    )

    # Load and render template
    template = env.get_template(template_name)
    rendered = template.render(config=config)

    # Write output
    with open(output_path, 'w') as f:
        f.write(rendered)

    print(f"✓ Generated {output_path}")
    print(f"  - Project: {config['project']['name']}")
    print(f"  - Options: {len(config.get('options', []))}")
    print(f"  - Targets: {len(config.get('targets', []))}")
    print(f"  - Tests: {len(config.get('test_targets', []))}")


def validate_config(config: dict) -> bool:
    """Validate configuration structure."""
    required_keys = ['project']

    for key in required_keys:
        if key not in config:
            print(f"ERROR: Missing required key in config: {key}")
            return False

    # Validate project structure
    project = config['project']
    if 'name' not in project or 'cmake_minimum_version' not in project:
        print("ERROR: project must have 'name' and 'cmake_minimum_version'")
        return False

    # Validate targets if present
    for target in config.get('targets', []):
        if 'name' not in target or 'type' not in target:
            print(f"ERROR: target missing 'name' or 'type': {target}")
            return False

    return True


def main():
    parser = argparse.ArgumentParser(
        description="Generate CMakeLists.txt from JSON configuration",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 generate_cmake.py \\
    --config cmake_config.json \\
    --template CMakeLists.txt.jinja2 \\
    --output CMakeLists.txt

  # Validate config only
  python3 generate_cmake.py --config cmake_config.json --validate
        """
    )

    parser.add_argument(
        '--config',
        required=True,
        help='Path to cmake_config.json'
    )
    parser.add_argument(
        '--template',
        default='CMakeLists.txt.jinja2',
        help='Path to CMakeLists.txt.jinja2 template (default: CMakeLists.txt.jinja2)'
    )
    parser.add_argument(
        '--output',
        default='CMakeLists.txt',
        help='Output CMakeLists.txt path (default: CMakeLists.txt)'
    )
    parser.add_argument(
        '--validate',
        action='store_true',
        help='Validate config without generating'
    )

    args = parser.parse_args()

    # Load and validate config
    try:
        config = load_config(args.config)
    except FileNotFoundError:
        print(f"ERROR: Config file not found: {args.config}")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"ERROR: Invalid JSON in {args.config}: {e}")
        sys.exit(1)

    if not validate_config(config):
        sys.exit(1)

    if args.validate:
        print(f"✓ Config is valid: {args.config}")
        return

    # Generate CMakeLists.txt
    try:
        generate_cmake(config, args.template, args.output)
    except FileNotFoundError as e:
        print(f"ERROR: Template file not found: {args.template}")
        sys.exit(1)
    except Exception as e:
        print(f"ERROR: Failed to generate CMakeLists.txt: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()
