#!/usr/bin/env python3
"""
ESP32-S3 Crash Test Suite
=========================

This script provides comprehensive testing of the crash log manager by:
1. Triggering various types of crashes via serial commands
2. Monitoring crash detection and logging
3. Verifying crash log storage and retrieval
4. Testing system recovery after crashes

Crash Types Tested:
- Null pointer dereference
- Stack overflow
- Heap corruption
- Assertion failures
- Watchdog timeouts
- Manual test crashes

Requirements:
    pip install pyserial

Usage:
    python crash_test_suite.py --port COM3 --test all
    python crash_test_suite.py --port COM3 --test null_pointer
    python crash_test_suite.py --port COM3 --test watchdog
"""

import argparse
import time
import serial
import re
import sys
from datetime import datetime
from typing import List, Dict, Optional, Tuple
import json

class CrashTestSuite:
    def __init__(self, port: str, baudrate: int = 115200):
        """Initialize crash test suite."""
        self.port = port
        self.baudrate = baudrate
        self.serial_conn = None
        self.test_results = {}
        self.crash_logs_detected = []

        # Crash test definitions
        self.crash_tests = {
            'soft_test': {
                'name': 'Soft Test Crash',
                'command': 'TEST_CRASH_SOFT',
                'description': 'Tests logging without actual crash',
                'expect_reboot': False,
                'timeout': 10
            },
            'null_pointer': {
                'name': 'Null Pointer Dereference',
                'command': 'TEST_CRASH_NULL',
                'description': 'Triggers null pointer access violation',
                'expect_reboot': True,
                'timeout': 15
            },
            'stack_overflow': {
                'name': 'Stack Overflow',
                'command': 'TEST_CRASH_STACK',
                'description': 'Triggers stack overflow through recursion',
                'expect_reboot': True,
                'timeout': 15
            },
            'heap_corruption': {
                'name': 'Heap Corruption',
                'command': 'TEST_CRASH_HEAP',
                'description': 'Triggers heap corruption detection',
                'expect_reboot': True,
                'timeout': 15
            },
            'assert_fail': {
                'name': 'Assertion Failure',
                'command': 'TEST_CRASH_ASSERT',
                'description': 'Triggers assertion failure',
                'expect_reboot': True,
                'timeout': 15
            },
            'watchdog': {
                'name': 'Watchdog Timeout',
                'command': 'TEST_CRASH_WATCHDOG',
                'description': 'Triggers watchdog timeout',
                'expect_reboot': True,
                'timeout': 30
            }
        }

    def connect(self) -> bool:
        """Connect to ESP32-S3 device."""
        try:
            self.serial_conn = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=1,
                write_timeout=1
            )
            print(f"✓ Connected to {self.port} at {self.baudrate} baud")
            # Clear any pending data
            self.serial_conn.reset_input_buffer()
            self.serial_conn.reset_output_buffer()
            return True
        except Exception as e:
            print(f"✗ Failed to connect to {self.port}: {e}")
            return False

    def disconnect(self):
        """Disconnect from device."""
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()
            print("✓ Disconnected from device")

    def send_command(self, command: str) -> bool:
        """Send command to ESP32-S3."""
        try:
            cmd = f"{command}\n"
            self.serial_conn.write(cmd.encode('utf-8'))
            self.serial_conn.flush()
            print(f"📤 Sent command: {command}")
            return True
        except Exception as e:
            print(f"✗ Failed to send command: {e}")
            return False

    def read_serial_with_timeout(self, timeout: float) -> List[str]:
        """Read serial output with timeout and return lines."""
        lines = []
        start_time = time.time()
        buffer = ""

        while (time.time() - start_time) < timeout:
            if self.serial_conn.in_waiting > 0:
                try:
                    data = self.serial_conn.read(self.serial_conn.in_waiting).decode('utf-8', errors='ignore')
                    buffer += data

                    # Split into lines
                    while '\n' in buffer:
                        line, buffer = buffer.split('\n', 1)
                        line = line.strip()
                        if line:
                            timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
                            print(f"[{timestamp}] {line}")
                            lines.append(line)

                except Exception as e:
                    print(f"⚠️  Error reading serial: {e}")

            time.sleep(0.001)  # Small delay to prevent excessive CPU usage

        return lines

    def wait_for_system_ready(self, timeout: float = 60.0) -> bool:
        """Wait for system to be ready and crash handler initialized."""
        print("🔄 Waiting for system initialization...")
        lines = self.read_serial_with_timeout(timeout)

        ready_indicators = [
            'System Monitor - Fully Initialized',
            'Crash handler initialized',
            'Crash log manager initialized'
        ]

        found_indicators = []
        for line in lines:
            for indicator in ready_indicators:
                if indicator in line:
                    found_indicators.append(indicator)

        if len(found_indicators) >= 2:
            print(f"✓ System ready ({len(found_indicators)} indicators found)")
            return True
        else:
            print(f"✗ System not ready (only {len(found_indicators)} indicators found)")
            return False

    def detect_reboot(self, lines: List[str]) -> bool:
        """Detect if system rebooted by looking for boot messages."""
        reboot_indicators = [
            'ESP-ROM:esp32s3',
            'rst:0x',
            'ESP-IDF v5.5 2nd stage bootloader',
            'Loaded app from partition'
        ]

        for line in lines:
            for indicator in reboot_indicators:
                if indicator in line:
                    return True
        return False

    def parse_crash_logs(self, lines: List[str]) -> List[Dict]:
        """Parse crash log entries from output."""
        crash_logs = []

        patterns = {
            'init': r'Crash log manager initialized - (\d+) logs stored',
            'stored': r'logs stored|Storing crash log',
            'test_logged': r'Test crash logged: (.+)',
            'recovery': r'System recovered from crash: (.+)'
        }

        for line in lines:
            for pattern_name, pattern in patterns.items():
                match = re.search(pattern, line, re.IGNORECASE)
                if match:
                    crash_logs.append({
                        'type': pattern_name,
                        'line': line,
                        'data': match.groups() if match.groups() else None,
                        'timestamp': datetime.now()
                    })

        return crash_logs

    def trigger_soft_crash_test(self) -> bool:
        """Trigger a soft crash test (no actual crash)."""
        print("🧪 Testing soft crash logging...")

        # This would use the existing crash_handler_trigger_test function
        lines = self.read_serial_with_timeout(5.0)

        # Look for test crash in logs
        test_indicators = [
            'Test crash logged',
            'TEST:',
            'Manual crash test'
        ]

        found_test = False
        for line in lines:
            for indicator in test_indicators:
                if indicator in line:
                    found_test = True
                    print(f"✓ Found test crash indicator: {line}")

        return found_test

    def run_crash_test(self, test_name: str) -> Dict:
        """Run a specific crash test."""
        if test_name not in self.crash_tests:
            return {'success': False, 'error': f'Unknown test: {test_name}'}

        test_config = self.crash_tests[test_name]
        print(f"\n🔥 Running crash test: {test_config['name']}")
        print(f"📝 Description: {test_config['description']}")

        result = {
            'test_name': test_name,
            'success': False,
            'crash_detected': False,
            'reboot_detected': False,
            'crash_logs': [],
            'duration': 0,
            'error': None
        }

        start_time = time.time()

        try:
            # Special handling for soft test
            if test_name == 'soft_test':
                result['success'] = self.trigger_soft_crash_test()
                result['duration'] = time.time() - start_time
                return result

            # Send crash trigger command
            if not self.send_command(test_config['command']):
                result['error'] = 'Failed to send command'
                return result

            # Monitor for crash and recovery
            print(f"⏱️  Monitoring for {test_config['timeout']} seconds...")
            lines = self.read_serial_with_timeout(test_config['timeout'])

            # Analyze results
            result['reboot_detected'] = self.detect_reboot(lines)
            result['crash_logs'] = self.parse_crash_logs(lines)
            result['crash_detected'] = len(result['crash_logs']) > 0

            # Determine success based on expectations
            if test_config['expect_reboot']:
                result['success'] = result['reboot_detected'] and result['crash_detected']
            else:
                result['success'] = result['crash_detected'] and not result['reboot_detected']

            # Wait for system to come back up if it rebooted
            if result['reboot_detected']:
                print("🔄 System rebooted, waiting for recovery...")
                if self.wait_for_system_ready(30.0):
                    print("✓ System recovered successfully")
                else:
                    print("⚠️  System recovery incomplete")
                    result['success'] = False

        except Exception as e:
            result['error'] = str(e)

        result['duration'] = time.time() - start_time
        return result

    def run_test_suite(self, tests_to_run: List[str]) -> Dict:
        """Run a suite of crash tests."""
        print("🚀 Starting Crash Test Suite")
        print("=" * 50)

        if not self.connect():
            return {'error': 'Connection failed'}

        # Wait for initial system ready
        if not self.wait_for_system_ready():
            self.disconnect()
            return {'error': 'System not ready'}

        suite_results = {
            'tests': {},
            'summary': {
                'total': 0,
                'passed': 0,
                'failed': 0,
                'errors': 0
            },
            'start_time': datetime.now(),
            'end_time': None
        }

        try:
            for test_name in tests_to_run:
                if test_name in self.crash_tests:
                    print(f"\n{'='*20} Test {suite_results['summary']['total']+1} {'='*20}")

                    result = self.run_crash_test(test_name)
                    suite_results['tests'][test_name] = result
                    suite_results['summary']['total'] += 1

                    if result.get('error'):
                        suite_results['summary']['errors'] += 1
                        print(f"❌ Test failed with error: {result['error']}")
                    elif result['success']:
                        suite_results['summary']['passed'] += 1
                        print(f"✅ Test passed!")
                    else:
                        suite_results['summary']['failed'] += 1
                        print(f"❌ Test failed!")

                    # Wait between tests
                    if test_name != tests_to_run[-1]:
                        print("⏸️  Waiting 5 seconds before next test...")
                        time.sleep(5)
                else:
                    print(f"⚠️  Skipping unknown test: {test_name}")

        finally:
            suite_results['end_time'] = datetime.now()
            self.disconnect()

        return suite_results

    def print_summary(self, results: Dict):
        """Print test results summary."""
        print("\n" + "=" * 60)
        print("📊 CRASH TEST SUITE SUMMARY")
        print("=" * 60)

        if 'error' in results:
            print(f"❌ Suite failed: {results['error']}")
            return

        summary = results['summary']
        duration = (results['end_time'] - results['start_time']).total_seconds()

        print(f"⏱️  Total Duration: {duration:.1f} seconds")
        print(f"📈 Tests Run: {summary['total']}")
        print(f"✅ Passed: {summary['passed']}")
        print(f"❌ Failed: {summary['failed']}")
        print(f"💥 Errors: {summary['errors']}")

        success_rate = (summary['passed'] / summary['total'] * 100) if summary['total'] > 0 else 0
        print(f"🎯 Success Rate: {success_rate:.1f}%")

        print("\n📋 Individual Test Results:")
        print("-" * 40)

        for test_name, result in results['tests'].items():
            test_config = self.crash_tests.get(test_name, {})
            status = "✅ PASS" if result['success'] else "❌ FAIL"

            if result.get('error'):
                status = f"💥 ERROR: {result['error']}"

            print(f"{test_config.get('name', test_name):<25} {status}")

            if result['success'] and not result.get('error'):
                details = []
                if result.get('crash_detected'):
                    details.append("crash detected")
                if result.get('reboot_detected'):
                    details.append("reboot detected")
                if details:
                    print(f"{'':>27} └─ {', '.join(details)}")

        # Overall assessment
        print(f"\n🔍 Crash Log Manager Assessment:")
        if success_rate >= 80:
            print("🎉 EXCELLENT - Crash logging system working well")
        elif success_rate >= 60:
            print("👍 GOOD - Crash logging mostly functional")
        elif success_rate >= 40:
            print("⚠️  FAIR - Some crash logging issues detected")
        else:
            print("❌ POOR - Significant crash logging problems")

    def save_results(self, results: Dict, filename: str = None):
        """Save test results to JSON file."""
        if not filename:
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            filename = f"crash_test_results_{timestamp}.json"

        # Convert datetime objects to strings for JSON serialization
        json_results = json.loads(json.dumps(results, default=str))

        try:
            with open(filename, 'w') as f:
                json.dump(json_results, f, indent=2)
            print(f"💾 Results saved to: {filename}")
        except Exception as e:
            print(f"⚠️  Failed to save results: {e}")

def main():
    """Main function."""
    parser = argparse.ArgumentParser(description="ESP32-S3 Crash Test Suite")
    parser.add_argument("--port", "-p", default="COM3", help="Serial port (default: COM3)")
    parser.add_argument("--baudrate", "-b", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--test", "-t", default="all",
                       help="Test to run: all, soft_test, null_pointer, stack_overflow, heap_corruption, assert_fail, watchdog")
    parser.add_argument("--save", "-s", action="store_true", help="Save results to JSON file")
    parser.add_argument("--output", "-o", help="Output filename for results")

    args = parser.parse_args()

    print("ESP32-S3 Crash Test Suite")
    print("=========================")
    print(f"Port: {args.port}")
    print(f"Baud Rate: {args.baudrate}")
    print(f"Test: {args.test}")
    print()

    tester = CrashTestSuite(args.port, args.baudrate)

    # Determine which tests to run
    if args.test == "all":
        tests_to_run = list(tester.crash_tests.keys())
    else:
        tests_to_run = [args.test] if args.test in tester.crash_tests else []
        if not tests_to_run:
            print(f"❌ Unknown test: {args.test}")
            print(f"Available tests: {', '.join(tester.crash_tests.keys())}")
            sys.exit(1)

    # Run tests
    results = tester.run_test_suite(tests_to_run)

    # Print results
    tester.print_summary(results)

    # Save results if requested
    if args.save and 'error' not in results:
        tester.save_results(results, args.output)

    # Exit with appropriate code
    if 'error' in results:
        sys.exit(1)

    success_rate = (results['summary']['passed'] / results['summary']['total'] * 100) if results['summary']['total'] > 0 else 0
    sys.exit(0 if success_rate >= 50 else 1)

if __name__ == "__main__":
    main()
