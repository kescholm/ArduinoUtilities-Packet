/**
 * \file
 * Functions and types for CRC checks.
 *
 * Generated on Tue Oct 13 17:08:11 2020
 * by pycrc v0.9.2, https://pycrc.org
 * using the configuration:
 *  - Width         = 16
 *  - Poly          = 0x1021
 *  - XorIn         = 0x0000
 *  - ReflectIn     = True
 *  - XorOut        = 0x0000
 *  - ReflectOut    = True
 *  - Algorithm     = table-driven
 *
 * This file defines the functions crc_init(), crc_update() and crc_finalize().
 *
 * The crc_init() function returns the inital \c crc value and must be called
 * before the first call to crc_update().
 * Similarly, the crc_finalize() function must be called after the last call
 * to crc_update(), before the \c crc is being used.
 * is being used.
 *
 * The crc_update() function can be called any number of times (including zero
 * times) in between the crc_init() and crc_finalize() calls.
 *
 * This pseudo-code shows an example usage of the API:
 * \code{.c}
 * crc_t crc;
 * unsigned char data[MAX_DATA_LEN];
 * size_t data_len;
 *
 * crc = crc_init();
 * while ((data_len = read_data(data, MAX_DATA_LEN)) > 0) {
 *     crc = crc_update(crc, data, data_len);
 * }
 * crc = crc_finalize(crc);
 * \endcode
 */
#ifndef ARD_CRC_KERMIT_H
#define ARD_CRC_KERMIT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * http://automationwiki.com/index.php?title=CRC-16-CCITT
 * KERMIT (CRC-16/CCITT)
 * width=16 poly=0x1021 init=0x0000 refin=true refout=true xorout=0x0000
 * check=0x2189 name="KERMIT" CRC-CCITT    0x1021
 * x^16 + x^12 + x^5 + 1
 *
 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat-bits.16
 * CRC-16/KERMIT
 * width=16 poly=0x1021 init=0x0000 refin=true refout=true xorout=0x0000 check=0x2189 residue=0x0000
 * name="CRC-16/KERMIT"
 *
 * https://pycrc.org/models.html#kermit
 *
 */

/**
 * @defgroup   ArdCrc Cyclic Redundancy Check
 * @brief      Cyclic Redundancy Check
 *
 * Code was generated using pycrc:
 *
 * ```python
 * python -m pycrc --model kermit --algorithm table-driven --generate h -o ArdCrc.h
 * python -m pycrc --model kermit --algorithm table-driven --generate c -o ArdCrc.c
 * ```
 *
 * @{
 *
 */

/**
 * The definition of the used algorithm.
 *
 * This is not used anywhere in the generated code, but it may be used by the
 * application code to call algorithm-specific code, if desired.
 */
#define CRC_ALGO_TABLE_DRIVEN 1

/**
 * The type of the CRC values.
 *
 * This type must be big enough to contain at least 16 bits.
 */
typedef uint_fast16_t crc_t;

/**
 * Calculate the initial crc value.
 *
 * \return     The initial crc value.
 */
static inline crc_t crc_init(void) { return 0x0000; }

/**
 * Update the crc value with new data.
 *
 * \param[in] crc      The current crc value.
 * \param[in] data     Pointer to a buffer of \a data_len bytes.
 * \param[in] data_len Number of bytes in the \a data buffer.
 * \return             The updated crc value.
 */
crc_t crc_update(crc_t crc, const void *data, size_t data_len);

/**
 * Calculate the final crc value.
 *
 * \param[in] crc  The current crc value.
 * \return     The final crc value.
 */
static inline crc_t crc_finalize(crc_t crc) { return crc; }

/**
 * @brief Check if CRC is verified
 *
 * @param data Buffer
 * @param len Lnegth of data buffer
 * @return @c true if CRC is verified
 * @return @c false if CRC is not verified
 */
bool ard_crc16_kermit_test(const uint8_t * data, const size_t len);

/**
 * @brief Calculate and append CRC to buffer
 *
 * There MUST be at least 2 contiguous free bytes allocated at the end of the
 * array
 *
 * @param data Buffer
 * @param len Length of data buffer, not including trailing 2 bytes available
 * for CRC
 * @return uint16_t CRC value
 */
uint16_t ard_crc16_kermit_append(uint8_t * data, const size_t len);

/**
 * @}
 */

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* ARD_CRC_KERMIT_H */
