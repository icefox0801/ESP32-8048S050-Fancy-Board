/**
 * @file common_types.h
 * @brief Common data types shared across modules
 *
 * This header contains data structures that are shared between modules
 * but doesn't include any module-specific headers to maintain decoupling.
 */

#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // ═══════════════════════════════════════════════════════════════════════════════
  // DATA STRUCTURES
  // ═══════════════════════════════════════════════════════════════════════════════

  /**
   * @brief CPU monitoring data structure
   */
  struct cpu_info
  {
    uint8_t usage; ///< CPU usage percentage (0-100)
    uint8_t temp;  ///< CPU temperature in Celsius
    uint32_t freq; ///< CPU frequency in MHz
    uint16_t fan;  ///< CPU fan speed in RPM
    char name[32]; ///< CPU name/model string
  };

  /**
   * @brief GPU monitoring data structure
   */
  struct gpu_info
  {
    uint8_t usage;      ///< GPU usage percentage (0-100)
    uint8_t temp;       ///< GPU temperature in Celsius
    char name[32];      ///< GPU name/model string
    uint32_t mem_used;  ///< GPU memory used (MB)
    uint32_t mem_total; ///< GPU memory total (MB)
  };

  /**
   * @brief Memory monitoring data structure
   */
  struct memory_info
  {
    uint8_t usage; ///< Memory usage percentage (0-100)
    float used;    ///< Memory used (GB)
    float total;   ///< Memory total (GB)
    float avail;   ///< Memory available (GB)
  };

  /**
   * @brief System monitoring data structure
   *
   * Contains all system metrics received via JSON from serial port:
   * - Timestamp for data freshness tracking
   * - CPU usage, temperature, and identification
   * - GPU usage, temperature, memory, and identification
   * - System memory usage and availability
   */
  typedef struct system_data
  {
    // Timestamp (milliseconds since epoch)
    uint64_t timestamp;

    // CPU Information Section
    struct cpu_info cpu;

    // GPU Information Section
    struct gpu_info gpu;

    // System Memory Information Section
    struct memory_info mem;
  } system_data_t;

#ifdef __cplusplus
}
#endif

#endif // COMMON_TYPES_H
